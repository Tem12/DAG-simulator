import sys
import pandas as pd
import argparse
import os

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--data', type=str, required=True,
                        help='Data file generated by simulation')
    parser.add_argument('--csv', required=False, action='store_true', dest='csv',
                        help='Output data in csv format for another process')
    parser.add_argument('--power-thold', type=float, required=True,
                        help='Miners mining power minimum threshold to be included in output')
    parser.add_argument('--no-extra-info', required=False, action='store_true', dest='no_extra_info',
                        help='Script will output only miners profit numbers withou seed, config and data paths')
    parser.add_argument('--split', required=False, action='store_true', dest='split',
                        help='Payoff function - Split')
    parser.add_argument('--halving', required=False, action='store_true', dest='halving',
                        help='Payoff function - Halving')
    parser.add_argument('--halving-boost', required=False, action='store_true', dest='halving_boost',
                        help='Payoff function - Halving with boosting honest behavior by \
                        distributing part of malicious actors\' profit')
    parser.add_argument('--block-reward', type=float, required=False,
                            help='Set block rewards (default: 0)', default=0.0)
    parser.add_argument('--max-tx-occur', type=int, required=False, default=1,
                        help='Ignore profit from transactions with occur position greater than MAX_TX_OCCUR in \
                        halving payoff funcion (default: 1)')
    parser.add_argument('--malicious-penalization', type=float, required=False, default=0.6,
                        help='Malicious miners penalization in percentage interval <0,1> for halving boost payoff \
                        function (default: 0.6)')

    args = parser.parse_args()

    # Check for valid payoff function selection
    if args.split and args.halving:
        print('Cannot choose both payoff functions', file=sys.stderr)
        exit(1)

    # ================= Read metadata =================
    read_config = False
    read_seed = False
    read_blocks = False
    read_block_size = False
    read_miners_honest = False
    read_miners_malicious = False
    read_miners_kaspalike = False

    blocks = 0
    block_size = 0
    config = ''
    seed = 0
    miners_count = 0

    if not os.path.exists(args.data):
        print('Specified path to data file does not exists', file=sys.stderr)
        exit(1)

    # Cannot directly replace 'data_' because it can be used as name/part of the name of some folder in specified path
    data_prefix_index = args.data.rfind('data_')

    if data_prefix_index == -1:
        print('Invalid data file name', file=sys.stderr)
        exit(1)

    if data_prefix_index == 0:
        # data_...
        metadata_path = args.data.replace('data_', 'metadata_', 1)
    else:
        # {path}/data_...
        metadata_path = '/metadata_'.join(args.data.rsplit('/data_', 1))

    # Replace file extension
    metadata_path = metadata_path[:len(metadata_path) - 3] + 'data'

    if not os.path.exists(metadata_path):
        print(
            f'Metadata file for {args.data} cannot be found', file=sys.stderr)
        exit(1)

    with open(metadata_path, 'r') as metadata_file:
        for row in metadata_file:
            value = row.rstrip('\n')
            if read_config and read_seed and read_blocks and read_block_size and read_miners_honest and read_miners_malicious and read_miners_kaspalike:
                break

            if value.startswith('cfg_path='):
                config = value[len('cfg_path='):]
                read_config = True
            elif value.startswith('seed='):
                seed = int(value[len('seed='):])
                read_seed = True
            elif value.startswith('blocks='):
                blocks = int(value[len('blocks='):])
                read_blocks = True
            elif value.startswith('block_size='):
                block_size = int(value[len('block_size='):])
                read_block_size = True
            elif value.startswith('malicious_miners='):
                miners_count += int(value[len('malicious_miners='):])
                read_miners_malicious = True
            elif value.startswith('honest_miners='):
                miners_count += int(value[len('honest_miners='):])
                read_miners_honest = True
            elif value.startswith('kaspalike_miners='):
                miners_count += int(value[len('kaspalike_miners='):])
                read_miners_honest = True

    if not read_config:
        print(f'Metadata does not contain config path', file=sys.stderr)
        exit(1)

    if not read_seed:
        print(f'Metadata does not contain seed', file=sys.stderr)
        exit(1)

    if not read_blocks:
        print(f'Metadata does not contain number of blocks', file=sys.stderr)
        exit(1)

    if not read_block_size:
        print(f'Metadata does not contain block size', file=sys.stderr)
        exit(1)

    # =================================================

    honest_miners, malicious_miners, kaspalike_miners = parse_config(config, args.power_thold)

    tx_count = blocks * block_size

    miners_profit = [0] * miners_count
    miners_block_profit = [0] * miners_count
    miners_collision_sum = [0] * miners_count
    miners_tx_count = [0] * miners_count

    # Store total profit gained in simulation by all miners for next calculations
    total_profit = args.block_reward * blocks

    # Store ids of all processed transactions
    processed_tx = {}

    # Store ids of all processed blocks
    processed_blocks = {}
    
    df = pd.read_csv(args.data)

    # Process transactions in data
    if args.split:
        for i in range(0, tx_count):
            tx_id = df['TransactionID'][i]
            if tx_id not in processed_tx:
                processed_tx[tx_id] = {'fee': df['Fee'][i], 'count': 1}
                total_profit += df['Fee'][i]
            else:
                processed_tx[tx_id]['count'] += 1

        for i in range(0, tx_count):
            tx_id = df['TransactionID'][i]
            minerId = df['MinerID'][i]
            miners_profit[minerId] += float(df['Fee'][i]) / processed_tx[tx_id]['count']
            miners_tx_count[minerId] += 1
            miners_collision_sum[minerId] += processed_tx[tx_id]['count']

            block_id = df['BlockID'][i]
            if block_id not in processed_blocks:
                processed_blocks[block_id] = True   # Mark block as proccesed
                miners_block_profit[minerId] += args.block_reward
    elif args.halving:
        # Halving-boost can be also combined halving and burning

        # Halving payoff function

        # Miners profit = fee / (2^i) + (fee/2^total)/total
        # i  = starts from 1, find order position
        # total = number of miners who find it
        # fee / 2**i + (fee/2**total)/total

        for i in range(0, tx_count):
            tx_id = df['TransactionID'][i]
            if tx_id not in processed_tx:
                processed_tx[tx_id] = {'fee': df['Fee'][i], 'count': 1, 'position': 1}
                total_profit += df['Fee'][i]
            else:
                processed_tx[tx_id]['count'] += 1

        for i in range(0, tx_count):
            tx_id = df['TransactionID'][i]
            minerId = df['MinerID'][i]
            fee = float(df['Fee'][i])
            minePosition = processed_tx[tx_id]['position']

            count = processed_tx[tx_id]['count']
            miners_tx_count[minerId] += 1
            miners_collision_sum[minerId] += count

            # Limit where malicious miners do not earn any reward (this reward will be burned)
            if minePosition > args.max_tx_occur:
                continue

            miners_profit[minerId] += fee / 2**minePosition + (fee / 2**count)/count
            processed_tx[tx_id]['position'] += 1

            block_id = df['BlockID'][i]
            if block_id not in processed_blocks:
                processed_blocks[block_id] = True   # Mark block as proccesed
                miners_block_profit[minerId] += args.block_reward
    else:
        # No payoff function
        for i in range(0, tx_count):
            tx_id = df['TransactionID'][i]
            if tx_id not in processed_tx:
                processed_tx[tx_id] = {'fee': df['Fee'][i], 'count': 1}
                minerId = df['MinerID'][i]
                miners_profit[minerId] += df['Fee'][i]

                block_id = df['BlockID'][i]
                if block_id not in processed_blocks:
                    processed_blocks[block_id] = True   # Mark block as proccesed
                    miners_block_profit[minerId] += args.block_reward

                total_profit += df['Fee'][i]
            else:
                processed_tx[tx_id]['count'] += 1

        for i in range(0, tx_count):
            tx_id = df['TransactionID'][i]
            minerId = df['MinerID'][i]
            miners_tx_count[minerId] += 1
            miners_collision_sum[minerId] += processed_tx[tx_id]['count']

    ##### Assign profits to honest and malicious that will go to output
    for id in honest_miners:
        if id >= len(miners_profit):
            continue
        honest_miners[id]['profit'] = miners_profit[id] + miners_block_profit[id]
        if miners_tx_count[id]:
            honest_miners[id]['tx_collision_index'] = float(miners_collision_sum[id]) / miners_tx_count[id]

    for id in malicious_miners:
        if id >= len(miners_profit):
            continue
        malicious_miners[id]['profit'] = miners_profit[id] + miners_block_profit[id]
        if miners_tx_count[id]:
            malicious_miners[id]['tx_collision_index'] = float(miners_collision_sum[id]) / miners_tx_count[id]

    for id in kaspalike_miners:
        if id >= len(miners_profit):
            continue
        kaspalike_miners[id]['profit'] = miners_profit[id] + miners_block_profit[id]
        if miners_tx_count[id]:
            kaspalike_miners[id]['tx_collision_index'] = float(miners_collision_sum[id]) / miners_tx_count[id]

    if args.halving_boost and len(malicious_miners) > 0:
            # Move X perc. profit from malicious actors to honest as penalization
            PENALIZATION = args.malicious_penalization

            malicious_miners_total_power = 0.0
            for malicious_id in malicious_miners:
                malicious_miners_total_power += malicious_miners[malicious_id]['power']
            
            malicious_miners_power_div = 1.0 / malicious_miners_total_power

            for malicious_id in malicious_miners:
                # Equal reward - each honest miner get same part
                # honest_bonus_reward = malicious_miners[malicious_id]['profit'] * PENALIZATION / (miners_count - len(malicious_miners))

                malicious_miners[malicious_id]['profit'] = malicious_miners[malicious_id]['profit'] * (1 - PENALIZATION)

                for honest_id in honest_miners:
                    # Fairly reward - each honest miner get part based on his mining power (in reality, number of blocks he mined)
                    honest_bonus_reward = malicious_miners[malicious_id]['profit'] * PENALIZATION * (honest_miners[honest_id]['power'] * malicious_miners_power_div)
                    honest_miners[honest_id]['profit'] += honest_bonus_reward

    profits_sum = 0.0

    if not args.csv:
        if not args.no_extra_info:
            print(f'Data: {args.data}')
            print(f'Config: {config}')
            print(f'Seed: {seed}')

        i = 0
        for id in honest_miners:
            profit = round(honest_miners[id]["profit"] / total_profit * 100, 2)
            profits_sum += profit
            print(f'Honest miner #{i} profit: {profit}%, txci: {round(honest_miners[id]["tx_collision_index"], 2)}')
            i += 1
        
        i = 0
        for id in malicious_miners:
            profit = round(malicious_miners[id]["profit"] / total_profit * 100, 2)
            profits_sum += profit
            print(f'Malicious miner #{i} profit: {profit}%, txci: {round(malicious_miners[id]["tx_collision_index"], 2)}')
            i += 1

        i = 0
        for id in kaspalike_miners:
            profit = round(kaspalike_miners[id]["profit"] / total_profit * 100, 2)
            profits_sum += profit
            print(
                f'Kaspa-like miner #{i} profit: {profit}%, txci: {round(kaspalike_miners[id]["tx_collision_index"], 2)}')
            i += 1

        print(f'Profit of the remaining miners: {round(100.0 - profits_sum, 2)}%')

    else:
        # Output format: (each value is only in percentage format)
        # {data_path},{config_path},{seed},{honest_miner[0]_perc_profit},{honest_miner[1]_perc_profit},{...}, \
        # {malicious_miner[0]_perc_profit}{malicious_miner[1]_perc_profit}{...}{remaining_miners_profit}
        #
        # Example:
        # outputs/data_mining_topo.cfg_0000.csv,mining_topo.cfg,9.3831,9.5216,28.8292,35.0043,17.2618

        # Flag to not print extra comma if no_extra_info flag enabled
        print_comma_sep = False

        if not args.no_extra_info:
            print(f'{args.data},{config},{seed}', end='')
            print_comma_sep = True

        for id in honest_miners:
            if print_comma_sep:
                print(',', end='')
            else:
                print_comma_sep = True
            profit = round(honest_miners[id]["profit"] / total_profit * 100, 4)
            profits_sum += profit
            print(f'{profit}', end='')

        for id in malicious_miners:
            if print_comma_sep:
                print(',', end='')
            else:
                print_comma_sep = True
            profit = round(malicious_miners[id]["profit"] / total_profit * 100, 4)
            profits_sum += profit
            print(f'{profit}', end='')

        for id in kaspalike_miners:
            if print_comma_sep:
                print(',', end='')
            else:
                print_comma_sep = True
            profit = round(kaspalike_miners[id]["profit"] / total_profit * 100, 4)
            profits_sum += profit
            print(f'{profit}', end='')

        if print_comma_sep:
                print(',', end='')
        print(f'{round(100.0 - profits_sum, 4)}', end='')

        print('')   # New line
    
    return 0


def parse_config(file_path, threshold):
    honest_miners = {}
    malicious_miners = {}
    kaspalike_miners = {}

    with open(file_path, 'r') as file:
        i = 0
        for row in file:
            if row.startswith('biconnect'):
                # Part of config with miners is searched, close config
                break
            if row.startswith('miner'):
                tokens = row.rstrip('\n').split(' ')
                power = float(tokens[0].split('=')[1])

                if power < threshold:
                    i += 1
                    continue

                if tokens[1] == 'honest':
                    honest_miners[i] = {'power': power, 'profit': 0.0, 'tx_collision_index': 0.0}
                elif tokens[1] == "malicious":
                    malicious_miners[i] = {'power': power, 'profit': 0.0, 'tx_collision_index': 0.0}
                elif tokens[1] == "kaspa-like":
                    kaspalike_miners[i] = {'power': power, 'profit': 0.0, 'tx_collision_index': 0.0}

                i += 1

    return honest_miners, malicious_miners, kaspalike_miners


if __name__ == '__main__':
    main()
