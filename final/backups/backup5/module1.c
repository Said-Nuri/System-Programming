#include <stdio.h>
#include <pthread.h> 
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>

#define BUF_NUM 100; 

typedef struct matrix{
	float** m;
}matrix_t;

typedef struct lu{
	float** l;
	float** u;
}lu_t;

void print_usage();
void produce_matrix(void*);
int randomMatrixProducer(void*);
int l_u_decomposer();
void decompose(void* arg);
void decomposing(float**, float**);
int inversTaker();

float determinant(float** ,float );
void cofactor(float**, float** , float** );
void transpose(float** ,float** , float );

pthread_mutex_t decompose_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t matrix_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t prod_decom_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t inverser_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t decom_inv_lock = PTHREAD_MUTEX_INITIALIZER;

matrix_t *matrix_buffer;
lu_t *lu_buffer;

int mbuf_index = -1;
int lu_buf_index = -1;
int NUM_OF_THREAD;
int MATRIX_SIZE;
int BUFFER_SIZE;
int produce_counter = 0;
int MAX_PRODUCE;

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0;

/* This static function to handle specified signal */
static void signal_func(int signo) {
    /* The signal flag */    
    die_flag = 1;
    printf("singal has caught! Please wait....\n");    
}

int main(int argc, char const *argv[]){
	
	int numberof_buffer = BUF_NUM;
	int i,j,k;
	pthread_t rmp_thread, decom_thread, invtak_thread;

	/* Command line argumant control */
	if(argc < 5 || argc > 5){
		puts("Invalid operation!");
		print_usage();
		exit(EXIT_FAILURE);
	}

	/* Signal handling */
    if (signal(SIGINT, signal_func) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        exit( EXIT_FAILURE);
   	}

	/* Matrix size control */
	if( (MATRIX_SIZE = atoi(argv[1])) > 40){
		printf("matrix size cannot be larger than 40 \n");
		exit(EXIT_FAILURE);
	}
	printf("MATRIX_SIZE: %d\n", MATRIX_SIZE);

	if(MATRIX_SIZE < 1){
		printf("Invalid matrix size! %d \n", MATRIX_SIZE);
		exit(EXIT_FAILURE);
	}

	/* Control of number of threads*/
	if((NUM_OF_THREAD = atoi(argv[2])) < 1){
		printf("Invalid number of threads!\n");
		exit(EXIT_FAILURE);	
	}
	printf("NUM_OF_THREAD %d\n", NUM_OF_THREAD);

	if((MAX_PRODUCE = atoi(argv[3])) < 1){
		printf("Invalid number of MAX_PRODUCE!\n");
		exit(EXIT_FAILURE);	
	}	
	printf("MAX_PRODUCE %d\n", MAX_PRODUCE);

	if((BUFFER_SIZE = atoi(argv[4])) < 1){
		printf("Invalid number of BUFFER_SIZE!\n");
		exit(EXIT_FAILURE);	
	}	
	printf("BUFFER_SIZE %d\n", BUFFER_SIZE);


	matrix_buffer = calloc(BUFFER_SIZE, sizeof(matrix_t));
	lu_buffer = calloc(BUFFER_SIZE, sizeof(lu_t));

	/* Create threads for each module */
	pthread_create(&rmp_thread, NULL, (void*) &randomMatrixProducer, NULL);
	pthread_create(&decom_thread, NULL, (void*) &l_u_decomposer, NULL);
	pthread_create(&invtak_thread, NULL, (void*) &inversTaker, NULL);

	pthread_join(rmp_thread, NULL);
	pthread_join(decom_thread, NULL);
	pthread_join(invtak_thread, NULL);

	// for(i = 0; i <= mbuf_index; ++i){
	// 	for (j = 0; j < MATRIX_SIZE; ++j)	{
	// 		free(matrix_buffer[i].m[j]);
	// 	}
	// 	free(matrix_buffer[i].m);		
	// }

	// for(i = 0; i <= lu_buf_index; ++i){
	// 	for (j = 0; j < MATRIX_SIZE; ++j){
	// 		free(lu_buffer[i].l[j]);
	// 		free(lu_buffer[i].u[j]);
	// 	}
	// 	free(lu_buffer[i].l);		
	// 	free(lu_buffer[i].u);		
	// }

	return 0;
}

int randomMatrixProducer(void* arg){	
	
	int i;
	pthread_t *threads;	

	/* Allocation of threads */
	threads = (pthread_t*) calloc(NUM_OF_THREAD, sizeof(pthread_t));

	/* Thread creation */
	for (i = 0; i < NUM_OF_THREAD; ++i){
        pthread_create(&threads[i], NULL, (void *) &produce_matrix, NULL);
    }	

    // /* Wait for thread to be killed */
    for(i = 0; i < NUM_OF_THREAD; ++i){
    	pthread_join(threads[i], NULL);
    }    
    free(threads);
    printf("randomMatrixProducer\n");
	return 0;
}

