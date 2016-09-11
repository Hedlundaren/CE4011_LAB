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

	//compute the reference result.
	int *ref = (int*)malloc(sizeof(int)*N*N);
	memcpy(ref, mat, sizeof(int)*N*N);

	//Carry out APSP on reference matrix
	ST_APSP(ref, N);

	//Copy random matrix into result for parallel APSP
	int *result = (int*)malloc(sizeof(int)*N*N);
	memcpy(result, mat, sizeof(int)*N*N);

	//Init MPI parallel (passes arg and argv to processes)
	MPI_Init(&argc, &argv);

	//Submit our random matrix for parallel processing and save in result
	PT_APSP(result, N);
	MPI_Finalize();

	PrintMatrix(result,N,N);

	//compare parallel result with reference from ST_APSP
	if(CmpArray(result, ref, N*N)){
		printf("Your result is correct.\n");
	}
	else{
		printf("Your result is wrong.\n");
	}

	//We should be calling free here to free the malloc?
	free(mat);
	free(ref);
	free(result);
}
