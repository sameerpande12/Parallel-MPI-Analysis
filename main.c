#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
// #include <random>
#include <time.h>
#include <unistd.h>
void Multiply_serial(float* A,float *B, float *C, int m,int n, int p ){
//A is mXn,  B is n X p
  int i,j,k;
  for(i=0;i<m;i++){
    for(j=0;j<p;j++){
      C[i*p+j]=0;
      for(k = 0;k<n;k++){
        C[i*p + j] += A[i*n + k] * B[k * p + j];
      }
    }
  }
}

int IsEqual(float *A, float *B, int m, int n){
    for(int i = 0;i<m;i++){
        for(int j = 0;j<n;j++){
            if(A[i*n + j] != B[i*n + j]){
              printf("difference %d %d %f %f\n",i,j,A[i*n + j],B[i*n + j]);
              return 0;
            }

        }
    }
    return 1;
}

void printMatrix(float * matrix,int nrows,int ncols){
  for(int i = 0;i < nrows ; i++){
    for(int j = 0; j<ncols ; j++){
      printf("%f ",matrix[i*ncols + j]);
    }
    printf("\n");
  }
  printf("\n");
}

int main(int argc, char**argv){

    int rank,numProcesses;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&numProcesses);

    if(rank==0){

        int aRows = atoi(argv[1]);
        int aCols = atoi(argv[2]);
        int bRows = aCols;
        int bCols = atoi(argv[3]);

        int aRowSize = aCols;
        int aColSize = aRows;
        int bRowSize = bCols;
        int bColSize = bRows;

        float* A = (float *)malloc(sizeof(float)*aRows*aCols);
        float* B = (float *)malloc(sizeof(float)*bRows*bCols);
        float* C = (float *)malloc(sizeof(float)*aRows*bCols);

        srand48(time(0));
        for(int i =0 ;i<aRows*aCols;i++){
          A[i] = ((float)rand()) / ((float)(RAND_MAX));
        }
        for(int i =0 ;i<bRows*bCols;i++){
          B[i] = ((float)rand()) / ((float)(RAND_MAX));
        }
        int numWorkers = numProcesses -1;

        int nRows=0;
        int numExtraRows = 0;
        if(numWorkers >=1){
          nRows = aRows/numWorkers;
          numExtraRows = aRows%numWorkers;
        }

        int startRow = 0;
        MPI_Bcast(&aCols,1,MPI_INT,0,MPI_COMM_WORLD);
        MPI_Bcast(&bCols,1,MPI_INT,0,MPI_COMM_WORLD);
        MPI_Bcast(B,bRowSize*bColSize,MPI_FLOAT,0,MPI_COMM_WORLD);

        for(int workerNumber = 1;workerNumber<=numWorkers;workerNumber++){

          int numRowsSent = nRows;
          if(workerNumber <= numExtraRows)
            numRowsSent++;//send additional row

          MPI_Send(&numRowsSent,1,MPI_INT,workerNumber,0,MPI_COMM_WORLD);
          MPI_Send(&startRow,1,MPI_INT,workerNumber,1,MPI_COMM_WORLD);
          // printf("0 %d\n",numRowsSent * aRowSize);
          MPI_Send( A + startRow * aRowSize, numRowsSent * aRowSize,MPI_FLOAT,workerNumber,2,MPI_COMM_WORLD);
          startRow = startRow + numRowsSent;
        }

        MPI_Status status;
        for(int workerNumber =1; workerNumber <= numWorkers;workerNumber ++){
            int numRowsReceived;
            MPI_Recv(&numRowsReceived,1,MPI_INT,workerNumber,2,MPI_COMM_WORLD,&status);
            MPI_Recv(&startRow,1,MPI_INT,workerNumber,3,MPI_COMM_WORLD,&status);

            printf("from worker = %d, numRowsReceived = %d, startRowIndex = %d\n",workerNumber,numRowsReceived,startRow);

            MPI_Recv(&C[(startRow*aRowSize)],numRowsReceived * bCols,MPI_FLOAT,workerNumber,4,MPI_COMM_WORLD,&status);

            printMatrix(C+startRow*aRowSize,numRowsReceived,bCols);
            // printf("workerNumber = %d\n",workerNumber);
        }


        float * C_serial = (float *)malloc(sizeof(float * )*aRows*bCols);
        Multiply_serial(A,B,C_serial,aRows,aCols,bCols);

        printMatrix(C,aRows,bCols);
        // printMatrix(C_serial,aRows,bCols);
        // printf("IsEqual = %d\n",IsEqual(C_serial,C,aRows,bCols));
    }
    else{
        MPI_Status status;
        int numColsA,numColsB,numRows,numRowsB,startRow;
        const int source = 0;//parent
        numColsA = 0;
        numColsB = 0;
        numRows = 0;
        startRow = 0;
        // printf("Worker Number = %d\n",rank);

        MPI_Bcast(&numColsA,1,MPI_INT,source,MPI_COMM_WORLD);
        numRowsB = numColsA;

        MPI_Bcast(&numColsB,1,MPI_INT,source,MPI_COMM_WORLD);


        float * B = (float *)malloc(sizeof(float)*numColsA * numColsB);

        MPI_Bcast(B,numRowsB * numColsB,MPI_FLOAT,0,MPI_COMM_WORLD);

        MPI_Recv(&numRows,1,MPI_FLOAT,source,0,MPI_COMM_WORLD,&status);

        MPI_Recv(&startRow,1,MPI_FLOAT,source,1,MPI_COMM_WORLD,&status);

        float * A_part = (float *)malloc(sizeof(float)* numRows * numColsA);

        MPI_Recv(A_part,numRows * numColsA,MPI_FLOAT,source,2,MPI_COMM_WORLD,&status);


        float * C = (float *)malloc(sizeof(float)*numRows*numColsB);

        for(int i = 0;i<numRows;i++){
          for(int j =0 ;j<numColsB;j++){
            C[i*numColsB + j ] = 0;
            for(int k = 0;k<numColsA;k++){
              C[i*numColsB + j] += (A_part[i* numColsA + k] * B[k * numColsB + j]);
            }
          }
        }
        // printf("Worker : rank = %d, rows = %d \n",rank,numRows);
        // printMatrix(C,numRows,numColsB);


        MPI_Send(&numRows,1,MPI_INT,source,2,MPI_COMM_WORLD);
        MPI_Send(&startRow,1,MPI_INT,source,3,MPI_COMM_WORLD);
        MPI_Send(C,numRows*numColsB,MPI_INT,source,4,MPI_COMM_WORLD);

    }
    MPI_Finalize();
}
