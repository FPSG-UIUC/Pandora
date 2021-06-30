#!/usr/bin/env python

import argparse
from functools import partial
from tqdm import tqdm

class slst():
    def __init__(self):
        self.issued = 0
        self.received = 0
        self.received_squashed = 0
        self.dropped = 0
        self.in_flight = dict()
        self.all_time = dict()
        self.mif = dict()

    def send(self, seqnum):
        self.issued += 1

        if seqnum in self.in_flight:
            self.in_flight[seqnum] += 1
        else:
            self.in_flight[seqnum] = 1

        if seqnum in self.all_time:
            self.all_time[seqnum] += 1
        else:
            self.all_time[seqnum] = 1

    def decrement(self, seqnum):
        self.in_flight[seqnum] -= 1
        if self.in_flight[seqnum] == 0:
            self.in_flight.pop(seqnum)

    def receive(self, seqnum):
        self.received += 1
        self.decrement(seqnum)

    def receive_squashed(self, seqnum):
        self.received_squashed += 1
        self.decrement(seqnum)

    def drop(self, seqnum):
        self.dropped += 1
        self.decrement(seqnum)

    def sn_if(self, cond, sn):
        return sn if cond(self.all_time[sn]) else -1

    def __gt__(self, value):
        gt = partial(self.sn_if, lambda x: x > value)
        res = map(gt, self.all_time.keys())
        return [s for s in res if s != -1]

    def sum_sent(self, inst_list):
        return sum([self.all_time[s] for s in inst_list])

    def remaining(self):
        return [k for k in self.in_flight.keys()]

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('filename')
    args = parser.parse_args()

    observed_insts = slst()

    with open(args.filename, 'r') as inst_list:
        for line in inst_list:
            seqnum, pkt_type = line.split(' ')
            pkt_type = pkt_type.strip()

            if pkt_type == '1-issued':
                observed_insts.send(seqnum)
            elif pkt_type == '2-received':
                observed_insts.receive(seqnum)
            elif pkt_type == '3-dropping':
                observed_insts.drop(seqnum)
            elif pkt_type == '4-received_squashed':
                observed_insts.receive_squashed(seqnum)
            else:
                print(pkt_type)
                raise NotImplementedError

    print(f'{observed_insts.issued} silent-store loads issued')
    print(f'{observed_insts.received} silent-store loads received')
    print(f'{observed_insts.received_squashed} silent-store loads received sq')
    print(f'{observed_insts.dropped} silent-store loads dropped')

    missing = observed_insts.issued - (observed_insts.received + \
            observed_insts.dropped + observed_insts.received_squashed)
    print(f'Missing pkts: {missing}')

    print(f'{len(observed_insts.all_time)} unique instructions')

    sg1 = observed_insts > 1
    print(f'Sent > 1: {len(sg1)} insts ({observed_insts.sum_sent(sg1)} pkts)')

    rem = observed_insts.remaining()
    print(f'Incomplete: {len(rem)} inst ({observed_insts.sum_sent(rem)} pkts)')

    # print(f'MIF: {", ".join([v for v in observed_insts > 1])}')
    print(f'            '
          f'{", ".join([str(v) for v in observed_insts.remaining()[:5]])}')
