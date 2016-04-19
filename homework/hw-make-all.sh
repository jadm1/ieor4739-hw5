#!/bin/sh
make all
# generate returns
python python/compute_asset_returns.py data/p.txt data/returns.txt
# generate V, F, Q, D, and input file for robust optimization
python python/compute_vfqd.py data/returns.txt data/family.txt data/v.txt data/f.txt data/q.txt data/d.txt data/input_rob.txt
