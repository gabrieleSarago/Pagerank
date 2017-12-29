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
	GRAPHD g = _mm_malloc(nodes*nodes*sizeof(double),16); //alloca la struttura dati contenente il grafo
	/*
	 * Nota: per accedere all'elemento g[i][j]
	 * bisogna fare g[i*nodes + j]
	 * perchè si tratta di una allocazione di una matrice, simulata
	 * con un vettore, usando il calcolo esplicito degli elementi.
	 */
	for (i = 0; i < arcs; i++) {
		status = fread(&sorg, sizeof(int), 1, fp);
		status = fread(&dest, sizeof(int), 1, fp);
		// aggiungi l'arco (sorg,dest) a g
		/*
		 * Riga = sorg, colonna = dest, g[sorg][dest] = 1
		 */
		g[sorg*nodes + dest] = 1;
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
	GRAPHS s = _mm_malloc(nodes*nodes*sizeof(float), 16); //alloca la struttura dati contenente il grafo
	/*
	 * Anche in questo caso s[i][j] = s[i*nodes + j]
	 */
	for (i = 0; i < arcs; i++) {
		status = fread(&sorg, sizeof(int), 1, fp);
		status = fread(&dest, sizeof(int), 1, fp);
		// aggiungi l'arco (sorg,dest) a s
		s[sorg*nodes + dest] = 1;
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
extern void pagerank32(params* input);


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
    
    pagerank32(input); // Esempio di chiamata di funzione assembly

    // -------------------------------------------------

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
