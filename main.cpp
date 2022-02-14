/**
 * @file main.cpp
 * @brief Main file for DAG simulator implementation
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @date 2021 - 2022
 *
 * Original @author Gavin Andresen <gavinandresen@gmail.com>
 */
/*
 * Copyright (C) BUT Security@FIT, 2021 - 2022
 *
 * LICENSE TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#include "scheduler.h"

#include <assert.h>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/exponential_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/random_number_generator.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <random>
#include <cstdint>
#include <dirent.h>

#include "standard_miner.hpp"

#define TOTAL_HASHPOWER_EPS 0.000001 // max deviation of valid hashpower
#define DATA_OUTPUT_DIR "outputs" // name of the directory for output files
#define DATA_OUTPUT_MAX_RUN_ID 1000

std::string config_file_path;
std::string config_filename;
time_t sim_start_time;
int n_blocks = 0;
bool end_simulation = false;

int max_mp_size = 0; // Maximum mempool size for each miner

int min_tx_gen_size = 0; // Minimum number of generated transactions per
                         // distribution
int max_tx_gen_size = 0; // Maximum number of generated transactions per
                         // distribution

double min_tx_gen_secs = 0.0; // Minimum seconds of simulation time to
                              // generate new transactions
double max_tx_gen_secs = 0.0; // Maximum seconds of simulation time to
                              // generate new transactions

int blockSize = 0; // How many transactions contains each block

double lambda = 0.0;

int config_variant_id = 0;
int run_id = 0;

// Global vars for logging
std::vector<Miner *> miners;
int honest_miner_id = -1;
int malicious_miner_id = -1;

// ===================================== Optimization experiment code =====================================
// Time estimation data, optimization_exp/time_est_{CFG_variant}_{RUN_ID}
// std::string time_est_filename = "experiment_mempool_optimization/time_est_";
// FILE *time_est_file;

// Mempool fullness data, optimization_exp/mempool_sizes_{CFG_variant}_{RUN_ID}
// std::string mempool_size_filename = "experiment_mempool_optimization/mempool_sizes_";
// FILE *mempool_sizes_file;

// Total time of running application data,
// optimization_exp/total_time_{CFG_variant}_{RUN_ID}
// std::string total_time_filename = "experiment_mempool_optimization/total_time_";
// FILE *total_time_file;
// ========================================================================================================

// Simulation logging files
// Progress of simulation, equal to stdout log
FILE *progress_file = nullptr;
FILE *mempool_stats_file = nullptr;
FILE *data_stats_file = nullptr;
FILE *metadata_file = nullptr;

static void Connect(Miner *m1, Miner *m2, double latency)
{
    m1->AddPeer(m2, latency);
    m2->AddPeer(m1, latency);
}

double random_real(boost::random::mt19937 &rng, double min, double max)
{
    boost::random::uniform_real_distribution<> d(min, max);
    return d(rng);
}

uint64_t txID = 0;
void mempool_update(boost::random::mt19937 &rng, std::vector<Miner *> &miners, CScheduler &s)
{
    // std::random_device rd; // obtain a random number from hardware
    // std::mt19937 gen(rd()); // seed the generator
    // std::uniform_int_distribution<> distr(90, 110); // define the range
    boost::random::variate_generator<boost::random::mt19937 &, boost::random::exponential_distribution<>> fee_gen(
        rng, boost::random::exponential_distribution<>(1.0));

    double gen_lambda = 150.0;
    // gen 1000 txs at the beginning of simulation to everybody's mempool
    if (txID < 1) {
        for (int i = 0; i < 1000; i++) {
            auto in = fee_gen() * gen_lambda;
            // int in = distr(rng);
            for (auto miner : miners) {
                auto htab_it = miner->mem_pool->insert({ .minerID = (uint32_t)miner->mID, .txID = txID });
                htab_it->setValue((uint32_t)in);
            }
            txID++;
        }
    }

    int txsz;
    double tx_wait_secs;

    if (min_tx_gen_size == max_tx_gen_size) {
        txsz = min_tx_gen_size;
    } else {
        // TODO: make these uniform distribution global, remove init in each
        // function call
        boost::random::uniform_int_distribution<> tx_gen_distr(min_tx_gen_size, max_tx_gen_size);
        txsz = tx_gen_distr(rng);
    }

    if (min_tx_gen_secs == max_tx_gen_secs) {
        tx_wait_secs = min_tx_gen_secs;
    } else {
        // TODO: make these uniform distribution global, remove init in each
        // function call
        boost::random::uniform_real_distribution<> tx_wait_secs_distr(min_tx_gen_secs, max_tx_gen_secs);

        tx_wait_secs = tx_wait_secs_distr(rng);
    }

    // gen 65 transactions
    for (int i = 0; i < txsz; i++) {
        // int in = distr(rng);
        auto in = fee_gen() * gen_lambda;
        for (auto miner : miners) {
            if (miner->mem_pool->size() + txsz > max_mp_size) {
                miner->RemoveMP(txsz);
            }
            auto htab_it = miner->mem_pool->insert({ .minerID = (uint32_t)miner->mID, .txID = txID });
            htab_it->setValue((uint32_t)in);
        }
        txID++;
    }

    // every 2 second add new transactions
    double tNext = s.getSimTime() + tx_wait_secs;

    if (!end_simulation) {
        auto f = boost::bind(&mempool_update, rng, miners, boost::ref(s));
        s.schedule(f, tNext);
    }
}

int run_simulation(boost::random::mt19937 &rng, std::vector<Miner *> &miners)
{
    CScheduler simulator;

    std::vector<double> probabilities;
    for (auto miner : miners)
        probabilities.push_back(miner->GetHashFraction());
    boost::random::discrete_distribution<> dist(probabilities.begin(), probabilities.end());
    boost::random::variate_generator<boost::random::mt19937 &, boost::random::exponential_distribution<>> block_time_gen(
        rng, boost::random::exponential_distribution<>(1.0));

    std::map<int, int> block_owners; // Map block number to miner who found that
                                     // block

    // This loops primes the simulation with n_blocks being found at random
    // intervals starting from t=0:
    double t = 0.0;
    for (int i = 0; i < n_blocks; i++) {
        int which_miner = dist(rng);
        block_owners.insert(std::make_pair(i, which_miner));
        auto t_delta = block_time_gen() * lambda;
#ifdef TRACE
        // std::cout << t_delta << "\n";
#endif
        auto t_found = t + t_delta;
        auto f = boost::bind(&Miner::FindBlock, miners[which_miner], rng, boost::ref(simulator), blockSize, i);
        simulator.schedule(f, t_found);
        t = t_found;
    }

    mempool_update(rng, miners, simulator);

    // Simulation started
    sim_start_time = time(nullptr);

    simulator.serviceQueue();

    // miners[0]->PrintStats();

    // blocks_found.clear();
    // blocks_found.insert(blocks_found.begin(), miners.size(), 0);

    // Run through miner0's idea of the best chain to count
    // how many blocks each miner found in that chain:
    // std::vector<int> best_chain = miners[0]->GetBestChain();
    // for (int i = 0; i < best_chain.size(); i++) {
    //     int b = best_chain[i];
    //     int m = block_owners[b];
    //     ++blocks_found[m];
    // }
    // return best_chain.size();
    return 0;
}

int main(int argc, char **argv)
{
    namespace po = boost::program_options;

    double block_latency = 1.0;
    int n_runs = 1;
    int rng_seed = 0;

    po::options_description desc("Command-line options");
    desc.add_options()("help", "show options")("blocks", po::value<int>(&n_blocks)->default_value(10000),
                                               "number of blocks to simulate")(
        "latency", po::value<double>(&block_latency)->default_value(5.0),
        "block relay/validate latency (in seconds) to simulate")("runs", po::value<int>(&n_runs)->default_value(1),
                                                                 "number of times to run simulation")(
        "rng_seed", po::value<int>(&rng_seed)->default_value(4544), "random number generator seed")(
        "max_mp_size", po::value<int>(&max_mp_size)->default_value(10000),
        "maximum mempool size for each miner")("min_tx_gen_size", po::value<int>(&min_tx_gen_size)->default_value(40),
                                               "minimum number of generated transactions per distribution")(
        "max_tx_gen_size", po::value<int>(&max_tx_gen_size)->default_value(80),
        "maximum number of generated transactions per distribution")(
        "min_tx_gen_secs", po::value<double>(&min_tx_gen_secs)->default_value(2.0),
        "minimum seconds of simulation time to generate new transactions")(
        "max_tx_gen_secs", po::value<double>(&max_tx_gen_secs)->default_value(5.0),
        "maximum seconds of simulation time to generate new transactions")(
        "block_size", po::value<int>(&blockSize)->default_value(100), "transactions count in each block")(
        "lambda", po::value<double>(&lambda)->default_value(20.0), "lambda used in simulation")(
        "run_id", po::value<int>(&run_id)->default_value(0),
        "run id for optimization experiment")("config_variant_id", po::value<int>(&config_variant_id)->default_value(0),
                                              "config id for optimization experiment")(
        "config", po::value<std::string>(&config_file_path)->default_value("mining.cfg"), "Mining config filename");
    po::variables_map vm;

    po::options_description config("Mining config file options");
    config.add_options()("miner", po::value<std::vector<std::string>>()->composing(), "hashrate type")(
        "biconnect", po::value<std::vector<std::string>>()->composing(),
        "m n connection_latency")("description", po::value<std::string>(), "Configuration description");

    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    std::ifstream f(config_file_path.c_str());
    if (!f) {
        std::cout << "Cannot open config file: " << config_file_path << std::endl;
        return EXIT_FAILURE;
    }

    po::store(po::parse_config_file<char>(f, config), vm);
    f.close();
    po::notify(vm);

    if (vm.empty()) {
        printf("is empty\n");
    }

    if (vm.count("help")) {
        std::cout << desc << "\n";
        std::cout << config << "\n";
        return 1;
    }

    boost::random::mt19937 rng;
    rng.seed(rng_seed);

    // Validate config file
    if (vm.count("miner") == 0) {
        std::cout << "You must configure one or more miner in " << config_file_path << "\n";
        return 1;
    }

    double totalHashpower = 0.0;
    int malicious_miners = 0;
    int honest_miners = 0;
    double malicious_hashpower = 0.0;
    double honest_hashpower = 0.0;

    for (auto m : vm["miner"].as<std::vector<std::string>>()) {
        std::vector<std::string> v;
        boost::split(v, m, boost::is_any_of(" \t,"));
        if (v.size() < 2) {
            std::cout << "Couldn't parse miner description: " << m << "\n";
            continue;
        }
        double hashpower = atof(v[0].c_str());
        totalHashpower += hashpower;
        double latency = block_latency;
        if (v.size() > 2) {
            latency = atof(v[2].c_str());
        }
        if (v[1] == "honest") {
            miners.push_back(new Miner(hashpower, latency, HONEST, boost::bind(random_real, boost::ref(rng), _1, _2)));
            honest_miners++;
            honest_hashpower += hashpower;
            if (honest_miner_id == -1) {
                honest_miner_id = miners.back()->mID;
            }
        } else if (v[1] == "malicious") {
            miners.push_back(
                new Miner(hashpower, latency, MALICIOUS, boost::bind(random_real, boost::ref(rng), _1, _2)));
            malicious_miners++;
            malicious_hashpower += hashpower;
            if (malicious_miner_id == -1) {
                malicious_miner_id = miners.back()->mID;
            }
        } else {
            std::cout << "Invalid miner description (can be either honest or malicious) - " << m << "\n";
            return EXIT_FAILURE;
        }
    }

    // Summing all haspowers produce floating point error. To avoid this, TOTAL_HASHPOWER_EPS contains maximum
    // correction
    if ((totalHashpower - 1.0 > TOTAL_HASHPOWER_EPS) || (totalHashpower + 1.0 < TOTAL_HASHPOWER_EPS)) {
        std::cout << "Hashpower of all miner needs to be 100%" << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::string> c = vm["biconnect"].as<std::vector<std::string>>();
    for (auto m : c) {
        std::vector<std::string> v;
        boost::split(v, m, boost::is_any_of(" \t,"));
        if (v.size() < 3) {
            std::cout << "Couldn't parse biconnect description: " << m << "\n";
            continue;
        }
        int m1 = atoi(v[0].c_str());
        int m2 = atoi(v[1].c_str());
        double latency = atof(v[2].c_str());
        if (m1 >= miners.size() || m2 >= miners.size()) {
            std::cout << "Couldn't parse biconnect description: " << m << "\n";
            continue;
        }
        Connect(miners[m1], miners[m2], latency);
    }

    //    std::cout << "Simulating " << n_blocks << " blocks, default latency " << block_latency << "secs, ";
    //    std::cout << "with " << miners.size() << " miners over " << n_runs << " runs\n";
    //    if (vm.count("description")) {
    //        std::cout << "Configuration: " << vm["description"].as<std::string>() << "\n";
    //    }

    // ===================================== Optimization experiment code =====================================
    //    time_est_filename.append(std::to_string(config_variant_id))
    //        .append("_")
    //        .append(std::to_string(run_id))
    //        .append(".csv");
    //
    //    mempool_size_filename.append(std::to_string(config_variant_id))
    //        .append("_")
    //        .append(std::to_string(run_id))
    //        .append(".csv");
    //
    //    total_time_filename.append(std::to_string(config_variant_id))
    //        .append("_")
    //        .append(std::to_string(run_id))
    //        .append(".csv");
    //
    //    time_est_file = fopen(time_est_filename.c_str(), "w");
    //    mempool_sizes_file = fopen(mempool_size_filename.c_str(), "w");
    //    total_time_file = fopen(total_time_filename.c_str(), "w");
    //
    //    if (time_est_file == nullptr) {
    //        std::cout << "Cannot open time_est_file for optimization experiment" << std::endl;
    //        return EXIT_FAILURE;
    //    }
    //
    //    if (mempool_sizes_file == nullptr) {
    //        std::cout << "Cannot open mempool_sizes_file for optimization experiment" << std::endl;
    //        return EXIT_FAILURE;
    //    }
    // ========================================================================================================

    // Get config filename from config path
    size_t config_filename_pos = config_file_path.find_last_of('/');
    if (config_filename_pos == std::string::npos) {
        config_filename = config_file_path;
    } else {
        config_filename = config_file_path.substr(config_filename_pos + 1);
    }

    int sim_run_id = -1;
    char sim_run_id_str[5] = { 0 }; // number: XXXX\0

    // Check for available output files
    DIR *dir = opendir(DATA_OUTPUT_DIR);

    if (dir == nullptr) {
        std::filesystem::create_directory(DATA_OUTPUT_DIR);
    } else {
        closedir(dir);
    }

    for (int i = 0; i < DATA_OUTPUT_MAX_RUN_ID; i++) {
        std::string search_filename = DATA_OUTPUT_DIR;
        sprintf(sim_run_id_str, "%04d", i);
        search_filename.append("/progress_").append(config_filename).append("_").append(sim_run_id_str).append(".out");
        if (access(search_filename.c_str(), F_OK) != 0) {
            sim_run_id = i;
            break;
        }
    }

    if (sim_run_id == -1) {
        std::cout << "Maximum number of output files for same config exceeded ("
                  << std::to_string(DATA_OUTPUT_MAX_RUN_ID) << ")" << std::endl;
        return EXIT_FAILURE;
    }

    // Convert int to string with padded zeroes
    sprintf(sim_run_id_str, "%04d", sim_run_id);

    // outputs/progress_{CFG}_{RUN_ID}.out
    std::string progress_filename = "outputs/progress_";

    // outputs/mempool_{CFG}_{RUN_ID}.csv
    std::string mempool_stats_filename = "outputs/mempool_";

    // outputs/data_{CFG}_{RUN_ID}.csv
    std::string data_stats_filename = "outputs/data_";

    // outputs/metadata_{CFG}_{RUN_ID}.data
    std::string metadata_filename = "outputs/metadata_";

    progress_filename.append(config_filename).append("_").append(sim_run_id_str).append(".out");
    mempool_stats_filename.append(config_filename).append("_").append(sim_run_id_str).append(".csv");
    data_stats_filename.append(config_filename).append("_").append(sim_run_id_str).append(".csv");
    metadata_filename.append(config_filename).append("_").append(sim_run_id_str).append(".data");

    progress_file = fopen(progress_filename.c_str(), "w");
    mempool_stats_file = fopen(mempool_stats_filename.c_str(), "w");
    data_stats_file = fopen(data_stats_filename.c_str(), "w");
    metadata_file = fopen(metadata_filename.c_str(), "w");

    if (progress_file == nullptr) {
        printf("Cannot open progress file\n");
        return EXIT_FAILURE;
    }

    if (mempool_stats_file == nullptr) {
        printf("Cannot open mempool stats file\n");
        return EXIT_FAILURE;
    }

    if (data_stats_file == nullptr) {
        printf("Cannot open data stats file\n");
        return EXIT_FAILURE;
    }

    if (metadata_file == nullptr) {
        printf("Cannot open metadata file\n");
        return EXIT_FAILURE;
    }

    std::string config_full_path = std::filesystem::canonical(std::filesystem::absolute(config_file_path));

    // Print simulation start
    log_progress("Simulation %s_%s\n"
                 "Config: %s\n"
                 "Blocks: %d\n"
                 "Malicious miners: %d (%.2lf%% power)\n"
                 "Honest miners: %d (%.2lf%% power)\n"
                 "Block latency: %.2lf secs\n"
                 "Random seed: %d\n"
                 "Max mempool size: %d txs\n"
                 "Block size: %d txs\n"
                 "Min. transaction generate speed: %.2lf secs\n"
                 "Max. transaction generate speed: %.2lf secs\n"
                 "Min. transaction generate size: %d\n"
                 "Max. transaction generate size: %d\n"
                 "========================================================\n",
                 config_filename.c_str(), sim_run_id_str, config_full_path.c_str(), n_blocks, malicious_miners,
                 malicious_hashpower * 100, honest_miners, honest_hashpower * 100, block_latency, rng_seed, max_mp_size,
                 blockSize, min_tx_gen_secs, max_tx_gen_secs, min_tx_gen_size, max_tx_gen_size);

    // Create headers in csv files
    log_mempool("MinerID,Progress,MempoolSize\n");
    log_data_stats("TransactionID,Fee,BlockID,Depth,MinerID\n");

    // Print simulation params info to metadata file
    log_metadata("name=%s_%s\n"
                 "cfg_path=%s\n"
                 "blocks=%d\n"
                 "seed=%d\n"
                 "block_size=%d\n"
                 "block_latency=%.5lf\n"
                 "max_mempool_size=%d\n"
                 "malicious_miners=%d\n"
                 "honest_miners=%d\n"
                 "malicious_power=%.5lf\n"
                 "honest_power=%.5lf\n",
                 config_filename.c_str(), sim_run_id_str, config_full_path.c_str(), n_blocks, rng_seed, blockSize,
                 block_latency, max_mp_size, malicious_miners, honest_miners, malicious_hashpower, honest_hashpower);

    // ========================================================
    // Print 0% progress
    time_t curr_time = time(nullptr);
    char time_str[26];
    struct tm *tm_info;

    tm_info = localtime(&curr_time);

    strftime(time_str, 26, "%m-%d %H:%M:%S", tm_info);
    log_progress("[%s]\t0%%\n", time_str);
    // ========================================================

    // int best_chain_sum = 0;
    // double fraction_orphan_sum = 0.0;
    // std::vector<int> blocks_found_sum;
    // blocks_found_sum.assign(miners.size(), 0);
    for (int run = 0; run < n_runs; run++) {
#ifdef TRACE
        std::cout << "Run " << run << "\n";
#endif
        // for (auto miner : miners) miner->ResetChain();

        // std::vector<int> blocks_found;
        int best_chain_length = run_simulation(rng, miners);
        // best_chain_sum += best_chain_length;
        // fraction_orphan_sum += 1.0 -
        // (double)best_chain_length/(double)n_blocks; for (int i = 0; i <
        // blocks_found.size(); i++) blocks_found_sum[i] += blocks_found[i];
    }

    // std::cout.precision(4);
    // std::cout << "Orphan rate: " << (fraction_orphan_sum*100.0)/n_runs <<
    // "%\n";
    /*
        std::cout << "Miner hashrate shares (%):";
        for (int i = 0; i < miners.size(); i++) {
            std::cout << " " << miners[i]->GetHashFraction()*100;
        }
        std::cout << "\n";
    */
    // std::cout << "Miner block shares (%):";

    // for (int i = 0; i < miners.size(); i++) {
    //     double average_blocks_found = (double)blocks_found_sum[i]/n_runs;
    //     double average_best_chain = (double)best_chain_sum/n_runs;
    //     double fraction = average_blocks_found/average_best_chain;
    //     std::cout << " " << fraction*100;
    // }
    // std::cout << "\n";

    log_progress("Simulation duration: ");
    auto time_diff = (time_t)difftime(time(nullptr), sim_start_time);
    print_diff_time(time_diff);
    log_progress("\n");

    // ===================================== Optimization experiment code =====================================
    //    fclose(time_est_file);
    //    fclose(mempool_sizes_file);
    //    fclose(total_time_file);
    // ========================================================================================================

    fclose(progress_file);
    fclose(mempool_stats_file);
    fclose(data_stats_file);
    fclose(metadata_file);

    return EXIT_SUCCESS;
}

void sim_err_exit_out_of_tx(Miner *miner)
{
    log_progress("========================================================\n");
    log_progress("Simulation error, taking snapshot of miners mempools:\n");
    log_progress("============================ Start of snapshot ============================\n");
    log_progress("MinerID\tMempoolSize\n");
    for (auto single_miner : miners) {
        log_progress("%d\t%ld\n", single_miner->mID, single_miner->mem_pool->size());
    }
    log_progress("============================= End of snapshot =============================\n");
    log_progress("Simulation error: Miner[%d] was chosen to generate block but has run out of transactions\n",
                 miner->mID);
    log_progress("Miner[%d] - %s with %.2lf%% hashpower and block latency %.2lf secs\n", miner->mID,
                 miner->type == HONEST ? "Honest" : "Malicious", miner->GetHashFraction() * 100,
                 miner->GetBlockLatency());

    // Close logging files
    fclose(progress_file);
    fclose(mempool_stats_file);
    fclose(data_stats_file);
    fclose(metadata_file);

    exit(EXIT_FAILURE);
}
