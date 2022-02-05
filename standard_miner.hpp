/**
 * @file standard_miner.hpp
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

#ifndef STANDARD_MINER_H
#define STANDARD_MINER_H

#include <boost/function.hpp>
#include <list>
#include <iterator>
#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <vector>
#include <unordered_map>

#include <iostream>
#include <string>
#include <string_view>
#include <fstream>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/functional/hash.hpp>

#include "est_time.h"
#include "log.h"
#include "htab.h"

class Miner;
class PeerInfo
{
  public:
    PeerInfo(Miner *_peer, int _chain_tip, double _latency) : peer(_peer), chain_tip(_chain_tip), latency(_latency)
    {
    }

    Miner *peer;
    int chain_tip;
    double latency;
};

using namespace boost::multi_index;

extern time_t sim_start_time;
extern int n_blocks;
extern int max_mp_size;
extern bool end_simulation;

// Used for logging
extern std::vector<Miner *> miners;
extern int honest_miner_id;
extern int malicious_miner_id;

// Used for error exit simulation
extern void sim_err_exit_out_of_tx(Miner *miner);

int miners_finished = 0;

// ===================================== Optimization experiment code =====================================
// Extern fp from main.cpp
// extern FILE *time_est_file;
// extern FILE *mempool_sizes_file;
// extern FILE *total_time_file;
// ========================================================================================================

int progress = 0; // progress in percent
time_t last_progress_time = 0; // last progress print time

struct Record {
    uint64_t id;
    uint32_t fee;

    Record(uint64_t id, uint32_t fee) : id(id), fee(fee)
    {
    }

    bool operator<(const Record &a) const
    {
        return fee < a.fee;
    }
    bool operator==(const Record &a) const
    {
        return id == a.id;
    }
};

struct Block {
    int id;
    int depth;
    std::vector<Record> txn;
};

enum miner_type { HONEST, MALICIOUS };

typedef Htab *Mempool;

class Miner
{
  public:
    // public
    const int mID;
    std::shared_ptr<std::vector<Block>> blocks;
    std::shared_ptr<Htab> mem_pool_ptr;
    Mempool mem_pool;
    miner_type type; // Miner can either honest or malicious
    u_int64_t balance;

    int depth;
    // std::map<int, int> depth_map;

    // Return a random double in the range passed.
    typedef boost::function<double(double, double)> JitterFunction;

    Miner(double _hash_fraction, double _block_latency, miner_type _type, JitterFunction _func)
        : hash_fraction(_hash_fraction), type(_type), block_latency(_block_latency), jitter_func(_func), mID(nextID++)
    {
        // best_chain = std::make_shared<std::vector<int>>();

//        mem_pool = new Htab((size_t)max_mp_size);
        //std::unique_ptr<Htab> _mempool(new Htab((size_t)max_mp_size));
        mem_pool_ptr = std::make_shared<Htab>(static_cast<size_t>(max_mp_size));
        mem_pool = mem_pool_ptr.get();
        blocks = std::make_shared<std::vector<Block>>();
        balance = 0;
        depth = 0;
    }

    void AddPeer(Miner *peer, double latency)
    {
        peers.push_back(PeerInfo(peer, -1, latency));
        // adjTable[(peer->mID)] = std::map<int, bool>{};
    }

    static bool SortCmpAsc(HtabItem *a, HtabItem *b)
    {
        return a->data < b->data;
    }

    static bool SortCmpDesc(HtabItem *a, HtabItem *b)
    {
        return a->data > b->data;
    }

    std::vector<HtabItem *> getSortedMPAsc()
    {
        size_t mem_pool_size = mem_pool->size();
        std::vector<HtabItem *> txs(mem_pool_size, nullptr);

        auto it = mem_pool->begin();
        for (int i = 0; i < mem_pool_size; i++) {
            txs[i] = (it->item);
            it->next();
        }

        std::sort(txs.begin(), txs.end(), SortCmpAsc);
        return txs;
    }

    std::vector<HtabItem *> getSortedMPDesc()
    {
        size_t mem_pool_size = mem_pool->size();
        std::vector<HtabItem *> txs(mem_pool_size, nullptr);

        auto it = mem_pool->begin();
        for (int i = 0; i < mem_pool_size; i++) {
            txs[i] = it->item;
            it->next();
        }

        std::sort(txs.begin(), txs.end(), SortCmpDesc);
        return txs;
    }

    void RemoveMP(const int size)
    {
        std::vector<HtabItem *> sortedMp = getSortedMPAsc();

        for (int i = 0; i < size; i++) {
            auto htab_it = mem_pool->find(sortedMp[i]->key);
            mem_pool->erase(htab_it.get());
        }
    }

    virtual void FindBlock(boost::random::mt19937 &rng, CScheduler &s, int blockSize, int blockNumber)
    {
        // Extend the chain:
        // auto chain_copy =
        // std::make_shared<std::vector<int>>(best_chain->begin(),
        //                                                      best_chain->end());
        // chain_copy->push_back(blockNumber);
        // best_chain = chain_copy;
        balance++;
        block_bdcast[blockNumber] = true;
        // Transactions in block
        // auto blocks_copy =
        // std::make_shared<std::vector<Block>>(blocks->begin(),
        //                                                         blocks->end());
        // std::cout << rand_index[0].name << '\n';

        Block tmp_block;
        tmp_block.id = blockNumber;

        depth++;
        tmp_block.depth = depth;
        // depth_map[blockNumber] = depth;
        // Mempool::nth_index<0>::type &id_index = mem_pool.get<0>();

        // std::random_device rd; // hardware
        // std::mt19937 gen(rd()); // seed the generator
        // std::uniform_int_distribution<> distr(0, mem_pool.size());

        if (this->type == HONEST) {
            // Honest miners

            for (int i = 0; i < blockSize; i++) {
                std::uniform_int_distribution<> distr(0, (int)mem_pool->arrSize - 1);
                //                for (auto& it : mem_pool) {
                //                    std::cout << it.first << ' '
                //                              << it.second << std::endl;
                //                }
                //
                //                printf("%\n", mem_pool.size() - 1);

                //                int rand_index = distr(rng);

                //                uint64_t id = (mem_pool.find(rand_index))->first;
                //                uint32_t fee = (mem_pool.find(rand_index))->second;

                // Get random tx by accessing begin. Randomness is created by custom key
                // which is then hashed and consists of 2 elements: tx_id and miner_id
                auto mem_pool_it = mem_pool->findClosest(distr(rng));

                if (!mem_pool_it->isValid()) {
                    // Invalid transaction, stop the simulation
                    sim_err_exit_out_of_tx(this);
                }

                uint64_t id = mem_pool_it->item->keyContent.txID;
                uint32_t fee = mem_pool_it->item->data;

                tmp_block.txn.push_back(Record{ id, fee });

                log_data_stats("%lld,%u,%d,%d,%d\n", id, fee, tmp_block.id, tmp_block.depth, this->mID);

                //                auto it = mem_pool.find(id);
                mem_pool->erase(mem_pool_it.get());
            }
        } else if (this->type == MALICIOUS) {
            // Malicious miners

            std::vector<HtabItem *> sortedMp = getSortedMPDesc();
            for (int i = 0; i < blockSize; i++) {
                if (i >= sortedMp.size()) {
                    sim_err_exit_out_of_tx(this);
                }

                tmp_block.txn.push_back(Record{ sortedMp[i]->keyContent.txID, sortedMp[i]->data });
                //                abc[sortedMp[i].first.tx_id].first = sortedMp[i].second;
                //                abc[sortedMp[i].first.tx_id].second.push_back(std::tuple<int, int>(depth, mID));

                log_data_stats("%lld,%u,%d,%d,%d\n", sortedMp[i]->keyContent.txID, sortedMp[i]->data, tmp_block.id,
                               tmp_block.depth, this->mID);

                // Remove processed transactions
                auto it = mem_pool->find(sortedMp[i]->key);
                mem_pool->erase(it.get());
            }

            // blocks_copy->push_back(tmp_block);
            // blocks = blocks_copy;
        }

#ifdef TRACE
        // std::cout << "Miner " << hash_fraction << " found block at simulation
        // time "
        //           << s.getSimTime() << "\n";
#endif
        // RelayChain(this, s, blocks_copy, tmp_block, block_latency);
        RelayChain(this, s, tmp_block, block_latency);
    }

    virtual void ConsiderChain(Miner *from, CScheduler &s,
                               //    std::shared_ptr<std::vector<int>> chain,
                               //    std::shared_ptr<std::vector<Block>> blcks,
                               Block &b, double latency)
    {
        // if (blcks->size() > blocks->size()) {
#ifdef TRACE
        // std::cout << "Miner " << hash_fraction
        //           << " relaying chain at simulation time " << s.getSimTime()
        //           <<
        //           "\n";
#endif
        // best_chain = chain;
        // blocks = blcks;
        // TODO change local mempool
        if (b.depth > depth) {
            depth = b.depth;
        }
        // depth_map[b.id] = b.depth;

        if (!block_bdcast[b.id]) { // not found
            block_bdcast[b.id] = true;

            // update local mempool
            for (auto &elem : b.txn) {
                auto mempool_processed_tx_it = mem_pool->find({ .minerID = (uint32_t)this->mID, .txID = elem.id });

                if (mempool_processed_tx_it->isValid()) {
                    mem_pool->erase(mempool_processed_tx_it.get());
                }
            }
            RelayChain(this, s, b, latency);
        } // else -> found
          // RelayChain(from, s, blcks, b, latency);
          // RelayChain(this, s, b, latency);
          // }

        // Check if all blocks were processed by all miners, if yes stop the simulation
        if (b.id == n_blocks - 1) {
            miners_finished++;

            if (miners_finished == nextID) {
                end_simulation = true;
            }
        }
    }

    virtual void RelayChain(Miner *from, CScheduler &s,
                            // std::shared_ptr<std::vector<int>> chain,
                            // std::shared_ptr<std::vector<Block>> blcks,
                            Block &b, double latency)
    {
        for (auto &&peer : peers) {
            // if (peer.chain_tip == chain->back()) continue; // Already relayed
            // to this peer peer.chain_tip = chain->back();
            if (peer.peer == from)
                continue; // don't relay to peer that just sent it!

            double jitter = 0;
            if (peer.latency > 0)
                jitter = jitter_func(-peer.latency / 1000., peer.latency / 1000.);
            double tPeer = s.getSimTime() + peer.latency + jitter + latency;
            // auto f = boost::bind(&Miner::ConsiderChain, peer.peer, from,
            // boost::ref(s),
            //                      blcks, b, block_latency);

            auto f = boost::bind(&Miner::ConsiderChain, peer.peer, this, boost::ref(s), b, block_latency);
            s.schedule(f, tPeer);
        }

        // TODO: b.id is not synced, some block can occur lately than some fewer (eg. 22 before 20)
        if (from->mID == 0 && (b.id + 1) * 100 / n_blocks > progress) {
            progress++;

            if (progress == 1) {
                last_progress_time = sim_start_time;
            }

            time_t curr_time = time(nullptr);

            // ========================================================
            char time_str[26];
            struct tm *tm_info;

            tm_info = localtime(&curr_time);

            strftime(time_str, 26, "%m-%d %H:%M:%S", tm_info);
            log_progress("[%s]\t", time_str);
            // ========================================================

            log_progress("%d%%\tBlock %d\t", progress, b.id);
            auto time_diff = (time_t)(difftime(curr_time, last_progress_time)) * (100 - progress);
            print_diff_time(time_diff);

            // Print mempool fullness for 1st honest miner (if exists)
            if (honest_miner_id != -1) {
                Miner *honest_miner = miners.at(honest_miner_id);
                log_progress("\t| Honest miner[%d] - %ld (%.2f%%)\t", honest_miner_id,
                             mem_pool->size(),
                             ((double)mem_pool->size() / max_mp_size) * 100.0);
            }

            if (malicious_miner_id != -1) {
                // Print mempool fullness for 1st malicious miner (if exists)
                Miner *malicious_miner = miners.at(malicious_miner_id);
                log_progress("\t| Malicious miner[%d] - %ld (%.2f%%)", malicious_miner_id,
                             mem_pool->size(),
                             ((double)mem_pool->size() / max_mp_size) * 100.0);
            }

            log_progress("\n");

            last_progress_time = curr_time;

            for (auto miner : miners) {
                log_mempool("%d,%d,%ld\n", miner->mID, progress, mem_pool->size());
            }

            // ===================================== Optimization experiment code =====================================
            // Save simulation est time
            //            fprintf(time_est_file, "%d, %ld\n", progress, time_diff);
            // ========================================================================================================
        }

        // ===================================== Optimization experiment code =====================================
        // Save current mempool fullness for each node
        //        fprintf(mempool_sizes_file, "%d, %lu\n", this->mID, mem_pool.size());
        // ========================================================================================================

        //        if (b.id == n_blocks - 1) {
        //            printf("%ld\n", (time_t)difftime(time(nullptr), sim_start_time));
        //
        //            // ===================================== Optimization experiment code
        //            =====================================
        //            // Save total time of program running
        //            fprintf(total_time_file, "%ld\n", (time_t)difftime(time(nullptr), sim_start_time));
        //
        //            fclose(time_est_file);
        //            fclose(mempool_sizes_file);
        //            fclose(total_time_file);
        //            //
        //            ========================================================================================================
        //            exit(0);
        //        }
    }

    // virtual void ResetChain() {
    //     best_chain->clear();
    // }

    const double GetHashFraction() const
    {
        return hash_fraction;
    }

    const double GetBlockLatency() const
    {
        return block_latency;
    }
    // std::vector<int> GetBestChain() const { return *best_chain; }

  protected:
    double hash_fraction; // This miner has hash_fraction of hash rate
    // This miner produces blocks that take block_latency seconds to
    // relay/validate
    double block_latency;
    JitterFunction jitter_func;

    // std::shared_ptr<std::vector<int>> best_chain;
    std::list<PeerInfo> peers;

    // std::vector<std::vector<int>> adjTable;
    // std::vector<int> adjTable;
    // std::map<int, std::map<int, bool>> adjTable;
    bool *block_bdcast = new bool[n_blocks]{ false };
    static int nextID;
};

int Miner::nextID = 0;

#endif /* STANDARD_MINER_H */
