import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('repetitions', type=int,
                    help='Number of repetitions for each type')
parser.add_argument('rdtsc_outputs', help='csv of rdtsc outputs', nargs='+')
parser.add_argument('--verbose', '-v', action='store_true',
                    help='Display non-matching runs too')
parser.add_argument('--single', '-s', action='store_true',
                    help='Generate final versions')
parser.add_argument('--display', '-d', action='store_true',
                    help='Display instead of saving')
parser.add_argument('--max', '-m', type=int, default=None,
                    help='Static cutoff value for timing')
args = parser.parse_args()


def show_raw(pd_dat):
    rolls = [2, 5, 10, 20]
    rolls = [r for r in rolls if r < args.repetitions]

    if not args.verbose:
        ncols = 1
    else:
        ncols = len(pd_dat) // args.repetitions
    nrows = 2 + len(rolls)

    print(ncols)

    fig = plt.figure(figsize=(ncols * 5, nrows * 4))

    for r in range(0, len(pd_dat) if args.verbose else args.repetitions,
                   args.repetitions):
        end = r + args.repetitions
        idx_base = r // args.repetitions + 1
        title = 'Matching ptext' if r == 0 else 'Different ptext'

        ax = fig.add_subplot(nrows, ncols, idx_base)
        ax.set_title(f'{title} histogram')
        pd_dat[r:end].plot.hist(ax=ax)
        idx_base += 1

        ax = fig.add_subplot(nrows, ncols, idx_base)
        ax.set_title(title + '(raw data)')

        std_filter = pd_dat[r:end] - pd_dat[r:end].mean() < 2 * pd_dat[r:end].std()

        pd_dat[r:end][std_filter].plot(style='.', ax=ax)

        for ridx, roll in enumerate(rolls):
            ax = fig.add_subplot(nrows, ncols, idx_base + ncols * (ridx + 1))
            ax.set_title(f'{title} (roll={roll})')
            pd_dat[r:end].rolling(roll).mean().plot(style='.', ax=ax,
                                yerr=pd_dat[r:end].rolling(roll).std())

    print(pd_dat[:args.repetitions].mean())
# print(pd_dat[:args.repetitions].std())
    diff = pd_dat[:args.repetitions].mean()["wrong"] - \
            pd_dat[:args.repetitions].mean()["correct"]
    print(f'{diff:.4f} Cycle difference')

    fig.tight_layout()
    plt.show()


ncols = len(args.rdtsc_outputs)
if args.verbose: nrows = 4
elif args.single: nrows = 1
else: nrows = 2
fig = plt.figure(figsize=(ncols * 4, nrows * 3))
idx_base = 1
all_dat = dict()
for rdtsc_output in args.rdtsc_outputs:
    dat = dict()
    with open(rdtsc_output) as in_f:
        for line in in_f:
            line = line.split(',')
            dat[line[0]] = [int(v) for v in line[1:]]

    pd_dat = pd.DataFrame.from_records(dat)
    pd_dat.index.rename('Repetition', inplace=True)
    pd_dat.to_csv(rdtsc_output + '.parsed.csv')
    all_dat[rdtsc_output] = pd_dat

    name = list()
    for param in rdtsc_output.split('_'):
        if 'dram' in param:
            ns = param.split('ns')[0][-2:]
            if ns == 'al':
                ns = '21'
            if 'real' in param:
                name.append('DDR4')
            elif not args.single:
                name.append('simple_' + ns + 'ns')
        elif 'alloc' in param and not args.single:
            name.append(param)
        elif 'single' in param:
            name.append(param)
        elif 'double' in param:
            name.append(param)

    if len(name) == 0:
        name = 'default mem model'
    else:
        name = ' '.join(name)

    print(rdtsc_output)
    print('Means:')
    if args.max is None:
        print(pd_dat[:args.repetitions].mean())
        diff = pd_dat[:args.repetitions]['wrong'].mean() - \
                pd_dat[:args.repetitions]['correct'].mean()

    else:
        max_filt = pd_dat[:args.repetitions] < args.max
        print(pd_dat[:args.repetitions][max_filt].mean())
        diff = pd_dat[:args.repetitions][max_filt]['wrong'].mean() - \
                pd_dat[:args.repetitions][max_filt]['correct'].mean()

    print('Delta = ', diff)

    name += f' (∆={diff:.2f})'

    ax = fig.add_subplot(nrows, ncols, idx_base)
    ax.set_title(name)
    value_count = len(pd_dat.value_counts())
    # print(value_count, value_count//3*2)
    if args.max is not None:
        pd_dat[:args.repetitions][max_filt].plot.hist(
            ax=ax, alpha=0.8, bins=100)
    else:
        # pd_dat[:args.repetitions].plot.hist(ax=ax, alpha=0.8,
        # bins=value_count//3*2)
        pd_dat[:args.repetitions].plot.hist(ax=ax, alpha=0.8, bins=200)

    if not args.single:
        # print('idx:', idx_base, idx_base + len(args.rdtsc_outputs))
        ax = fig.add_subplot(nrows, ncols, idx_base + len(args.rdtsc_outputs))
        ax.set_title(name + ' raw')
        pd_dat[:args.repetitions].plot(ax=ax, style='.')

        if args.verbose:
            ax = fig.add_subplot(nrows, ncols, idx_base + 2 *
                                 len(args.rdtsc_outputs))
            ax.set_title(name)
            value_count = len(pd_dat.value_counts())
            # print(value_count, value_count//3*2)
            if args.max is not None:
                pd_dat[args.repetitions:][max_filt].plot.hist(
                    ax=ax, alpha=0.8, bins=100)
            else:
                # pd_dat[:args.repetitions].plot.hist(ax=ax, alpha=0.8,
                # bins=value_count//3*2)
                pd_dat[args.repetitions:].plot.hist(ax=ax, alpha=0.8, bins=200)

            # print('vidx:', idx_base + 2 * len(args.rdtsc_outputs),
            #       idx_base + 3 * len(args.rdtsc_outputs))
            ax = fig.add_subplot(nrows, ncols,
                                 idx_base + 3 * len(args.rdtsc_outputs))
            ax.set_title(name + ' raw')
            pd_dat[args.repetitions:].plot(ax=ax, style='.')

    idx_base += 1


fig.tight_layout()
if not args.display:
    plt.savefig('histograms.pdf')
else:
    plt.show()
plt.close()


