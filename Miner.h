/**
 * @file Miner.h
 * @brief Implementation of a node in a peer-to-peer network that involves mining
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @author Gavin Andresen <gavinandresen@gmail.com>
 * @date 2021 - 2022
 */

#ifndef MINER_H
#define MINER_H

#include <vector>
#include <list>
#include <random>
#include "Simulation.h"
#include "Block.h"
#include "Peer.h"
#include "Scheduler.h"
#include "Mempool.h"

enum MinerType {
	HONEST,
	MALICIOUS
};

class Simulation;

class Peer;

const double BLOCK_PROPAGAITON_JITTER_DIFF_MIN = -1000.0;
const double BLOCK_PROPAGAITON_JITTER_DIFF_MAX = 1000.0;

static uint32_t nextId = 0;
static uint32_t minersFinished = 0;
static uint32_t lastMinedBlockId = 0;

class Miner {
	uint32_t minerId;
	Simulation &simulation;
	double miningPower;
	MinerType type;
	std::list<Peer> peers;
	Mempool mempool;
	uint32_t depth;
	std::vector<bool> receivedBlocks;

	void broadcastBlock(Miner &fromMiner, const Block& block);
	void receiveBlock(const Block& block);

public:
	/**
	 *
	 * @param _miningPower mining power of miner relative to the network
	 * @param _type Type can be either honest or malicious
	 * @param _simulation Simulation instance reference
	 */
	Miner(double _miningPower, MinerType _type, Simulation &_simulation);

	/**
	 *
	 * @param miner peer that is also a miner
	 * @param latency block propagation delay
	 */
	void addPeer(Miner &miner, double latency);

	/**
	 *
	 * @param txId transaction id
	 * @param fee transaction fee
	 */
	void insertTransaction(uint64_t txId, uint32_t fee);

	/**
	 * @brief Sorted remove
	 * @param size number of transactions to remove
	 */
	void removeTransactionsRationally(const uint32_t size);

	/**
	 * @brief Random remove
	 * @param size number of transactions to remove
	 */
	void removeTransactionsRandom(const uint32_t size);

	/**
	 * @brief Miner generate a block event
	 * @param blockNumber new block unique number
	 */
	void mineBlock(uint32_t blockNumber);

	/**
	 *
	 * @return current miner id
	 */
	uint32_t getMinerId() const;

	/**
	 *
	 * @return mining power relative to the network
	 */
	double getMiningPower() const;

	/**
	 *
	 * @return miner type (honest or malicious)
	 */
	MinerType getType() const;

	/**
	 *
	 * @return miners' peers
	 */
	const std::list<Peer> &getPeers() const;

	/**
	 *
	 * @return Maximum number of transactions that be stored in miner's mempool
	 */
	size_t getMempoolFullness() const;
};


#endif //MINER_H
