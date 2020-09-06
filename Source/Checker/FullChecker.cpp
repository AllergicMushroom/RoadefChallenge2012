#include "FullChecker.hpp"

#include "Log/Log.hpp"

#include <future>

const CheckerOutput FullChecker::checkSolution(const Solution& solution)
{
	MachinesResourcesUsage machinesResourcesUsage = mData->calculateMachinesResourcesUsage(solution);

	std::future<bool> capacityCRespected		= std::async(&FullChecker::checkCapacityConstraints, this, machinesResourcesUsage);
	std::future<bool> conflictCRespected		= std::async(&FullChecker::checkConflictConstraints, this, solution);
	std::future<bool> spreadCRespected			= std::async(&FullChecker::checkSpreadConstraints, this, solution);
	std::future<bool> dependencyCRespected		= std::async(&FullChecker::checkDependencyConstraints, this, solution);
	std::future<bool> transientUsageCRespected	= std::async(&FullChecker::checkTransientUsageConstraints, this, solution, machinesResourcesUsage);

	bool isValid = true;
	if (!capacityCRespected.get() || !conflictCRespected.get() || !spreadCRespected.get() || !dependencyCRespected.get() || !transientUsageCRespected.get())
		isValid = false;

	CheckerOutput output;
	output.isValid = isValid;

	Costs solutionCosts = calculateSolutionCosts(solution, machinesResourcesUsage);
	output.costs = solutionCosts;

	return output;
}

const Costs FullChecker::calculateSolutionCosts(const Solution& solution, const MachinesResourcesUsage& machinesResourcesUsage)
{
	std::future<int64> loadCost			= std::async(&FullChecker::calculateLoadCost, this, machinesResourcesUsage);
	std::future<int64> balanceCost		= std::async(&FullChecker::calculateBalanceCost, this, machinesResourcesUsage);
	std::future<int64> processMoveCost	= std::async(&FullChecker::calculateProcessMoveCost, this, solution);
	std::future<int64> serviceMoveCost	= std::async(&FullChecker::calculateServiceMoveCost, this, solution);
	std::future<int64> machineMoveCost	= std::async(&FullChecker::calculateMachineMoveCost, this, solution);

	Costs costs;

	costs.loadCost			= loadCost.get();
	costs.balanceCost		= balanceCost.get();
	costs.processMoveCost	= processMoveCost.get();
	costs.serviceMoveCost	= serviceMoveCost.get();
	costs.machineMoveCost	= machineMoveCost.get();

	costs.totalCost = costs.loadCost + costs.balanceCost + costs.processMoveCost + costs.serviceMoveCost + costs.machineMoveCost;

	return costs;
}

bool FullChecker::checkCapacityConstraints(const MachinesResourcesUsage& machinesResourcesUsage)
{
	bool areValid = true;

	for (MachineID machineID = 0; machineID < mData->getNbMachines(); ++machineID)
	{
		if (!mMicroChecker->checkMachineCapacityConstraints(machineID, machinesResourcesUsage[machineID]))
		{
			APP_WARN("Capacity constraint violated: Machine #{0}", machineID);
			areValid = false;
		}
	}

	return areValid;
}

bool FullChecker::checkConflictConstraints(const Solution& solution)
{
	bool areValid = true;

	for (ServiceID serviceID = 0; serviceID < mData->getNbServices(); ++serviceID)
	{
		if (!mMicroChecker->checkServiceConflictConstraints(solution, serviceID))
		{
			APP_WARN("Conflict constraint violated: Service #{0}", serviceID);
			areValid = false;
		}
	}

	return areValid;
}

bool FullChecker::checkSpreadConstraints(const Solution& solution)
{
	bool areValid = true;

	for (ServiceID serviceID = 0; serviceID < mData->getNbServices(); ++serviceID)
	{
		if (!mMicroChecker->checkServiceSpreadConstraints(solution, serviceID))
		{
			APP_WARN("Spread constraint violated: Service #{0}", serviceID);
			areValid = false;
		}
	}

	return areValid;
}

bool FullChecker::checkDependencyConstraints(const Solution& solution)
{
	for (ServiceID serviceID = 0; serviceID < mData->getNbServices(); ++serviceID)
	{
		if (!mMicroChecker->checkServiceDependencyConstraints(solution, serviceID))
		{
			APP_WARN("Dependency constraints violated");
			return false;
		}
	}

	return true;
}

