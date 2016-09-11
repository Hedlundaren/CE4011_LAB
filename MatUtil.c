#include "MatUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

//This is for printing our matrix
void PrintMatrix(int *mat, int rows, int cols)
{
	//Print Random matrix
	printf("Random Matrix\n\n");
	for (int rowIndex = 0; rowIndex < rows; rowIndex++) {
		for (int colIndex = 0; colIndex < cols; colIndex++){
			printf("%d, ",mat[rowIndex*cols+colIndex]);
		}
		printf("\n");
	}
	printf("\n");
}

void GenMatrix(int *mat, const size_t N)
{
	for(int i = 0; i < N*N; i ++)
		mat[i] = rand()%32 - 1;
	for(int i = 0; i < N; i++)
		mat[i*N + i] = 0;

}

bool CmpArray(const int *l, const int *r, const size_t eleNum)
{

	//Modify to show all errors
	bool val = true;
	
	for(int i = 0; i < eleNum; i ++){
		if(l[i] != r[i])
		{
			printf("ERROR: para[%d] = %d, seq[%d] = %d\n", i, l[i], i, r[i]);
			val = false;
		}
	}
	return val;
}


/*
	Sequential (Single Thread) APSP on CPU.
*/
void ST_APSP(int *mat, const size_t N)
{
	for(int k = 0; k < N; k ++){

		for(int i = 0; i < N; i ++){

			for(int j = 0; j < N; j ++)
			{
				int i0 = i*N + j;
				int i1 = i*N + k;
				int i2 = k*N + j;

				if(mat[i1] != -1 && mat[i2] != -1){ 
				  int sum =  (mat[i1] + mat[i2]);
				  if (mat[i0] == -1 || sum < mat[i0])
						     mat[i0] = sum;
				}
			}
		}
	}
}

/*
	Parallel
*/
void PT_APSP(int *result, const size_t N)
{
	int numprocs, rank, namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	//Owner of current matrix
	int owner;
	//Offset of current matrix
	int offset;

	//Main proc
	int root = 0;

	//Status returned from MPI on receive
	MPI_Status status;

	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(processor_name, &namelen);

	//MPI version for debugging?
	int version, subversion;
 	MPI_Get_version (&version,&subversion);

	int rowsPerProc = N/numprocs;

	int *temp = (int*)malloc(sizeof(int)*N);

	//For matrix size we need to calculate according to rank and matrix per processor
	for(int k=0;k<N;k++){

		owner = k / numprocs; //Divide all k into numprocs

		if(rank == owner){
			offset = k - rank*rowsPerProc;
			//Copy
			for(int b=0;b<N; b++){
				temp[b] = result[b+offset*N];
			}
		}

		MPI_Bcast(temp,N,MPI_INT,owner,MPI_COMM_WORLD);

		for (int rowIndex = 0; rowIndex < N; rowIndex++) 
		{
			for (int colIndex = 0; colIndex < N; colIndex++)
			{
				int ij = rowIndex*N +colIndex; //This is self
				int jk = rowIndex*N + k; //This is relaxing column
				int ki = k*N + colIndex; //This is relaxing row

				//If we have a path otherwise we will go to the next colIndex
				if (result[jk] != -1 && result[ki] != -1)
				{
					//Min function
					int sum = (result[jk] + result[ki]);
					//If we have no path at current area or our sum is less
					if (result[ij] == -1 || sum < result[ij]){	
						result[ij] = sum;
					}
				}
			}
		}
		//Collect from all processors
		//MPI_Reduce(buf+(N*N*offset),result,N*N,MPI_INT,MPI_MIN,root,MPI_COMM_WORLD);
		//MPI_Barrier(MPI_COMM_WORLD);

	}

}

