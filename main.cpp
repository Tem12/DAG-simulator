#include "ArgParser.h"

int main(int argc, char *argv[]) {
	Simulation simulation = ArgParser{}.createSimulation(argc, argv);
	simulation.runSimulation();
	simulation.finishSimulation();
	return EXIT_SUCCESS;
}
