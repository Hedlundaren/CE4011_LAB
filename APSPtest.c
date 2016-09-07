#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "MatUtil.h"

int main(int argc, char **argv)
{
	
	int numprocs, rank, namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];

	if(argc != 2)
	{
		printf("Usage: test {N}\n");
		exit(-1);
	}

	 printf("hiuhihuih %s\n", "HI Mami");

	//generate a random matrix.
	size_t N = atoi(argv[1]);
	int *mat = (int*)malloc(sizeof(int)*N*N);
	GenMatrix(mat, N);

	//compute the reference result.
	int *ref = (int*)malloc(sizeof(int)*N*N);
	memcpy(ref, mat, sizeof(int)*N*N);
	
	ST_APSP(ref, N);
	//compute your results
	
	
	int *result = (int*)malloc(sizeof(int)*N*N);
	memcpy(result, mat, sizeof(int)*N*N);
	//replace by parallel algorithm

	//ST_APSP(result, N);
	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(processor_name, &namelen);

	printf("Process %d on %s out of %d\n", rank, processor_name, numprocs);
	
	MPI_Finalize();

	
	//compare your result with reference result
	if(CmpArray(result, ref, N*N))
		printf("Your result is correct.\n");
	else
		printf("Your result is wrong.\n");

}
