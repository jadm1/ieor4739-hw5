#!/usr/bin/python

# Part a) and b)
# This file reads the returns, computes V, F, Q, and D (=Q-VTFV) and saves them to a file

import sys


import numpy as np
import statsmodels.api as sm

# beginning

if len(sys.argv) != 8:
  sys.exit("usage: " + sys.argv[0] + " <returns file name> <best family input file> <V> <F> <Q> <D> <Output Dat file>")

# fetch parameters
returns_filename = sys.argv[1]
family_filename = sys.argv[2]
v_filename = sys.argv[3]
f_filename = sys.argv[4]
q_filename = sys.argv[5]
d_filename = sys.argv[6]
dat_filename = sys.argv[7]

print "reading data..."

# open files 

try:
    returns_file = open(returns_filename, 'r')
except IOError:
    print "Cannot open file %s\n" % returns_filename
    sys.exit("bye")

try:
    family_file = open(family_filename, 'r')
except IOError:
    print "Cannot open file %s\n" % family_filename
    sys.exit("bye")
    
try:
    v_file = open(v_filename, 'w')
except IOError:
    print "Cannot open file %s\n" % v_filename
    sys.exit("bye")
    
try:
    f_file = open(f_filename, 'w')
except IOError:
    print "Cannot open file %s\n" % f_filename
    sys.exit("bye")

try:
    q_file = open(q_filename, 'w')
except IOError:
    print "Cannot open file %s\n" % q_filename
    sys.exit("bye")

try:
    d_file = open(d_filename, 'w')
except IOError:
    print "Cannot open file %s\n" % d_filename
    sys.exit("bye")

try:
    dat_file = open(dat_filename, 'w')
except IOError:
    print "Cannot open file %s\n" % dat_filename
    sys.exit("bye")

# import data from the returns file

N = int(returns_file.readline().split()[1])
T = int(returns_file.readline().split()[1])
dates = returns_file.readline().split()

name = []
returns = np.ndarray(shape=(N,T))
mu = np.ndarray(shape=(N))
assetid_dict = {}

for id in xrange(N):
    
    returns[id,:] = [float("nan") for t in xrange(T)]
    
    line = returns_file.readline().split()
    asset_name = line[0]
    name.append(asset_name)
    assetid_dict[asset_name] = id
    
    line = returns_file.readline().split()
    for t in xrange(T):
        returns[id, t] = float(line[t+1]) 
    
    mu[id] = np.mean(returns[id, :])

returns_file.close()

# read asset family found last time
line = family_file.readline().split()
fN = int(line[1])
print "fN: " + str(fN)
line = family_file.readline().split()
family = []
for i in xrange(fN):
    family.append(line[i])
family_file.close()

print "family: " + str(family)

 # build range of indices from the family asset names
s = []
for asset_name in family:
    s.append(assetid_dict[asset_name])
print "indices: " + str(s)

# initialize V
V = np.ndarray(shape=(fN, N))


x = np.ndarray(shape=(fN, T))
for i in xrange(fN):
    x[i,:] = returns[s[i]]

# OLS linear regression
print "Linear regression ..."
rsquared = 0.0
X = np.transpose(x)
for i in xrange(N): # do regression with all assets to get all coefficients
    results = sm.OLS(returns[i], X).fit()
    # set V's i-th column to be the coefficients returned by the linear regression
    V[:, i] = results.params
    rsquared += results.rsquared
print "R2:" + str(rsquared)

# compute other matrices

F = np.cov(x)
#print "F shape:"+str(F.shape) # show matrix dimensions

Q = np.cov(returns)
#print "Q shape:"+str(Q.shape) # show matrix dimensions

VTFV = np.dot(np.dot(np.transpose(V), F), V)
D = Q - VTFV # residual matrix
D = np.diag(D) # only keep the diagonal (those are the sigma squared coefficients)


# output results to files
print "saving V to %s ..." % v_filename
for i in xrange(fN):
    for j in xrange(N):
        v_file.write(str(V[i, j]) + " ")
    v_file.write("\n")
v_file.close()


print "saving F to %s ..." % f_filename
for i in xrange(fN):
    for j in xrange(fN):
        f_file.write(str(F[i, j]) + " ")
    f_file.write("\n")
f_file.close()


print "saving Q to %s ..." % q_filename
q_file.write("n " + str(N) + "\n")
q_file.write("matrix\n")
for i in xrange(N):
    for j in xrange(N):
        q_file.write(str(Q[i, j]) + " ")
    q_file.write("\n")
q_file.write("end\n")
q_file.close()

print "saving D to %s ..." % d_filename
for i in xrange(N):
    d_file.write(str(D[i]) + " ")
d_file.write("\n")
d_file.close()

# this is a  file that is created in a similar way to f3.dat in lecture 18
# so that it can be used as input for the program
print "saving data to %s ..." % dat_filename
dat_file.write("assets " + str(N) + " factors " + str(fN) + "\n")
dat_file.write("\n")
dat_file.write("asset_returns\n")
for i in xrange(N):
    dat_file.write(str(mu[i]) + "\n") 
dat_file.write("\n")
dat_file.write("asset_upper_bounds\n")
for i in xrange(N):
    dat_file.write(str(1.0) + "\n") # no upper bounds 
dat_file.write("\n")
dat_file.write("asset_residual_variances\n")
for i in xrange(N):
    dat_file.write(str(D[i]) + "\n")
dat_file.write("\n")
dat_file.write("Vtranspose\n")
for i in xrange(N):
    for j in xrange(fN):
        dat_file.write(str(V[j, i]) + " ")
    dat_file.write("\n")
dat_file.write("\n")
dat_file.write("F\n")
for i in xrange(fN):
    for j in xrange(fN):
        dat_file.write(str(F[i, j]) + " ")
    dat_file.write("\n")
dat_file.write("\n")
dat_file.write("END\n")
dat_file.close()

print "done"
sys.stdout.flush()


# end



