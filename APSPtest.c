#include "MatUtil.h"
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char **argv)
{
	int rank;
	struct timeval tv1,tv2;

	if (argc != 2)
	{
		printf("Usage: test {N}\n");
		exit(-1);
	}
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//generate a random matrix.
	size_t N = atoi(argv[1]);
	int *mat = (int*)malloc(sizeof(int)*N*N);
	GenMatrix(mat, N);

	//Allocate mem for the reference result from the sequential Algorithm
	int *ref = (int*)malloc(sizeof(int)*N*N);
	memcpy(ref, mat, sizeof(int)*N*N);

	//THis will be carried out (Number of processor) times if we run using MPI
	//Other wise will happen only once 
	//Carry out APSP on reference matrix

	gettimeofday(&tv1,NULL);
	ST_APSP(ref, N);
	gettimeofday(&tv2,NULL);
	printf("Seq Time = %ld usecs\n",(tv2.tv_sec - tv1.tv_sec)*1000000+tv2.tv_usec-tv1.tv_usec);

	//Copy random matrix into result so we can use it in the parallel APSP
	int *result = (int*)malloc(sizeof(int)*N*N);
	memcpy(result, mat, sizeof(int)*N*N);

	printf("Starting Computation of APSP for Matrix of size: %zu x %zu\n",N,N);

	//Init MPI parallel (passes arg and argv to processes)
	PT_APSP(result, N);
	MPI_Finalize();

	//Compare parallel result with reference from ST_APSP
	//TODO since we are not broadcasting the matrix to all processor only thr root will  have the result
	if(rank == 0){
		if(CmpArray(result, ref, N*N)){
			printf("Your result is correct.\n");
		}
		else{
			printf("Your result is wrong.\n");
		}
	}

	//We should be calling free here to free up the malloc memory
	free(mat);
	free(ref);
	free(result);
}
