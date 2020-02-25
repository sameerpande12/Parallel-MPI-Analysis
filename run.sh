numWorkers=`expr $2 + 1`

mpicc main_nonblocking.c -o main_nonblocking.exe
mpirun -np $numWorkers ./main_nonblocking.exe $1 32 $1

mpicc main_blocking.c -o main_blocking.exe
mpirun -np $numWorkers ./main_blocking.exe $1 32 $1

mpicc main_collective.c -o main_collective.exe
mpirun -np $numWorkers ./main_collective.exe $1 32 $1
