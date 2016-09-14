#include "MatUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: test {N}\n");
		exit(-1);
	}

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
	ST_APSP(ref, N);

	//Copy random matrix into result so we can use it in the parallel APSP
	int *result = (int*)malloc(sizeof(int)*N*N);
	memcpy(result, mat, sizeof(int)*N*N);

	//Init MPI parallel (passes arg and argv to processes)
	MPI_Init(&argc, &argv);
	PT_APSP(result, N);
	MPI_Finalize();


	//Compare parallel result with reference from ST_APSP
	if(CmpArray(result, ref, N*N)){
		//PrintMatrix(result,N,N);
		printf("Your result is correct.\n");
	}
	else{
		//PrintMatrix(result,N,N);
		printf("Your result is wrong.\n");
	}


	//We should be calling free here to free up the malloc memory
	//free(mat);
	//free(ref);
	//free(result);
}
