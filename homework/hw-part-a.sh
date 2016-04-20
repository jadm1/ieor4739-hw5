#!/bin/sh

cd runs
# I set lambda around 30 where the other gamma and Gamma parameters seem to have more influence
../bin/rob ../data/input_rob.txt 30.0 0.1 0.4 x.txt
cp x.txt ../data/x.txt

