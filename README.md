```
./counter --seed 21   --rounds 47 --message-bits 500 --hash-bits 1 > tosample
cp tosample /home/soos/development/sat_solvers/scalmc/build/
cp tosample /home/soos/development/sat_solvers/cryptominisat/build/
```

ScalMC:
```
./approxmc --seed 11 tosample  -v1 --samples 100 --sampleout x1 --multisample 1
awk '{print $1}' x1 | sort | uniq -c
```

sample.sh:
```
./cryptominisat5 --maxsol 100 --restart geom --maple 0 --verb 0 -n 1 --presimp 0 --polar rnd --freq 0.9999 --random $1 $2
./sampling.sh 1 tosample > x
grep "v 1 " x | wc -l
```
