/**************************************************************************************
 * 
 * CdL Magistrale in Ingegneria Informatica
 * Corso di Architetture e Programmazione dei Sistemi di Elaborazione - a.a. 2017/18
 * 
 * Progetto dell'algoritmo di PageRank
 * in linguaggio assembly x86-32 + SSE
 * 
 * Fabrizio Angiulli, 23 novembre 2017
 * 
 **************************************************************************************/

/*
 
 Software necessario per l'esecuzione:

     NASM (www.nasm.us)
     GCC (gcc.gnu.org)

 entrambi sono disponibili come pacchetti software 
 installabili mediante il packaging tool del sistema 
 operativo; per esempio, su Ubuntu, mediante i comandi:

     sudo apt-get install nasm
     sudo apt-get install gcc

 potrebbe essere necessario installare le seguenti librerie:

     sudo apt-get install lib32gcc-4.8-dev (o altra versione)
     sudo apt-get install libc6-dev-i386

 Per generare il file eseguibile:

 nasm -f elf32 pagerank32.nasm && gcc -O0 -m32 -msse pagerank32.o pagerank32c.c -o pagerank32c && ./pagerank32c
 
 oppure
 
 ./runpagerank32

 Compilazione e test parte in C:

 Posizionarsi nella cartella dove si trova il file (/git/Pagerank/Pagerank/src)
 gcc -O0 pagerank32c.c -o pagerank32c
 Test precisione singola:
 ./pagerank32c /home/<user>/git/Pagerank/Pagerank/src/test1 -d -sparse -single -c 0.85 -eps 1e-5
 Test precisione doppia:
 ./pagerank32c /home/<user>/git/Pagerank/Pagerank/src/test1 -d -sparse -double -c 0.85 -eps 1e-5
 Sostituire <user> col proprio nome utente

*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <xmmintrin.h>

/*
 * NOTA 1: define sostituisce le occorrenze (MATRIX, VECTOR, etc.) col valore corrispondente
 * siccome è qualcosa che fa il preprocessore, utilizzarle non oppupa memoria e non inficia
 * le prestazioni. Utilizzare GRAPHD e GRAPHS risulta quindi una scelta appropriata.
 *
 * NOTA 2: per eseguire il codice bisognerà utilizzare gcc e non eclipse, dato che librerie come
 * math.h hanno bisogno dell'argomento -lm per poter funzionare.
 */

#define	MATRIX	double*
#define	VECTOR	double*
#define	GRAPH	location*	// DECIDERE LA RAPPRESENTAZIONE IN MEMORIA (dev'essere un puntatore) (float* o double*)

/*
 * Struct riferita agli archi del grafo (?), per ora non usata.
 */
typedef struct {
	int x;
	int y;
} location;


typedef struct {
	char* file_name;
	MATRIX P; // codifica dense
	GRAPH G; // codifica full (sparse) double
	int N; // numero di nodi
	int M; // numero di archi
	int O; // padding
	double c; // default=0.85
	double eps; // default=1e-5
	int format; // 0=sparse, 1=full
	int prec; // 0=single, 1=double
	int opt; // 0=nopt, 1=opt
	double* pagerank;
	int silent;
	int display;
} params;


/*
 * 
 *	Le funzioni sono state scritte assumendo che le matrici siano memorizzate
 * 	mediante un array (float*), in modo da occupare un unico blocco
 * 	di memoria, ma a scelta del candidato possono essere 
 * 	memorizzate mediante array di array (float**).
 * 
 * 	In entrambi i casi il candidato dovrà inoltre scegliere se memorizzare le
 * 	matrici per righe (row-major order) o per colonne (column major-order).
 *
 * 	L'assunzione corrente è che le matrici siano in row-major order.
 *
 * 	La scelta è quella di mantenere il row-major order, in quanto
 * 	risulta intuitivo elaborare gli elementi.
 * 
 */

void* get_block(int size, int elements) {
	/*
	 * _mm_malloc si utilizza per allocare un blocco "allineato" di memoria.
	 * Siccome ogni sistema operativo ha una funzione diversa per fare ciò,
	 * utilizzare _mm_malloc consente di fare la stessa operazione in modo
	 * indipendente dal S.O. in uso (POSIX => Linux/non-POSIX => Windows).
	 */
	return _mm_malloc(elements*size,16);
}


void free_block(void* p) { 
	_mm_free(p);
}


MATRIX alloc_matrix(int rows, int cols) {
	return (MATRIX) get_block(sizeof(double),rows*cols);
}


void dealloc_matrix(MATRIX mat) {
	free_block(mat);
}


