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

#include "est_time.h"

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

// ===================================== Optimization experiment code =====================================
// Extern fp from main.cpp
extern FILE *time_est_file;
extern FILE *mempool_sizes_file;
extern FILE *total_time_file;
// ========================================================================================================

// FIXME : temp variable
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
typedef std::unordered_map<uint64_t, uint32_t> Mempool;

enum miner_type { HONEST, MALICIOUS };

class Miner
{
  public:
    // public
    const int mID;
    std::shared_ptr<std::vector<Block>> blocks;
    Mempool mem_pool;
    miner_type type; // Miner can either honest or malicious
    u_int64_t reward;
    u_int64_t balance;

    int txnum;
    std::map<uint32_t, bool> tx;

    int depth;
    // std::map<int, int> depth_map;

    void PrintStats()
    {
        if (this->mID == 0) {
            int fluke = 0;
            int total = 0;
            std::vector<int> em(nextID);

            std::cout << "Stats t_all:" << txnum << " t_dup:" << tx.size() << " => " << std::setprecision(3)
                      << 100 - (((double)tx.size() / (double)txnum) * 100) << "%\n";
            // for (const auto& [key, value] : depth_map) {
            // std::cout << "[" << key << ":" << value << "]";
            // }
            std::cout << "Blocks:" << 10000 << " Depth:" << depth << " => " << std::setprecision(3)
                      << (double)(10000 - depth) / 10000.0 * 100 << "%\n";

            // TODO: edit/separate printing each miner/block stats

            //            std::ofstream myfile;
            //            myfile.open(config_file.append(".details.txt"));
            //
            //            // std::cout << abc.size() << "\n";
            //            // int i = 0;
            //            for (auto const &[key, val] : abc) {
            //                // myfile << i++ << "\n";
            //                total += val.first;
            //                if (val.second.size() > 1) {
            //                    if (val.second.size() > 3)
            //                        fluke++;
            //                    myfile << key << " - [" << val.first << "] ";
            //                    for (auto it = val.second.begin(); it != val.second.end();
            //                         ++it) {
            //                        myfile << std::get<0>(*it) << ',' << std::get<1>(*it)
            //                               << ' ';
            //                        int t = std::get<1>(*it);
            //                        // payoff function
            //                        /* split evenly distributed */
            //                        // em[t] += val.first / val.second.size();
            //                        /* only first come first serve */
            //                        em[t] += val.first;
            //                        break;
            //                    }
            //                    myfile << '\n';
            //                } else {
            //                    int t = std::get<1>(*val.second.data());
            //                    em[t] += val.first;
            //                }
            //            }
            //
            //            std::cout << "[";
            //            for (int e : em) {
            //                std::cout << std::setprecision(3)
            //                          << ((double)e / (double)total * 100.0) << "% ";
            //            }
            //            std::cout << "]" << std::endl;
            //
            //            std::cout << "fluke count " << fluke << std::endl;
            //            // std::cout << total;
            //            // for (auto it = abc.begin(); it != abc.end(); ++it) {
            //            //     myfile << it->first << "- ";
            //            //     for (auto it2 = it->second.begin(); it2 != it->second.end();
            //            //     ++it2) {
            //            //         myfile << std::get<0>(*it2) << ',' << std::get<1>(*it2)
            //            //         << ' ';
            //            //     }
            //            //     myfile << '\n';
            //            // }
            //            myfile.close();
        }
    }

    // Return a random double in the range passed.
    typedef boost::function<double(double, double)> JitterFunction;

    Miner(double _hash_fraction, double _block_latency, miner_type _type, JitterFunction _func)
        : hash_fraction(_hash_fraction), type(_type), block_latency(_block_latency), jitter_func(_func), mID(nextID++)
    {
        // best_chain = std::make_shared<std::vector<int>>();
        blocks = std::make_shared<std::vector<Block>>();
        reward = 0;
        balance = 0;
        txnum = 0;
        depth = 0;
    }

    void AddPeer(Miner *peer, double latency)
    {
        peers.push_back(PeerInfo(peer, -1, latency));
        // adjTable[(peer->mID)] = std::map<int, bool>{};
        // map_bcst
    }

    static bool SortCmpAsc(std::pair<uint64_t, uint32_t> &a, std::pair<uint64_t, uint32_t> &b)
    {
        return a.first < b.first;
    }

    static bool SortCmpDesc(std::pair<uint64_t, uint32_t> &a, std::pair<uint64_t, uint32_t> &b)
    {
        return a.first > b.first;
    }

