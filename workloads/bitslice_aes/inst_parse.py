#/usr/bin/env python3

import argparse
import pandas as pd
import numpy as np
import pickle

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

class Inst():
    def __init__(self, fetch_line: str, short: bool =False):
        self.short: bool = short
        self.stages = dict()

        fetch_line = fetch_line.split(":")

        self.full_inst: str = " : ".join(fetch_line[-2:])
        self.sn: str = f'sn:{fetch_line[5]}'.rjust(10, ' ')
        self.pc: str = fetch_line[3]

        self.add_stage(fetch_line[1], int(fetch_line[2]))

        self.squashed = True

    def add_stage(self, stage_name: str, tick: int):
        assert(self.stages.get(stage_name) is None), \
                f'Duplicate {stage_name} stage?'

        self.stages[stage_name] = tick

        if stage_name == 'retire' and tick != 0:
            self.squashed = False

    def get_tick(self, stage_name: str):
        assert(self.stages.get(stage_name) is not None), 'Invalid stage'

        return self.stages[stage_name]

    def get_events(self):
        vals = list()
        for st in ['fetch', 'decode', 'rename', 'dispatch', 'issue', 'complete',
                   'retire', 'store']:
            vals.append(self.stages[st] if self.stages[st] > 0 else None)
        return vals

    def seqnum(self) -> str:
        return self.sn

    def store_time(self):
        if self.stages['store'] == 0:
            return 0

        return self.stages['store'] - self.stages['retire']

    def times(self):
        ret = {
            'fetch': self.stages['retire'] - self.stages['fetch'],
            'decode': self.stages['retire'] - self.stages['decode'],
            'rename': self.stages['retire'] - self.stages['rename'],
            'dispatch': self.stages['retire'] - self.stages['dispatch'],
            'issue': self.stages['retire'] - self.stages['issue'],
            'complete': self.stages['retire'] - self.stages['complete']
            # 'retire': self.stages['store'] - self.stages['retire']
        }
        if self.stages['store'] != 0:
            ret['retire'] = self.stages['store'] - self.stages['retire']
        else:
            ret['retire'] = 'NA'

        return ret

    def __str__(self) -> str:
        info = ','.join([f'{self.sn}',
                         f'{self.pc}',
                         f'{self.full_inst}'
                        ])

        times = self.times()
        info += f' ({" : ".join([str(times[k]) for k in times])})'

        if self.short:
            return info

        stages = '\n'.join([f'{stage}:{self.stages[stage]}' for stage in
                            self.stages])
        return info + '\n' + stages

    def compare(self, other_tick: int, func):
        return func(self.get_tick('fetch'), other_tick)

    def __le__(self, other):
        return self.compare(other, lambda x,y: x <= y)

    def __ge__(self, other):
        return self.compare(other, lambda x,y: x >= y)

    def __lt__(self, other):
        return self.compare(other, lambda x,y: x < y)

    def __gt__(self, other):
        return self.compare(other, lambda x,y: x > y)


def get_next():
    while True:
        # get first fetch
        try:
            fetch = input()
            while 'fetch' not in fetch or 'O3PipeView' not in fetch:
                if args.verbose:
                    print(fetch)
                fetch = input()
        except EOFError:
            break

        if args.verbose:
            print(fetch)

        n_inst = Inst(fetch, short=args.stages)
        for _ in range(6):
            inp = input().split(':')
            if inp[0] != 'O3PipeView':
                break
            n_inst.add_stage(inp[1], int(inp[2]))

        if inp[0] != 'O3PipeView':
            continue

        # retire also has store
        n_inst.add_stage(inp[3], int(inp[4]))

        yield n_inst

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', '-v', action='store_true')
    parser.add_argument('--max_runs', '-m', type=int, default=None)
    parser.add_argument('--stages', '-s', action='store_false')
    parser.add_argument('outfile', type=str, help='pickle file to save to')
    parser.add_argument('target_pc', type=str)
    args = parser.parse_args()

    all_inst = dict()
    com_inst = dict()

    for inst in get_next():
        assert all_inst.get(inst.seqnum()) is None, f'Duplicate SN {inst.seqnum()}'
        all_inst[inst.seqnum()] = inst
        if not inst.squashed:
            # print(inst)
            com_inst[inst.seqnum()] = inst

# correct, incorrect, victim
    guesses = [dict(), dict(), dict()]
    call_type = ['correct', 'incorrect', 'victim']
    call_type = [s.ljust(9, ' ') for s in call_type]
    guess_type:int = -1

    last: Inst = ''

    printed = 0

    for inst in com_inst:
        if args.verbose: print('Com: ', com_inst[inst])

        if '0xfeed' in com_inst[inst].full_inst:  # correct
            print('oi', call_type[guess_type], last)
            guess_type = 0
            last = None
            if args.verbose:
                print(call_type[guess_type], com_inst[inst])
            continue

        elif '0xbeef' in com_inst[inst].full_inst:  # incorrect
            print('oi', call_type[guess_type], last)
            guess_type = 1
            last = None
            if args.verbose:
                print(call_type[guess_type], com_inst[inst])
            continue

        elif '0xdead' in com_inst[inst].full_inst:  # victim
            guess_type = 2
        elif '0xbead' in com_inst[inst].full_inst:  # from clear_and_trash
            guess_type = -1

        if guess_type == -1:  # not from ROI
            continue

        if com_inst[inst].pc == args.target_pc:
            if last is None:
                print((bcolors.OKGREEN if guess_type == 0 else bcolors.OKBLUE)
                      + 'oi', call_type[guess_type], com_inst[inst],
                      bcolors.ENDC, '\n')
                printed += 1
            elif args.verbose:
                print(call_type[guess_type], com_inst[inst])
            guesses[guess_type][inst] = com_inst[inst]

            last = com_inst[inst]

            if args.max_runs is not None and printed >= args.max_runs:
                break

    if args.verbose:
        for inst in com_inst:
            print('Com2: ', com_inst[inst])

    pickle.dump(com_inst, open(args.outfile, 'wb'))
    pickle.dump(all_inst, open('/'.join(args.outfile.split('/')[:-1]) + '/' +
                               'all_' + args.outfile.split('/')[-1], 'wb'))