/*
 * 
 * 	load_dense
 * 	===========
 * 
 *	Legge da file una matrice di N righe
 * 	e M colonne e la memorizza in un array lineare in row-major order
 * 
 * 	Codifica del file:
 * 	primi 4 byte: numero di righe (N) --> numero intero a 32 bit
 * 	successivi 4 byte: numero di colonne (M) --> numero intero a 32 bit
 * 	successivi N*M*4 byte: matrix data in row-major order --> numeri floating-point a precisione doppia
 * 	Siccome è precisione doppia (double) non è N*M*4 ma N*M*8, dato che un double occupa 8 byte
 * 
 *****************************************************************************
 *	Se lo si ritiene opportuno, è possibile cambiare la codifica in memoria
 * 	della matrice. 
 *****************************************************************************
 * 
 */
MATRIX load_dense(char* filename, int *n, int *m) {
	FILE* fp;
	int rows, cols, status, i;
	char fpath[256];

	//sprintf salva il contenuto puntato da filename in un array di max 256 caratteri
	sprintf(fpath, "%s.matrix", filename);
	//apre il file non come un file di testo ma come un file binario (read binary - rb)
	fp = fopen(fpath, "rb");

	if (fp == NULL) {
		printf("'%s' : bad matrix file name!\n", fpath);
		exit(0);
	}

	/*fread salva in un blocco di memoria puntato dal primo argomento
	* (in questo caso &rows) il numero di byte letti, ottenuto come
	* numero_elementi*size_elementi (in questo caso 1*4, dove 4
	* è dato da sizeof(int))
	*/
	status = fread(&rows, sizeof(int), 1, fp);
	status = fread(&cols, sizeof(int), 1, fp);

	MATRIX data = alloc_matrix(rows,cols);
	//Salva la matrice (in precisione doppia) nel blocco di memoria puntato da data
	status = fread(data, sizeof(double), rows*cols, fp);
	fclose(fp);

	//salva le dimensioni della matrice in n ed m
	*n = rows;
	//dovrebbe essere *m = cols;
	*m = rows*cols; //(?)
	return data;
}

/*
 * 
 * 	load_sparse
 * 	===========
 * 
 *	Legge da file il grafo
 * 
 * 	Codifica del file:
 * 	primi 4 byte: numero di nodi (N) --> numero intero
 * 	successivi 4 byte: numero di archi (M) --> numero intero
 * 	successivi M*2*4 byte: M archi rappresentati come coppie (i,j) di interi a 32 bit 
 * 
 */
location* load_sparse(char* filename, int *n, int *m) {
	FILE* fp;
	int nodes, arcs, status, i;
	char fpath[256];
	int sorg, dest;
	
	sprintf(fpath, "%s.graph", filename);
	fp = fopen(fpath, "rb");
	
	if (fp == NULL) {
		printf("'%s' : bad graph file name!\n", fpath);
		exit(0);
	}
	
	status = fread(&nodes, sizeof(int), 1, fp);
	status = fread(&arcs, sizeof(int), 1, fp);

	location *l = malloc(arcs*sizeof(location));
	for (i = 0; i < arcs; i++) {
		status = fread(&sorg, sizeof(int), 1, fp);
		status = fread(&dest, sizeof(int), 1, fp);
		//Popola le struct
		l[i].x = sorg;
		l[i].y = dest;
	}

	fclose(fp);

	*n = nodes;
	*m = arcs;
	return l;
}

/*
 * Salva su file di testo i risultati
 */
void save_pageranks(char* filename, int n, VECTOR pagerank) {	
	FILE* fp;
	int i;
	char fpath[256];
	
	sprintf(fpath, "%s_pageranks.txt", filename);
	fp = fopen(fpath, "w");
	for (i = 0; i < n; i++)
		fprintf(fp, "%.14g\n", pagerank[i]);
	fclose(fp);
}

/*
 * Prototipo che si riferisce a una funzione presente in
 * un altro file.
 */
//extern void pagerank32(params* input);

float*	get_adiacency_matrix_single(int n, int m, location *l, int *o,  int *cnz, int *cz, int *z, int *nz);
void get_outdegree_single(int n, int cnz, float *A, float* d, int o);
float* get_matrix_P_single(int n, int cnz, float *A, float *d, int o);
void get_matrix_P_primo_single(int n, int cnz, float *P, float *d, int o);
void get_matrix_P_secondo_single(int n, int cnz, float *P, double c, int o);
void getVectorPiIn_single(int n, float e, int o, float *Pi);
void getVectorPik_single(float *P, float *Pi0, float *Pik, int n, int o, int cnz, int cz, int *nz, int *z);
void getPagrnk_single(int n, float *Pik);
void getDelta_single(float *Pi0, float *Pik, int n, float *delta);
void cvtPagerank(int n, float *Pik, double *Piconv);
void getPagerank_single(float *Pi0, float *Pik, float *P, double eps, int n, int o, int cnz, int cz, int *nz, int *z, double *Piconv);

