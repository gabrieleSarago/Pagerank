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
#define	GRAPHD	double*	// DECIDERE LA RAPPRESENTAZIONE IN MEMORIA (dev'essere un puntatore) (float* o double*)
#define GRAPHS	float* //rappresentazione in precisione singola

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
	GRAPHD G; // codifica full (sparse) double
	GRAPHS S; // codifica sparse single
	int N; // numero di nodi
	int M; // numero di archi
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

/*
 * TODO: possibile accorpamento delle seguenti 4 funzioni in due.
 * Fare chiamate a più funzioni comporta allocare più activation frame
 * nello stack, e questo fa sprecare tempo. Bisogna attuare un compromesso tra
 * velocità di esecuzione e modularità del codice.
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
	*m = cols; //(?)
	
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
GRAPHD load_sparse_double(char* filename, int *n, int *m) {
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

	/*
	 * Matrice di adiacenza n*n, dove n è il numero di nodi
	 * il puntatore g punta a un blocco di memoria di nodes*nodes elementi
	 * dove ogni elemento occupa 8 byte. In questo momento la matrice contiene solo
	 * 0 e 1, ma in seguito il risultato sarà la matrice P, che è una matrice di double.
	 */
	GRAPHD g = (GRAPHD)_mm_malloc(nodes*nodes*sizeof(double),16); //alloca la struttura dati contenente il grafo
	/*
	 * Nota: per accedere all'elemento g[i][j]
	 * bisogna fare g[i*nodes + j]
	 * perchè si tratta di una allocazione di una matrice, simulata
	 * con un vettore, usando il calcolo esplicito degli elementi.
	 */
	/*for(int i = 12000; i < nodes; i++){
		for(int j = 12000; j < nodes; j++){
			printf("g[%d][%d] = %f\n",i,j,g[i*nodes+j]);
		}
	}*/
	for (i = 0; i < arcs; i++) {
		status = fread(&sorg, sizeof(int), 1, fp);
		status = fread(&dest, sizeof(int), 1, fp);

		// aggiungi l'arco (sorg,dest) a g
		/*
		 * Riga = sorg, colonna = dest, g[sorg][dest] = 1
		 */
		/*if(sorg == 2317 && dest == 2306){
			printf("Trovato! dest = %d\n",dest);
		}*/
		//printf("sorg = %d dest = %d\n", sorg, dest);

		g[(sorg-1)*nodes + (dest-1)] = 1;
	}

	fclose(fp);

	*n = nodes;
	*m = arcs;
	return g;
}

/*
 * Versione a precisione singola
 */

