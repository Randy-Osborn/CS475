#!/bin/bash

# Load the OpenMPI module
module load openmpi

# Compile the MPI program
mpic++ proj07.cpp -o proj07 -lm

# Array of process counts to run the program with
process_counts=(1 2 4 6 8)

# Loop over each process count and run the program
for np in "${process_counts[@]}"
do
  echo "Running with $np processes"
  mpiexec -mca btl self,tcp -np $np ./proj07
done
