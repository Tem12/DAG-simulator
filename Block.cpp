/**
 * @file Block.cpp
 * @brief Contains a definition of a block structure
 * @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
 * @author Martin Peresini <iperesini@fit.vut.cz>
 * @date 2021 - 2022
 */

#include "Block.h"

Block::Block(uint32_t _id, uint32_t _depth, uint32_t blockSize): id(_id), depth(_depth) {
	transactions.reserve(blockSize);
}