double* getMatrix(int n, double *P, int *o);
double* get_adiacency_matrix_double(int n, int m, location *l, int *o,  int *cnz, int *cz, int *z, int *nz);
void get_outdegree_double(int n, int cnz, double *A, double* d, int o);
double* get_matrix_P_double(int n, int cnz, double *A, double *d, int o);
void get_matrix_P_primo_double(int n, int cnz, double *P, double *d, int o);
//double* get_matriceTeletrasporto_double(int n, double *v);
void get_matrix_P_secondo_double(int n, int cnz, double *P, double c, int o);
void getVectorPiIn_double(int n, double e, int o, double *Pi);
//double* get_v_double(int n);
void getVectorPik_double(double *P, double *Pi0, double *Pik, int n, int o, int cnz, int cz, int *nz, int *z);
void getVectorPik_dense(double *P, double *Pi0, double *Pik, int n, int o);
void getPagrnk_double(int n, double *Pik);
void getDelta_double(double *Pi0, double *Pik, int n, double *delta);
void getPagerank_dense(double *Pi0, double *Pik, double *P, double eps, int n, int o);
void getPagerank_double(double *Pi0, double *Pik, double *P, double eps, int n, int o, int cnz, int cz, int *nz, int *z);

/*
 *	pagerank
 * 	====
 * 
 *	img contiene l'immagine codificato come una matrice di N righe
 * 	ed M colonne memorizzata in un array lineare in row-major order
 * 
 *	Se lo si ritiene opportuno, è possibile cambiare la codifica in memoria
 * 	dell'immagine.
 * 
 * 
 */
void pagerank(params* input) {
	
    // -------------------------------------------------
    // Codificare qui l'algoritmo risolutivo
    // -------------------------------------------------
    /*
     * 1° step: passare dalla matrice G o S alla matrice P, utilizzando l'outdegree.
     * In particolare si vuole ottenere prima il vettore degli outdegree e poi imporre:
     * P[i][j] = (G oppure S)[i][j]/outdegree[i];
     */
	//verifica formato sparse single o double
	//precisione singola
	if(!input->format && !input->prec){
		int *nz = (int *)_mm_malloc(input->N*sizeof(int), 16);
		int *z = (int *)_mm_malloc(input->N*sizeof(int), 16);
		int cnz = 0;
		int cz = 0;
		float *A = get_adiacency_matrix_single(input->N, input->M, input->G, &input->O, &cnz, &cz, z, nz);
		float *d = (float *)_mm_malloc(cnz*sizeof(float), 16);
		get_outdegree_single(input->N, cnz, A, d, input->O);
		//matrice P
		float *P = get_matrix_P_single(input->N, cnz, A, d, input->O);

		//TEST matrice P
		//Basta sostituire P con input->S per testare la matrice di adiacenza.
		/*for(int i = 0; i < (input->N); i++){
			for(int j = 0; j < (input->N); j++){
				printf("elemento P[%d][%d] = %f\n", i, j, P[i*(input->N)+j]);
			}
		}*/

		/*
		 * 2° step: passare dalla matrice P a una matrice di transizione valida P'
		 * data da P'[i][j] >= 0 e somma(P'[i]) = 1.
		 * Gli elementi di P sono ottenuti tramite divisione di due quantità positive
		 * perchè gli elementi di A sono [0,1] e gli elementi di d sono >= 0.
		 * Di conseguenza la matrice P ha elementi >= 0. La seconda proprietà può non essere
		 * soddisfatta se è presente un elemento di d nullo, perchè questo significa che
		 * esiste un nodo che non ha archi uscenti. Se ciò si verifica si impone:
		 * P' = P + D con D = delta * v
		 * altrimenti P' = P
		 * Tutto ciò si traduce sostituendo le righe di P a cui corrisponde un outdegree nullo
		 * con v = (1/n,...,1/n). Al fine di evitare ulteriori allocamenti di memoria, P' sarà
		 * ottenuta modificando la matrice P, per questo motivo la seguente funzione non restituisce
		 * nessuna matrice.
		 */
		float e = 1/(float)input->N;
		/*
		 * Essendo la matrice privata delle righe nulle, questa è già una matrice
		 * di transizione valida.
		 */
		//get_matrix_P_primo_single(input->N, P, d, input->O);
		//float *v = get_v_single(input->N);
		//float *E = get_matriceTeletrasporto_single(input->N, v);
		get_matrix_P_secondo_single(input->N, cnz, P, input->c, input->O);
		float* Pi0 =(float*)_mm_malloc((input->N+input->O)*sizeof(float),16);
		getVectorPiIn_single(input->N, e, input->O, Pi0);
		float *Pik = (float *)_mm_malloc((input->N+input->O)*sizeof(float), 16);
		input->pagerank = (double *) _mm_malloc((input->N+input->O)*sizeof(double), 16);
		getPagerank_single(Pi0, Pik, P, input->eps, input->N, input->O, cnz, cz, nz, z, input->pagerank);
	}
	//precisione doppia
	else if(!input->format && input->prec){
		int *nz = (int *)_mm_malloc(input->N*sizeof(int), 16);
		int *z = (int *)_mm_malloc(input->N*sizeof(int), 16);
		int cnz = 0;
		int cz = 0;
		double *A = get_adiacency_matrix_double(input->N, input->M, input->G, &input->O, &cnz, &cz, z, nz);
		double *d = (double *)_mm_malloc(input->N*sizeof(double), 16);
		get_outdegree_double(input->N, cnz, A, d, input->O);
		double *P = get_matrix_P_double(input->N, cnz, A, d, input->O);
		/*for(int i = 0; i < (input->N); i++){
			for(int j = 0; j < (input->N); j++){
				if(P[i*(input->N)+j] != 0){
					printf("elemento P[%d][%d] = %f\n", i, j, P[i*(input->N)+j]);
				}
			}
		}*/
		double e = 1/(double)input->N;
		//get_matrix_P_primo_double(input->N, cnz, P, d, input->O);
		get_matrix_P_secondo_double(input->N, cnz, P, input->c, input->O);
		double* Pi0 =(double*)_mm_malloc((input->N+input->O)*sizeof(double),16);
		getVectorPiIn_double(input->N, e, input->O, Pi0);
		input->pagerank = (double *)_mm_malloc((input->N+input->O)*sizeof(double), 16);
		getPagerank_double(Pi0, input->pagerank, P, input->eps, input->N, input->O, cnz, cz, nz, z);
	}
	//verifica formato dense
	else {
		/*
		 * Seconda Parte Algoritmo
		 */
		//double *v = get_v_double(input->N);
		input->P = getMatrix(input->N, input->P, &input->O);
		double e = 1/(double)input->N;
		double* Pi0 =(double*)_mm_malloc((input->N+input->O)*sizeof(double),16);
		getVectorPiIn_double(input->N, e, input->O, Pi0);
		input->pagerank = (double *)_mm_malloc((input->N+input->O)*sizeof(double), 16);
		getPagerank_dense(Pi0, input->pagerank, input->P, input->eps, input->N, input->O);
	}

    //pagerank32(input); // Esempio di chiamata di funzione assembly

    // -------------------------------------------------

}
/*
 * Descrizione: serve per capire, dato un indice di riga,
 * il nuovo indice di riga nella matrice ridotta.
 */

