// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file Scheduler.h
 * @brief It contains the simple implementation of an event calendar to run discrete event simulations
 * @author The Bitcoin Core developers
 * @date 2021 - 2022
 */

#ifndef BITCOIN_SCHEDULER_H
#define BITCOIN_SCHEDULER_H

#include <functional>
#include <map>
#include <utility>

class CScheduler {
public:
	CScheduler();

	~CScheduler();

	typedef std::function<void(void)> Function;

	/**
	 * @brief Plan a function call f to simulation time t
	 * @param f function to be called
	 * @param t simulation when to be called
	 */
	void schedule(const Function &f, double t);

	/**
	 * @brief start simulation event process
	 */
	void serviceQueue();

	/**
	 *
	 * @return current simulation time
	 */
	double getSimTime() {
		return simTime;
	}

private:
	std::multimap<double, Function> taskQueue;
	double simTime;
};


#endif // BITCOIN_SCHEDULER_H