void produce_matrix(void* size){
	int error, error2;
	int i,j;	
	
	while(!die_flag){
		//printf("flag5\n");
		// printf("p thread: %u waits for lock \n", (unsigned int) pthread_self());
		if(pthread_mutex_lock(&matrix_thread_lock) < 0){
			continue;
		}
		
		/* If maximum produce number reached, exit */
		if(produce_counter >= MAX_PRODUCE){
			printf("reached maximum produce number %d\n", produce_counter);
			pthread_mutex_unlock(&matrix_thread_lock);
			break;
		}

		//printf("flag6\n");
		// printf("p thread: %u in critical section \n", (unsigned int) pthread_self());
		/******************* -Produce- *************************/
		if(mbuf_index >= BUFFER_SIZE){
			if(pthread_mutex_unlock(&matrix_thread_lock) < 0){
				perror("matrix_thread_unlock");
				exit(EXIT_FAILURE);
			}
			printf("mbuf_index higher than %d!. lu_buf_index %d\n", BUFFER_SIZE, lu_buf_index);
			continue;
		}
		
		//printf("p thread: %u waits for lock \n", (unsigned int) pthread_self());
		if(error2 = pthread_mutex_lock(&prod_decom_lock) < 0){
			perror("prod_decom_lock");
			exit(EXIT_FAILURE);
		}
		//printf("p thread: %u in critical section \n", (unsigned int) pthread_self());
		//printf("flag7\n");
		// printf("p before mbuf_index %d, BUFFER_SIZE %d\n", mbuf_index, BUFFER_SIZE);
		
		++mbuf_index;
		++produce_counter;
		printf("produce_counter: %d\n", produce_counter);

		matrix_buffer[mbuf_index].m = calloc(MATRIX_SIZE, sizeof(float*));
		
		for(i = 0; i < MATRIX_SIZE; ++i){
			matrix_buffer[mbuf_index].m[i] = calloc(MATRIX_SIZE, sizeof(float));
		}
		
		srand(time(NULL));
		for(i = 0; i < MATRIX_SIZE; ++i){
			for (j = 0; j < MATRIX_SIZE; ++j){								
				matrix_buffer[mbuf_index].m[i][j] = abs((rand())%50);
			}
		}
		printf("p mbuf_index: %d\n", mbuf_index);
	 //    for(i = 0; i < MATRIX_SIZE; i++){
		// 	for(j = 0; j < MATRIX_SIZE; j++)
		// 		printf("%6.2f ", matrix_buffer[mbuf_index].m[i][j]);
		// 	printf("\n");
		// }
		
		//printf("flag1\n");
		
		if(error = pthread_mutex_unlock(&prod_decom_lock) < 0){
			perror("prod_decom_unlock");
			exit(EXIT_FAILURE);
		}
		//printf("flag2\n");
		// printf("p thread: %u out of prod_decom_lock \n", (unsigned int) pthread_self());
		/**************************************************************/

		//printf("thread: %u in critical section \n", (unsigned int) pthread_self());
		//printf("flag2\n");
		if(error = pthread_mutex_unlock(&matrix_thread_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}
		//printf("flag4\n");
	}
	printf("thread bitti\n");
}

void print_usage(){
	puts("Usage: ./module1 <size of matrices> <# of threads>");
	puts("Example: ./module1 20 5");
}

int l_u_decomposer(){
	int i;
	pthread_t* threads;
	float matrixL[MATRIX_SIZE][MATRIX_SIZE],
		  matrixU[MATRIX_SIZE][MATRIX_SIZE];

	/* Create threads for LU decomposing */	  
	threads = calloc(NUM_OF_THREAD, sizeof(pthread_t));
	
	for(i = 0; i < NUM_OF_THREAD; ++i){
		pthread_create(&threads[i], NULL, (void*) decompose, NULL);
	}

	for(i = 0; i < NUM_OF_THREAD; ++i){
		pthread_join(threads[i], NULL);
	}		  

	free(threads);
	printf("l_u_decomposer\n");
}
void decompose(void* arg){
	int error, error2;
	int BUFFER_SIZE = BUF_NUM;
	int i, j;

	while(!die_flag){
		// printf("d thread: %u waits for lock \n", (unsigned int) pthread_self());

		if(pthread_mutex_lock(&decompose_thread_lock) < 0){
			perror("decompose_thread_lock");
			exit(EXIT_FAILURE);
		}
		// printf("d thread: %u in critical section \n", (unsigned int) pthread_self());

		/******************* -Consume- *************************/
		if(mbuf_index <= 0){
			if(error = pthread_mutex_unlock(&decompose_thread_lock) < 0){
				perror("decompose_thread_lock");
				exit(EXIT_FAILURE);
			}
			continue;
		}

		if(!(lu_buf_index < BUFFER_SIZE-1)){
			printf("thread: %u\n", (unsigned int)pthread_self());
			printf("lu_buffer exceeded the size! %d\n", lu_buf_index);
			pthread_mutex_unlock(&decompose_thread_lock);
			continue;
		}

		// printf("d thread: %u waits for lock \n", (unsigned int) pthread_self());
		if(pthread_mutex_lock(&prod_decom_lock) < 0){
			perror("prod_decom_lock");
			exit(EXIT_FAILURE);
		}

		
		// printf("d thread: %u in critical section \n", (unsigned int) pthread_self());

		// printf("d before lu_buf_index %d, BUFFER_SIZE %d\n", lu_buf_index, BUFFER_SIZE);

		/******************** Allocation for lu_matrices *************************/

		++lu_buf_index;
		// printf("bayrak1\n");
		printf("d lu_buf_index %d\n", lu_buf_index);
		// sleep(1);

		// printf("bayrak2\n");
		lu_buffer[lu_buf_index].l = calloc(MATRIX_SIZE, sizeof(float*));
		lu_buffer[lu_buf_index].u = calloc(MATRIX_SIZE, sizeof(float*));
		// printf("bayrak3\n");
		for(i = 0; i < MATRIX_SIZE; ++i){
			lu_buffer[lu_buf_index].l[i] = calloc(MATRIX_SIZE, sizeof(float));
			lu_buffer[lu_buf_index].u[i] = calloc(MATRIX_SIZE, sizeof(float));
		}

		/*************************************************************************/
		// printf("bayrak4\n");
		printf("d mbuf_index %d\n", mbuf_index);
		
		/* Copy incoming matrix from matrix buffer to new LU matrices */
		for(i = 0; i < MATRIX_SIZE; ++i){
			for(j = 0; j < MATRIX_SIZE; ++j){
				lu_buffer[lu_buf_index].l[i][j] = matrix_buffer[mbuf_index].m[i][j];
				lu_buffer[lu_buf_index].u[i][j] = matrix_buffer[mbuf_index].m[i][j];
			}
		}

		/* LU decomposition function */
		decomposing(lu_buffer[lu_buf_index].l, lu_buffer[lu_buf_index].u);

		// printf("bayrak5\n");
		/********************** deallocate unusued buffer matrix ****************************************/
		for(i = 0; i < MATRIX_SIZE; ++i){
			free(matrix_buffer[mbuf_index].m[i]);
		}
		// printf("bayrak6\n");
		free(matrix_buffer[mbuf_index].m);

		--mbuf_index;
		/**********************************************************************************/
		printf("after decompose mbuf_index %d\n", mbuf_index);


		if(error2 = pthread_mutex_unlock(&prod_decom_lock) < 0){
			perror("prod_decom_unlock");
			exit(EXIT_FAILURE);
		}
		printf("d thread: %u out of prod_decom_lock \n", (unsigned int) pthread_self());
		/*************************************************************************/
		if(error = pthread_mutex_unlock(&decompose_thread_lock) < 0){
			perror("decompose_thread_unlock");
			exit(EXIT_FAILURE);
		}
	}
	printf("decompose\n");
}
void decomposing(float** l , float** u){
	float temp;
	int i, j, k;

	/* Lower triangular matrix */
	
	for(i = 0; i < MATRIX_SIZE; i++)
	{
		for(j = i; j < MATRIX_SIZE ; j++)
		{
			if(i != j)
			{
				temp = l[j][i]/l[i][i];
				for(k = 0; k < MATRIX_SIZE; k++)
				{
					l[j][k] -= ( temp * l[i][k] );
					if(l[j][k] < 0.00000)         
						l[j][k] = fabs(l[j][k]);	/* To get rid of negative zero values like -0.00...*/			
				}
			}
		}	
	}

	
	for(i = MATRIX_SIZE-1; i >= 0; i--)
	{
		for(j = i; j >= 0 ; j--)
		{
			if(i != j && u[i][i] != 0)
			{
				temp = (u[j][i]/u[i][i]);
				for(k = 0; k <= MATRIX_SIZE; k++)
				{
					u[j][k] -= ( temp * u[i][k] );				
					if(u[j][k] < 0.00000)         
						u[j][k] = fabs(u[j][k]);	/* To get rid of negative zero values like -0.00...*/					
				}
			}
		}	
	}

	// printf("Lower triangular matrix\n");
	// for(i = 0; i < MATRIX_SIZE; i++)
	// {
	// 	for(j = 0; j < MATRIX_SIZE; j++)
	// 		printf("%6.1f ", l[i][j]);
	// 	printf("\n");
	// }
	// printf("\n");
	// printf("Upper triangular matrix\n");
	// for(i = 0; i < MATRIX_SIZE; i++)
	// {
	// 	for(j = 0; j < MATRIX_SIZE; j++)
	// 		printf("%6.1f ", u[i][j]);
	// 	printf("\n");
	// }
}

int inversTaker(){

}