int getRow(int i, int *nz, int cnz){
	for(int k = 0; k < cnz; k++){
		if(nz[k] == i){
			return k;
		}
	}
	return -1;
}

float* get_adiacency_matrix_single(int n, int m, location *l, int *o, int *cnz, int *cz, int *z, int *nz){
	/*
	* Matrice di adiacenza n*n, dove n è il numero di nodi
	* il puntatore g punta a un blocco di memoria di nodes*nodes elementi
	* dove ogni elemento occupa 4 byte. In questo momento la matrice contiene solo
	* 0 e 1, ma in seguito il risultato sarà la matrice P, che è una matrice di double.
	*/
	//Padding che rende le colonne della matrice multiple di 16
	/*Il padding comporta due benefici:
	 * 1) Elimina il ciclo resto
	 * 2) Elimina il ciclo quoziente nel loop unrolling
	 * All'inizio le colonne della matrice erano multiple di 4 perchè
	 * la loop vectorization prendeva 4 elementi per volta
	 * Ora diventano multiple di 16 dato che l'unrolling prende 16 elementi per volta
	 */
	*o = 0;
	int cols = n;
	if(n % 16 != 0){
		int a = n/16;
		a = a*16;
		int b = n-a;
		*o = 16-b;
		cols+=*o;
	}
	//contatore elementi nulli
	*cz = 0;
	//contatore elementi non nulli
	*cnz = 0;
	int trovato = 0;
	/*
	 * Per ogni nodo si va a vedere se ha archi in uscita
	 * Se sono presenti l'indice di riga è messo in nz
	 * Altrimenti è messo in z
	 */
	for(int i = 0; i < n; i++){
		for(int k = 0; k < m; k++){
			if((l[k].x-1) == i){
				nz[*cnz] = i;
				trovato = 1;
				*cnz += 1;
				break;
			}
		}
		if(!trovato){
			z[*cz] = i;
			*cz+=1;
		}
		trovato = 0;
	}

	/*
	 * A questo punto cnz contiene il numero di righe non nulle
	 * Quindi è possibile allocare una matrice di dimensione cnz*(n+o)
	 */

	float* s = (float *)_mm_malloc((*cnz)*cols*sizeof(float), 16);
	int i, j;
	for(int k = 0; k < m; k++){
		i = getRow(l[k].x-1, nz, *cnz);
		j = l[k].y-1;
		s[i*cols+j] = 1.0;
		/*
		 * Invece di calcolare l'outdegree successivamente, si sceglie di calcolarlo
		 * in questo momento per evitare di ricontrollare la matrice.
		 */
		//printf("%f\n", d[i]);

	}
	return s;
}

