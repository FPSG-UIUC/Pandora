#!/usr/bin/env python3
import itertools
import numpy as np
from tqdm import tqdm
from time import sleep


class Stat():
    def __init__(self):
        self.data = list()

    def __iadd__(self, other: float):
        self.data.append(other)
        return self

    def comparison(self, other, func) -> bool:
        if len(self) == 0 or len(other) == 0:
            return False

        return func(self.get()[-1], other.get()[-1])

    def __len__(self):
        return len(self.data)

    def get(self) -> float:
        if len(self) > 0:
            return np.mean(self.data), np.std(self.data), \
                    np.mean(self.data[1:]), np.std(self.data[1:])
        return None

    def __str__(self) -> str:
        val, val_std, drop, drop_std = self.get()
        if val is not None:
            return f'{val:.4f},{val_std:.4f},{drop:.4f},{drop_std:.4f}'
        return '---None---'

    def __gt__(self, other: 'Stat') -> bool:
        return self.comparison(other, lambda s,o: s > o)

    def __ge__(self, other: 'Stat') -> bool:
        return self.comparison(other, lambda s,o: s >= o)

    def __lt__(self, other: 'Stat') -> bool:
        return self.comparison(other, lambda s,o: s > o)

    def __le__(self, other: 'Stat') -> bool:
        return self.comparison(other, lambda s,o: s > o)

    def __eq__(self, other: 'Stat') -> bool:
        return self.comparison(other, lambda s,o: s > o)

    def __ne__(self, other: 'Stat') -> bool:
        return self.comparison(other, lambda s,o: s > o)

outname = input('Enter prefix: ')

reps = 128
msg_lens = list()
timing = dict()
for i in range(12):
    msg_lens = itertools.chain(msg_lens, itertools.repeat(2 ** i, reps))
    timing[f'{2 ** i}'] = Stat()


def put_message(message: str) -> (float, float):
    with open('./input_file', 'w') as server_inp:
        server_inp.write(message)

    sleep(1)

    with open('./output_file', 'r') as server_out:
        s_len, s_time = server_out.readline().strip().split(',')
    return s_len, s_time

prev = 0
with tqdm(msg_lens, desc='Same', unit='message',
          total=len(timing) * reps, dynamic_ncols=True) as mbar:
    for l in mbar:
        if l != prev:
            base_msg = itertools.cycle('This is a test message')
            msg = f'{str(l).zfill(4)}'
            for _ in range(len(msg), l + 4):
                msg += str(next(base_msg))
            # tqdm.write(msg + '\n')
            prev = l
            mbar.set_postfix(l=f'{l}')

        s_len, s_time = put_message(msg)
        timing[s_len] += int(s_time)
        # tqdm.write(f'{s_len}: {s_time} ({str(timing[s_len])})')

all_stats = '\n'.join(f'{k},{str(timing[k])},{len(timing[k])}' for k in timing)
print(all_stats)
with open(f'{outname}_same.stats', 'w+') as final_out:
    final_out.write('msg_len,timing,std,timing_warm,std_warm,count\n')
    final_out.write(all_stats)

msg_lens = list()
timing = dict()
for i in range(12):
    msg_lens = itertools.chain(msg_lens, itertools.repeat(2 ** i, reps))
    timing[f'{2 ** i}'] = Stat()

prev = 0
with tqdm(msg_lens, desc='Different', unit='message',
          total=len(timing) * reps, dynamic_ncols=True) as mbar:
    for l in mbar:
        if l != prev:
            base_msg  = itertools.cycle('This is a test message')
            wrong_msg = itertools.cycle('ATOTALLYDIFFERENTMESSAGE')
            msg  = f'{str(l).zfill(4)}'
            wmsg = f'{str(l).zfill(4)}'
            for _ in range(l):
                msg  += str(next(base_msg))
                wmsg += str(next(wrong_msg))
            # tqdm.write(msg + '\n')
            prev = l
            mbar.set_postfix(l=f'{l}')

        s_len, s_time = put_message(msg)
        timing[s_len] += int(s_time)
        s_len, s_time = put_message(wmsg)
        timing[s_len] += int(s_time)
        # tqdm.write(f'{s_len}: {s_time} ({str(timing[s_len])})')

all_stats = '\n'.join(f'{k},{str(timing[k])},{len(timing[k])}' for k in timing)
print(all_stats)
with open(f'{outname}_diff.stats', 'w+') as final_out:
    final_out.write('msg_len,timing,std,timing_warm,std_warm,count\n')
    final_out.write(all_stats)

