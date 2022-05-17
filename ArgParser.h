/**
 * @file ArgParser.h
 * @brief Parse and validate program arguments
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @date 2021 - 2022
 */

#ifndef ARGPARSER_H
#define ARGPARSER_H

#include <getopt.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Simulation.h"

class Simulation;

enum argumentOptions {
	OPT_HELP, OPT_SEED, OPT_CONFIG, OPT_MP_CAPACITY, OPT_MAX_TX_GEN_COUNT, OPT_MIN_TX_GEN_COUNT,
	OPT_MAX_TX_GEN_TIME, OPT_MIN_TX_GEN_TIME, OPT_BLOCK_SIZE, OPT_BLOCKS, OPT_LAMBDA, OPT_INIT_TX_COUNT,
	OPT_HONEST_RAND_REMOVE, OPT_MP_PRINT_DATA, OPT_INVALID
};

class ArgParser {

public:
	/**
	 *
	 * @param argc
	 * @param argv
	 * @return created Simulation instance
	 */
	Simulation createSimulation(int argc, char *argv[]);

	/**
	 * @brief Print help message
	 */
	void printHelp();

	/**
	 * @param msg error message
	 * @brief Prints error message and exit
	 */
	void errorExit(const std::string &msg);
};


#endif //ARGPARSER_H
