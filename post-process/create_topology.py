import argparse
import random
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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--honest', type=int, required=True,
                        help='Number of honest nodes')
    parser.add_argument('-m', '--malicious', type=int, required=True,
                        help='Number of honest nodes')
    parser.add_argument('-d', '--node_degree', type=str, required=True,
                        help='Node degree data filepath')
    parser.add_argument('-p', '--malicious_power', type=float, required=True,
                        help='Malicious node power')
    parser.add_argument('-n', '--name', type=str, required=True,
                        help='Outputs name')

    args = parser.parse_args()

    if not os.path.exists(args.node_degree):
        print('Specified path to data file does not exists', file=sys.stderr)
        exit(1)

    df = pd.read_csv(args.node_degree)

    node_degree_gen = stats.rv_discrete(name='node_degree', seed=12, values=(
        list(df['node-degree']), list(df['probabilities'])))

    # prob = []
    # for _ in range(0, 100000):
    #     prob.append(node_degree_gen.rvs())
    # plt.hist(prob, bins=57)
    # plt.show()

    total_node_count = args.honest + args.malicious

    g = nx.Graph()

    total_edges = 0
    edge_drop_counter = 0

    # Number of nodes to test edge connection score (potential connection)
    END_NODES_TEST = math.ceil(28)
    PATH_NODES_TEST = total_node_count  # Number of nodes to create average hop count

    MAX_DIFF_EDGES_TRY = 30
    MAX_EDGES_COUNT_TRY = 10

    if END_NODES_TEST >= total_node_count:
        print('Nodes to test must smaller than all nodes in output topology')
        exit(1)

    node_peers = {}

    # Add nodes
    for i in range(0, total_node_count):
        # Add node to graph struct
        g.add_node(i)
        
        # Set each node number of peers
        edges = node_degree_gen.rvs()
        total_edges += edges
        node_peers[i] = {'total': edges, 'free': edges}
    
    # Add edges
    for i in tqdm(range(0, total_node_count)):
        edges = 0

        edges_count_try = 0
        while edges_count_try < MAX_EDGES_COUNT_TRY:
                
            # If it was not possible to find some node to connect, cut off remaining edges,
            # however, this node can still receive connection from other node
            if edges_count_try + 1 < MAX_EDGES_COUNT_TRY:
                edges = node_peers[i]['free']
            else:
                edge_drop_counter += node_peers[i]['free']
                edges = 0

            for e in range(0, edges):
                edge_added = 0

                while edge_added < MAX_DIFF_EDGES_TRY:
                    start_node = i
                    end_nodes = random.sample(
                        range(0, total_node_count), END_NODES_TEST)
                    scores = {}

                    test_nodes1 = [random.randrange(total_node_count)
                                   for _ in range(PATH_NODES_TEST)]
                    test_nodes2 = [random.randrange(total_node_count)
                                   for _ in range(PATH_NODES_TEST)]

                    while start_node in end_nodes:
                        end_nodes = random.sample(
                            range(0, total_node_count), END_NODES_TEST)

                    # Fill initial score
                    for n in end_nodes:
                        scores[n] = 0

                    for n in range(0, len(end_nodes)):
                        end_node = end_nodes[n]
                        newG = copy.deepcopy(g)
                        newG.add_edge(start_node, end_node)

                        hops = []
                        for t in range(0, len(test_nodes1)):
                            while test_nodes1[t] == test_nodes2[t]:
                                test_nodes2[t] = random.randint(
                                    0, total_node_count - 1)

                            try:
                                path = list(nx.shortest_path(
                                    newG, source=test_nodes1[t], target=test_nodes2[t], method='dijkstra'))
                                hops.append(len(path))
                            except:
                                hops.append(0)

                        avg_hop_count = sum(hops) / len(hops)
                        scores[end_nodes[n]] = avg_hop_count

                    # print(scores)
                    sorted_score_keys = list(
                        dict(sorted(scores.items(), key=lambda item: item[1])).keys())

                    while True:
                        if len(sorted_score_keys) == 0:
                            edge_added += 1
                            break
                            
                        new_end_node = sorted_score_keys[random.randint(math.ceil(
                            (len(sorted_score_keys) - 1) / 2), 0 if len(sorted_score_keys) == 0 else len(sorted_score_keys) - 1)]

                        if node_peers[new_end_node]['free'] > 0:
                            g.add_edge(start_node, new_end_node)
                            node_peers[start_node]['free'] -= 1
                            node_peers[new_end_node]['free'] -= 1
                            edge_added = 2 * MAX_DIFF_EDGES_TRY
                            break
                        else:
                            sorted_score_keys.remove(new_end_node)
                            
            if edge_added != MAX_DIFF_EDGES_TRY or edges == 0:
                edges_count_try = 2 * MAX_EDGES_COUNT_TRY
            else:
                edges_count_try += 1

    # Plot connection histogram
    prob = []
    for i in range(0, len(list(nx.nodes(g)))):
        prob.append(len(list(nx.neighbors(g, i))))
    plt.hist(prob, bins=57)
    plt.savefig(f'hist_{args.name}.pdf')

    fc = open(f'peers_{args.name}.cfg', 'w')
    for prob_i in range(min(prob), max(prob) + 1):
        fc.write(f'{prob_i}, {prob.count(prob_i)}\n')
    fc.close()

    # Plot graph network visualization
    # nx.draw_networkx(g)
    # ax = plt.gca()
    # plt.axis("off")
    # # plt.show()
    # plt.savefig('./vizualization.pdf')
    # nx.draw_networkx(g, node_size=0.05, font_size=0.05, width=0.01, alpha=0.5, node_color='#069AF390')
    # nx.draw(G, node_size=10)

    # ax = plt.gca()
    # plt.figure(1, figsize=(100000, 100000), dpi=600)
    # plt.axis("off")
    # plt.tight_layout()
    # plt.savefig(f'vizualization_{args.name}.pdf')

    # Generate configuration
    f = open(f'{args.name}.cfg', 'w')

    honest_haspower = (1 - args.malicious * args.malicious_power) / \
        (total_node_count - args.malicious)
    malicious_haspower = args.malicious_power
    for i in range(0, args.honest):
        f.write(f'miner={honest_haspower} honest\n')

    for i in range(0, args.malicious):
        f.write(f'miner={malicious_haspower} malicious\n')

     # Print separator line
    f.write('\n')

    resolved_nodes = {}
    for i in range(0, total_node_count):
        for j in list(nx.neighbors(g, i)):
            if j in resolved_nodes:
                continue
            else:
                f.write(f'biconnect={i} {j} 500\n')
        resolved_nodes[i] = True
    f.close()

    print(f'Totla edges: {total_edges}')
    print(f'Possible edge drop count: {edge_drop_counter}')


if __name__ == '__main__':
    main()
