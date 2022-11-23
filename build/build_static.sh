#!/bin/bash

set -e

rm -rf cm* CM* lib*  Testing* tests* include tests cnf_gen
cmake -DSTATICCOMPILE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j26
strip cryptominisat5
