/**
 * @file ConfigParser.h
 * @brief Parse and validate the input configuration file
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @date 2021 - 2022
 */

#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <random>
#include "Simulation.h"
#include "Miner.h"
#include "distributionData/BlockPropagationDelay.h"

class Miner;

// max deviation of total mining power of all miners
const double TOTAL_HASHPOWER_EPS = 0.000001;

class ConfigParser {
	// Required number of token for miner definition
	const size_t MINER_TOKEN_COUNT = 2;

	// Required number of tokens for bidirectional connection definition
	const size_t BICONN_TOKEN_COUNT = 2;

public:
	/**
	 *
	 * @param simulation Simulator instance
	 * @return parsed and created miners from config
	 */
	std::vector<Miner> parseConfig(Simulation &simulation) const;
};


#endif //CONFIGPARSER_H
