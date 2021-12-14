//
// Standard miner. Relays and mines on longest chain it has seen.
//
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

// FIXME : temp added to extern globally
extern std::string config_file;
extern time_t sim_start_time;
extern int n_blocks;
extern int max_mp_size;
extern bool end_simulation;

// Used for logging
extern std::vector<Miner *> miners;
extern int honest_miner_id;
extern int malicious_miner_id;

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

// uint_64 id, uint_32 fee
// typedef multi_index_container<Record,
//                              indexed_by<hashed_unique<member<Record, uint64_t, &Record::id>>,
//                                         ordered_non_unique<member<Record, uint32_t, &Record::fee>>, random_access<>>>
//    Mempool;

enum miner_type { HONEST, MALICIOUS };

// Hastab
struct Key {
    uint64_t tx_id;
    int miner_id;

    bool operator==(const Key &other) const
    {
        return (tx_id == other.tx_id && miner_id == other.miner_id);
    }
};

struct KeyHasher {
    std::size_t operator()(const Key &k) const
    {
        // Start with a hash value of 0    .
        std::size_t seed = 0;

        // Modify 'seed' by XORing and bit-shifting in
        // one member of 'Key' after the other:
        boost::hash_combine(seed, boost::hash_value(k.tx_id));
        boost::hash_combine(seed, boost::hash_value(k.miner_id));

        // Return the result.
        return seed;
    }
};

typedef std::unordered_map<Key, uint32_t, KeyHasher> Mempool;

class Miner
{
  public:
    // public
    const int mID;
    std::shared_ptr<std::vector<Block>> blocks;
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
        blocks = std::make_shared<std::vector<Block>>();
        balance = 0;
        depth = 0;
    }

    void AddPeer(Miner *peer, double latency)
    {
        peers.push_back(PeerInfo(peer, -1, latency));
        // adjTable[(peer->mID)] = std::map<int, bool>{};
        // map_bcst
    }

    static bool SortCmpAsc(std::pair<Key, uint32_t> &a, std::pair<Key, uint32_t> &b)
    {
        return a.second < b.second;
    }

    static bool SortCmpDesc(std::pair<Key, uint32_t> &a, std::pair<Key, uint32_t> &b)
    {
        return a.second > b.second;
    }

    std::vector<std::pair<Key, uint32_t>> getSortedMPAsc()
    {
        std::vector<std::pair<Key, uint32_t>> txs(mem_pool.begin(), mem_pool.end());
        std::sort(txs.begin(), txs.end(), SortCmpAsc);
        return txs;
    }

    std::vector<std::pair<Key, uint32_t>> getSortedMPDesc()
    {
        std::vector<std::pair<Key, uint32_t>> txs(mem_pool.begin(), mem_pool.end());
        std::sort(txs.begin(), txs.end(), SortCmpDesc);
        return txs;
    }

    void RemoveMP(const int size)
    {
        std::vector<std::pair<Key, uint32_t>> sortedMp = getSortedMPAsc();

        for (int i = 0; i < size; i++) {
            mem_pool.erase(sortedMp[i].first);
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
        map_bcst[blockNumber] = true;
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
                //                std::uniform_int_distribution<> distr(0, mem_pool.size() - 1);

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
                auto mem_pool_it = mem_pool.begin();

                // TODO: add check if current size of mempool is less then block size
                uint64_t id = mem_pool_it->first.tx_id;
                uint32_t fee = mem_pool_it->second;

                tmp_block.txn.push_back(Record{ id, fee });

                log_data_stats("%lld,%u,%d,%d,%d\n", id, fee, tmp_block.id, tmp_block.depth, this->mID);

                //                auto it = mem_pool.find(id);
                mem_pool.erase(mem_pool_it);
            }
        } else if (this->type == MALICIOUS) {
            // Malicious miners

            std::vector<std::pair<Key, uint32_t>> sortedMp = getSortedMPDesc();
            for (int i = 0; i < blockSize; i++) {
                tmp_block.txn.push_back(Record{ sortedMp[i].first.tx_id, sortedMp[i].second });
//                abc[sortedMp[i].first.tx_id].first = sortedMp[i].second;
//                abc[sortedMp[i].first.tx_id].second.push_back(std::tuple<int, int>(depth, mID));

                log_data_stats("%lld,%u,%d,%d,%d\n", sortedMp[i].first.tx_id, sortedMp[i].second, tmp_block.id,
                               tmp_block.depth, this->mID);

                // Remove processed transactions
                mem_pool.erase(sortedMp[i].first);
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

        if (map_bcst.find(b.id) == map_bcst.end()) { // not found
            map_bcst[b.id] = true;

            // update local mempool
            for (auto &elem : b.txn) {
                auto mempool_processed_tx = mem_pool.find({ elem.id, this->mID });

                if (mempool_processed_tx != mem_pool.end()) {
                    mem_pool.erase(mempool_processed_tx);
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
        // TODO : parametrize printing update stats
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
                log_progress("\t| Honest miner[%d] - %ld (%.2f%%)\t", honest_miner_id, honest_miner->mem_pool.size(),
                             ((double)honest_miner->mem_pool.size() / max_mp_size) * 100.0);
            }

            if (malicious_miner_id != -1) {
                // Print mempool fullness for 1st malicious miner (if exists)
                Miner *malicious_miner = miners.at(malicious_miner_id);
                log_progress("\t| Malicious miner[%d] - %ld (%.2f%%)", malicious_miner_id, malicious_miner->mem_pool.size(),
                             ((double)malicious_miner->mem_pool.size() / max_mp_size) * 100.0);
            }

            log_progress("\n");

            last_progress_time = curr_time;

            for (auto miner : miners) {
                log_mempool("%d,%d,%d\n", miner->mID, progress, miner->mem_pool.size());
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

        // FIXME: End simulation is properly computed yet, this temporary solution will stop simulation on last block
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
    std::map<int, bool> map_bcst;
    static int nextID;
};

int Miner::nextID = 0;

#endif /* STANDARD_MINER_H */