double* get_adiacency_matrix_double(int n, int m, location *l, int *o, int *cnz, int *cz, int *z, int *nz){
	*o = 0;
	int cols = n;
	//Padding che rende le colonne della matrice multiple di 8 elementi
	//Dato che il loop unrolling prende 8 elementi per volta
	if(cols % 8 != 0){
		int a = n/8;
		a = a*8;
		int b = n-a;
		*o = 8-b;
		cols+=*o;
	}
	int trovato = 0;
	/*
	 * Per ogni nodo si va a vedere se ha archi in uscita
	 * Se sono presenti l'indice di riga è messo in nz
	 * Altrimenti è messo in z
	 */
	for(int i = 0; i < n; i++){
		for(int k = 0; k < m; k++){
			if((l[k].x-1) == i){
				nz[*cnz] = i;
				trovato = 1;
				*cnz += 1;
				break;
			}
		}
		if(!trovato){
			z[*cz] = i;
			*cz+=1;
		}
		trovato = 0;
	}

	/*
	 * A questo punto cnz contiene il numero di righe non nulle
	 * Quindi è possibile allocare una matrice di dimensione cnz*(n+o)
	 */

	double* s = (double *)_mm_malloc((*cnz)*cols*sizeof(double), 16);
	int i, j;
	for(int k = 0; k < m; k++){
		i = getRow(l[k].x-1, nz, *cnz);
		j = l[k].y-1;
		s[i*cols+j] = 1.0;
		/*
		 * Invece di calcolare l'outdegree successivamente, si sceglie di calcolarlo
		 * in questo momento per evitare di ricontrollare la matrice.
		 */
		//printf("%f\n", d[i]);

	}
	return s;
}

double* getMatrix(int n, double *P, int *o){
		int cols = n;
		*o = 0;
		//Padding che rende le colonne della matrice multiple di 8 elementi
		if(cols % 8 != 0){
			int a = cols/8;
			a = a*8;
			int b = cols-a;
			*o = 8-b;
			cols+=*o;
		}
		MATRIX nData = alloc_matrix(n,cols);
		for(int i = 0; i < n; i++){
			for(int j = 0; j < n; j++){
				nData[i*cols+j] = P[i*n+j];
			}
		}
		return nData;
}

/*
 * Descrizione = funzione che calcola il vettore di outdegree
 * Ingresso = dimesione della matrice A (n), matrice di adiacenza A
 * Uscita = vettore degli outdegree d
 */

void get_outdegree_single(int n, int cnz, float *A, float *d, int o){
	//vettore di outdegree
	for(int i = 0; i < cnz; i++){
		int out = 0;
		for(int j = 0; j < n; j++){
			/*conta ogni volta che è presente un 1 nella riga i
			 * corrisponde al numero di archi uscenti da i
			*/
			if(A[i*(n+o) + j]){
				out++;
			}
		}
		//inserisce l'outdegree nel vettore d
		*(d+i) = out;
	}
}

void get_outdegree_double(int n, int cnz, double *A, double *d, int o){
	//vettore di outdegree
	for(int i = 0; i < cnz; i++){
		int out = 0;
		for(int j = 0; j < n; j++){
			if(A[i*(n+o) + j]){
				out++;
			}
		}
		//inserisce l'outdegree nel vettore
		*(d+i) = out;
	}
}

/*
 * Descrizione = funzione che ricava la matrice delle probabilità
 * di transizione P, a partire dal vettore di outdegree (d) e dalla matrice di
 * adiacenza (A)
 * Ingresso = dimesione matrice di adiacenza (n), matrice di adiacenza (A)
 * 				vettore di outdegree (d)
 * Uscita = matrice delle probabilità di transizione P = A/d
 */

float* get_matrix_P_single(int n, int cnz, float *A, float *d, int o){
	for(int i = 0; i < cnz; i++){
		for(int j = 0; j < n; j++){
			//la verifica serve per evitare divisioni inutili
			if(A[i*(n+o) + j] != 0){
				A[i*(n+o) + j] = A[i*(n+o) + j]/d[i];
			}
		}
	}
	return A;
}

