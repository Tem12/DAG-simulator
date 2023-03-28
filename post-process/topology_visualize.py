import argparse
import random
import networkx as nx
from tqdm import tqdm
import matplotlib.pyplot as plt
from matplotlib import colors as mcolors, path
import statistics

# Traverse graph from node X to node Y from all nodes stored in configuration. Do this process n times for groups where:
# nodes are badly connected (1 - 2 connections)
# nodes are commonly connected (3 - 12 connections)
# nodes are very well connected (12 - 100 connections)
# For each shortest path output following data:
#   - Number of hops
#   - Network propagation time (Tau)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', type=str, required=True,
                        help='Network topology required by simulation')

    args = parser.parse_args()

    G = nx.Graph()

    with open(args.config, 'r') as metadata_file:
        for row in metadata_file:
            value = row.rstrip('\n')
            if value[:10] == 'biconnect=':
                rowStr = value[10:].split(' ')
                node1 = int(rowStr[0])
                node2 = int(rowStr[1])
                delay = int(rowStr[2])

                if node1 not in G.nodes:
                    G.add_node(node1)

                if node2 not in G.nodes:
                    G.add_node(node2)

                G.add_edge(node1, node2, weight=delay)

    # pos = nx.spring_layout(G, k=100.0, iterations=200)
    nx.draw_networkx(G, node_size=0.05, font_size=0.05, width=0.01,
                     alpha=0.5, node_color='#069AF390')
    # nx.draw(G, node_size=10)

    ax = plt.gca()
    plt.figure(1, figsize=(100000, 100000), dpi=600)
    plt.axis("off")
    plt.tight_layout()
    plt.savefig(f'{args.config[:-4]}.pdf')


if __name__ == '__main__':
    main()
