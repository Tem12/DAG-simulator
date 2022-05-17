/**
 * @file Block.h
 * @brief Contains a definition of a block structure
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @date 2021 - 2022
 */

#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <cstdint>

class Transaction {
public:
	uint64_t txId;
	uint32_t fee;
};

class Block {
public:
	uint32_t id;
	uint32_t depth;
	std::vector<Transaction> transactions;

	/**
	 *
	 * @param _id block unique id
	 * @param _depth blockchain depth (height)
	 * @param blockSize number of transactions in the block
	 */
	Block(uint32_t _id, uint32_t _depth, uint32_t blockSize);
};

#endif //BLOCK_H