double* get_matrix_P_double(int n, int cnz, double *A, double *d, int o){
	for(int i = 0; i < cnz; i++){
		for(int j = 0; j < n; j++){
			if(A[i*(n+o) + j] != 0){
				A[i*(n+o) + j] = A[i*(n+o) + j]/d[i];
			}
		}
	}
	return A;
}

/*
 * Descrizione = modifica la matrice P in modo che sia una
 * matrice di transizione P' valida.
 * Ingresso = dimensione matrice P (n), matrice di transizione (P), vettore degli
 * outdegree (d)
 * Post-Condizione = Matrice di transizione P valida
 */

void get_matrix_P_primo_single(int n, int cnz, float *P, float *d, int o){
	//verifica i nodi "i" che non hanno link di uscita
	for(int i = 0; i < cnz; i++){
		//se l'outdegree è nullo sostituisce la riga con v
		if(d[i] == 0){
			for(int j = 0; j < n; j++){
				P[i*(n+o) + j] = 1/(float)n;
			}
		}
	}
}

void get_matrix_P_primo_double(int n, int cnz, double *P, double *d, int o){
	//verifica i nodi "i" che non hanno link di uscita
	for(int i = 0; i < cnz; i++){
		if(d[i] == 0){
			for(int j = 0; j < n; j++){
				P[i*(n+o) + j] = 1/(double)n;
			}
		}
	}
}

void get_matrix_P_secondo_single(int n, int cnz, float* P, double c, int o){

	/*Andiamo a calcolare P'' a precisione singola seguendo la formula P''=cP'+(1-c)E
		 * dove c è un valore compreso tra [0,1] che noi considereremo pari a 0.85
		 * */
	/*Andiamo a calcolare separatamente c*P' e (1-c)*E per poi andare a sommare i risultati*/

	float e = (1-c)*(1/(float)n);
	for(int i=0; i<cnz; i++){
		for(int j=0; j<n; j++){
			P[i*(n+o)+j]=c*P[i*(n+o)+j] + e;// P2[i*n+j]=c*P1[i*n+j] + (1-c)*(1/n);
		}
	}
}

void get_matrix_P_secondo_double(int n, int cnz, double* P, double c, int o){

	/*Andiamo a calcolare P'' a precisione singola seguendo la formula P''=cP'+(1-c)E
		 * dove c è un valore compreso tra [0,1] che noi considereremo pari a 0.85
		 * */
	/*Andiamo a calcolare separatamente c*P' e (1-c)*E per poi andare a sommare i risultati*/
	double e = (1-c)*(1/(double)n);
	for(int i=0; i<cnz; i++){
		for(int j=0; j<n; j++){
			P[i*(n+o)+j]=c*P[i*(n+o)+j] + e;// P1[i*n+j]=c*P1[i*n+j] + (1-c)*(1/n);
		}
	}
}

void getVectorPiIn_single(int n, float e, int o, float *Pi){
	for (int i=0; i<n; i++){
		Pi[i]=e;
	}

	/*for(int i = 0; i < o; i++){
		Pi[n+i] = 0;
	}*/
}

void getVectorPiIn_double(int n, double e, int o, double *Pi){
	for (int i=0; i<n; i++){
		Pi[i]=e;
	}
	//Serve in assembly per evitare che ci sia 1/n al posto degli zeri di padding
	/*for(int i = 0; i < o; i++){
		Pi[n+i] = 0;
	}*/
}

void getVectorPik_single(float *P, float *Pi0, float *Pik, int n, int o, int cnz, int cz, int *nz, int *z){
	for(int i = 0; i < cnz; i++){
		Pik[nz[i]] = 0;
		for(int j = 0; j < cnz; j++){
			Pik[nz[i]] += P[j*(n+o)+nz[i]]*Pi0[nz[j]];
		}
		for(int j = 0; j < cz; j++){
			Pik[nz[i]] += (1/(float)n)*Pi0[z[j]];
		}
	}
	for(int i = 0; i < cz; i++){
		Pik[z[i]] = 0;
		for(int j = 0; j < cnz; j++){
			Pik[z[i]] += P[j*(n+o)+z[i]]*Pi0[nz[j]];
		}
		for(int j = 0; j < cz; j++){
			Pik[z[i]] += (1/(float)n)*Pi0[z[j]];
		}
	}
}

