#!/bin/bash
set -e
#set -x

confl=$1
if [ "$#" -ne 1 ]; then
    echo "Illegal number of parameters"
    exit -1
fi

echo "Relatively easy problem (16 rounds):"
./cnf_gen --rounds 16 > out
echo "CryptoMiniSat as CMSGen with $confl conf restart, 100 solutions:"
./sample.sh $confl 1 out  | egrep "v \-?1 " > solutions

echo "Hard solutions found:"
grep "v -1 " solutions | wc -l
echo "Easy solutions found:"
grep "v 1 " solutions | wc -l

echo "CMSGen with $confl restart, 100 solutions:"
set +e
./cmsgen --fixedconfl $confl out
set -e

echo "Hard solutions found:"
egrep "^-1 " samples.out  | wc -l
echo "Easy solutions found:"
egrep "^1 " samples.out  | wc -l

