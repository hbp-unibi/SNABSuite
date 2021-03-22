#!/usr/bin/env python
import argparse
parser = argparse.ArgumentParser(description='Plot slam maps')


parser.add_argument("files", metavar="files", nargs=1, help="files to plot")
parser.add_argument("-o", type=str, help="Output filename", default="")


args = parser.parse_args()


import numpy as np
import matplotlib.pyplot as plt
import os


def cm2inch(value):
    return value / 2.54


fig = plt.figure(figsize=(cm2inch(12), cm2inch(6.0)))
ax = fig.add_subplot(111)

file1 = args.files[0]
results = np.genfromtxt(file1 + "TargetMap.csv", delimiter=',', names=None)
rows = results.shape[0]
cols =  len(results[0]) -1
data = np.zeros((rows, cols))
for i in range(rows):
    for j in range(cols):
        data[i,j] = results[j][i]

results = np.genfromtxt(file1 + "LearntMap.csv", delimiter=',', names=None).reshape((rows,cols))
results2 = np.genfromtxt(file1 + "LearntMapTh.csv", delimiter=',', names=None).reshape((rows,cols))

#plt.figure()
plt.subplot(131)
plt.imshow(data)
plt.subplot(132)
plt.imshow(results)
plt.subplot(133)
plt.imshow(results2)
plt.tight_layout()

if args.o == "":
    if args.files[-1].split('/')[-2]:
        if not os.path.exists("images/" + args.files[-1].split('/')[-2]):
            os.mkdir("images/" + args.files[-1].split('/')[-2])
        fig.savefig("images/" + args.files[-1].split('/')[-2] + "/" + args.files[-1].split(
                '/')[-1].split('_')[0] + ".pdf", format='pdf', bbox_inches='tight')
    else:
        fig.savefig("images/" + args.files[-1].split('/')[-1].split('_')
                    [0] + ".pdf", format='pdf', bbox_inches='tight')
else: 
    fig.savefig(args.o, format='pdf', bbox_inches='tight')
    
