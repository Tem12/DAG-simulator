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

    args = parser.parse_args()

    if not os.path.exists(args.node_degree):
        print('Specified path to data file does not exists', file=sys.stderr)
        exit(1)

    df = pd.read_csv(args.node_degree)

    node_degree_gen = stats.rv_discrete(name='node_degree', seed=12, values=(list(df['node-degree']), list(df['probabilities'])))
    
    # prob = []
    # for _ in range(0, 100000):
    #     prob.append(node_degree_gen.rvs())
    # plt.hist(prob, bins=57)
    # plt.show()

    total_node_count = args.honest + args.malicious
    
    g = nx.Graph()

    END_NODES_TEST = math.ceil(total_node_count - 1) # Number of nodes to test edge connection score (potential connection)
    PATH_NODES_TEST = total_node_count * 3 # Number of nodes to create average hop count

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
        node_peers[i] = {'total': edges, 'free': edges}

    # Add edges
    for i in range(0, total_node_count):
        print(f'Node {i}')
        edges = node_peers[i]['free']

        for e in range(0, edges):
            # print(f'Edge {e}')

            # existing_cliques = list(itertools.takewhile(lambda x: len(x) <= MAX_CLIQUE_SIZE, nx.enumerate_all_cliques(g)))
            # ex_clique_sizes = [0] * (MAX_CLIQUE_SIZE - (MIN_CLIQUE_SIZE - 1))
            # for c in existing_cliques:
            #     if len(c) >= MIN_CLIQUE_SIZE and len(c) <= MAX_CLIQUE_SIZE:
            #         ex_clique_sizes[len(c) - MIN_CLIQUE_SIZE] += 1

            start_node = i
            end_nodes = random.sample(range(0, total_node_count), END_NODES_TEST)
            scores = {}
            
            test_nodes1 = [random.randrange(total_node_count) for _ in range(PATH_NODES_TEST)]
            test_nodes2 = [random.randrange(total_node_count) for _ in range(PATH_NODES_TEST)]
            
            while start_node in end_nodes:
                end_nodes = random.sample(range(0, total_node_count), END_NODES_TEST)

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
                        test_nodes2[t] = random.randint(0, total_node_count - 1)
                    
                    try:
                        path = list(nx.shortest_path(newG, source=test_nodes1[t], target=test_nodes2[t], method='dijkstra'))
                        hops.append(len(path))
                    except:
                        hops.append(0)
                
                avg_hop_count = sum(hops) / len(hops)
                scores[end_nodes[n]] = avg_hop_count

                # clique_sizes = [0] * (MAX_CLIQUE_SIZE - (MIN_CLIQUE_SIZE - 1))
                # for c in cliques:
                #     if len(c) >= MIN_CLIQUE_SIZE and len(c) <= MAX_CLIQUE_SIZE:
                #         clique_sizes[len(c) - MIN_CLIQUE_SIZE] += 1
                
                # for i in range(0, len(clique_sizes)):
                #     scores[end_nodes[n]] += (clique_sizes[i] - ex_clique_sizes[i]) * 1
                    
            # print(scores)
            sorted_score_keys = list(dict(sorted(scores.items(), key=lambda item: item[1])).keys())

            while True:
                if len(sorted_score_keys) == 0:
                    print('Error: Ran out of nodes to connect. Possibly increase NODES_TEST variable.')
                    break
                
                new_end_node = sorted_score_keys[random.randint(math.ceil((len(sorted_score_keys) - 1) / 2), 0 if len(sorted_score_keys) == 0 else len(sorted_score_keys) - 1)]

                if node_peers[new_end_node]['free'] > 0:
                    g.add_edge(start_node, new_end_node)
                    node_peers[start_node]['free'] -= 1
                    node_peers[new_end_node]['free'] -= 1
                    break
                else:
                    sorted_score_keys.remove(new_end_node)

    # nx.draw_networkx(g)
    # ax = plt.gca()
    # plt.axis("off")
    # # plt.show()
    # plt.savefig('./vizualization.pdf')
    nx.draw_networkx(g, node_size=0.05, font_size=0.05, width=0.01,
                     alpha=0.5, node_color='#069AF390')
    # nx.draw(G, node_size=10)

    ax = plt.gca()
    plt.figure(1, figsize=(100000, 100000), dpi=600)
    plt.axis("off")
    plt.tight_layout()
    plt.savefig('./vizualization.pdf')

            
            

if __name__ == '__main__':
    main()
