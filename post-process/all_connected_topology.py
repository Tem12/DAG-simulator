import argparse
import random
import numpy as np
import pandas as pd
import networkx as nx
from tqdm import tqdm
import matplotlib.pyplot as plt
from scipy import stats
import os
import sys
from block_prop_data import block_prop_delay_data


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--honest', type=int, required=True,
                        help='Number of honest nodes')
    parser.add_argument('-m', '--malicious', type=int, required=True,
                        help='Number of honest nodes')
    parser.add_argument('-d', '--delay', type=int, required=True,
                        help='Connection delay between two nodes in miliseconds')
    parser.add_argument('-p', '--malicious_power', type=float, required=True,
                        help='Malicious node power')

    args = parser.parse_args()

    total_node_count = args.honest + args.malicious

    honest_haspower = (1 - args.malicious * args.malicious_power) / \
        (total_node_count - args.malicious)
    malicious_haspower = args.malicious_power
    for i in range(0, args.honest):
        print(f'miner={honest_haspower} honest')

    for i in range(0, args.malicious):
        print(f'miner={malicious_haspower} malicious')

    # Print separator line
    print('')

    xk = np.arange(30000)
    block_prop_delay_distr = stats.rv_discrete(
        values=(xk, block_prop_delay_data), seed=12)

    for i in range(0, total_node_count):
        for j in range(0, total_node_count):
            if i == j:
                continue
            else:
                print(f'biconnect={i} {j} {block_prop_delay_distr.rvs()}')
                # print(f'biconnect={i} {j} {args.delay}')

if __name__ == '__main__':
    main()
