/**
 * @file Miner.cpp
 * @brief Implementation of a node in a peer-to-peer network that involves mining
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @author Gavin Andresen <gavinandresen@gmail.com>
 * @date 2021 - 2022
 */

#include "Miner.h"

Miner::Miner(double _miningPower, MinerType _type, Simulation &_simulation) : miningPower(_miningPower), type(_type),
                                                                              simulation(_simulation),
                                                                              minerId(nextId++),
                                                                              depth(0),
                                                                              mempool(simulation.getMpCapacity()),
                                                                              receivedBlocks(simulation.getBlockCount(),
                                                                                             false) {
}

void Miner::addPeer(Miner &miner, double latency) {
	peers.emplace_back(miner, latency);
}

void Miner::mineBlock(uint32_t blockNumber) {
	depth++;

	receivedBlocks[blockNumber] = true;
	Block minedBlock{blockNumber, depth, simulation.getBlockSize()};

	// Stop simulation if miner has not enough transaction to fill the block
	if (simulation.getBlockSize() > getMempoolFullness()) {
		simulation.errorOutOfTxsExit(*this);
	}

	if (type == HONEST) {
		for (uint32_t i = 0; i < simulation.getBlockSize(); ++i) {
			HtabIterator it = mempool.getRandomTransaction(simulation.getRandomGen());

			uint64_t txId = it.iterator->txId;
			uint32_t fee = it.iterator->fee;

			minedBlock.transactions.push_back({txId, fee});

			// Log mined block
			simulation.logData(txId, fee, minedBlock.id, depth, minerId);

			mempool.eraseTransaction(it);
		}
	}
	else if (type == MALICIOUS) {
		for (uint32_t i = 0; i < simulation.getBlockSize(); ++i) {
			HtabIterator it = mempool.getSortedTransactionDescending();

			uint64_t txId = it.iterator->txId;
			uint32_t fee = it.iterator->fee;

			minedBlock.transactions.push_back({txId, fee});

			// Log mined block
			simulation.logData(txId, fee, minedBlock.id, depth, minerId);

			mempool.eraseTransaction(it);
		}
	}

	lastMinedBlockId++;
	if (lastMinedBlockId * 100 / simulation.getBlockCount() > simulation.getProgress()) {
		simulation.incrementProgress();

		simulation.logProgress(lastMinedBlockId);
	}

	if (simulation.mpPrintDataEnabled()) {
		simulation.logMempoolDataOfAllMiners();
	}

	broadcastBlock(*this, minedBlock);
}

void Miner::broadcastBlock(Miner &fromMiner, const Block& block) {
	for (Peer &peer: peers) {

		// Do not relay to peer that just sent this block
		if (&peer.getMiner() == &fromMiner) {
			continue;
		}

		double jitter = 0;
		if (peer.getLatency() > 0) {
			std::uniform_real_distribution<> jitterDelay(peer.getLatency() / BLOCK_PROPAGAITON_JITTER_DIFF_MIN,
														peer.getLatency() / BLOCK_PROPAGAITON_JITTER_DIFF_MAX);
			jitter = jitterDelay(simulation.getRandomGen());
		}

		double peerLatencyTime = simulation.getScheduler().getSimTime() + peer.getLatency() + jitter;

//		auto function = std::bind(&Miner::receiveBlock, peer.getMiner(), *this, block, peer.getLatency());
		auto function = [&peer, block]() {
			peer.getMiner().receiveBlock(block);
		};
		simulation.getScheduler().schedule(function, peerLatencyTime);
	}
}

void Miner::receiveBlock(const Block& block) {
	if (block.depth > depth) {
		depth = block.depth;
	}

	// Check if miner already processed this block
	if (!receivedBlocks[block.id]) {
		receivedBlocks[block.id] = true;

		// Update miners mempool
		for (Transaction transaction: block.transactions) {
			HtabIterator htabIterator = mempool.find(minerId, transaction.txId);
			mempool.eraseTransaction(htabIterator);
		}
		broadcastBlock(*this, block);

		// Check (approximately) if all blocks were processed by all miners, if yes stop generate new transactions
		// and finish simulation
		if (block.id == simulation.getBlockCount() - 1) {
			minersFinished++;

			if (minersFinished == nextId - 1) {
				simulation.stopGenerateTransactions();
			}
		}
	}
}

void Miner::insertTransaction(uint64_t txId, uint32_t fee) {
	mempool.insert(minerId, txId, fee);
}

// Sorted remove
void Miner::removeTransactionsRationally(const uint32_t size) {
	mempool.eraseTransactionsAscending(size);
}

// Random remove
void Miner::removeTransactionsRandom(const uint32_t size) {
	mempool.eraseRandomTransactions(simulation.getRandomGen(), size);
}

uint32_t Miner::getMinerId() const {
	return minerId;
}

double Miner::getMiningPower() const {
	return miningPower;
}

MinerType Miner::getType() const {
	return type;
}

const std::list<Peer> &Miner::getPeers() const {
	return peers;
}

size_t Miner::getMempoolFullness() const {
	return mempool.size();
}