GRAPHS load_sparse_single(char* filename, int *n, int *m){
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

	/*
	 * Matrice di adiacenza n*n, dove n è il numero di nodi
	 * il puntatore g punta a un blocco di memoria di nodes*nodes elementi
	 * dove ogni elemento occupa 4 byte. In questo momento la matrice contiene solo
	 * 0 e 1, ma in seguito il risultato sarà la matrice P, che è una matrice di double.
	 * TODO: bisogna convertire la matrice da float* a double*
	 */
	GRAPHS s = (GRAPHS)_mm_malloc(nodes*nodes*sizeof(float), 16); //alloca la struttura dati contenente il grafo
	/*
	 * Anche in questo caso s[i][j] = s[i*nodes + j]
	 */
	for (i = 0; i < arcs; i++) {
		status = fread(&sorg, sizeof(int), 1, fp);
		status = fread(&dest, sizeof(int), 1, fp);
		// aggiungi l'arco (sorg,dest) a s
		s[(sorg-1)*nodes + (dest-1)] = 1;
	}
	fclose(fp);

	*n = nodes;
	*m = arcs;

	return s;
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

float* get_outdegree_single(int n, float *A);
float* get_matrix_P_single(int n, float *A, float *d);
void get_matrix_P_primo_single(int n, float *P, float *d);
float* get_matriceTeletrasporto_single(int n, float *v);
void get_matrix_P_secondo_single(int n, float *P, float *E, double c);
float* getVectorPiIn_single(int n, float *v);
float* get_v_single(int n);
double* getPagerank_single(float *Pi0, float *P, double eps, int n);

double* get_outdegree_double(int n, double *A);
double* get_matrix_P_double(int n, double *A, double *d);
void get_matrix_P_primo_double(int n, double *P, double *d);
double* get_matriceTeletrasporto_double(int n, double *v);
void get_matrix_P_secondo_double(int n, double *P, double *E, double c);
double* getVectorPiIn_double(int n, double *v);
double* get_v_double(int n);
double* getPagerank_double(double *Pi0, double *P, double eps, int n);

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
	if(input->S != NULL){
		float *d = get_outdegree_single(input->N, input->S);
		//matrice P
		float *P = get_matrix_P_single(input->N, input->S, d);
		/*TEST matrice P
		Basta sostituire P con input->S per testare la matrice di adiacenza.
		for(int i = 0; i < (input->N); i++){
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
		get_matrix_P_primo_single(input->N, P, d);
		float *v = get_v_single(input->N);
		float *E = get_matriceTeletrasporto_single(input->N, v);
		get_matrix_P_secondo_single(input->N, P, E, input->c);
		float *Pi0 = getVectorPiIn_single(input->N, v);
		input->pagerank = getPagerank_single(Pi0, P, input->eps, input->N);
	}
	//precisione doppia
	else if(input->G != NULL){
		//TEST con grafo di piccole dimensioni
		/*input->N = 4;
		input->M = 6;
		double m[4][4] = {{0,1,1,1},
						  {1,0,1,0},
						  {0,0,0,0},
						  {1,0,0,0}};
		input->G = (double *) m;*/
		double *d = get_outdegree_double(input->N, input->G);
		double *P = get_matrix_P_double(input->N, input->G, d);
		/*for(int i = 0; i < (input->N); i++){
			for(int j = 0; j < (input->N); j++){
				if(P[i*(input->N)+j] != 0){
					printf("elemento P[%d][%d] = %f\n", i, j, P[i*(input->N)+j]);
				}
			}
		}*/
		get_matrix_P_primo_double(input->N, P, d);
		/*for(int i = 0; i < (input->N); i++){
			for(int j = 0; j < (input->N); j++){
				if(P[i*(input->N)+j] != 0){
					printf("elemento P'[%d][%d] = %f\n", i, j, P[i*(input->N)+j]);
				}
			}
		}*/
		double *v = get_v_double(input->N);
		double *E = get_matriceTeletrasporto_double(input->N, v);
		get_matrix_P_secondo_double(input->N, P, E, input->c);
		/*for(int i = 0; i < (input->N); i++){
			for(int j = 0; j < (input->N); j++){
				printf("elemento P''[%d][%d] = %f\n", i, j, P[i*(input->N)+j]);
			}
		}*/
		double *Pi0 = getVectorPiIn_double(input->N, v);
		input->pagerank = getPagerank_double(Pi0, P, input->eps, input->N);
	}
	//verifica formato dense
	else{
		/*
		 * Seconda Parte Algoritmo
		 */
		double *v = get_v_double(input->N);
		double *Pi0 = getVectorPiIn_double(input->N, v);
		input->pagerank = getPagerank_double(Pi0, input->P, input->eps, input->N);

	}

    //pagerank32(input); // Esempio di chiamata di funzione assembly

    // -------------------------------------------------

}

/*
 * Descrizione = funzione che calcola il vettore di outdegree
 * Ingresso = dimesione della matrice A (n), matrice di adiacenza A
 * Uscita = vettore degli outdegree d
 */

float* get_outdegree_single(int n, float *A){
	//vettore di outdegree
	float *d = (float *)_mm_malloc(n*sizeof(float), 16);
	for(int i = 0; i < n; i++){
		int out = 0;
		for(int j = 0; j < n; j++){
			/*conta ogni volta che è presente un 1 nella riga i
			 * corrisponde al numero di archi uscenti da i
			*/
			if(A[i*n + j]){
				out++;
			}
		}
		//inserisce l'outdegree nel vettore d
		*(d+i) = out;
	}
	return d;
}

double* get_outdegree_double(int n, double *A){
	//vettore di outdegree
	double *d = (double *)_mm_malloc(n*sizeof(double), 16);
	for(int i = 0; i < n; i++){
		int out = 0;
		for(int j = 0; j < n; j++){
			if(A[i*n + j]){
				out++;
			}
		}
		//inserisce l'outdegree nel vettore
		*(d+i) = out;
	}
	return d;
}

/*
 * Descrizione = funzione che ricava la matrice delle probabilità
 * di transizione P, a partire dal vettore di outdegree (d) e dalla matrice di
 * adiacenza (A)
 * Ingresso = dimesione matrice di adiacenza (n), matrice di adiacenza (A)
 * 				vettore di outdegree (d)
 * Uscita = matrice delle probabilità di transizione P = A/d
 */

float* get_matrix_P_single(int n, float *A, float *d){
	float *P = (float *)_mm_malloc(n*n*sizeof(float), 16);
	for(int i = 0; i < n; i++){
		for(int j = 0; j < n; j++){
			//la verifica serve per evitare divisioni inutili
			if(A[i*n + j] != 0){
				P[i*n + j] = A[i*n + j]/d[i];
			}
		}
	}
	return P;
}

double* get_matrix_P_double(int n, double *A, double *d){
	double *P = (double *)_mm_malloc(n*n*sizeof(double), 16);
	for(int i = 0; i < n; i++){
		for(int j = 0; j < n; j++){
			if(A[i*n + j] != 0){
				P[i*n + j] = A[i*n + j]/d[i];
			}
		}
	}
	return P;
}

