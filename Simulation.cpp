/**
 * @file Simulation.cpp
 * @brief The main module of the whole simulation process
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @author Gavin Andresen <gavinandresen@gmail.com>
 * @date 2021 - 2022
 */

#include "Simulation.h"

Simulation::Simulation() : txGenCountDistribution(int(minTxGenCount), int(maxTxGenCount)),
                           txGenTimeDistribution(int(minTxGenTime), int(maxTxGenTime)),
                           feeGenDistribution(1.0) {
}

void Simulation::runSimulation() {
	// Set random generator initial seed
	randomGen.seed(seed);

	// Setup simulation from configuration
	try {
		miners = ConfigParser{}.parseConfig(*this);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		std::exit(EXIT_FAILURE);
	}

	prepareOutput();
	printSimulationStart();
	startProgress();

	// Prepare transaction generation distributions
	txGenCountDistribution = std::uniform_int_distribution<>(int(minTxGenCount), int(maxTxGenCount));
	txGenTimeDistribution = std::uniform_int_distribution<>(int(minTxGenTime), int(maxTxGenTime));
	feeGenDistribution = std::exponential_distribution(1.0);

	// Schedule miners block generations
	scheduleBlockGenerations();

	// Start generating transactions
	generateInitialTransactions();
	generateTransactions();

	scheduler.serviceQueue();
}

void Simulation::scheduleBlockGenerations() {
	// Schedule events for each that will be mined by miner with probability of his mining power
	std::vector<double> mineProbabilities;
	for (auto &miner: miners) {
		mineProbabilities.push_back(miner.getMiningPower());
	}

	std::discrete_distribution<> minerBlockFindDistribution(mineProbabilities.begin(), mineProbabilities.end());
	std::exponential_distribution<> blockTimeGenerationDistribution(1.0);

	// Map block to miner (index) who found that block
	std::map<uint32_t, size_t> blockOwners;

	double time = 0.0;
	for (uint32_t i = 0; i < blocks; i++) {
		size_t minerIndex = minerBlockFindDistribution(randomGen);
		blockOwners.insert({i, minerIndex});

		double timeDelta = blockTimeGenerationDistribution(randomGen) * lambda;
		double timeFound = time + timeDelta;

//		auto function = std::bind(&Miner::mineBlock, miners[minerIndex], i);
		auto function = [this, minerIndex, i]() { this->miners[minerIndex].mineBlock(i); };
		scheduler.schedule(function, timeFound);
		time = timeFound;
	}
}

void Simulation::generateInitialTransactions() {
	for (int i = 0; i < initTxCount; i++) {
		double fee = feeGenDistribution(randomGen) * txGenerationLambda;

		for (auto &miner: miners) {
			miner.insertTransaction(txId, static_cast<uint32_t>(fee));
		}
		txId++;
	}
}

void Simulation::generateTransactions() {
	uint32_t txCount = txGenCountDistribution(randomGen);
	uint32_t txWaitTime = txGenTimeDistribution(randomGen);

	// Generate txCount transactions
	for (uint32_t i = 0; i < txCount; i++) {
		double fee = feeGenDistribution(randomGen) * txGenerationLambda;

		for (auto &miner: miners) {
			if (miner.getMempoolFullness() + txCount > mpCapacity) {
				if (honestRandomRemove && miner.getType() == HONEST) {
					miner.removeTransactionsRandom(txCount);
				}
				else {
					miner.removeTransactionsRationally(txCount);
				}
			}
			miner.insertTransaction(txId, static_cast<uint32_t>(fee));
		}
		txId++;
	}

	if (!stopGenerateTransactionsFlag) {
		// Plan in (currentTime + txWaitTime) next generation
		double nextGenerationTime = scheduler.getSimTime() + txWaitTime;

//		auto function = []() { &Simulation::generateTransactions; };
		auto function = [this]() { this->generateTransactions(); };
		scheduler.schedule(function, nextGenerationTime);
	}
}

