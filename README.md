Shakuni -- approximate counting & sampling tester
===========================================

Example use:

```
./counter --seed 23 --easy 11 --rounds 30 --message-bits 495 --hash-bits 6 > tosample
num hard solutions : 2056
num easy solutions : 2048
num total solutions: 4104
ratio should be    : 0.4990 vs 0.5010
```

The total number of message bits is 512, we set 495 of them and let 17 of them "loose". Then, we force 6 bits of the output to be a fixed value. This leaves 11 bits of room, hence there is approx 2^11 solutions to the "hard" part of the solution space.

The "easy" part of the solution space has 2^11 bits unset, hence has exactly 2^11 solutions.


Sample with ScalMC
-----

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
-----

Create `sample.sh`:
```
./cryptominisat5 --maxsol 100 --restart geom --maple 0 --verb 0 -n 1 --presimp 0 --polar rnd --freq 0.9999 --random $1 $2
```

Then run:

```
./sampling.sh 1 tosample > x
$ egrep "v -?1 " x | awk '{print $2}' | sort | uniq -c
```

This will give you the disparity. Should be approx 50-50 but it will be much worse, around 98:2.


Uneven solution space
-----

This creates 2^8 easy and 2^11 hard solutions:

```
./counter --easy 8 --seed 23   --rounds 30 --message-bits 495 --hash-bits 6 > tosample
num hard solutions : 2056
num easy solutions : 256
num total solutions: 2312
easy vs hard ratio : 0.1107 vs 0.8893
```

ScalMC gives:

```
$ awk '{print $1}' x1 | sort | uniq -c
99 -1
11 1
```

Whereas CMS gives

```
$ egrep "v -?1 " x | awk '{print $2}' | sort | uniq -c
      9 -1
     91 1
```

So completely off, the **wrong way around**


SharpSAT countable solutions
---

```
$ ./counter --easy 10 --seed 25   --rounds 10 --message-bits 500 --hash-bits 2 > tosample
num hard solutions : 976
num easy solutions : 1024
num total solutions: 2000
easy vs hard ratio : 0.5120 vs 0.4880
```

Running sharpSAT:

```
$ ./sharpSAT tosample
[...]
# solutions
2000
# END

time: 3.07024s
```

ScalMC counts this as:

```
./approxmc --seed 13 tosample  -v1 --samples 100 --sampleout x1 --multisample 1
[...]
[appmc] finished counting solutions in 3.22 s
[appmc] Number of solutions is: 63 x 2^5
```


Large number of solutions
---

```
$ ./counter --easy 19 --seed 25   --rounds 10 --message-bits 490 --hash-bits 4 > tosample
num hard solutions : 648192
num easy solutions : 524288
num total solutions: 1172480
```

SharpSAT:

```
./sharpSAT tosample
[...]
# solutions
1172480
# END

time: 3.07537s
```


Other interesting examples
---

```
./counter --easy 20 --seed 25   --rounds 14 --message-bits 488 --hash-bits 8 > tosample
num hard solutions : 67536
num easy solutions : 1048576
num total solutions: 1116112
easy vs hard ratio : 0.9395 vs 0.0605
```

ScalMC is again correct, but CMS gives:

```
soos@vvv-dejavu:build$ egrep "v -?1 " x | awk '{print $2}' | sort | uniq -c
     45 -1
     55 1
```

Which is wrong


# Mate's notes
below works with sampler's seed at 1 or 4, both are giving correct/wrong answers.

./counter --easy 8 --seed 25   --rounds 13 --message-bits 500 --hash-bits 4 > tosample
num hard solutions : 288
num easy solutions : 256
num total solutions: 544
easy vs hard ratio : 0.4706 vs 0.5294
./sample.sh 1 tosample  > x
egrep "v -?1 " x | awk '{print $2}' | sort | uniq -c
     54 -1
     46 1



./counter --easy 8 --seed 25   --rounds 23 --message-bits 500 --hash-bits 4 > tosample
num hard solutions : 241
num easy solutions : 256
num total solutions: 497
easy vs hard ratio : 0.5151 vs 0.4849
./sample.sh 1 tosample  > x
egrep "v -?1 " x | awk '{print $2}' | sort | uniq -c
      1 -1
     99 1


