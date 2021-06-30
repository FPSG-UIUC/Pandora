import numpy as np
import pandas as pd
import argparse
import matplotlib.pyplot as plt
import pickle
from tqdm import tqdm
from itertools import cycle

from inst_parse import Inst, bcolors

parser = argparse.ArgumentParser()
parser.add_argument('pickle', help='Pickled committed instructions')
parser.add_argument('start_pc', type=str)
parser.add_argument('end_pc', type=str)
parser.add_argument('--verbose', '-v', action='store_true')
parser.add_argument('--ticks', '-t', action='store_true')
parser.add_argument('--steady_state', '-s', action='store_true')
args = parser.parse_args()

com_inst = pickle.load(open(args.pickle, 'rb'))

call_type = ['correct', 'incorrect', 'victim']
call_type = [s.ljust(9, ' ') for s in call_type]
guess_type: int = -1

changed = 0
color = bcolors.ENDC

roi = [args.start_pc, args.end_pc]

state_count = 0

if args.verbose: print(len(com_inst))

for sn in com_inst:
    if args.verbose: print(com_inst[sn])

    if '0xfeed' in com_inst[sn].full_inst:  # correct
        color = bcolors.OKGREEN
        changed = 2
        guess_type = 0
        if args.verbose: print('Corr region'.ljust(40, '-'))
        state_count += 1

    elif '0xbeef' in com_inst[sn].full_inst:  # incorrect
        color = bcolors.OKBLUE
        changed = 2
        guess_type = 1
        if args.verbose: print('ICorr region'.ljust(40, '-'))
        state_count += 1

    elif '0xdead' in com_inst[sn].full_inst:  # victim
        guess_type = 2
        color = bcolors.ENDC
        if args.verbose: print('Victim region'.ljust(40, '-'))
        continue

    elif '0xbead' in com_inst[sn].full_inst:  # outside of ROI
        guess_type = -1
        if args.verbose: print('non-ROI region'.ljust(40, '-'))
        continue

    if guess_type == -1:
        continue

    if com_inst[sn].pc in roi and changed > 0:  # clflush before target
        if args.verbose: print(f'Found {com_inst[sn]}')

        if not args.ticks:
            if not args.steady_state or state_count >= 5:
                print(color if changed else '',
                    com_inst[sn].pc, call_type[guess_type],
                      com_inst[sn].get_tick('retire'), bcolors.ENDC)
                # if not args.verbose: print(bcolors.ENDC)

                if args.verbose: print(f'states: {state_count}'.center(50, '-'))

        else:
            if not args.steady_state:
                print(com_inst[sn].get_tick('retire'))
            elif state_count >= 5:
                print(com_inst[sn].get_tick('retire'))

            if args.verbose: print(f'states: {state_count}'.center(50, '-'))

        # changed = com_inst[sn].pc != roi[-1]
        changed -= 1

        if args.steady_state and state_count >= 6 and changed == 0:
            break

    elif args.verbose:
        if args.verbose: print(f'Analyzing {com_inst[sn].pc}')
