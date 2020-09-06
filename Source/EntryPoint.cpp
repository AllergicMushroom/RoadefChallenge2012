#include "Data/Data.hpp"
#include "Log/Log.hpp"
#include "Checker/FullChecker.hpp"
#include "Solver/Solver.hpp"

int main(int argc, char **argv)
{
	// Initialise loggers
	Log::init();

    // Check arguments
	if (argc != 5)
	{
		APP_INFO("Usage for solver: ./ReallocationChallenge solve <instance filepath> <initial assignment filepath> <output file filepath>");
		APP_INFO("Usage for checker: ./ReallocationChallenge check <instance filepath> <initial assignment filepath> <new assignment filepath>");
		return 0;
	}

	if (strcmp(argv[1], "solve") == 0)
	{
		std::shared_ptr<Data> instance = std::shared_ptr<Data>(new Data(argv[2], argv[3]));

		std::unique_ptr<Solver> solver = std::unique_ptr<Solver>(new Solver());
		solver->solveInstance(instance);

		instance->saveNewSolutionToFile(argv[4]);
	}
	else if (strcmp(argv[1], "check") == 0)
	{
		std::shared_ptr<Data> instance = std::shared_ptr<Data>(new Data(argv[2], argv[3]));
		instance->attachSolution(argv[4]);

		APP_INFO("Checking solution...");

		std::unique_ptr<FullChecker> solutionChecker = std::unique_ptr<FullChecker>(new FullChecker(instance));
		const CheckerOutput checkerOutput = solutionChecker->checkSolution(instance->getSolution());

		if (checkerOutput.isValid)
			APP_INFO("Solution is valid.");
		else
			APP_INFO("Solution is unvalid.");

		APP_INFO("Its cost is {0}.", checkerOutput.costs.totalCost);
	}

    return 0;
}