void Simulation::prepareOutput() {
	int simRunId = -1;

	// Get config filename from config path
	size_t config_filename_pos = this->configPath.find_last_of('/');
	if (config_filename_pos == std::string::npos) {
		configFilename = this->configPath;
	}
	else {
		configFilename = this->configPath.substr(config_filename_pos + 1);
	}

	// Check if output directory exists, if not create a new one
	try {
		if (!std::filesystem::exists(DATA_OUTPUT_DIR)) {
			std::filesystem::create_directory(DATA_OUTPUT_DIR);
		}
		else {
			if (!std::filesystem::is_directory(DATA_OUTPUT_DIR)) {
				std::cerr << "Cannot create missing \"" << DATA_OUTPUT_DIR << "\" directory" << std::endl;
				std::exit(EXIT_FAILURE);
			}
		}
	} catch (std::exception &e) {
		std::cerr << "Cannot access or create an \"" << DATA_OUTPUT_DIR << "\" directory" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	// Iterate over output files with same name and find id that follow
	for (int i = 0; i <= DATA_OUTPUT_MAX_RUN_ID; i++) {
		std::string searchFilename = DATA_OUTPUT_DIR;

		std::stringstream ss;
		ss << std::setw(DATA_OUTPUT_MAX_DIGITS) << std::setfill('0') << i;
		simRunIdString = ss.str();

		searchFilename
				.append("/progress_")
				.append(configFilename)
				.append("_")
				.append(simRunIdString)
				.append(".out");

		if (!std::filesystem::exists(searchFilename)) {
			simRunId = i;
			break;
		}
	}

	if (simRunId == -1) {
		std::cerr << "Maximum number of output files for same config exceeded ("
		          << std::to_string(DATA_OUTPUT_MAX_RUN_ID) << ")" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	// outputs/progress_{CFG}_{RUN_ID}.out
	std::string progressFilenamePath = "outputs/progress_";

	// outputs/mempool_{CFG}_{RUN_ID}.csv
	std::string mempoolStatsFilenamePath = "outputs/mempool_";

	// outputs/data_{CFG}_{RUN_ID}.csv
	std::string dataFilenamePath = "outputs/data_";

	// outputs/metadata_{CFG}_{RUN_ID}.data
	std::string metadataFilenamePath = "outputs/metadata_";

	progressFilenamePath.append(configFilename).append("_").append(simRunIdString).append(".out");
	mempoolStatsFilenamePath.append(configFilename).append("_").append(simRunIdString).append(".csv");
	dataFilenamePath.append(configFilename).append("_").append(simRunIdString).append(".csv");
	metadataFilenamePath.append(configFilename).append("_").append(simRunIdString).append(".data");

	this->progressOutput.open(progressFilenamePath);
	this->metadataOutput.open(metadataFilenamePath);
	this->dataOutput.open(dataFilenamePath);

	if (this->mpPrintData) {
		this->mempoolOutput.open(mempoolStatsFilenamePath);
	}
}

void Simulation::printSimulationStart() {
	// Create full absolute path to config file
	std::string configFullPath = std::filesystem::canonical(std::filesystem::absolute(this->configPath));

	double honestMinersPower = 0;
	double maliciousMinersPower = 0;

	int i = 0;
	for (auto &miner: miners) {
		if (miner.getType() == HONEST) {
			honestMinersCount++;
			honestMinersPower += miner.getMiningPower();

			if (!honestMinerIndexSet) {
				honestMinerIndexSet = true;
				firstHonestMinerIndex = i;
			}
		}
		else if (miner.getType() == MALICIOUS) {
			maliciousMinersCount++;
			maliciousMinersPower += miner.getMiningPower();

			if (!maliciousMinerIndexSet) {
				maliciousMinerIndexSet = true;
				firstMaliciousMinerIndex = i;
			}
		}
		i++;
	}

	// Output progress to file and stdout
	std::stringstream ss;

	ss << "Simulation: " << configFilename << "_" << simRunIdString << std::endl
	   << "Config: " << configFullPath << std::endl
	   << "Blocks: " << blocks << std::endl
	   << "Honest miners: " << honestMinersCount << " (" << std::fixed << std::setprecision(2)
	   << honestMinersPower * 100 << "% power)" << std::endl
	   << "Malicious miners: " << maliciousMinersCount << " (" << std::fixed << std::setprecision(2)
	   << maliciousMinersPower * 100 << "% power)" << std::endl
	   << "Seed: " << seed << std::endl
	   << "Mempool capacity: " << mpCapacity << std::endl
	   << "Block size: " << blockSize << std::endl
	   << "Lambda: " << lambda << std::endl
	   << "Min. transaction generation time: " << minTxGenTime << " sec" << std::endl
	   << "Max. transaction generation time: " << maxTxGenTime << " sec" << std::endl
	   << "Min. transaction generation count: " << minTxGenCount << std::endl
	   << "Max. transaction generation count: " << maxTxGenCount << std::endl
	   << "========================================================" << std::endl;

	progressOutput << ss.str();
	std::cout << ss.str();

	// Create headers in output csv files
	dataOutput << "TransactionID,Fee,BlockID,Depth,MinerID" << std::endl;

	if (this->mpPrintData) {
		mempoolOutput << "MinerID,Progress,MempoolSize" << std::endl;
	}

	// Print simulation params info to metadata file
	metadataOutput << "name=" << configFilename << "_" << simRunIdString << std::endl
	               << "cfg_path=" << configFullPath << std::endl
	               << "blocks=" << blocks << std::endl
	               << "seed=" << seed << std::endl
	               << "block_size=" << blockSize << std::endl
	               << "mempool_capacity=" << mpCapacity << std::endl
	               << "malicious_miners=" << maliciousMinersCount << std::endl
	               << "honest_miners=" << honestMinersCount << std::endl
	               << "malicious_power=" << std::fixed << std::setprecision(5) << maliciousMinersPower << std::endl
	               << "honest_power=" << std::fixed << std::setprecision(5) << honestMinersPower << std::endl;
}

void Simulation::startProgress() {
	time_t currTime = time(nullptr);
	char timeStr[26];
	struct tm *tmInfo;

	tmInfo = localtime(&currTime);
	strftime(timeStr, 26, "%m/%d/%Y %H:%M:%S", tmInfo);

	// Output progress to file and stdout
	std::stringstream ss;
	ss << "[" << timeStr << "]\t" << progress << "%" << std::endl;

	progressOutput << ss.str();
	std::cout << ss.str();

	simStartTime = currTime;
	lastProgressTime = currTime;
}

void Simulation::logProgress(uint32_t blockId) {
	time_t currTime = time(nullptr);
	char timeStr[26];
	struct tm *tmInfo;

	tmInfo = localtime(&currTime);
	strftime(timeStr, 26, "%m/%d/%Y %H:%M:%S", tmInfo);

	// Output progress to file and stdout
	std::stringstream ss;

	ss << "[" << timeStr << "]\t" << progress << "%\t" << "Block " << blockId
	   << "\tETA: ";

	// Get time difference from last progress and count time estimation for rest of the simulation
	long timeDiff = long(difftime(currTime, lastProgressTime)) * (100 - progress);

	logTimeInterval(timeDiff, ss);

	if (honestMinersCount > 0) {
		ss << "\t| Honest miner[" << firstHonestMinerIndex << "] - "
		   << std::fixed << std::setprecision(2)
		   << double(miners[firstHonestMinerIndex].getMempoolFullness()) / mpCapacity * 100 << "%";
	}

	if (maliciousMinersCount > 0) {
		ss << "\t| Malicious miner[" << firstMaliciousMinerIndex << "] - "
		   << std::fixed << std::setprecision(2)
		   << double(miners[firstMaliciousMinerIndex].getMempoolFullness()) / mpCapacity * 100 << "%";
	}

	ss << std::endl;

	progressOutput << ss.str();
	std::cout << ss.str();

	lastProgressTime = currTime;
}

void Simulation::logData(uint64_t id, uint32_t fee, uint32_t blockId, uint32_t depth, uint32_t minerId) {
	dataOutput << id << "," << fee << "," << blockId << "," << depth << "," << minerId << std::endl;
}

void Simulation::logMempoolDataOfAllMiners() {
	if (this->mpPrintData) {
		for (Miner &miner: miners) {
			mempoolOutput << miner.getMinerId() << "," << progress << "," << miner.getMempoolFullness()
			              << std::endl;
		}
	}
}

void Simulation::stopGenerateTransactions() {
	stopGenerateTransactionsFlag = true;
}

void Simulation::finishSimulation() {
	time_t currTime = time(nullptr);
	long timeDiff = long(difftime(currTime, simStartTime));

	std::stringstream ss;

	ss << "Simulation finished. Duration: ";
	logTimeInterval(timeDiff, ss);
	ss << std::endl;

	progressOutput << ss.str();
	std::cout << ss.str();
}

void Simulation::logTimeInterval(long timeDiff, std::stringstream &ss) {
	// Log time interval in human readable format without newline at the end

	if (timeDiff < 60) {
		// seconds
		ss << timeDiff << "s";
	}
	else if (timeDiff >= 60 && timeDiff < 3600) {
		// minutes
		ss << timeDiff / 60 << "m:" << timeDiff % 60 << "s";
	}
	else if (timeDiff >= 3600 && timeDiff < 86400) {
		// hours
		ss << timeDiff / 3600 << "h:" << timeDiff % 3600 / 60 << "m:" << timeDiff % 3600 % 60 << "s";
	}
	else {
		// days
		long daysCount = timeDiff / 86400;
		ss << daysCount;

		if (daysCount == 1) {
			ss << " day, ";
		}
		else {
			ss << " days, ";
		}

		ss << timeDiff % 86400 / 3600 << "h:" << timeDiff % 86400 % 3600 / 60
		   << "m:" << timeDiff % 86400 % 3600 % 60 << "s";
	}
}

void Simulation::errorOutOfTxsExit(Miner &miner) {
	std::stringstream ss;

	ss << "========================================================" << std::endl
	   << "Simulation error, taking snapshot of miners mempools:" << std::endl
	   << "=========================== Start of snapshot ===========================" << std::endl
	   << "MinerID\tMempoolFullness" << std::endl;
	for (Miner &searched_miner: miners) {
		ss << searched_miner.getMinerId() << "\t" << searched_miner.getMempoolFullness() << std::endl;
	}
	ss << "=========================== Start of snapshot ===========================" << std::endl
	   << "Miner[" << miner.getMinerId() << "] was chosen to generate block but has run of out of transactions"
	   << std::endl
	   << "Miner[" << miner.getMinerId() << "] - " << (miner.getType() == HONEST ? "Honest" : "Malicious") << " with "
	   << miner.getMiningPower() * 100 << "% mining power" << std::endl;

	progressOutput << ss.str();
	std::cout << ss.str();

	std::exit(EXIT_FAILURE);
}

const std::string &Simulation::getConfigPath() const {
	return configPath;
}

int32_t Simulation::getSeed() const {
	return seed;
}

std::mt19937 &Simulation::getRandomGen() {
	return randomGen;
}

uint32_t Simulation::getMpCapacity() const {
	return mpCapacity;
}

uint32_t Simulation::getMaxTxGenCount() const {
	return maxTxGenCount;
}

uint32_t Simulation::getMinTxGenCount() const {
	return minTxGenCount;
}

uint32_t Simulation::getMaxTxGenTime() const {
	return maxTxGenTime;
}

uint32_t Simulation::getMinTxGenTime() const {
	return minTxGenTime;
}

uint32_t Simulation::getBlockSize() const {
	return blockSize;
}

uint32_t Simulation::getBlockCount() const {
	return blocks;
}

uint32_t Simulation::getLambda() const {
	return lambda;
}

bool Simulation::honestRandomRemoveEnabled() const {
	return honestRandomRemove;
}

bool Simulation::mpPrintDataEnabled() const {
	return mpPrintData;
}

uint32_t Simulation::getProgress() const {
	return progress;
}

void Simulation::incrementProgress() {
	progress++;
}

CScheduler &Simulation::getScheduler() {
	return scheduler;
}
