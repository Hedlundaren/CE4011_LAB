#include "MatUtil.h"
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <mpi.h>

void PrintMatrix(int *mat, int rows, int cols)
{
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
	//bool val = true;
	for(int i = 0; i < eleNum; i ++){
		if(l[i] != r[i])
		{
			printf("ERROR: para[%d] = %d, seq[%d] = %d\n", i, l[i], i, r[i]);
			//val = false;
			return false;
		}
	}
	return true;
}


/*
Sequential (Single Thread) APSP on CPU.
*/
void ST_APSP(int *mat, const size_t N){
	for(int k = 0; k < N; k ++){
		for(int i = 0; i < N; i ++){
			for(int j = 0; j < N; j ++){
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
Parallel APSP on  Multiple CPUs
*/
void PT_APSP(int *result, const size_t N){

	struct timeval tv1,tv2;
	int numprocs;
	int rank;
	int namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Status status; //Status returned from MPI on receive
	int root = 0; //User defined root process

	int owner; //Processor which is in control of partial matrix
	int offset; //Dynamic Offset depends on the rowsPerProc, k and owner

	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(processor_name, &namelen);

	//Number of rows allocated for a processor to work on for each k
	//eg 8*8 matrix 8/4 processors = 2 (2rows of matrix for each P)
	int rowsPerProc = N/numprocs;

	//Partial Matrix for each processor is setup using row stripping
	int *part = (int*)malloc(sizeof(int)*N*rowsPerProc);
	//Temp row for kth iteration, this will be Broadcasted across processors
	int *temp = (int*)malloc(sizeof(int)*N);

	//Initial Time before we start communications
	gettimeofday(&tv1,NULL);

	//Scatter sections of input matrix to so all processors have a partial matrix
	MPI_Scatter(result,rowsPerProc*N,MPI_INT,part,rowsPerProc*N,MPI_INT,root,MPI_COMM_WORLD);

	//Start processing for the first kth on every processor
	for(int k = 0; k < N; k++){

		owner = k / rowsPerProc;

		//For the owner of the row/rows we need to do an operation from this processor
		if(rank == owner){

			offset = k - owner*rowsPerProc;

			//Copying temp from the partial matrix before we modify it
			for(int b = 0; b < N; b++){
				temp[b] = part[offset*N+b]; 
			}
		}

		//Owner sends kth rows in temp array to all other processors
		MPI_Bcast(temp,N,MPI_INT,owner,MPI_COMM_WORLD); 

		//For each row in our current processor in the local matrix
		for (int rowIndex = 0; rowIndex < rowsPerProc; rowIndex++){

			//For each row in our current processor go through each column N
			for (int colIndex = 0; colIndex < N; colIndex++){

				int ij = rowIndex*N + colIndex; //Cell we are wanting to check
				int kj = colIndex; //Relax column in the temp row (simply the index)
				int ik = rowIndex*N + k; //Relax row, Same row as partial matrix

				//If we have a path otherwise go to the next colIndex
				if (temp[kj] != -1 && part[ik] != -1){

					//part[i,k]: current cell for relaxing (Local) 
					//temp[kj]: k in the temp row (Distributed at start of kth iteration)
					int sum = (part[ik] + temp[kj]);

					//If we can update a "no path" or the path is less than current
					//update self with smaller path size
					if (part[ij] == -1 || sum < part[ij]){	
						part[ij] = sum;
					}
				}
			}
		}
	}

	//Gather partial matrices on root processor, giving the updated complete matrix on root proc
	//Other processors will have a different result so we need to check root only
	MPI_Gather(part,N*rowsPerProc,MPI_INT,result,N*rowsPerProc,MPI_INT,root,MPI_COMM_WORLD);

	//Finish timing and print timining for parallel function on this processor
	gettimeofday(&tv2,NULL);
	printf("Par Time = %ld usecs\n",(tv2.tv_sec - tv1.tv_sec)*1000000+tv2.tv_usec-tv1.tv_usec);

	//Free mem and clean up
	free(part);
	free(temp);
}