void getVectorPik_double(double *P, double *Pi0, double *Pik, int n, int o, int cnz, int cz, int *nz, int *z){
	for(int i = 0; i < cnz; i++){
			Pik[nz[i]] = 0;
			for(int j = 0; j < cnz; j++){
				Pik[nz[i]] += P[j*(n+o)+nz[i]]*Pi0[nz[j]];
			}
			for(int j = 0; j < cz; j++){
				Pik[nz[i]] += (1/(double)n)*Pi0[z[j]];
			}
		}
		for(int i = 0; i < cz; i++){
			Pik[z[i]] = 0;
			for(int j = 0; j < cnz; j++){
				Pik[z[i]] += P[j*(n+o)+z[i]]*Pi0[nz[j]];
			}
			for(int j = 0; j < cz; j++){
				Pik[z[i]] += (1/(double)n)*Pi0[z[j]];
			}
		}
}

void getVectorPik_dense(double *P, double *Pi0, double *Pik, int n, int o){
	for(int i = 0; i < n; i++){
		Pik[i] = 0;
		for(int j = 0; j < n; j++){
			Pik[i] += P[j*(n+o) + i]*Pi0[j];
		}
	}
}

void getPagrnk_single(int n, float *Pik){
	float somma = 0;
	for(int i = 0; i < n; i++)
		somma += fabsf(Pik[i]);
	for(int i = 0; i < n; i++)
		Pik[i] = Pik[i]/(float)somma;
}

void getPagrnk_double(int n, double *Pik){
	double somma = 0;
	for(int i = 0; i < n; i++)
		somma += fabs(Pik[i]);
	for(int i = 0; i < n; i++)
		Pik[i] = Pik[i]/(double)somma;
}

void getDelta_single(float *Pi0, float *Pik, int n, float *delta){
	for(int i = 0; i < n; i++){
			*delta += fabsf(Pi0[i]-Pik[i]);
			Pi0[i] = Pik[i];
	}
}

void getDelta_double(double *Pi0, double *Pik, int n, double *delta){
	for(int i = 0; i < n; i++){
			*delta += fabs(Pi0[i]-Pik[i]);
			Pi0[i] = Pik[i];
	}
}

void cvtPagerank(int n, float *Pik, double *Piconv){
	for(int i = 0; i < n; i++){
		Piconv[i] = (double) Pik[i];
	}
}


void getPagerank_single(float *Pi0, float *Pik, float *P, double eps, int n, int o, int cnz, int cz, int *nz, int *z, double *Piconv){
	/*
	 * Se il valore delta calcolato è minore di epsilon
	 * allora siamo arrivati all'iterazione che ci fa ottenere
	 * il vettore dei pagerank. Altrimenti si aggiorna Pi0 che conterrà
	 * l'iterazione attuale e si esegue un'altra iterazione.
	 */
	float delta = 0;
	getVectorPik_single(P, Pi0, Pik, n, o, cnz, cz, nz, z);
	getDelta_single(Pi0, Pik, n, &delta);
	while(delta > eps){
		getVectorPik_single(P, Pi0, Pik, n, o, cnz, cz, nz, z);
		/*
		 * Calcolo del valore delta = ||Pi(k) - Pi(k+1)||1
		 */
		delta = 0;
		getDelta_single(Pi0, Pik, n, &delta);
	}
	getPagrnk_single(n,Pik);
	cvtPagerank(n, Pik, Piconv);
}

void getPagerank_double(double *Pi0, double *Pik, double *P, double eps, int n, int o, int cnz, int cz, int *nz, int *z){
	/*
		 * Se il valore delta calcolato è minore di epsilon
		 * allora siamo arrivati all'iterazione che ci fa ottenere
		 * il vettore dei pagerank. Altrimenti si aggiorna Pi0 che conterrà
		 * l'iterazione attuale e si esegue un'altra iterazione.
		 */
		double delta = 0;
		getVectorPik_double(P, Pi0, Pik, n, o, cnz, cz, nz, z);
		getDelta_double(Pi0, Pik, n, &delta);
		while(delta > eps){
			getVectorPik_double(P, Pi0, Pik, n, o, cnz, cz, nz, z);
			/*
			 * Calcolo del valore delta = ||Pi(k) - Pi(k+1)||1
			 */
			delta = 0;
			getDelta_double(Pi0, Pik, n, &delta);
		}
		getPagrnk_double(n,Pik);
}


void getPagerank_dense(double *Pi0, double *Pik, double *P, double eps, int n, int o){
	/*
	 * Se il valore delta calcolato è minore di epsilon
	 * allora siamo arrivati all'iterazione che ci fa ottenere
	 * il vettore dei pagerank. Altrimenti si aggiorna Pi0 che conterrà
	 * l'iterazione attuale e si esegue un'altra iterazione.
	 */
	double delta = 0;
	getVectorPik_dense(P, Pi0, Pik, n, o);
	getDelta_double(Pi0, Pik, n, &delta);
	while(delta > eps){
		getVectorPik_dense(P, Pi0, Pik, n, o);
		/*
		 * Calcolo del valore delta = ||Pi(k) - Pi(k+1)||1
		 */
		delta = 0;
		getDelta_double(Pi0, Pik, n, &delta);
	}
	getPagrnk_double(n,Pik);
}


