import sys
from collections import deque
import numpy as np
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-f', "--file", help="config name to create", type=str, default="config_test.cfg")
parser.add_argument('-h', "--honest", help="number of honest miners", type=int, default=0)
parser.add_argument('-m', "--malicious", help="number of malicious miners", type=int, default=0)
parser.add_argument('-k', "--kaspa", help="number of kaspa-like miners", type=int, default=0)
parser.add_argument("--honest-power", help="total power of honest miners in float", type=float, default=0)
parser.add_argument("--malicious-power", help="total power of malicious miners in float", type=float, default=0)
parser.add_argument("--kaspa-power", help="total power of kaspa-like miners in float", type=float, default=0)
parser.add_argument("--delay", help="per connection network delay", type=int, default=5000)
args = parser.parse_args()

# likely imperfect, but functional config generator

total_power = args.honest_power + args.malicious_power + args.kaspa_power
total = args.honest + args.malicious + args.kaspa
if args.file == "":
    print("No file specified")
    sys.exit(0)
if total_power == 0:
    print("No power specified")
    sys.exit(0)
if total == 0:
    print("No miners specified")
    sys.exit(0)
if total_power < 1:
    print("Total power less than 100%")
    sys.exit(0)

honest_power = args.honest_power / total
malicious_power = args.malicious_power / total
kaspa_power = args.kaspa_power / total

if os.path.exists(f"./{args.file}"):
    os.remove(f"./{args.file}")

open(f"{args.file}", "w+").close()  # creates the file

with open(f"{args.file}", "r+") as f:
    kaspas = 0
    honests = 0
    malicious = 0
    connected = deque()
    for i in range(total):
        if honests < args.honest:
            f.write(f"miner={honest_power} honest\n")
            honests += 1
        if kaspas < args.kaspa:
            f.write(f"miner={kaspa_power} kaspa-like\n")
            kaspas += 1
        if malicious < args.malicious:
            f.write(f"miner={malicious_power} malicious\n")
            malicious += 1

    for i in range(total * 25): # brute forced connections, repetitions happen, can be improved
        x = np.random.randint(0, total)
        y = np.random.randint(0, total)
        connected.append(x)
        connected.append(y)
        f.write(f"biconnect={x} {y} {args.delay}\n")

    # connects the remaining unconnected nodes to a random node
    lines = f.readlines()
    list(connected).sort()
    connected = deque(connected)
    for i in range(total):
        if i != connected.popleft():
            cnt = np.random.randint(0, total)
            if cnt == i:
                np.random.randint(0, total)
            f.write(f"biconnect={i} {cnt} {args.delay}\n")
