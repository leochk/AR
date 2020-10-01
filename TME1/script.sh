#!/bin/bash

mpicc $1

i=0
while [[ $i -lt 10 ]]; do
  mpirun -np $2 --oversubscribe a.out
  echo "==========="
  i=$i+1
done
