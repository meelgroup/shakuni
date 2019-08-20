```
./counter --seed 21 --rounds 47 --message-bits 495 --hash-bits 6 > tosample
cp tosample /home/soos/development/sat_solvers/scalmc/build/
cp tosample /home/soos/development/sat_solvers/cryptominisat/build/
```

Sample with ScalMC
====

This is the multisample setup, could be slightly skewed:
```
./approxmc --seed 11 tosample  -v1 --samples 100 --sampleout x1 --multisample 1
awk '{print $1}' x1 | sort | uniq -c
```

For more perfect setup, use no multisample:
```
./approxmc --seed 11 tosample  -v1 --samples 100 --sampleout x1 --multisample 0
awk '{print $1}' x1 | sort | uniq -c
```


Sample with CryptoMiniSat
====

Create `sample.sh`:
```
./cryptominisat5 --maxsol 100 --restart geom --maple 0 --verb 0 -n 1 --presimp 0 --polar rnd --freq 0.9999 --random $1 $2
```

Then run:

```
./sampling.sh 1 tosample > x
grep "v 1 " x | wc -l
grep "v -1 " x | wc -l
```

This will give you the disparity. Should be approx 50-50 but it will be much worse, around 98:2.
