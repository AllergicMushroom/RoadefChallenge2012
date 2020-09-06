#include "MicroChecker.hpp"

bool MicroChecker::checkMachineCapacityConstraints(MachineID machineID, const std::vector<int64>& machineResourcesUsage)
{
	for (ResourceID resourceID = 0; resourceID < mData->getNbResources(); ++resourceID)
	{
		if (machineResourcesUsage[resourceID] > mData->getResourceCapacity(machineID, resourceID))
			return false;
	}

	return true;
}

bool MicroChecker::checkServiceConflictConstraints(const Solution& solution, ServiceID serviceID)
// TODO: Maybe we can only check the swapped processes
{
	const auto& serviceProcessesIDs = mData->getServiceProcessesIDs(serviceID);

	for (ProcessID processID1 : serviceProcessesIDs)
	{
		for (ProcessID processID2 : serviceProcessesIDs)
		{
			if (processID1 == processID2)
				continue;

			if (solution[processID1] == solution[processID2])
				return false;
		}
	}

	return true;
}

bool MicroChecker::checkServiceSpreadConstraints(const Solution& solution, ServiceID serviceID)
{
	const int64 spreadMin = mData->getServiceSpreadMin(serviceID);
	const auto& serviceProcessesIDs = mData->getServiceProcessesIDs(serviceID);

	int64 distinctLocationsSum = 0;
	for (int64 locationID = 0; locationID < mData->getNbLocations(); ++locationID)
	{
		for (const auto& processID : serviceProcessesIDs)
		{
			if (mData->getMachineLocation(solution[processID]) == locationID)
			{
				distinctLocationsSum += 1;
				break;
			}
		}
	}

	if (distinctLocationsSum < spreadMin)
		return false;

	return true;
}

bool MicroChecker::checkServiceDependencyConstraints(const Solution & solution, ServiceID serviceID)
{
	const auto& serviceDependenciesIDs = mData->getServiceDependencies(serviceID);
	const auto& serviceProcessesIDs = mData->getServiceProcessesIDs(serviceID);

	for (const auto& serviceDependencyID : serviceDependenciesIDs)
	{
		for (const auto& processID1 : serviceProcessesIDs)
		{
			bool dependencyExistsInNH = false;

			const auto& dependencyProcessesIDs = mData->getServiceProcessesIDs(serviceDependencyID);
			for (const auto& processID2 : dependencyProcessesIDs)
			{
				if (mData->getMachineNeighbourhood(solution[processID1]) == mData->getMachineNeighbourhood(solution[processID2]))
				{
					dependencyExistsInNH = true;
					break;
				}
			}

			if (!dependencyExistsInNH)
				return false;
		}
	}

	return true;
}

bool MicroChecker::checkMachineTransientResourcesConstraints(const Solution& solution, MachineID machineID, const std::vector<int64>& machineResourcesUsage)
{
	const auto& machineInitialProcessesIDs = mData->getMachineInitialProcessesIDs(machineID);

	for (const auto& resourceID : mData->getTransientResourcesIDs())
	{
		int64 resourceTransientUsage = 0;
		for (const auto& processID : machineInitialProcessesIDs)
		{
			/*
			If a process is not on assigned to the machine, but it was initially, it means its resources requirements are not counted,
			so we have to count them for transient resources. By doing this, we assert that every process isn't counted twice and isn't counted when not needed.
			*/
			if (solution[processID] != mData->getProcessInitialAssignment(processID))
				resourceTransientUsage += mData->getResourceRequirement(processID, resourceID);
		}

		if (machineResourcesUsage[resourceID] + resourceTransientUsage > mData->getResourceCapacity(machineID, resourceID))
			return false;
	}

	return true;
}

bool MicroChecker::checkSwapConflictConstraints(const Solution& solution, const Swap& swap)
{
	const ProcessID& processID1 = swap.processID1;
	const ProcessID& processID2 = swap.processID2;

	const auto& serviceProcessesIDs1 = mData->getServiceProcessesIDs(mData->getServiceID(processID1));
	const auto& serviceProcessesIDs2 = mData->getServiceProcessesIDs(mData->getServiceID(processID2));

	// First process
	for (ProcessID processID : serviceProcessesIDs1)
	{
		if (processID == processID1)
			continue;

		if (solution[processID] == solution[processID1])
			return false;
	}

	// Second process
	for (ProcessID processID : serviceProcessesIDs2)
	{
		if (processID == processID2)
			continue;

		if (solution[processID] == solution[processID2])
			return false;
	}

	return true;
}

int64 MicroChecker::calculateMachineLoadCost(MachineID machineID, const MachinesResourcesUsage& machinesResourcesUsage)
{
	const int64 nbResources = mData->getNbResources();

	int64 machineLoadCost = 0;
	for (ResourceID resourceID = 0; resourceID < nbResources; ++resourceID)
	{
		const int64 cost = machinesResourcesUsage[machineID][resourceID] - mData->getResourceSafetyLimit(machineID, resourceID);

		int64 resourceLoadCost = 0 < cost ? cost : 0;
		resourceLoadCost *= mData->getResourceLoadCostWeight(resourceID);
		machineLoadCost += resourceLoadCost;
	}

	return machineLoadCost;
}

int64 MicroChecker::calculateMachineBalanceCost(MachineID machineID, const MachinesResourcesUsage& machinesResourcesUsage)
{
	const int64 nbBalanceObjectives = mData->getNbBalanceObjectives();

	int64 machineBalanceCost = 0;
	for (BalanceObjectiveID balanceObjectiveID = 0; balanceObjectiveID < nbBalanceObjectives; ++balanceObjectiveID)
	{
		ResourceID firstResourceID = mData->getBalanceObjectiveFirstResource(balanceObjectiveID);
		ResourceID secondResourceID = mData->getBalanceObjectiveSecondResource(balanceObjectiveID);
		int64 targetRatio = mData->getBalanceObjectiveTargetRatio(balanceObjectiveID);

		int64 A1 = targetRatio * (mData->getResourceCapacity(machineID, firstResourceID) - machinesResourcesUsage[machineID][firstResourceID]);
		int64 A2 = mData->getResourceCapacity(machineID, secondResourceID) - machinesResourcesUsage[machineID][secondResourceID];

		int64 cost = A1 - A2;
		machineBalanceCost += mData->getBalanceObjectiveCostWeight(balanceObjectiveID) * cost;
	}

	return machineBalanceCost;
}
