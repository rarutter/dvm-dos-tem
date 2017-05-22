#!/bin/bash -l
# Job name, for clarity
#SBATCH --job-name="ddtTest"
#
# Output and error files, with job ID included
# temp SBATCH -o ddtTest-%j.out
# temp SBATCH -e ddtTest-%j.err
#
# Is this on the node, or local?
# temp SBATCH --workdir="./testing"
#
# Partition specification
#SBATCH -p main
#
# Node specifications 
#SBATCH --nodes 2
#SBATCH --exclusive    #This ensures dedicated nodes
#
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#equivalent to --ntasks-per-node
#SBATCH --tasks-per-node=1


# Set OMP_NUM_THREADS to the number of cores per task
#  to ensure
#export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

echo $SLURM_JOB_NODELIST
echo "starting loop"

#modify config file?
#need to customize:
#  input file location
#  output location
#  ??

#call ./dvmdostem for each chunk
srun ./dvmdostem -l fatal -p 2 -e 2 -s 2 -t 2 -n 2

