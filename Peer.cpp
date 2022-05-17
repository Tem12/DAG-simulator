/**
 * @file Peer.cpp
 * @brief Includes reference to a miner and block propagation latency to this miner
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @date 2021 - 2022
 */

#include "Peer.h"

Peer::Peer(Miner &_miner, double _latency) : miner(_miner), latency(_latency) {}

Miner &Peer::getMiner() const {
	return miner;
}

double Peer::getLatency() const {
	return latency;
}
