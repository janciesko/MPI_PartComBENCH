binary=$1
hosts=$2
if [ x"$binary" = x ]; then
  echo "Usage: sh run.sh binary hostlist"
  echo "hostlist : host1,host2"
  exit 1
fi

date
mpirun -n 2 --host ${hosts} hostname

total_partlen=2048
num_tasks=64
num_threads=64
num_part=$num_tasks

export QTHREAD_STACK_SIZE=16384
export OMPI_MCA_pml=ob1

for size in {1..3..1}; do 
    let num_partlen=($total_partlen/$num_tasks)
    echo "mpirun --map-by ppr:2:node --host  $hosts ./bench1 $num_tasks $num_threads $num_part $num_partlen"
    mpirun --map-by ppr:2:node --host  $hosts ./bench1 $num_tasks $num_threads $num_part $num_partlen
    let total_partlen=$total_partlen*2
done