/*
 * Descrizione = modifica la matrice P in modo che sia una
 * matrice di transizione P' valida.
 * Ingresso = dimensione matrice P (n), matrice di transizione (P), vettore degli
 * outdegree (d)
 * Post-Condizione = Matrice di transizione P valida
 */

void get_matrix_P_primo_single(int n, float *P, float *d){
	//verifica i nodi "i" che non hanno link di uscita
	for(int i = 0; i < n; i++){
		//se l'outdegree è nullo sostituisce la riga con v
		if(d[i] == 0){
			for(int j = 0; j < n; j++){
				P[i*n + j] = 1/(float)n;
			}
		}
	}
}

void get_matrix_P_primo_double(int n, double *P, double *d){
	//verifica i nodi "i" che non hanno link di uscita
	for(int i = 0; i < n; i++){
		if(d[i] == 0){
			for(int j = 0; j < n; j++){
				P[i*n + j] = 1/(double)n;
			}
		}
	}
}

float* get_matriceTeletrasporto_single(int n, float *v){
/*Dobbiamo ricavare la matrice E detta MATRICE DI TELETRASPORTO;
	  * E è uguale al prodotto tra i 2 vettori u e v
	  * vettore riga di personalizzazione -> v=(1/n,1/n,....,1/n)
	  * u è un vettore colonna i cui elementi sono tutti 1 -> u=(1,1,....,1)^T(trasposto)
	  * E sarà una matrice con un numero di righe pari agli elementi di u e un numero di colonne
	  * pari agli elementi di v
	  * */
	int* u=(int*)_mm_malloc(n*sizeof(int),16);
	for (int j=0;j<n;j++){
		u[j]=1;
	}

	float* E=(float*)_mm_malloc(n*n*sizeof(float),16);
		for(int i=0; i<n;i++){
			for(int j=0;j<n;j++){
				E[i*n+j]=u[i]*v[j];
			}
		}
	return E;

}

double* get_matriceTeletrasporto_double(int n, double *v){
/*Dobbiamo ricavare la matrice E detta MATRICE DI TELETRASPORTO;
	  * E è uguale al prodotto tra i 2 vettori u e v
	  * vettore riga di personalizzazione -> v=(1/n,1/n,....,1/n)
	  * u è un vettore colonna i cui elementi sono tutti 1 -> u=(1,1,....,1)^T(trasposto)
	  * E sarà una matrice con un numero di righe pari agli elementi di u e un numero di colonne
	  * pari agli elementi di v
	  * */

	// int u[1][n];
	int* u=(int*)_mm_malloc(n*sizeof(int),16);
	for (int j=0;j<n;j++){
		u[j]=1;
	}

	double* E=(double*)_mm_malloc(n*n*sizeof(double),16);
		for(int i=0; i<n;i++){
			for(int j=0;j<n;j++){
				E[i*n+j]=u[i]*v[j];
			}
		}
	return E;

}

float* get_v_single(int n){
	float *v = (float*)_mm_malloc(n*sizeof(float),16);
	for (int i=0; i <n;i++){
		v[i]=1/(float)n;
	}
	return v;
}

double* get_v_double(int n){
	double *v = (double*)_mm_malloc(n*sizeof(double),16);
	for (int i=0; i <n;i++){
		v[i]=1/(double)n;
	}
	return v;
}

void get_matrix_P_secondo_single(int n, float* P, float* E, double c){

	//float c= rand()/(float)RAND_MAX; <- non va scritto, viene dato da input
	//float c=0.85;// il parametro deve poter variare
	/*Andiamo a calcolare P'' a precisione singola seguendo la formula P''=cP'+(1-c)E
		 * dove c è un valore compreso tra [0,1] che noi considereremo pari a 0.85
		 * */
	/*Andiamo a calcolare separatamente c*P' e (1-c)*E per poi andare a sommare i risultati*/

		for(int i=0; i<n; i++){
			for(int j=0; j<n; j++){
				P[i*n+j]=c*P[i*n+j];// P1[i*n+j]=c*P1[i*n+j];
			}
		}

		for(int i=0; i<n; i++){
			for(int j=0; j<n; j++){
				E[i*n+j]=(1-c)*E[i*n+j];
			}
		}

		//sommiamo le 2 matrici risultanti per ottenere P''
		for (int i=0; i<n; i++){
			for(int j=0; j<n; j++){
				P[i*n+j]+=E[i*n+j];
			}
		}

}

