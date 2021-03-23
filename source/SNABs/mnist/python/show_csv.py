import matplotlib
matplotlib.use('AGG')
import numpy as np
import pandas as pd
import argparse
import re
import os
import sys
import matplotlib.pyplot as plt
import matplotlib.colors as mpc
from operator import mul

parser = argparse.ArgumentParser()
parser.add_argument('-p', '--path', help='File path', required=True)
parser.add_argument('-n', '--num', help='number of accuracies', type=int, default=3)
parser.add_argument('-g', '--group', help='Group accuracies', type=bool, default=False)
parser.add_argument('-o', '--out', help='Output folder for plotting', type=str, default='plots')
args = parser.parse_args()

if not args.path:
    print('Please give the path to the csv file!')
    print('Usage: show_csv.py -p FILEPATH [-n NUMBER OF accuracies/groups] '
          '[-g if accuracies should be grouped in e.g 0.7-0.79]')
    sys.exit(1)
path = args.out
figsize = (7, 8)
# figsize = (4, 5)

cols = list(pd.read_csv(args.path, nrows=1))

filter_in = [re.compile('[\w]*_weight'), re.compile('accuracy'), re.compile('pool_delay')]
csv_data = pd.read_csv(args.path, usecols=[i for i in cols if any(re.match(fil, i) for fil in filter_in)])

cols = []
data = []
for row, dat in csv_data.items():
    cols.append(row.replace('#', ''))
    data.append(dat)
data = np.array(data)
data = data[:, data[-1].argsort()[::-1]]
cols = cols[:-1]
uni_accuracies = np.unique(data[-1])


def print_tabular():
    if args.group:
        best_accuracy = uni_accuracies[-1]
        upper_thresh = np.floor(best_accuracy*10)/10
        lower_thresh = upper_thresh - 0.1 * (args.num - 1) - 0.0001
        i = lower_thresh
        best_accs = []
        while i < upper_thresh:
            j = i
            # avoid floating point error like 0.70001, which doesn't include 0.7 anymore
            i += 0.099998
            best_accs.append([acc for acc in uni_accuracies if j <= acc < i])
    else:
        best_accs = [[acc] for acc in uni_accuracies[-args.num:]]
    indent = max(8, 4+2*args.num)

    word = 'accuracy groups' if args.group else 'accuracies'

    print(f'Count of best {args.num} {word} in the tables below from right to left:')
    if not args.group:
        print([x[0] for x in best_accs])
    else:
        print([(x[0], x[-1]) for x in best_accs])
    if not args.group:
        print(f'0+1+2 means 0 occurrences of {best_accs[0]}, 1 occurrence of {best_accs[1]} and 2 occurrences of '
              f'{best_accs[2]} for this specific parameters')
    else:
        print(f'0+1+2 means 0 occurrences of {best_accs[0][0]}-{best_accs[0][-1]}, ', end='')
        if len(best_accs) > 1:
            print(f'1 occurrence of {best_accs[1][0]}-{best_accs[1][-1]} ', end='')
        if len(best_accs) > 2:
            print(f'and 2 occurrences of {best_accs[2][0]}-{best_accs[2][-1]} ', end='')
        print(f'for this specific parameters')
    print('')
    for idx in range(len(cols)):
        for idx2 in range(idx + 1, len(cols)):
            print(f'↓ {cols[idx]} → {cols[idx2]}')
            unique1 = np.unique(data[idx])
            unique2 = np.unique(data[idx2])
            print(f"{'':>{8}}", end='')
            for uni2 in unique2:
                print(f'{uni2.round(5):>{indent}}', end='')
            print('')
            for uni1 in unique1:
                print(f'{uni1.round(5):>8}', end='')
                for uni2 in unique2:
                    acc = [d[-1] for d in data.transpose() if d[idx] == uni1 and d[idx2] == uni2]
                    counts = []
                    for best_acc in best_accs:
                        counts.append(sum([x in best_acc for x in acc]))
                    count_str = ''
                    for count in counts:
                        count_str += str(count) + '+'
                    print(f'{count_str[:-1]:>{indent}}', end='')
                print('')
            print('')


def plot_tabular():
    for idx in range(len(cols)):
        for idx2 in range(idx + 1, len(cols)):
            unique1 = np.unique(data[idx])
            unique2 = np.unique(data[idx2])
            if len(unique2) < len(unique1):
                tmp = idx
                idx = idx2
                idx2 = tmp
                tmp = unique1
                unique1 = unique2
                unique2 = tmp
            fig, axs = plt.subplots(nrows=len(unique2), ncols=len(unique1), sharex='all', sharey='all', figsize=figsize)
            fig.suptitle(f'↓ {cols[idx2]} → {cols[idx]}')
            for uni1_idx, uni1 in enumerate(unique1):
                for uni2_idx, uni2 in enumerate(unique2):
                    acc_sum = list(np.zeros(uni_accuracies.shape))
                    for ind, acc in enumerate(uni_accuracies):
                        count_acc = [True for dat in data.transpose()
                                     if dat[idx] == uni1 and dat[idx2] == uni2 and dat[-1] == acc]
                        acc_sum[ind] = len(count_acc)
                    avg_acc = uni_accuracies - uni_accuracies[0]
                    avg_acc = avg_acc/uni_accuracies[-1]
                    avg_acc = list(map(mul, acc_sum, avg_acc))
                    avg_acc = sum(avg_acc)/sum(acc_sum)
                    acc_sum = np.cumsum(acc_sum)
                    hue = (-(1/3)+avg_acc/3)+1
                    hsv_colors = (hue, 1, 1)
                    ax = axs[uni2_idx][uni1_idx]
                    ax.plot(uni_accuracies, acc_sum, c=mpc.hsv_to_rgb(hsv_colors))
                    ax.grid()
                    if uni2_idx == 0:
                        ax.set_title(f'{uni1:.4f}')
                    if uni1_idx == 0:
                        ax.set_ylabel(f'{uni2:.4f}')
            os.makedirs(path, exist_ok=True)
            plt.savefig(f'{path}/{cols[idx2]}_{cols[idx]}.png', bbox_inches='tight')

            if idx2 < idx:
                tmp = idx
                idx = idx2
                idx2 = tmp


print_tabular()
plot_tabular()
