#!/usr/bin/bash
./cryptominisat5 --maxsol 100 --nobansol --restart geom --maple 0 --verb 0 --scc 1 -n 1 --presimp 0 --polar rnd --freq 0.9999 --random $1 $2