void get_matrix_P_secondo_double(int n, double* P, double* E, double c){
	//double c= rand()/(double)RAND_MAX;

	//double c=0.85; il parametro c deve poter variare
	/*Andiamo a calcolare P'' a precisione singola seguendo la formula P''=cP'+(1-c)E
		 * dove c è un valore compreso tra [0,1] che noi considereremo pari a 0.85
		 * */
	/*Andiamo a calcolare separatamente c*P' e (1-c)*E per poi andare a sommare i risultati*/
	for(int i=0; i<n; i++){
		for(int j=0; j<n; j++){
			P[i*n+j]=c*P[i*n+j];// P1[i*n+j]=c*P1[i*n+j];
		}
	}

	for(int i=0; i<n; i++){
		for(int j=0; j<n; j++){
			E[i*n+j]=(1-c)*E[i*n+j];
			//printf("(1-c)*E[%d][%d] = %.14f\n",i,j,E[i*n+j]);
		}
	}

	//sommiamo le 2 matrici risultanti per ottenere P''
	for (int i=0; i<n; i++){
		for(int j=0; j<n; j++){
			P[i*n+j]+= E[i*n+j];
		}
	}
}

float* getVectorPiIn_single(int n, float *v){
	float* Pi=(float*)_mm_malloc(sizeof(float)*n,16);
	for (int i=0; i<n; i++){
		Pi[i]=v[i];
	}
	return Pi;
}

double* getVectorPiIn_double(int n, double *v){
	double* Pi=(double*)_mm_malloc(sizeof(double)*n,16);
	for (int i=0; i<n; i++){
		Pi[i]=v[i];
	}
	return Pi;
}

void getVectorPik_single(float *P, float *Pi0, float *Pik, int n){
	for(int i = 0; i < n; i++){
		Pik[i] = 0;
		for(int j = 0; j < n; j++){
			Pik[i] += P[j*n + i]*Pi0[j];
		}
	}
}

void getVectorPik_double(double *P, double *Pi0, double *Pik, int n){
	for(int i = 0; i < n; i++){
		Pik[i] = 0;
		for(int j = 0; j < n; j++){
			Pik[i] += P[j*n + i]*Pi0[j];
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


double* getPagerank_single(float *Pi0, float *P, double eps, int n){
	int stop = 0;
	float delta = 0;
	float *Pik = (float *)_mm_malloc(n*sizeof(float), 16);
	while(!stop){
		getVectorPik_single(P, Pi0, Pik, n);
		/*
		 * Calcolo del valore delta = ||Pi(k) - Pi(k+1)||1
		 */
		delta = 0;
		for(int i = 0; i < n; i++){
			delta += fabsf(Pi0[i]-Pik[i]);
		}
		printf("delta = %f\n", delta);
		/*
		 * Se il valore delta calcolato è minore di epsilon
		 * allora siamo arrivati all'iterazione che ci fa ottenere
		 * il vettore dei pagerank. Altrimenti si aggiorna Pi0 che conterrà
		 * l'iterazione attuale e si esegue un'altra iterazione.
		 */

		for(int i = 0; i < n; i++){
			Pi0[i] = Pik[i];
		}
		if(delta < eps){
			stop = 1;
			break;
		}
	}
	getPagrnk_single(n,Pik);
	double *Piconv = (double *) _mm_malloc(n*sizeof(double), 16);
	for(int i = 0; i < n; i++){
		Piconv[i] = (double) Pik[i];
	}
	return Piconv;
}

double* getPagerank_double(double *Pi0, double *P, double eps, int n){
	int stop = 0;
	double delta = 0;
	double *Pik = (double *)_mm_malloc(n*sizeof(double), 16);
	while(!stop){
		getVectorPik_double(P, Pi0, Pik, n);
		/*
		 * Calcolo del valore delta = ||Pi(k) - Pi(k+1)||1
		 */
		delta = 0;
		for(int i = 0; i < n; i++){
			delta += fabs(Pi0[i]-Pik[i]);
		}
		//printf("delta = %.14f\n",delta);
		/*
		 * Se il valore delta calcolato è minore di epsilon
		 * allora siamo arrivati all'iterazione che ci fa ottenere
		 * il vettore dei pagerank. Altrimenti si aggiorna Pi0 che conterrà
		 * l'iterazione attuale e si esegue un'altra iterazione.
		 */
		/*
		 * Aggiorna i valori della corrente iterazione
		 */
		for(int i = 0; i < n; i++){
			Pi0[i] = Pik[i];
		}
		if(delta < eps){
			stop = 1;
			break;
		}
	}
	getPagrnk_double(n,Pik);
	return Pik;
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
	input->G = NULL; // sparse double format
	input->S = NULL; // sparse single format
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
	
	if (input->format == 0){
		if(input->prec == SINGLE)
			input->S = load_sparse_single(input->file_name, &input->N, &input->M);
		else
			input->G = load_sparse_double(input->file_name, &input->N, &input->M);
	}
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
			
	if (input->pagerank != NULL)
	{
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
