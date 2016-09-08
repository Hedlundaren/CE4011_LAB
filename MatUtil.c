#include "MatUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void GenMatrix(int *mat, const size_t N)
{
	for(int i = 0; i < N*N; i ++)
		mat[i] = rand()%32 - 1;
	for(int i = 0; i < N; i++)
		mat[i*N + i] = 0;

}

bool CmpArray(const int *l, const int *r, const size_t eleNum)
{
	for(int i = 0; i < eleNum; i ++)
		if(l[i] != r[i])
		{
			printf("ERROR: l[%d] = %d, r[%d] = %d\n", i, l[i], i, r[i]);
			return false;
		}
	return true;
}


/*
	Sequential (Single Thread) APSP on CPU.
*/
void ST_APSP(int *mat, const size_t N)
{
	for(int k = 0; k < N; k ++)
		for(int i = 0; i < N; i ++)
			for(int j = 0; j < N; j ++)
			{
				int i0 = i*N + j;
				int i1 = i*N + k;
				int i2 = k*N + j;

				if(mat[i1] != -1 && mat[i2] != -1)
                     { 
			          int sum =  (mat[i1] + mat[i2]);
                          if (mat[i0] == -1 || sum < mat[i0])
 					     mat[i0] = sum;
				}
			}
}

/*
	Parallel
*/
void PT_APSP(int *mat, const size_t N)
{

	int numprocs, rank, namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];

	//Our initial processor
	int root = 0;
	int buf;

	//This is our temp variable for the row striping
	int *temp;
	int dest; //This is our destination for sending the rows we have split up
	int src;

	//Status returned from MPI on receive
	MPI_Status status;

	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(processor_name, &namelen);

	//This gives the number of rows we need per matrix row
	//Split matrix across processors, row striping move convenient in C
	//It is assummed n*n matrix n/p (n exactly divisible by p)
	int rowsPerProc = N/numprocs;

	//Debug printing on root processor only?
	if(rank == root)
	{
		printf("Starting Parallel APSP on Process %d on %s out of %d\n", rank, processor_name, numprocs);
		printf("Rows of matrix per processor = %d Matrix size = %d ",numprocs,N);
	}


	/////////////////////////////////////////
	//Distributing
	/////////////////////////////////////////

	//Calculate destintaion rank and mod with numprocs to prevent overflow
	dest = (rank + 1) % numprocs;
	src = (rank - 1) % numprocs;

	//For every pair a[i,j] we need to iterate through the row and column
	//This happens sequentially on all processors??
	for (int middleVal = 0; middleVal < N; middleVal++)
	{
		//The process that owns this row according to split
		//Eg. We have a correct split
		owner = middleVal/rowsPerProc;

		//If proc owns the row
		if(rank == owner){
			//Offset of row
			//This is for accessing the next row in our array (ie. This is not a multidimensional array)
			int offset = middleVal-rank*rowsPerProc;
			
			//For every value in row, copy this
			for(rowj = 0; rowj < N; rowJ++){
				//make temp array equal the matrix
				temp[rowJ]=mat[offset][rowJ];
			}
		}
		
		//TODO Every processor sends to next processor in a cycle so not just the root is sending
		
		int tag = 1;

		//Send to rank + 1 in a cycle
		MPI_Send(&temp,count,MPI_INT,dest,tag,MPI_COMM_WORLD);

		//Receive from rank -1
		MPI_Recv(&temp,count,MPI_INT,src,MPI_ANY_TAG,MPI_COMM_WORLD,&status);

		//or
		//MPI_Bcast(&buf,1,


		/////////////////////////////////////////
		//Updating our strip accordingly to APSP
		//For all rows in processor we need to iterate through each col
		/////////////////////////////////////////

		for (int rowIndex = 0; rowIndex < rowsPerProc; rowIndex++) 
		{
			for (int colIndex = 0; colIndex < N; colIndex++)
			{
				//This is for accessing the next row in our array (This is not a multidimensional array)
				//TODO we don't have N here though?? so should be "rowsPerProc"??
				int i0 = rowIndex*rowsPerProc + colIndex;
				int i1 = rowIndex*rowsPerProc + middleVal;
				int i2 = middleVal*rowsPerProc + colIndex;

				//If we have a path otherwise we will go to the next colIndex
				if (mat[i1] != -1 && mat[i2] != -1)
				{
					//Min function
					int sum = (mat[i1] + mat[i2]);

					//If we have no path at current area or our sum is less
					if (mat[i0] == -1 || sum < mat[i0])
					{	
						mat[i0] = sum;
					}

				}

			}

		}

	}


	/////////////////////////////////////////
	//Collecting Results
	/////////////////////////////////////////


}


