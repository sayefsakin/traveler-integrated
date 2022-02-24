#!/bin/bash
cd profiling_tools/clibs
python3 rp_extension_build.py
mv _cCalcBin.*.so ../
rm _cCalcBin.* calcBin.o
cd ../..
python3 serve.py
