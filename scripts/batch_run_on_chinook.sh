#!/bin/bash


for index in $(eval echo {$2..$3}); do
  # echo "$index"
  #construct batch directory prefix
  echo "${1}/batch-run/batch-$index/slurm_runner.sh"
  sbatch "${1}/batch-run/batch-$index/slurm_runner.sh"
done




