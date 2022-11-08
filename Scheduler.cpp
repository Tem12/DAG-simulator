// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file Scheduler.cpp
 * @brief It contains the simple implementation of an event calendar to run discrete event simulations
 * @author The Bitcoin Core developers
 * @date 2021 - 2022
 */

#include "Scheduler.h"

CScheduler::CScheduler() : simTime(0.0) {
}

CScheduler::~CScheduler() {
}

void CScheduler::serviceQueue() {
	while (!taskQueue.empty()) {
		simTime = taskQueue.begin()->first;
		Function f = taskQueue.begin()->second;
		taskQueue.erase(taskQueue.begin());
		f();
	}
}

void CScheduler::schedule(const CScheduler::Function& f, double t) {
	taskQueue.insert(std::make_pair(t, f));
}