#pragma once

#include "Core.hpp"

#include "Checker/MicroChecker.hpp"
#include "Data/Data.hpp"

#include <memory>

struct Costs
{
	int64 loadCost;
	int64 balanceCost;
	int64 processMoveCost;
	int64 serviceMoveCost;
	int64 machineMoveCost;

	int64 totalCost;
};

struct CheckerOutput
{
	bool isValid;

	Costs costs;
};

class FullChecker
{
public:
	FullChecker() = delete;
	FullChecker(const std::shared_ptr<Data>& data) : mData(data) { mMicroChecker = std::shared_ptr<MicroChecker>(new MicroChecker(data)); }

	const CheckerOutput checkSolution(const Solution& solution);
	const Costs calculateSolutionCosts(const Solution& solution, const MachinesResourcesUsage& machinesResourcesUsage);

	bool checkCapacityConstraints		(const MachinesResourcesUsage& machinesResourcesUsage);
	bool checkConflictConstraints		(const Solution& solution);
	bool checkSpreadConstraints			(const Solution& solution);
	bool checkDependencyConstraints		(const Solution& solution);
	bool checkTransientUsageConstraints	(const Solution& solution, const MachinesResourcesUsage& machinesResourcesUsage);

	int64 calculateLoadCost		(const MachinesResourcesUsage& machinesResourcesUsage);
	int64 calculateBalanceCost		(const MachinesResourcesUsage& machinesResourcesUsage);
	int64 calculateProcessMoveCost	(const Solution& solution);
	int64 calculateServiceMoveCost	(const Solution& solution);
	int64 calculateMachineMoveCost	(const Solution& solution);

private:
	std::shared_ptr<const Data> mData;
	std::shared_ptr<MicroChecker> mMicroChecker;
};
