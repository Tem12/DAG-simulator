CFLAGS=-Wextra -Wall -pedantic -O2

# Please note, that for GNU implementation prior to 9.1 requires linking
# with -lstdc++fs and LLVM implementation prior to LLVM 9.0 requires linking with -lc++fs.
# Source: https://en.cppreference.com/w/cpp/filesystem
#
# This is by default disabled:
# LIBS=-lstdc++fs

all:
	c++ --std=c++17 $(CFLAGS) -o dag-simulator main.cpp ArgParser.cpp Block.cpp ConfigParser.cpp Mempool.cpp Miner.cpp Peer.cpp Scheduler.cpp Simulation.cpp $(LIBS)

doc:
	doxygen doxygen.cfg

clean:
	rm -f dag-simulator
