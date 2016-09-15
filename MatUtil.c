#include "MatUtil.h"
#include <stdio.h>
#include <sys/time.h>
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
	Parallel of Multiple CPU
*/
void PT_APSP(int *result, const size_t N)
{
	struct timeval tv1,tv2;
	int numprocs;
	int rank;
	int namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Status status; //Status returned from MPI on receive
	int root = 0; //User defined root process

	//Variables for specific processing of problem
	//Owner of current matrix
	int owner;
	//Offset of current matrix
	int offset;

	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(processor_name, &namelen);

	//Number of rows allocated for a processor to work on for each k
	//eg 8*8 matrix 8/4=2 (2rows of matrix each to work on for each k)
	//Therefore 2 rows need to be broadcasted from each processor and all
	//other receiving processors will update there current matrix with the new values for the next K
	int rowsPerProc = N/numprocs;

	/*
	N*N gives a safety size so we can optimise later
	Optimise according to no diagonals etc.
	TODO this should be the size according to the divided k below???
	TODO shouldn it be N*rowsPerProc?? to give correct size?
	This is our full size array that we need to merge across k.
	Values that have not been set will be zero
	Or Maybe we can do a merge on the "result" array?
	*/

	//Partial Matrix equal size per processor
	int *part = (int*)malloc(sizeof(int)*N*rowsPerProc);
	//Temp row for k
	int *temp = (int*)malloc(sizeof(int)*N);

	/*
	1. Distribute the matrix and split via row striping
	2. For example a 8*8 matrix with 4 processors will split
	   the matrix into multiple rows and hand them over the processors
	   This split would be 8/4 = 2 => 2 rows per processor
	3. These rows are now split across the processor for the kth iteration.
	4. We now bcast a single row of the kth values, this reduce our dependcy required by the split matrices
	5. However, the kth column is inherited in each row we split
	6. This means we access the kth row value from the row local to the processor and we also get the kth
	   column value from the broadcasted single row of kth values for the kth iteration
	7. Next once we have completed all iterations of the row and columns within the local matrix
	8. We need to distribute and collect the changes we have made to the seperate matrices
	9. Each processor will shares its changes with other processors
	10.Once the changes have been shared we can iterate k
	11.k is now iterated so we need to broadcast a new temp row of the new kth values
	12.We also need to split up our new total matrix area as we did before and send each part to a diff processor
	*/

	//Maybe not the most efficient way to do things
	//http://stackoverflow.com/questions/9269399/sending-blocks-of-2d-array-in-c-using-mpi/

	//TIMING
	gettimeofday(&tv1,NULL);

	//TODO not as efficient due to every processor having this data in the result anyway
	//We can access it on the first iteration of k
	MPI_Scatter(result,rowsPerProc*N,MPI_INT,part,rowsPerProc*N,MPI_INT,root,MPI_COMM_WORLD);

	//For matrix size we need to calculate according to rank and matrix per processor
	for(int k = 0; k < N; k++){

		owner = k / rowsPerProc;

		//For the owner of the row/rows we need to do an operation from this processor
		if(rank == owner){

			//This is our offset for our local matrix, size N*rowsPerProc
			offset = k - owner*rowsPerProc;

			//This is our temp data or the kth row
			for(int b = 0; b < N; b++){
				temp[b] = part[offset*N+b]; 
			}
		}

		//Owner sends of row sends temp to all other processors
		// buf, count, type, source, comm
		MPI_Bcast(temp,N,MPI_INT,owner,MPI_COMM_WORLD); 

		//For every row we have in our current processor
		for (int rowIndex = 0; rowIndex < rowsPerProc; rowIndex++) {
			//For every row we have in our current processor we need to go through each column N
			for (int colIndex = 0; colIndex < N; colIndex++){

				int ij = rowIndex*N + colIndex; //This is cell we are wanting to replace
				int kj = colIndex; //This is relaxing column In the temp row, so it is simply the index
				int ik = rowIndex*N + k; //This is relaxing row, in the same row in the partial mat, offset by k

				//If we have a path otherwise we will go to the next colIndex
				if (temp[kj] != -1 && part[ik] != -1){

					//temp[colIndex] is the k in the columns that we need to access
					//part[i,k] is in the current row
					int sum = (part[ik] + temp[kj]); //Sum of path distance

					//If we have no path in adjacency matrix shown but we have 
					//a sum from above we can create a new path
					//Or if our sum of the new path is less than the current
					if (part[ij] == -1 || sum < part[ij]){	

						//Update self with smaller path size
						part[ij] = sum;
					}
				}
			}
		}

		//Gather once we have calculated our partial matrices update the results matrix
		//MPI_Barrier(MPI_COMM_WORLD);
		// sendbuf, sendcount, type, recvbuf, recvcount, recvtype, target, comm
	}

	//Return our result to all processors so the return value is the same
	//TODO if this is large then will take a long time sending data
	//MPI_Bcast(result,N*N,MPI_INT,root,MPI_COMM_WORLD);
	MPI_Gather(part,N*rowsPerProc,MPI_INT,result,N*rowsPerProc,MPI_INT,root,MPI_COMM_WORLD);

	gettimeofday(&tv2,NULL);
	printf("Par Time = %ld usecs\n",(tv2.tv_sec - tv1.tv_sec)*1000000+tv2.tv_usec-tv1.tv_usec);

	//Free after malloc
	free(part);
	free(temp);
}

