/**
 * @file ArgParser.cpp
 * @brief Parse and validate program arguments
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @date 2021 - 2022
 */

#include "ArgParser.h"

Simulation ArgParser::createSimulation(int argc, char *argv[]) {
	const option long_opts[] = {
			{"help",                 no_argument,       nullptr, OPT_HELP},
			{"config",               required_argument, nullptr, OPT_CONFIG},
			{"seed",                 required_argument, nullptr, OPT_SEED},
			{"mp_capacity",          required_argument, nullptr, OPT_MP_CAPACITY},
			{"max_tx_gen_count",     required_argument, nullptr, OPT_MAX_TX_GEN_COUNT},
			{"min_tx_gen_count",     required_argument, nullptr, OPT_MIN_TX_GEN_COUNT},
			{"max_tx_gen_time",      required_argument, nullptr, OPT_MAX_TX_GEN_TIME},
			{"min_tx_gen_time",      required_argument, nullptr, OPT_MIN_TX_GEN_TIME},
			{"block_size",           required_argument, nullptr, OPT_BLOCK_SIZE},
			{"blocks",               required_argument, nullptr, OPT_BLOCKS},
			{"lambda",               required_argument, nullptr, OPT_LAMBDA},
			{"init_tx_count",        required_argument, nullptr, OPT_INIT_TX_COUNT},
			{"honest_random_remove", no_argument,       nullptr, OPT_HONEST_RAND_REMOVE},
			{"mp_print_data",        no_argument,       nullptr, OPT_MP_PRINT_DATA},
			{nullptr,                no_argument,       nullptr, OPT_INVALID}
	};

	Simulation simulation;

	while (true) {
		const int option = getopt_long_only(argc, argv, "", long_opts, nullptr);

		if (option == -1) {
			break;
		}

		switch (option) {
			case OPT_CONFIG:
				simulation.configPath = optarg;
				break;
			case OPT_SEED:
				try {
					simulation.seed = std::stoi(optarg);
				} catch (std::exception &e) {
					this->errorExit("Invalid seed argument");
				}
				break;
			case OPT_MP_CAPACITY:
				try {
					simulation.mpCapacity = std::stoul(optarg);
				}
				catch (std::exception &e) {
					this->errorExit("Invalid mempool capacity argument");
				}
				break;
			case OPT_MAX_TX_GEN_COUNT:
				try {
					simulation.maxTxGenCount = std::stoul(optarg);
				}
				catch (std::exception &e) {
					this->errorExit("Invalid maximum transaction generation count argument");
				}
				break;
			case OPT_MIN_TX_GEN_COUNT:
				try {
					simulation.minTxGenCount = std::stoul(optarg);
				}
				catch (std::exception &e) {
					this->errorExit("Invalid minimum transaction generation count argument");
				}
				break;
			case OPT_MAX_TX_GEN_TIME:
				try {
					simulation.maxTxGenTime = std::stoul(optarg);
				}
				catch (std::exception &e) {
					this->errorExit("Invalid maximum transaction generation time argument");
				}
				break;
			case OPT_MIN_TX_GEN_TIME:
				try {
					simulation.minTxGenTime = std::stoul(optarg);
				}
				catch (std::exception &e) {
					this->errorExit("Invalid minimum transaction generation time argument");
				}
				break;
			case OPT_BLOCK_SIZE:
				try {
					simulation.blockSize = std::stoul(optarg);
				}
				catch (std::exception &e) {
					this->errorExit("Invalid block size argument");
				}
				break;
			case OPT_BLOCKS:
				try {
					simulation.blocks = std::stoul(optarg);
				}
				catch (std::exception &e) {
					this->errorExit("Invalid block count argument");
				}
				break;
			case OPT_LAMBDA:
				try {
					simulation.lambda = std::stoul(optarg);
				}
				catch (std::exception &e) {
					this->errorExit("Invalid lambda argument");
				}
				break;
			case OPT_INIT_TX_COUNT:
				try {
					simulation.initTxCount = std::stoul(optarg);
				}
				catch (std::exception &e) {
					this->errorExit("Invalid initial transaction count argument");
				}
				break;
			case OPT_HONEST_RAND_REMOVE:
				simulation.honestRandomRemove = true;
				break;
			case OPT_MP_PRINT_DATA:
				simulation.mpPrintData = true;
				break;
			case OPT_HELP:
				this->printHelp();
				std::exit(EXIT_SUCCESS);
			case OPT_INVALID:
			case '?':
			default:
				this->printHelp();
				std::exit(EXIT_FAILURE);
		}
	}

	if (simulation.blocks == 0) {
		this->errorExit("Invalid number of simulated blocks; it must be greater than 0");
	}

	if (simulation.blockSize == 0) {
		this->errorExit("Invalid block size; it must be greater than 0");
	}

	if (simulation.mpCapacity == 0) {
		this->errorExit("Invalid mempool capacity; it must be greater than 0");
	}

	if (simulation.lambda == 0) {
		this->errorExit("Invalid lambda; it must be greater than 0");
	}

	if (simulation.minTxGenCount > simulation.maxTxGenCount) {
		this->errorExit("Invalid transaction generation count (min must be less or equal than max)");
	}

	if (simulation.minTxGenTime > simulation.maxTxGenTime) {
		this->errorExit("Invalid transaction generation time (min must be less or equal than max)");
	}

	return simulation;
}

void ArgParser::printHelp() {
	std::cout << "DAG-based consensus simulator, Copyright (C) BUT Security@FIT, 2021 - 2022" << std::endl
	          << "Program options:" << std::endl
	          << "  --help                      show this help message" << std::endl
	          << "  --config arg                input configuration file" << std::endl
	          << "  --seed arg                  seed for random number generator" << std::endl
	          << "  --blocks arg                number of blocks to simulate" << std::endl
	          << "  --block_size arg            number of transactions in block" << std::endl
	          << "  --mp_capacity arg           mempool capacity for each miner" << std::endl
	          << "  --init_tx_count arg         initial transaction count to generate on start" << std::endl
	          << "  --max_tx_gen_count arg      max number of transactions in single generation" << std::endl
	          << "  --min_tx_gen_count arg      min number of transactions in single generation" << std::endl
	          << "  --max_tx_gen_time arg       max seconds of simulation time to generate new transaction "
	          << std::endl
	          << "  --min_tx_gen_time arg       min seconds of simulation time to generate new transaction" << std::endl
	          << "  --lambda arg                block creation rate in seconds" << std::endl
	          << "  --honest_random_remove      honest miners remove transactions randomly on full mempool" << std::endl
	          << "  --mp_print_data             output mempool stats of all miners during simulation" << std::endl
	          << std::endl << "Configuration file options:" << std::endl
	          << "  --description <text>" << std::endl
	          << "  --miner <relative_power> <behavior>" << std::endl
	          << "  --biconnect <miner1> <miner2> <block_prop_delay>" << std::endl;
}

void ArgParser::errorExit(const std::string &msg) {
	std::cerr << msg << std::endl;
	std::exit(EXIT_FAILURE);
}
