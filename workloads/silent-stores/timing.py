#!/usr/bin/env python
'''Companion parser for victim.cpp'''

import os
import sys
import argparse


DEBUG = False


def dprint(output='\n'):
    if DEBUG:
        print(output)


class Stat():
    def __init__(self, data=0):
        self.data = data
        self.count = 0

    def __iadd__(self, other):
        self.data += other
        self.count += 1
        return self

    def __str__(self):
        if self.count == 0:
            return '0.0'

        return f'{self.data}/{self.count} = {self.data / self.count}'


def process(fpath):
    use_def = args.default_path or input(f'Use {fpath}? [y]/n') or 'y'
    if not use_def and use_def.lower() != 'y':
        fpath = input('Enter path: ')

    tpath = f'{fpath}.timing'
    if not os.path.exists(tpath):
        assert(os.path.exists(fpath))

        print(f'Creating {tpath}')
        os.system(f"rg -e 'fetch.*0x.*dump' -e 'fetch.*0x.*work' {fpath}"
                  f" | sed -e 's|.*:0:||' > {tpath}")

    dprint(f'Processing {fpath} ({tpath})')

    regions = ['victim', 'advcor', 'advwrg']

    region_stats = {k: Stat() for k in regions}

    end = False
    with open(tpath, 'r') as roi_file:
        while roi_file.readable():
            # drop work begin
            roi_file.readline()

            for roi_count in range(3):
                # get dump reset stats
                try:
                    roi_start, inst_name = roi_file.readline().split(':')
                except ValueError:
                    end = True
                    break

                inst_name_start = inst_name.strip()

                # get work begin of next section
                roi_end, inst_name = roi_file.readline().split(':')
                inst_name_end = inst_name.strip()

                # region is the time between last dump and next work start
                roi_time = int(roi_end) - int(roi_start)

                dprint(f'Region {regions[roi_count]}: {roi_time}  '
                       f'({inst_name_start} {roi_start} -- '
                       f'{inst_name_end} {roi_end})')

                region_stats[regions[roi_count]] += roi_time

            if end:
                break
            # drop dump reset stats
            roi_file.readline()
            dprint()

    for region in region_stats:
        dprint(f'{region}: {region_stats[region]}')

    return region_stats


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--default_path', '-d', action='store_true',
                        help='Use default path')
    parser.add_argument('--name', '-n', type=str, default="sl_st_pipe",
                        help='Use default path')
    args = parser.parse_args()

    if args.default_path:
        stats = {}
        for fname in ['baseline', args.name]:
            stats[fname] = process(f"{fname}_victim/{fname}_victim.debug.out")
    else:
        stats[args.name] = process(args.name)

    for run_name in stats:
        print()
        for stat in stats[run_name]:
            print(f'{run_name}\t{stat}: {str(stats[run_name][stat])}')
