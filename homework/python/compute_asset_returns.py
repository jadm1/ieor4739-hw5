#!/usr/bin/python

import sys

from datetime import date, timedelta
from pandas import *
from math import isnan

# counts the number of days between two dates
def date_from_str(date_str):
    y, m, d = map(int, date_str.split('-'))
    return date(y, m, d)

def date_to_str(date):
    return str(date.year)+"-"+str(date.month)+"-"+str(date.day)

def days_elapsed(ref_date_str, cur_date_str):
    delta = date_from_str(cur_date_str) - date_from_str(ref_date_str)
    return delta.days

def next_day(day_str):
    return date_to_str(date_from_str(day_str) + timedelta(days=1))

def prev_day(day_str):
    return date_to_str(date_from_str(day_str) + timedelta(days=-1))

def week_day_num(day_str):
    return date_from_str(day_str).weekday()


############## beginning

if len(sys.argv) != 3:
  sys.exit("usage: " + sys.argv[0] + " <assets file name> <output returns file name>")

assets_filename = sys.argv[1]
returns_filename = sys.argv[2]

print "reading data from %s ..." % assets_filename

# opening input file
try:
    assets_file = open(assets_filename, 'r')
except IOError:
    print "Cannot open file %s\n" % assets_filename
    sys.exit("bye")

# opening output file
try:
    returns_file = open(returns_filename, 'w')
except IOError:
    print "Cannot open file %s\n" % returns_filename
    sys.exit("bye")


################# reading data from input file

N = int(assets_file.readline().split()[1])
T = int(assets_file.readline().split()[1])
dates = assets_file.readline().split()

name = {}
volume = {}
adj_close = {}
high = {}
low = {}
close = {}
open = {}
asset_return = {}

for id in xrange(N):
    
    volume[id] = [float("nan") for t in xrange(T)]
    adj_close[id] = [float("nan") for t in xrange(T)]
    high[id] = [float("nan") for t in xrange(T)]
    low[id] = [float("nan") for t in xrange(T)]
    close[id] = [float("nan") for t in xrange(T)]
    open[id] = [float("nan") for t in xrange(T)]
    asset_return[id] = [float("nan") for t in xrange(T - 1)]
    
    line = assets_file.readline().split()
    asset_name = line[0]
    name[id] = asset_name
    #line = assets_file.readline().split()
    #for t in xrange(T):
    #    volume[id][t] = float(line[t+1])
    line = assets_file.readline().split()
    for t in xrange(T):
        adj_close[id][t] = float(line[t+1])
    #line = assets_file.readline().split()
    #for t in xrange(T):
    #    high[id][t] = float(line[t+1])
    #line = assets_file.readline().split()
    #for t in xrange(T):
    #    low[id][t] = float(line[t+1])
    #line = assets_file.readline().split()
    #for t in xrange(T):
    #    close[id][t] = float(line[t+1])
    #line = assets_file.readline().split()
    #for t in xrange(T):
    #    open[id][t] = float(line[t+1])

assets_file.close()

###################### process data ################
print "computing asset return deviations ..."

for id in xrange(N):
    #print "processing asset " + name[id]
    avg_return = 0
    for t in xrange(T - 1): # from 0 to T-2 (length T-1)
        asset_return[id][t] = (adj_close[id][t+1] - adj_close[id][t])/adj_close[id][t]
        avg_return += asset_return[id][t]/(T-1)
    # subtract the average
    #for t in xrange(T - 1):
    #    asset_return[id][t] -= avg_return


# output results to output file
print "saving data to %s ..." % returns_filename

returns_file.write("N: "+str(N)+"\n")
returns_file.write("T: "+str(T-1)+"\n")
dstr = ""
for d in dates:
	dstr += d + " "
returns_file.write(dstr+"\n")

for id in xrange(N):
    returns_file.write(name[id])
    returns_file.write("\n")
    returns_file.write("Returns: ")
    for t in xrange(T - 1):
        returns_file.write(str(asset_return[id][t]) + " ")
    returns_file.write("\n")

returns_file.close()
print "done"


################### end



