/**
 * @file Simulation.h
 * @brief The main module of the whole simulation process
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @author Gavin Andresen <gavinandresen@gmail.com>
 * @date 2021 - 2022
 */

#ifndef SIMULATION_H
#define SIMULATION_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <random>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include "ArgParser.h"
#include "ConfigParser.h"
#include "Scheduler.h"

class ArgParser;

class Miner;

class Simulation {
	friend class ArgParser;

	const std::string DATA_OUTPUT_DIR = "outputs";
	const int DATA_OUTPUT_MAX_RUN_ID = 9999;
	const int DATA_OUTPUT_MAX_DIGITS = 4;

	std::string configPath;
	int32_t seed = 0;
	uint32_t mpCapacity = 5000;
	uint32_t maxTxGenCount = 150;
	uint32_t minTxGenCount = 100;
	uint32_t maxTxGenTime = 20;
	uint32_t minTxGenTime = 10;
	uint32_t blockSize = 100;
	uint32_t blocks = 1000;
	uint32_t lambda = 20;   // Block creation rate in seconds
	uint32_t initTxCount = 1000;
	bool honestRandomRemove = false;
	bool mpPrintData = false;

	uint32_t progress = 0; // %

	CScheduler scheduler;
	std::mt19937 randomGen;
	std::vector<Miner> miners;

	std::string configFilename;
	std::string simRunIdString;

	uint32_t honestMinersCount = 0;
	uint32_t maliciousMinersCount = 0;
	uint32_t kaspalikeMinersCount = 0;

	std::ofstream progressOutput;
	std::ofstream mempoolOutput;
	std::ofstream dataOutput;
	std::ofstream metadataOutput;

	time_t simStartTime;
	time_t lastProgressTime;

	// Store index of each miner type used in progress logging
	size_t firstHonestMinerIndex = 0;
	size_t firstMaliciousMinerIndex = 0;
	size_t firstKaspalikeMinerIndex = 0;
	bool honestMinerIndexSet = false;
	bool maliciousMinerIndexSet = false;
	bool kaspalikeMinerIndexSet = false;

	size_t txId = 0;

	double txGenerationLambda = 150.0;

	std::uniform_int_distribution<> txGenCountDistribution;
	std::uniform_int_distribution<> txGenTimeDistribution;
	std::exponential_distribution<> feeGenDistribution;

	bool stopGenerateTransactionsFlag = false;

	/**
	 * @brief Schedule block event generations
	 */
	void scheduleBlockGenerations();

	/**
	 * @brief Event to generate initial transactions to all miners
	 */
	void generateInitialTransactions();

	/**
	 * @brief Event that continuously generate set of transactions
	 */
	void generateTransactions();

	/**
	 * @brief Create output files and prepare csv headers
	 */
	void prepareOutput();

	/**
	 * @brief Print simulation information on start in easily readable format
	 */
	void printSimulationStart();

	/**
	 * @brief Print first progress step of the simulation (0%)
	 */
	void startProgress();

	/**
	 *
	 * @param timeDiff second time difference with previously stored time
	 * @param ss string stream
	 */
	void logTimeInterval(long timeDiff, std::stringstream &ss);

public:
	Simulation();

	/**
	 * @brief Called after successful argument parsing. Continue with config parsing and starting the simulation
	 */
	void runSimulation();

	/**
	 * @brief Store progress output in file
	 * @param blockId Id of a block
	 */
	void logProgress(uint32_t blockId);

	/**
	 *
	 * @param id transaction id
	 * @param fee transaction fee
	 * @param blockId id of a block
	 * @param depth blockchain depth (height)
	 * @param minerId id of a miner
	 */
	void logData(uint64_t id, uint32_t fee, uint32_t blockId, uint32_t depth, uint32_t minerId);

	/**
	 * @brief Store mempool data of all miners
	 */
	void logMempoolDataOfAllMiners();

	/**
	 * @brief When all blocks are mined, transaction generation can be stopped
	 */
	void stopGenerateTransactions();

	/**
	 * @brief Output a simulation duration
	 */
	void finishSimulation();

	/**
	 * @brief Error when some miner runs out of transactions
	 * @param miner Miner that runs out of transactions
	 */
	void errorOutOfTxsExit(Miner &miner);

	/**
	 *
	 * @return Path to a configuration file
	 */
	const std::string &getConfigPath() const;

	/**
	 *
	 * @return Seed used in a simulation
	 */
	int32_t getSeed() const;

	/**
	 *
	 * @return Random geneator
	 */
	std::mt19937 &getRandomGen();

	/**
	 *
	 * @return Mempool capacity used by all miners
	 */
	uint32_t getMpCapacity() const;

	/**
	 *
	 * @return Maximum transaction count that be generated
	 */
	uint32_t getMaxTxGenCount() const;

	/**
	 *
	 * @return Minimum transaction count that be generated
	 */
	uint32_t getMinTxGenCount() const;

	/**
	 *
	 * @return Maximum transaction generation next schedule time
	 */
	uint32_t getMaxTxGenTime() const;

	/**
	 *
	 * @return Minimum transaction generation next schedule time
	 */
	uint32_t getMinTxGenTime() const;

	/**
	 *
	 * @return Number of transactions that can be stored in a block
	 */
	uint32_t getBlockSize() const;

	/**
	 *
	 * @return Number of blocks to be simulated
	 */
	uint32_t getBlockCount() const;

	/**
	 *
	 * @return Block creation rate in seconds
	 */
	uint32_t getLambda() const;

	/**
	 *
	 * @return Bool if honest miners remove their transactions randomly on full mempool
	 */
	bool honestRandomRemoveEnabled() const;

	/**
	 *
	 * @return Bool if mempool stats of each miner should be printed
	 */
	bool mpPrintDataEnabled() const;

	/**
	 *
	 * @return Current simulation percentage progress
	 */
	uint32_t getProgress() const;

	/**
	 * @brief Increment simulation progress
	 */
	void incrementProgress();

	/**
	 *
	 * @return Scheduler reference
	 */
	CScheduler &getScheduler();
};


#endif //SIMULATION_H
