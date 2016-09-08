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

	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(processor_name, &namelen);

	if (rank == 0) {

		printf("Starting Parallel APSP Process %d on %s out of %d\n", rank, processor_name, numprocs);

		for (int k = 0; k < N; k++) {

			for (int i = 0; i < N; i++) {

				for (int j = 0; j < N; j++){

					int i0 = i*N + j;
					int i1 = i*N + k;
					int i2 = k*N + j;

					//min
					if (mat[i1] != -1 && mat[i2] != -1)
					{
						int sum = (mat[i1] + mat[i2]);
						if (mat[i0] == -1 || sum < mat[i0])
							mat[i0] = sum;
					}
				}
			}
		}
	}
}


