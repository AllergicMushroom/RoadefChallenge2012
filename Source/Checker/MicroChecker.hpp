#pragma once

#include "Core.hpp"

#include "Data/Data.hpp"
#include "Solver/Swap.hpp"

#include <memory>

class MicroChecker
{
public:
	MicroChecker() = delete;
	MicroChecker(const std::shared_ptr<Data>& data) : mData(data) {}

	bool checkMachineCapacityConstraints(MachineID machineID, const std::vector<int64>& machineResourcesUsage);
	bool checkServiceConflictConstraints(const Solution& solution, ServiceID serviceID);
	bool checkServiceSpreadConstraints(const Solution& solution, ServiceID serviceID);
	bool checkServiceDependencyConstraints(const Solution& solution, ServiceID serviceID);
	bool checkMachineTransientResourcesConstraints(const Solution& solution, MachineID machineID, const std::vector<int64>& machineResourcesUsage);

	bool checkSwapConflictConstraints(const Solution& solution, const Swap& swap);

	int64 calculateMachineLoadCost(MachineID machineID, const MachinesResourcesUsage& machinesResourcesUsage);
	int64 calculateMachineBalanceCost(MachineID machineID, const MachinesResourcesUsage& machinesResourcesUsage);

private:
	std::shared_ptr<Data> mData;
};