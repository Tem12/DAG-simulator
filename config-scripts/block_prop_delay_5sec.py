import sys
import numpy as np
import argparse


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', type=str, required=True,
                        help='Configuration for simulator')
    args = parser.parse_args()

    with open(args.config, 'r') as fileobj:
        for row in fileobj:
            row = row.rstrip('\n')
            if row.startswith('biconnect='):
                connections = row.split('biconnect=', maxsplit=1)[1].split(' ')
                val = 5000

                if len(connections) < 2 or len(connections) > 3:
                    print('Invalid config', file=sys.stderr)
                    exit(1)
                
                if len(connections) == 2:
                    print(f'{row} {val}')
                elif len(connections) == 3:
                    print(f'biconnect={connections[0]} {connections[1]} {val}')
            else:
                print(row)

if __name__ == '__main__':
    main()
