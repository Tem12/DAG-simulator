from concurrent.futures import ProcessPoolExecutor
import subprocess
from datetime import datetime

# ================= CONFIG =================
# Required params
MAX_INSTANCES = 18
CONFIGS = ['mining_bigsmall.cfg', 'mining_bigsmall2.cfg', 'mining_bigsmall3.cfg']
SEEDS = [42, 512, 8010, 12321, 65781]

# Optional params, set value to None to use default
BLOCKS = 200
BLOCK_SIZE = 20
MAX_MP_SIZE = 10000
MIN_TX_GEN_SIZE = 200
MAX_TX_GEN_SIZE = 400
MIN_TX_GEN_SECS = 20
MAX_TX_GEN_SECS = 40
NETWORK_LATENCY = 5
LAMBDA = None
# ==========================================

finished_simulations = 0
total_simulations = 0


def main():
    global total_simulations

    # Create an array of all simulations that will fill the queue
    simulations = create_simulation_queue()
    
    total_simulations = len(simulations)
    
    # Log number of launched simulations
    date_time = datetime.now().strftime('%m-%d-%Y %H:%M:%S')
    print(f'[{date_time}] Queued: {total_simulations} simulations, running on {MAX_INSTANCES} CPUs')

    # Start simulations on separate CPUs
    with ProcessPoolExecutor(max_workers=MAX_INSTANCES) as executor:
        while len(simulations) > 0:
            sim = simulations.pop(0)
            future = executor.submit(run_simulation, sim)
            future.add_done_callback(log_finished_simulation)


def run_simulation(simulation):
    subprocess.run(simulation, stdout=subprocess.DEVNULL)


def log_finished_simulation(_):
    global finished_simulations
    finished_simulations += 1
    date_time = datetime.now().strftime('%m-%d-%Y %H:%M:%S')
    print(f'[{date_time}] Finished: {finished_simulations}/{total_simulations}')


def create_simulation_queue():
    simulations = []

    run_base = ['./mining_simulator']

    if BLOCKS is not None:
        run_base.append('--blocks')
        run_base.append(f'{BLOCKS}')

    if BLOCK_SIZE is not None:
        run_base.append('--block_size')
        run_base.append(f'{BLOCK_SIZE}')

    if MAX_MP_SIZE is not None:
        run_base.append('--max_mp_size')
        run_base.append(f'{MAX_MP_SIZE}')

    if MIN_TX_GEN_SIZE is not None:
        run_base.append('--min_tx_gen_size')
        run_base.append(f'{MIN_TX_GEN_SIZE}')

    if MAX_TX_GEN_SIZE is not None:
        run_base.append('--max_tx_gen_size')
        run_base.append(f'{MAX_TX_GEN_SIZE}')

    if MIN_TX_GEN_SECS is not None:
        run_base.append('--min_tx_gen_secs')
        run_base.append(f'{MIN_TX_GEN_SECS}')

    if MAX_TX_GEN_SECS is not None:
        run_base.append('--max_tx_gen_secs')
        run_base.append(f'{MAX_TX_GEN_SECS}')

    if NETWORK_LATENCY is not None:
        run_base.append('--latency')
        run_base.append(f'{NETWORK_LATENCY}')

    if LAMBDA is not None:
        run_base.append('--lambda')
        run_base.append(f'{LAMBDA}')

    for config in CONFIGS:
        for seed in SEEDS:
            simulations.append(
                run_base + ['--rng_seed', f'{seed}', '--config', f'{config}'])

    return simulations


if __name__ == '__main__':
    main()
