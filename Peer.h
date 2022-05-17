/**
 * @file Peer.h
 * @brief Includes reference to a miner and block propagation latency to this miner
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @date 2021 - 2022
 */

#ifndef PEER_H
#define PEER_H

#include "Miner.h"

class Miner;

class Peer {
	Miner &miner;
	double latency;
public:
	/**
	 *
	 * @param _miner peer that is also a miner
	 * @param _latency block propagation delay
	 */
	Peer(Miner &_miner, double _latency);

	/**
	 *
	 * @return Miner reference
	 */
	Miner &getMiner() const;

	/**
	 *
	 * @return block propagation delay from miner to this peer
	 */
	double getLatency() const;
};


#endif //PEER_H
