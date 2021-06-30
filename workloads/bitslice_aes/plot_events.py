import argparse
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from tqdm import tqdm
import pickle

from inst_parse import Inst

parser = argparse.ArgumentParser()
parser.add_argument('pickle', help='Pickled committed instructions')
parser.add_argument('target')
parser.add_argument('delaying', nargs='+')
parser.add_argument('--save_path', '-p', default=None, type=str,
                    help='Save plots instead of showing them')
parser.add_argument('--verbose', '-v', action='store_true')
parser.add_argument('--store_verbose', '-s', action='store_true')
args = parser.parse_args()

assert('0x' in args.target)
for delaying in args.delaying:
    assert('0x' in delaying)

com_inst = pickle.load(open(args.pickle, 'rb'))

def get_next():
    while True:
        try:
            state = input()
            roi_type = state.split('/')[-1].split('.')[0]
            tick = int(state.split(':')[1])

            ret = dict()

            if state.split(' ')[-1] == 'instructions.':
                ret['rob_entries'] = int(state.split(' ')[-2])
            else:
                ret['sq_entries'] = int(state.split(' ')[-1])

        except EOFError:
            break

        yield (roi_type, ret, tick)


sq_events = {ev_ty: dict() for ev_ty in ['icorr_roi', 'corr_roi']}

start_tick = {ev_ty: None for ev_ty in ['icorr_roi', 'corr_roi']}
end_tick = {ev_ty: None for ev_ty in ['icorr_roi', 'corr_roi']}

full_color_list = iter([f'tab:{c}' for c in ['pink', 'brown', 'purple']])
named_colors = dict()

for ev_ty, sq_ev, ev_ti in get_next():
    if start_tick[ev_ty] is None or ev_ti < start_tick[ev_ty]:
        start_tick[ev_ty] = ev_ti
    if end_tick[ev_ty] is None or ev_ti > end_tick[ev_ty]:
        end_tick[ev_ty] = ev_ti

    for event in sq_ev:
        if sq_events[ev_ty].get(event) is None:
            sq_events[ev_ty][event] = dict()
            sq_events[ev_ty][event][f'{ev_ty[:-3]}{event}'] = list()
            sq_events[ev_ty][event]['tick'] = list()

            if named_colors.get(event) is None:
                named_colors[event] = next(full_color_list)

        sq_events[ev_ty][event][f'{ev_ty[:-3]}{event}'].append(sq_ev[event])
        sq_events[ev_ty][event]['tick'].append(ev_ti)

print(start_tick, end_tick)

color = iter(['tab:red'] * 4 + ['tab:blue'] * 4)

special_insts = {d: [f'delaying{i}', next(color), '+-'] for i,d in
                 enumerate(args.delaying)}
special_insts[args.target] = ['target', 'tab:green', '.-']
print(special_insts)

ax = None
fig = plt.figure(figsize=(20, 10))

name = list()
for info in args.pickle.split('/')[-2].split('_'):
    if 'sq' in info:
        name.append(info)
    elif 'asc' in info:
        name.append(info)
    elif 'dram' in info:
        name.append(info)
if len(name) == 0:
    print('Failed to find name')
    name = ['']

fig.suptitle('_'.join(name))
all_ax = list()

titles = {'corr_roi': 'silent', 'icorr_roi': 'not silent'}

for idx, ev_ty in enumerate(sq_events):
    if ax is not None:
        ax = fig.add_subplot(2, 1, idx + 1, sharex=ax)
    else:
        ax = fig.add_subplot(2, 1, idx + 1)
    all_ax.append(ax)
    ax.set_title(titles[ev_ty])

    ax2 = ax.twinx()
    ax2.set_yticks([v for v in range(8)])
    ax2.set_yticklabels(['fetch', 'decode', 'rename', 'dispatch', 'issue',
                         'complete', 'retire', 'store'])

    for inst in tqdm(com_inst):
        if com_inst[inst] >= start_tick[ev_ty] and \
           com_inst[inst] <= end_tick[ev_ty]:

            if com_inst[inst].pc in special_insts:
                tqdm.write(f'{ev_ty}: '.ljust(11, ' ') +
                           f'{special_insts[com_inst[inst].pc][0]}: '.ljust(
                               11, ' ') +
                           f'{com_inst[inst].sn}')
                tqdm.write('\t' + str(com_inst[inst]))
                pd_inst = pd.DataFrame.from_records({
                    special_insts[com_inst[inst].pc][0]: [v for v in range(8)],
                    'tick': com_inst[inst].get_events()
                })

                pd_inst['tick'] -= start_tick[ev_ty]

                if pd_inst['tick'].isnull().sum() > 0:
                    sty = 'x-'
                else:
                    sty = special_insts[com_inst[inst].pc][2]

                pd_inst.plot(x='tick', ax=ax2,
                             style=sty,
                             markersize=20, linewidth=3,
                             drawstyle='steps-post',
                             color=special_insts[com_inst[inst].pc][1])
                # tqdm.write(str(com_inst[inst]))

            elif (args.store_verbose or args.verbose) and \
                    'st' in com_inst[inst].full_inst:
                pd_inst = pd.DataFrame.from_records({
                    com_inst[inst].pc: [v for v in range(8)],
                    'tick': com_inst[inst].get_events()
                })

                pd_inst['tick'] -= start_tick[ev_ty]

                if pd_inst['tick'].isnull().sum() > 0:
                    sty = '^--'
                else:
                    sty = 'v--'

                pd_inst.plot(x='tick', ax=ax2, style='v--', markersize=8,
                             drawstyle='steps-post', legend=False, linewidth=1)

            elif args.verbose:
                pd_inst = pd.DataFrame.from_records({
                    com_inst[inst].pc: [v for v in range(8)],
                    'tick': com_inst[inst].get_events()
                })

                pd_inst['tick'] -= start_tick[ev_ty]

                # print(pd_inst)
                if pd_inst['tick'].isnull().sum() > 1:
                    sty = 'x:'
                else:
                    sty = '.:'

                pd_inst.plot(x='tick', ax=ax2, style=sty, markersize=8,
                             drawstyle='steps-post', legend=False, linewidth=1)

    ax2.grid()

    for event in sq_events[ev_ty]:
        pd_sq_events = pd.DataFrame.from_records(sq_events[ev_ty][event])
        pd_sq_events['tick'] -= start_tick[ev_ty]
        pd_sq_events.plot(x='tick', ax=ax, style='+-', drawstyle='steps-post',
                          linewidth=3, color=named_colors[event])

    ax.grid(which='both', axis='x')

fig.tight_layout()
# plt.xlim([100000, 250000])
if args.save_path is None:
    plt.show()
else:
    plt.savefig(args.save_path)

for _ in range(20):
    plt.close()