bool FullChecker::checkTransientUsageConstraints(const Solution& solution, const MachinesResourcesUsage& machinesResourcesUsage)
{
	bool areValid = true;

	for (MachineID machineID = 0; machineID < mData->getNbMachines(); ++machineID)
	{
		if (!mMicroChecker->checkMachineTransientResourcesConstraints(solution, machineID, machinesResourcesUsage[machineID]))
		{
			APP_WARN("Transient usage constraint violated: Machine #{0}", machineID);
			areValid = false;
		}
	}

	return areValid;
}

int64 FullChecker::calculateLoadCost(const MachinesResourcesUsage& machinesResourcesUsage)
{
	int64 loadCost = 0;

	for (ResourceID resourceID = 0; resourceID < mData->getNbResources(); ++resourceID)
	{
		int64 resourceLoadCost = 0;
		for (MachineID machineID = 0; machineID < mData->getNbMachines(); ++machineID)
		{
			int64_t iCost = static_cast<int64_t>(machinesResourcesUsage[machineID][resourceID]) - static_cast<int64_t>(mData->getResourceSafetyLimit(machineID, resourceID));
			int64 cost = 0 < iCost ? static_cast<int64>(iCost) : 0;
			resourceLoadCost += cost;
		}

		resourceLoadCost *= mData->getResourceLoadCostWeight(resourceID);
		loadCost += resourceLoadCost;
	}
	
	return loadCost;
}

int64 FullChecker::calculateBalanceCost(const MachinesResourcesUsage& machinesResourcesUsage)
{
	int64 balanceCost = 0;
	for (BalanceObjectiveID balanceObjectiveID = 0; balanceObjectiveID < mData->getNbBalanceObjectives(); ++balanceObjectiveID)
	{
		for (MachineID machineID = 0; machineID < mData->getNbMachines(); ++machineID)
		{
			ResourceID firstResourceID = mData->getBalanceObjectiveFirstResource(balanceObjectiveID);
			ResourceID secondResourceID = mData->getBalanceObjectiveSecondResource(balanceObjectiveID);
			int64 targetRatio = mData->getBalanceObjectiveTargetRatio(balanceObjectiveID);

			int64 A1 = targetRatio * (mData->getResourceCapacity(machineID, firstResourceID) - machinesResourcesUsage[machineID][firstResourceID]);
			int64 A2 = mData->getResourceCapacity(machineID, secondResourceID) - machinesResourcesUsage[machineID][secondResourceID];

			int64_t iCost = static_cast<int64_t>(A1) - static_cast<int64_t>(A2);
			int64 unweightedCost = 0 < iCost? static_cast<int64>(iCost) : 0;
			balanceCost += mData->getBalanceObjectiveCostWeight(balanceObjectiveID) * unweightedCost;
		}
	}

	return balanceCost;
}

int64 FullChecker::calculateProcessMoveCost(const Solution& solution)
{
	int64 processMoveCost = 0;
	for (ProcessID processID = 0; processID < mData->getNbProcesses(); ++processID)
	{
		if (mData->getProcessInitialAssignment(processID) != solution[processID])
			processMoveCost += mData->getProcessMoveCost(processID);
	}

	processMoveCost *= mData->getPMCWeight();

	return processMoveCost;
}

int64 FullChecker::calculateServiceMoveCost(const Solution& solution)
{
	int64 serviceMoveCost = 0;
	for (ServiceID serviceID = 0; serviceID < mData->getNbServices(); ++serviceID)
	{
		const auto& serviceProcessesIDs = mData->getServiceProcessesIDs(serviceID);
		int64 movedProcessesCount = 0;
		for (const auto& processID : serviceProcessesIDs)
		{
			if (mData->getProcessInitialAssignment(processID) != solution[processID])
				++movedProcessesCount;
		}

		serviceMoveCost = serviceMoveCost < movedProcessesCount ? movedProcessesCount : serviceMoveCost;
	}

	serviceMoveCost *= mData->getSMCWeight();

	return serviceMoveCost;
}

int64 FullChecker::calculateMachineMoveCost(const Solution& solution)
{
	int64 machineMoveCost = 0;
	for (ProcessID processID = 0; processID < mData->getNbProcesses(); ++processID)
	{
		const MachineID initialMachineID = mData->getProcessInitialAssignment(processID);
		const MachineID newMachineID = solution[processID];

		machineMoveCost += mData->getMachineMoveCost(initialMachineID, newMachineID);
	}

	machineMoveCost *= mData->getMMCWeight();

	return machineMoveCost;
}