    std::vector<std::pair<uint64_t, uint32_t>> getSortedMPAsc()
    {
        std::vector<std::pair<uint64_t, uint32_t>> txs(mem_pool.begin(), mem_pool.end());
        std::sort(txs.begin(), txs.end(), SortCmpAsc);
        return txs;
    }

    std::vector<std::pair<uint64_t, uint32_t>> getSortedMPDesc()
    {
        std::vector<std::pair<uint64_t, uint32_t>> txs(mem_pool.begin(), mem_pool.end());
        std::sort(txs.begin(), txs.end(), SortCmpDesc);
        return txs;
    }

    void RemoveMP(const int size)
    {
        std::vector<std::pair<uint64_t, uint32_t>> sortedMp = getSortedMPAsc();

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
                std::uniform_int_distribution<> distr(0, mem_pool.size() - 1);
                int rand_index = distr(rng);

                //                for (auto& it : mem_pool) {
                //                    std::cout << it.first << ' '
                //                              << it.second << std::endl;
                //                }
                //
                //                printf("%\n", mem_pool.size() - 1);

                //                int rand_index = distr(rng);

                //                uint64_t id = (mem_pool.find(rand_index))->first;
                //                uint32_t fee = (mem_pool.find(rand_index))->second;

                auto mem_pool_tx = mem_pool.find(rand_index);
                uint64_t id = mem_pool_tx->first;
                uint32_t fee = mem_pool_tx->second;

                tmp_block.txn.push_back(Record{ id, fee });
                abc[id].first = fee;
                abc[id].second.push_back(std::tuple<int, int>(depth, mID));

                if (this->mID == 0) {
                    tx[id] = true;
                }

                //                auto it = mem_pool.find(id);
                mem_pool.erase(rand_index);
            }
            if (this->mID == 0) {
                txnum += tmp_block.txn.size();
            }

            // if (this->mID == 0) {
            //     txnum += tmp_block.txn.size();
            // }
        } else if (this->type == MALICIOUS) {
            // Malicious miners

            std::vector<std::pair<uint64_t, uint32_t>> sortedMp = getSortedMPDesc();
            for (int i = 0; i < blockSize; i++) {
                tmp_block.txn.push_back(Record{ sortedMp[i].first, sortedMp[i].second });
                abc[sortedMp[i].first].first = sortedMp[i].second;
                abc[sortedMp[i].first].second.push_back(std::tuple<int, int>(depth, mID));

                // Mark that transaction has been processed (to count duplicates)
                if (this->mID == 0) {
                    tx[sortedMp[i].first] = true;
                }

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
            if (this->mID == 0) {
                txnum += b.txn.size();
            }

            for (auto &elem : b.txn) {
                mem_pool.erase(elem.id);
                if (this->mID == 0) {
                    tx[elem.id] = true;
                }
            }
            RelayChain(this, s, b, latency);
        } // else -> found
          // RelayChain(from, s, blcks, b, latency);
          // RelayChain(this, s, b, latency);
          // }
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
        if (from->mID == 0 && b.id * 100 / n_blocks > progress) {
            progress++;

            if (progress == 1) {
                last_progress_time = sim_start_time;
            }

            time_t curr_time = time(nullptr);
            printf("%d%%\tBlock %d\t", progress, b.id);
            auto time_diff = (time_t)(difftime(curr_time, last_progress_time)) * (100 - progress);
            print_diff_time(time_diff);

            last_progress_time = curr_time;

            // ===================================== Optimization experiment code =====================================
            // Save simulation est time
            fprintf(time_est_file, "%d, %ld\n", progress, time_diff);
            // ========================================================================================================
        }

        // ===================================== Optimization experiment code =====================================
        // Save current mempool fullness for each node
        fprintf(mempool_sizes_file, "%d, %lu\n", this->mID, mem_pool.size());
        // ========================================================================================================

        // FIXME: End simulation is properly computed yet, this temporary solution will stop simulation on last block
        if (b.id == n_blocks - 1) {
            printf("%ld\n", (time_t)difftime(time(nullptr), sim_start_time));

            // ===================================== Optimization experiment code =====================================
            // Save total time of program running
            fprintf(total_time_file, "%ld\n", (time_t)difftime(time(nullptr), sim_start_time));

            fclose(time_est_file);
            fclose(mempool_sizes_file);
            fclose(total_time_file);
            // ========================================================================================================
            exit(0);
        }
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
    static std::map<uint64_t, std::pair<int, std::vector<std::tuple<int, int>>>> abc;
};

int Miner::nextID = 0;
std::map<uint64_t, std::pair<int, std::vector<std::tuple<int, int>>>> Miner::abc;

#endif /* STANDARD_MINER_H */
