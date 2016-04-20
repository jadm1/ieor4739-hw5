#!/bin/sh

cd runs
# rebalance every 90 periods with epsilon j's = 0.01
../bin/pfrebsim ../data/x.txt ../data/p.txt -q 100 -w 4 -b 1000000000 -p 504 -z 1e-6 -rp 90 -re 0.01 -op pf_values.txt -or pf_returns.txt  -ov pf_vars.txt
# no rebalancing
../bin/pfrebsim ../data/x.txt ../data/p.txt -q 100 -w 4 -b 1000000000 -p 504 -z 1e-6 -rp 0


