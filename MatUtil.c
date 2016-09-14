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
	Parallel of Multiple CPU
*/
void PT_APSP(int *result, const size_t N)
{
	//MPI Vars
	int numprocs;
	int rank;
	int namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Status status; //Status returned from MPI on receive
	int root = 0; //User defined root process
	int version; //MPI versions
	int subversion;

	//Variables for specific processing of problem
	//Owner of current matrix
	int owner;
	//Offset of current matrix
	int offset;

	////////////////////////////////////////////////
	//End of variable section
	////////////////////////////////////////////////

	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(processor_name, &namelen);
 	MPI_Get_version (&version,&subversion);

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
	int *part = (int*)malloc(sizeof(int)*N*rowsPerProc);
	memset(part, 0, N*rowsPerProc); //We should zero the array for each k

	int *temp = (int*)malloc(sizeof(int)*N);
	memset(temp, 0, N); //We should zero the array for each k

	//This is the matrix we receive after K has completed
	int *recvmat = (int*)malloc(sizeof(int)*N*N);
	memset(recvmat, 0, N*N); //We should zero the array for each k

	//////////////////////////////////////////////////////////////////////////////
	//WE NEED TO SPLIT THE MATRIX AS THE OFFSET BELOW IS ACCESSING A "PART" MATRIX
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

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
	MPI_Scatter(result,rowsPerProc*N,MPI_INT,part,rowsPerProc*N,MPI_INT,root,MPI_COMM_WORLD);

	//For matrix size we need to calculate according to rank and matrix per processor
	//TODO fix the 1 iteration here
	for(int k = 0; k < N; k++){

		//PrintMatrix(result,N,N);

		//This calculates the owner of this row
		owner = k / rowsPerProc; //Divide all k into numprocs

		//For the owner of the row/rows we need to do an operation from this processor
		if(rank == owner){

			//This is our offset for our local matrix, size N*rowsPerProc
			//Firstly we need the split matrix
			offset = k - owner*rowsPerProc;

			//TODO shouldn't this copy N*rowProc times to get as many rows as we need
			//This creates our row striping
			for(int b = 0; b < N; b++){
				temp[b] = part[offset*N+b]; //Offset returns 0,1,2,3 etc. so we need to multiply this by the size N
			}
		}

		//Here is where we bcast from the owner using our temporary row of k
		//Owner sends, all other processors will receive the part
		// buf, count, type, source, comm
		MPI_Bcast(temp,N,MPI_INT,owner,MPI_COMM_WORLD); 

		//For every row we have in our current processor
		for (int rowIndex = 0; rowIndex < rowsPerProc; rowIndex++) {
			for (int colIndex = 0; colIndex < N; colIndex++){

				//Below is equivalent to our min Function
				//part[i,j] = min(part[i,j], part[i,k], + temp[j]);

				int ij = rowIndex*N + colIndex; //This is cell we are wanting to replace
				int kj = colIndex; //This is relaxing column In the temp row, so it is simply the index
				int ik = rowIndex*N + k; //This is relaxing row, in the same row in the partial mat, offset by k

				// SUPERIOR DEBBUGING PRINT
				// printf("self: %d colk: %d rowk: %d\n",part[ij],temp[kj],part[ik]);

				//If we have a path otherwise we will go to the next colIndex
				//TODO We should be accessing the tempK rows/row?
				if (temp[kj] != -1 && part[ik] != -1){

					//temp[colIndex] is the k in the columns that we need to access
					//part[i,k] is in the current row

					int sum = (part[ik] + temp[kj]); //Sum of path distance

					//If we have no path in adjacency matrix shown but we have 
					//a sum from above we can create a new path

					//Or if our sum of the new path is less than the current
					if (part[ij] == -1 || sum < part[ij]){	

						//Update self with new sum of small path
						part[ij] = sum;
					}
				}
			}
		}

		//Gather once we have calculated our partial matrices update the results variable
		MPI_Barrier(MPI_COMM_WORLD);
		// sendbuf, sendcount, type, recvbuf, recvcount, recvtype, target, comm
		MPI_Gather(part,N*rowsPerProc,MPI_INT,result,N*rowsPerProc,MPI_INT,root,MPI_COMM_WORLD);
	}

	PrintMatrix(result, N, N);

	//Return our result to all processors so the return value is the same
	MPI_Bcast(result,N*N,MPI_INT,root,MPI_COMM_WORLD); //SHOULD THIS BE A GROUP INSTEAD??
	
}

