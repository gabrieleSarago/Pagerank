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

#define	MATRIX	double*
#define	VECTOR	double*
#define	GRAPH	location*

/*
 * Struct riferita agli archi del grafo, dove x è il nodo sorgente
 * mentre y è il nodo destinazione.
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
	int NO; // padding
	double c; // default=0.85
	double eps; // default=1e-5
	int format; // 0=sparse, 1=full
	int prec; // 0=single, 1=double
	int opt; // 0=nopt, 1=opt
	double* pagerank; //output dell'algoritmo
	int silent;
	int display;
} params;


/*
 * 
 *	Le funzioni sono state scritte assumendo che le matrici siano memorizzate
 * 	mediante un array (float*), questo fa in modo che gli elementi della matrice
 * 	siano contigue e questo può velocizzare l'accesso alla matrice grazie all'utilizzo
 * 	della cache.
 * 
 * 	L'ordine di memorizzazione della matrice è per righe (row-major order)
 * 
 */

void* get_block(int size, int elements) {
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
 * 	successivi N*M*8 byte: matrix data in row-major order --> numeri floating-point a precisione doppia
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
	sprintf(fpath, "%s.matrix", filename);
	fp = fopen(fpath, "rb");

	if (fp == NULL) {
		printf("'%s' : bad matrix file name!\n", fpath);
		exit(0);
	}
	status = fread(&rows, sizeof(int), 1, fp);
	status = fread(&cols, sizeof(int), 1, fp);

	MATRIX data = alloc_matrix(rows,cols);
	status = fread(data, sizeof(double), rows*cols, fp);
	fclose(fp);

	*n = rows;
	*m = rows*cols;
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
	/*
	 * Si salva il contenuto del file tramite una lista di struct location
	 */
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
float* get_adiacency_matrix_single(int n, int m, location *l, int *no, float *d);
extern void get_matrix_P_single(int n, float *A, float *d, int no, double c, float e, float b);
extern void getVectorPiIn_single(int n, float e, int no, float *Pi);
extern void getVectorPik_single(float *P, float *Pi0, float *Pik, int n, int no);
extern void getPagrnk_single(int n, float *Pik);
extern void getDelta_single(float *Pi0, float *Pik, int n, float *delta);
extern void cvtPagerank(int n, float *Pik, double *Piconv);
void getPagerank_single(float *Pi0, float *Pik, float *P, double eps, int n, int no, double *Piconv);

double* get_adiacency_matrix_double(int n, int m, location *l, int *no, double *d);
double* getMatrix(int n, double *P, int *no);
extern void get_matrix_P_double(int n, double *A, double *d, int no, double c, double e, double b);
extern void getVectorPiIn_double(int n, double e, int no, double *Pi);
extern void getVectorPik_double(double *P, double *Pi0, double *Pik, int n, int no);
extern void getPagrnk_double(int n, double *Pik);
extern void getDelta_double(double *Pi0, double *Pik, int n, double *delta);
void getPagerank_double(double *Pi0, double *Pik, double *P, double eps, int n, int no);


void pagerank(params* input) {
	
    /*
     * 1° step: passare dal grafo G, inteso come lista di struct location, alla matrice P, utilizzando l'outdegree.
     * In particolare si vuole ottenere prima il vettore degli outdegree e poi imporre:
     * P[i][j] = A[i][j]/outdegree[i];
     * A differenza della versione precedente, dove l'outdegree veniva calcolato in una funzione apposita
     * Si sceglie di accorpare questa operazione a quella della memorizzazione della matrice sparsa.
     * In questo modo si evita di ricontrollare la matrice.
     */
	//verifica formato sparse single o double
	//precisione singola
	if(!input->format && !input->prec){
		/*
		 * Si costruisce la matrice di adiacenza a partire da una lista di struct location
		 * dove ogni location corrisponde a un arco dal nodo x al nodo y.
		 */
		float *d = (float *)_mm_malloc(input->N*sizeof(float), 16);
		float *P = get_adiacency_matrix_single(input->N, input->M, input->G, &input->NO, d);
		//get_outdegree_single(input->N, A, d, input->NO);
		float b = 1/(float)input->N;
		float e = (1-input->c)*b;
		//Dalla matrice di adiacenza a P''
		clock_t t = clock();
		get_matrix_P_single(input->N, P, d, input->NO, input->c, e, b);
		t = clock() - t;
		printf("\nExecution time = %.3f seconds\n", ((float)t)/CLOCKS_PER_SEC);
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
		/*
		 * A differenza della precedente versione, si provvede a un calcolo diretto dalla matrice
		 * di adiacenza alla matrice di transizione P'', i cui elementi sono non-negativi.
		 * Come per l'outdegree, in questo modo si evita di rivedere la matrice più volte.
		 */
		//get_matrix_P_primo_single(input->N, P, d, input->NO);
		//get_matrix_P_secondo_single(input->N, P, input->c, input->NO);
		float* Pi0 =(float*)_mm_malloc(input->NO*sizeof(float),16);
		getVectorPiIn_single(input->N, b, input->NO, Pi0);
		float *Pik = (float *)_mm_malloc(input->NO*sizeof(float), 16);
		input->pagerank = (double *) _mm_malloc(input->NO*sizeof(double), 16);
		getPagerank_single(Pi0, Pik, P, input->eps, input->N, input->NO, input->pagerank);
	}
	//precisione doppia
	else if(!input->format && input->prec){
		double *d = (double *)_mm_malloc(input->N*sizeof(double), 16);
		double *P = get_adiacency_matrix_double(input->N, input->M, input->G, &input->NO, d);
		double b = 1/(double)input->N;
		double e = (1-input->c)*b;
		get_matrix_P_double(input->N, P, d, input->NO, input->c, e, b);
		double* Pi0 =(double*)_mm_malloc(input->NO*sizeof(double),16);
		getVectorPiIn_double(input->N, b, input->NO, Pi0);
		input->pagerank = (double *)_mm_malloc(input->NO*sizeof(double), 16);
		getPagerank_double(Pi0, input->pagerank, P, input->eps, input->N, input->NO);
	}
	//verifica formato dense
	else {
		input->P = getMatrix(input->N, input->P, &input->NO);
		double e = 1/(double)input->N;
		double* Pi0 =(double*)_mm_malloc(input->NO*sizeof(double),16);
		getVectorPiIn_double(input->N, e, input->NO, Pi0);
		input->pagerank = (double *)_mm_malloc(input->NO*sizeof(double), 16);
		getPagerank_double(Pi0, input->pagerank, input->P, input->eps, input->N, input->NO);
	}
}
/*
 * Descrizione: costruisce una matrice di adiacenza a partire da una lista
 * di struct location. Inoltre provvede a calcolare gli outdegree, che saranno
 * contenuti nel vettore d.
 */

float* get_adiacency_matrix_single(int n, int m, location *l, int *no, float *d){
	//Padding che rende le colonne della matrice multiple di 16
	/*Il padding comporta due benefici:
	 * 1) Elimina il ciclo resto
	 * 2) Elimina il ciclo quoziente nel loop unrolling
	 * All'inizio le colonne della matrice erano multiple di 4 perchè
	 * la loop vectorization prendeva 4 elementi per volta
	 * Ora diventano multiple di 16 dato che l'unrolling prende 16 elementi per volta
	 */
	int o = 0;
	int cols = n;
	if(n % 16 != 0){
		int a = n/16;
		a = a*16;
		int b = n-a;
		o = 16-b;
		cols+=o;
	}
	*no = cols;
	/*
	 * La dimensione della matrice s è n*(n+o)
	 */
	float* s = (float*)_mm_malloc(n*cols*sizeof(float), 16);
	int i, j;
	for(int k = 0; k < m; k++){
		i = l[k].x-1;
		j = l[k].y-1;
		s[i*cols+j] = 1.0;
		d[i]++;
	}
	return s;
}

double* get_adiacency_matrix_double(int n, int m, location *l, int *no, double *d){
	int o = 0;
	int cols = n;
	//Padding che rende le colonne della matrice multiple di 8 elementi
	//Dato che il loop unrolling prende 8 elementi per volta
	if(cols % 8 != 0){
		int a = n/8;
		a = a*8;
		int b = n-a;
		o = 8-b;
		cols+=o;
	}
	*no = cols;
	double* g = (double*)_mm_malloc(n*cols*sizeof(double),16);
	int i, j;
	for(int k = 0; k < m; k++){
		i = l[k].x-1;
		j = l[k].y-1;
		g[i*cols+j] = 1.0;
		d[i]++;
	}
	return g;
}

/*
 * Descrizione: funzione che ricava una matrice le cui colonne
 * sono multiple di 8, dato che la matrice in input potrebbe non esserlo.
 */

double* getMatrix(int n, double *P, int *no){
		int cols = n;
		int o = 0;
		//Padding che rende le colonne della matrice multiple di 8 elementi
		if(cols % 8 != 0){
			int a = cols/8;
			a = a*8;
			int b = cols-a;
			o = 8-b;
			cols+=o;
		}
		*no = cols;
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

/*void get_outdegree_single(int n, float *A, float *d, int no){
	//vettore di outdegree
	for(int i = 0; i < n; i++){
		int out = 0;
		for(int j = 0; j < n; j++){
			/*conta ogni volta che è presente un 1 nella riga i
			 * corrisponde al numero di archi uscenti da i
			*/
			/*if(A[i*no + j]){
				out++;
			}
		}
		//inserisce l'outdegree nel vettore d
		*(d+i) = out;
	}
}*/

/*void get_outdegree_double(int n, double *A, double *d, int no){
	//vettore di outdegree
	for(int i = 0; i < n; i++){
		int out = 0;
		for(int j = 0; j < n; j++){
			if(A[i*no + j]){
				out++;
			}
		}
		//inserisce l'outdegree nel vettore
		*(d+i) = out;
	}
}*/

/*
 * Descrizione = funzione che ricava la matrice delle probabilità
 * di transizione P'', a partire dal vettore di outdegree (d) e dalla matrice di
 * adiacenza (A).
 * Ingresso = dimesione matrice di adiacenza (n), matrice di adiacenza (A)
 * 				vettore di outdegree (d)
 * Uscita = matrice delle probabilità di transizione P = A/d
 */

/*void get_matrix_P_single(int n, float *A, float *d, int no, double c, float e, float b){
	for(int i = 0; i < n; i++){
		for(int j = 0; j < n; j++){
			/*la verifica serve per evitare calcoli inutili
			 * Dato che gli elementi con outdegree nullo sono
			 * sostituiti con 1/n abbiamo che nel calcolo di P'':
			 * P''[i][j] = c*(1/n) + (1-c)*(1/n) = 1/n
			 * Quindi è possibile sostituire direttamente con 1/n
			 */

			/*if(d[i] != 0){
				A[i*no + j] = A[i*no + j]/d[i];
				A[i*no+j]=c*A[i*no+j] + e;
			}
			else{
				A[i*no+j]= b;
			}
		}
	}
}*/

/*void get_matrix_P_double(int n, double *A, double *d, int no, double c, double e, double b){
		for(int i = 0; i < n; i++){
			for(int j = 0; j < n; j++){
				//la verifica serve per evitare divisioni inutili
				if(d[i] != 0){
					A[i*no + j] = A[i*no + j]/d[i];
					A[i*no+j]=c*A[i*no+j] + e;
				}
				else{
					A[i*no+j]= b;
				}
			}
		}
}*/

/*
 * Descrizione: vettore iniziale dei pagerank i cui elementi sono 1/n
 */

/*void getVectorPiIn_single(int n, float e, int no, float *Pi){
	for (int i=0; i<n; i++){
		Pi[i]=e;
	}
	/*for(int i = n; i < no; i++){
		Pi[n+i] = 0;
	}
}*/

/*void getVectorPiIn_double(int n, double e, int no, double *Pi){
	for (int i=0; i<n; i++){
		Pi[i]=e;
	}
	//Serve in assembly per evitare che ci sia 1/n al posto degli zeri di padding
	/*for(int i = n; i < no; i++){
		Pi[n+i] = 0;
	}
}*/

/*
 * descrizione: funzione che esegue il prototto Pik = (P'')'*Pi0
 * Dove Pi0 è il vettore dei pagerank all'iterazione precedente.
 * Invece di calcolare la trasversa di P'' si scegli di considerare
 * direttamente le sue colonne.
 */

/*void getVectorPik_single(float *P, float *Pi0, float *Pik, int n, int no){
	for(int i = 0; i < n; i++){
		Pik[i] = 0;
		for(int j = 0; j < n; j++){
			Pik[i] += P[j*no + i]*Pi0[j];
		}
	}
}

void getVectorPik_double(double *P, double *Pi0, double *Pik, int n, int no){
	for(int i = 0; i < n; i++){
		Pik[i] = 0;
		for(int j = 0; j < n; j++){
			Pik[i] += P[j*no + i]*Pi0[j];
		}
	}
}*/

/*
 * Descrizione: serve a calcolare i valori finali di pagerank
 */

/*void getPagrnk_single(int n, float *Pik){
	float somma = 0;
	for(int i = 0; i < n; i++)
		somma += fabsf(Pik[i]);
	for(int i = 0; i < n; i++)
		Pik[i] = Pik[i]/(float)somma;
}*/

/*void getPagrnk_double(int n, double *Pik){
	double somma = 0;
	for(int i = 0; i < n; i++)
		somma += fabs(Pik[i]);
	for(int i = 0; i < n; i++)
		Pik[i] = Pik[i]/(double)somma;
}*/

/*
 * Descrizione: si occupa di calcolare il delta delta = ||Pi(k) - Pi(k+1)||1
 * fatto ciò aggiorna gli elementi dell'iterazione precedente con quelli
 * dell'iterazione corrente.
 */

/*void getDelta_single(float *Pi0, float *Pik, int n, float *delta){
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
}*/

/*
 * Descrizione: converte i pagerank da float a double
 */

/*void cvtPagerank(int n, float *Pik, double *Piconv){
	for(int i = 0; i < n; i++){
		Piconv[i] = (double) Pik[i];
	}
}*/

/*
 * Descrizione: esegue il calcolo del delta e del vettore dei pagerank finchè delta > eps
 */


void getPagerank_single(float *Pi0, float *Pik, float *P, double eps, int n, int no, double *Piconv){
	float delta = 0;
	getVectorPik_single(P, Pi0, Pik, n, no);
	getDelta_single(Pi0, Pik, n, &delta);
	while(delta > eps){
		getVectorPik_single(P, Pi0, Pik, n, no);
		/*
		 * Calcolo del valore delta = ||Pi(k) - Pi(k+1)||1
		 */
		delta = 0;
		getDelta_single(Pi0, Pik, n, &delta);
	}
	getPagrnk_single(n,Pik);
	cvtPagerank(n, Pik, Piconv);
}

void getPagerank_double(double *Pi0, double *Pik, double *P, double eps, int n, int no){

	double delta = 0;
	getVectorPik_double(P, Pi0, Pik, n, no);
	getDelta_double(Pi0, Pik, n, &delta);
	while(delta > eps){
		getVectorPik_double(P, Pi0, Pik, n, no);
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
	
	params* input = malloc(sizeof(params));

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
