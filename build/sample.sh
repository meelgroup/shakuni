#!/usr/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Illegal number of parameters"
    exit -1
fi

# TODO: add --breakid 0
./cryptominisat5 --maxsol 100 --nobansol --restart fixed --maple 0 --verb 0 --scc 1 -n 1 --presimp 0 --polar rnd --freq 0.9999 --fixedconfl $1 --random $2 $3
