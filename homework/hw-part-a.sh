#!/bin/sh

cd runs
../bin/rob ../data/input_rob.txt 0.0 0.2 0.2 x.txt
cp x.txt ../data/x.txt

