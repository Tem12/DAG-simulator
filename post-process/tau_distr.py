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
import itertools
import copy
import math
from block_prop_data import block_prop_delay_data


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', type=str, required=True,
                        help='Network topology required by simulation')
    parser.add_argument('-n', '--name', type=str, required=True,
                        help='Outputs name')

    args = parser.parse_args()

    xk = np.arange(30000)
    propDelayGen = stats.rv_discrete(
        values=(xk, block_prop_delay_data), seed=12)

    g = nx.Graph()

    node_count = 0
    node_connections = {}
    honest_count = 0
    malicious_count = 0
    malicious_power = 0.0

    total_edges = 0
    non_empty_edges = 0

    with open(args.config, 'r') as metadata_file:
        for row in metadata_file:
            value = row.rstrip('\n')
            if value[:6] == 'miner=':
                node_connections[node_count] = {}
                node_count += 1

                if value[-6:] == 'honest':
                    honest_count += 1
                else:
                    malicious_count += 1
                    malicious_power = float(value.split('=')[1].split(' ')[0])
            elif value[:10] == 'biconnect=':
                rowStr = value[10:].split(' ')
                node1 = int(rowStr[0])
                node2 = int(rowStr[1])

                if node1 not in g.nodes:
                    g.add_node(node1)

                if node2 not in g.nodes:
                    g.add_node(node2)

                g.add_edge(node1, node2, weight=0)
                node_connections[node1][node2] = 0
                node_connections[node2][node1] = 0
                total_edges += 1

    # Try to create uTau distribution to the network

    print('Mean:', propDelayGen.mean())
    print('Median:', propDelayGen.median())

    INIT_NODES_TEST = node_count * 800
    # AVG_HOP_COUNT = 2.2
    AVG_HOP_COUNT = 3.545
    # AVG_HOP_COUNT = 1
    MAX_PATH_LEN = 6

    tn1 = [random.randrange(node_count)
           for _ in range(INIT_NODES_TEST)]
    tn2 = [random.randrange(node_count)
           for _ in range(INIT_NODES_TEST)]
    
    for pathLen in range(0, MAX_PATH_LEN - 1):
        print(MAX_PATH_LEN - pathLen)
        for i in tqdm(range(0, INIT_NODES_TEST)):
            if tn1[i] == tn2[i]:
                continue

            try:
                path = list(nx.shortest_path(
                    g, source=tn1[i], target=tn2[i], method='dijkstra'))
                # print(tn1[i])
                # print(tn2[i])
                # print(path)

                pathDelays = []
                pathFilled = 0

                # Testing
                if len(path) < MAX_PATH_LEN - pathLen:
                    continue

                for j in range(1, len(path)):
                    node1 = path[j - 1]
                    node2 = path[j]

                    if node_connections[node1][node2] != 0:
                        pathFilled += 1

                if pathFilled == 0:
                    for j in range(1, len(path)):
                        node1 = path[j - 1]
                        node2 = path[j]

                        delay = float(propDelayGen.rvs())
                        # delay = float(propDelayGen.rvs()) / AVG_HOP_COUNT
                        # delay = float(propDelayGen.rvs()) / len(path) - 1

                        nx.set_edge_attributes(g, {(node1, node2): {"weight": delay}})
                        node_connections[node1][node2] = delay
                        node_connections[node2][node1] = delay
                        non_empty_edges += 1


                # for j in range(0, len(path)):
                #     if j == 0:
                #         continue

                #     conn = node_connections[path[j - 1][path[j]]]

                #     if len(conn) == 0:
                #         pathDelays[j - 1] = 0
                #     else:
                #         pathDelays[j - 1] = avg(conn)

                # print(propDelayGen.rvs())
            except:
                continue

    print('Non empty edges:', non_empty_edges)
    print('Total edges:', total_edges)

     # Generate configuration
    f = open(f'{args.name}.cfg', 'w')

    honest_haspower = (1 - malicious_count * malicious_power) / \
        (node_count - malicious_count)
    for i in range(0, honest_count):
        f.write(f'miner={honest_haspower} honest\n')

    for i in range(0, malicious_count):
        f.write(f'miner={malicious_power} malicious\n')

     # Print separator line
    f.write('\n')

    histData = []

    resolved_nodes = {}
    for i in range(0, node_count):
        for j in list(nx.neighbors(g, i)):
            if j in resolved_nodes:
                continue
            else:
                weight = round(nx.path_weight(g, path=[i, j], weight='weight'))
                if weight == 0:
                    weight = float(propDelayGen.rvs())
                histData.append(weight)
                f.write(f'biconnect={i} {j} {weight}\n')
        resolved_nodes[i] = True
    f.close()

    plt.hist(histData, bins=200)
    plt.figure(1, figsize=(16, 9), dpi=600)
    plt.savefig(f'hist_{args.name}.pdf')
    plt.show()


def avg(arr):
    return sum(arr) / len(arr)

if __name__ == '__main__':
    main()