#define	SPARSE	0
#define	DENSE	1
#define	SINGLE	0
#define	DOUBLE	1
#define	NOPT	0
#define	OPT		1

int main(int argc, char** argv) {
	
	//alloca un blocco di memoria, puntato da input, che contiene una struct params
	params* input = malloc(sizeof(params));

	/*Inizializzazione membri della struct
	 * format indica il formato sparse o dense, default = sparse
	 * precisione di default = singola
	 */
	input->file_name = NULL;
	input->P = NULL; // dense format
	input->G = NULL; // sparse format
	input->N = 0; // number of nodes
	input->M = 0; // number of arcs
	input->c = 0.85;
	input->eps = 1e-5;
	input->format = SPARSE; // 0=sparse, 1=dense
	input->prec = SINGLE; // 0=single, 1=double
	input->opt = NOPT; // 0=nopt, 1=opt
	input->silent = 0; // 0=false,<>0=true
	input->display = 0; // 0=false <>0=true

	int i, j;

	int par = 1;
	while (par < argc) {
		if (par == 1) {
			input->file_name = argv[par];
			par++;
		} else if (strcmp(argv[par],"-s") == 0) {
			input->silent = 1;
			par++;
		} else if (strcmp(argv[par],"-d") == 0) {
			input->display = 1;
			par++;
		} else if (strcmp(argv[par],"-c") == 0) {
			par++;
			if (par >= argc) {
				printf("Missing c value!\n");
				exit(1);
			}
			input->c = atof(argv[par]);
			par++;
		} else if (strcmp(argv[par],"-eps") == 0) {
			par++;
			if (par >= argc) {
				printf("Missing eps value!\n");
				exit(1);
			}
			input->eps = atof(argv[par]);
			par++;
		} else if (strcmp(argv[par],"-sparse") == 0) {
			input->format = SPARSE;
			par++;
		} else if (strcmp(argv[par],"-dense") == 0) {
			input->format = DENSE;
			par++;
		} else if (strcmp(argv[par],"-single") == 0) {
			input->prec = SINGLE;
			par++;
		} else if (strcmp(argv[par],"-double") == 0) {
			input->prec = DOUBLE;
			par++;
		} else if (strcmp(argv[par],"-nopt") == 0) {
			input->opt = NOPT;
			par++;
		} else if (strcmp(argv[par],"-opt") == 0) {
			input->opt = OPT;
			par++;
		} else
			par++; //ignora i parametri che non rispettano le regole
	}
	
	if (!input->silent) {
		printf("Usage: %s <input_file_name> [-d][-s][-sparse|-dense][-single|-double][-nopt|-opt][-c <value>][-eps <value>]\n", argv[0]);
		printf("\nParameters:\n");
		printf("\t-d : display input and output\n");
		printf("\t-s : silent\n");
		printf("\t-sparse/-full: input format (sparse=list of arcs,full=matrix)\n");
		printf("\t-single/-double: floating-point precision (only sparse format)\n");
		printf("\t-nopt/-opt: disable/enable optimizations\n");
		printf("\t-c <value> : 1-teleportation_probability (default 0.85)\n");
		printf("\t-eps <value> : termination error (default 1e-5)\n");
		printf("\n");
	}
	
	if (input->file_name == NULL || strlen(input->file_name) == 0) {
		printf("Missing input file name!\n");
		exit(1);
	}
	
	if (input->format == 0)
		input->G = load_sparse(input->file_name, &input->N, &input->M);
	else
		input->P = load_dense(input->file_name, &input->N, &input->M);
		
	if (!input->silent) {
		printf("Input file name: '%s'\n", input->file_name);
		printf("Number of nodes: %d\n", input->N);
		printf("Number of arcs: %d\n", input->M);
		printf("Parameter c: %f\n", input->c);
		printf("Parameter eps: %f\n", input->eps);
	}
	
	clock_t t = clock();
	pagerank(input);
	t = clock() - t;
	
	if (!input->silent)
		printf("\nExecution time = %.3f seconds\n", ((float)t)/CLOCKS_PER_SEC);
	else
		printf("%.3f\n", ((float)t)/CLOCKS_PER_SEC);
			
	if (input->pagerank != NULL){
		if (!input->silent && input->display) {
			printf("\nPageRanks:\n");
			for (i = 0; i < input->N; i++) {
				printf("%d %.14g\n", i+1, input->pagerank[i]);
			}
		}
		save_pageranks(input->file_name, input->N, input->pagerank);
	}
	
	return 0;
}