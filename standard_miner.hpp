//
// Standard miner. Relays and mines on longest chain it has seen.
//
#ifndef STANDARD_MINER_H
#define STANDARD_MINER_H

#include <boost/function.hpp>
#include <list>
#include <vector>
#include <set>
#include <utility>

class Miner;
class PeerInfo {
public:
    PeerInfo(Miner* _peer, int _chain_tip, double _latency) :
        peer(_peer), chain_tip(_chain_tip), latency(_latency) { }

    Miner* peer;
    int chain_tip;
    double latency;
};

struct Record {
	int fee;
	int ID;
};

struct Block {
    std::vector<Record> trans;
};

bool operator < (const Record& l, const Record& r) {
	if (l.fee < r.fee)
		return true;
	return false;
};

class Miner
{
public:
    // public
    std::shared_ptr<std::vector<Block>> blocks;
    // std::vector<Block> blocks;
    std::multiset<Record> mem_pool;
    // std::shared_ptr<std::vector<Block>> blocks;
    // std::vector<Block> blocks;

    // Return a random double in the range passed.
    typedef boost::function<double(double, double)> JitterFunction;

    Miner(double _hash_fraction, double _block_latency, JitterFunction _func) :
        hash_fraction(_hash_fraction), block_latency(_block_latency), jitter_func(_func) {
        best_chain = std::make_shared<std::vector<int>>();
        blocks = std::make_shared<std::vector<Block>>();
    }

    void AddPeer(Miner* peer, double latency) {
        peers.push_back(PeerInfo(peer, -1, latency));
    }

    virtual void FindBlock(CScheduler& s, int blockNumber) {
        // Extend the chain:
        auto chain_copy = std::make_shared<std::vector<int>>(best_chain->begin(),
                                                             best_chain->end());
        chain_copy->push_back(blockNumber);
        best_chain = chain_copy;
        // Transactions in block
        auto blocks_copy = std::make_shared<std::vector<Block>>(blocks->begin(),
                                                                blocks->end());
        Block tmp_block;
        int i = 0;
        for (const auto& elem: mem_pool) {
            tmp_block.trans.push_back(elem);
            // mem_pool.erase(elem);
            if (i >= 2000) break; // max 2000 transactions
            i++;
        }
        // a.trans.push_back(Record{5,10});
        blocks_copy->push_back(tmp_block);
        blocks = blocks_copy;
        // auto it = blocks.begin();
        // blocks_copy[blockNumber]->trans.push_back(Record{5,10});

#ifdef TRACE
        std::cout << "Miner " << hash_fraction << " found block at simulation time "
                  << s.getSimTime() << "\n";
#endif
        RelayChain(this, s, chain_copy, blocks_copy, block_latency);
    }

    virtual void ConsiderChain(Miner* from, CScheduler& s,
                               std::shared_ptr<std::vector<int>> chain,
                               std::shared_ptr<std::vector<Block>> blck, double latency) {
        if (chain->size() > best_chain->size()) {
#ifdef TRACE
            std::cout << "Miner " << hash_fraction
                      << " relaying chain at simulation time " << s.getSimTime() << "\n";
#endif
            best_chain = chain;
            blocks = blck;
            // TODO change local mempool
            RelayChain(from, s, chain, blck, latency);
        }
    }

    virtual void RelayChain(Miner* from, CScheduler& s,
                            std::shared_ptr<std::vector<int>> chain,
                            std::shared_ptr<std::vector<Block>> blck, double latency) {
        for (auto&& peer : peers) {
            if (peer.chain_tip == chain->back()) continue; // Already relayed to this peer
            peer.chain_tip = chain->back();
            if (peer.peer == from) continue; // don't relay to peer that just sent it!

            double jitter = 0;
            if (peer.latency > 0) jitter =
                jitter_func(-peer.latency/1000., peer.latency/1000.);
            double tPeer = s.getSimTime() + peer.latency + jitter + latency;
            auto f = boost::bind(&Miner::ConsiderChain, peer.peer, from, boost::ref(s),
                                 chain, blck, block_latency);
            s.schedule(f, tPeer);
        }
    }

    virtual void ResetChain() {
        best_chain->clear();
    }

    const double GetHashFraction() const { return hash_fraction; }
    std::vector<int> GetBestChain() const { return *best_chain; }

protected:
    double hash_fraction; // This miner has hash_fraction of hash rate
    // This miner produces blocks that take block_latency seconds to relay/validate
    double block_latency;
    JitterFunction jitter_func;

    std::shared_ptr<std::vector<int>> best_chain;
    std::list<PeerInfo> peers;
};

#endif /* STANDARD_MINER_H */
