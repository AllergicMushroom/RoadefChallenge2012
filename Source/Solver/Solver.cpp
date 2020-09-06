#include "Solver.hpp"

#include "Checker/FullChecker.hpp"
#include "Log/Log.hpp"

#include <limits>

constexpr unsigned int sTimeOutMin = 30;

bool shouldStopCalculating(const std::chrono::steady_clock::time_point& startTime);

void Solver::solveInstance(const std::shared_ptr<Data>& data)
{
	APP_INFO("Solving...");
	auto startTime = std::chrono::steady_clock::now();
	auto currentTime = startTime;

	mData = data;
	mChecker = std::shared_ptr<MicroChecker>(new MicroChecker(data));
	mFullChecker = std::shared_ptr<FullChecker>(new FullChecker(data));

	mSolution = mData->getInitialSolution();

	mMachinesResourcesUsage = mData->calculateMachinesResourcesUsage(mSolution);
	mMachinesProcesses = mData->calculateMachinesProcesses(mSolution);

	mServicesLocationsSpread = mData->calculateServicesLocationsSpreads(mSolution);
	mServicesCost = std::vector<int64>(mData->getNbServices(), 0);

	mServicesSpreads = std::vector<int64>(mData->getNbServices(), 0);
	for (ServiceID serviceID = 0; serviceID < mData->getNbServices(); ++serviceID)
	{
		for (int64 locationID = 0; locationID < mData->getNbLocations(); ++locationID)
			mServicesSpreads[serviceID] += std::min(1LL, mServicesLocationsSpread[serviceID][locationID]);
	}

	int64 oldCost = mFullChecker->calculateSolutionCosts(mSolution, mMachinesResourcesUsage).totalCost;

	while (!shouldStopCalculating(startTime))
	{
		swapProcessesIntraServices(startTime);

		swapProcessesBruteForceAsBestFit(startTime);
		//break;
		currentTime = std::chrono::steady_clock::now();
	}

	mData->attachSolution(mSolution);
	int64 newCost = mFullChecker->calculateSolutionCosts(mSolution, mMachinesResourcesUsage).totalCost;

	APP_INFO("Solved.");
	APP_INFO("Old solution cost: {0}", oldCost);
	APP_INFO("New solution cost: {0}", newCost);
	APP_INFO("\t which is {0}% of the old one.", static_cast<float>(newCost) / static_cast<float>(oldCost) * 100);
}

bool Solver::isSwapValid(const Swap& swap, const int flags)
{
	const ProcessID& processID1 = swap.processID1;
	const ProcessID& processID2 = swap.processID2;

	const ServiceID& serviceID1 = mData->getServiceID(processID1);
	const ServiceID& serviceID2 = mData->getServiceID(processID2);

	applySwap(swap);

	const MachineID& newMachineID1 = mSolution[processID1];
	const MachineID& newMachineID2 = mSolution[processID2];

	bool loadConstraintRespected = mChecker->checkMachineCapacityConstraints(newMachineID1, mMachinesResourcesUsage[newMachineID1])
								&& mChecker->checkMachineCapacityConstraints(newMachineID2, mMachinesResourcesUsage[newMachineID2]);
	if (!loadConstraintRespected)
	{
		applySwap(swap);
		return false;
	}

	bool conflictConstraintRespected = true;
	if (!(flags & SwapFlag::IntraService))
	{
		conflictConstraintRespected = mChecker->checkSwapConflictConstraints(mSolution, swap);
	}

	if (!conflictConstraintRespected)
	{
		applySwap(swap);
		return false;
	}

	bool spreadConstraintRespected = mServicesSpreads[serviceID1] >= mData->getServiceSpreadMin(serviceID1) && mServicesSpreads[serviceID2] >= mData->getServiceSpreadMin(serviceID2);
	if (!spreadConstraintRespected)
	{
		applySwap(swap);
		return false;
	}

	bool dependencyConstraintsRespected = mFullChecker->checkDependencyConstraints(mSolution);
	if (!dependencyConstraintsRespected)
	{
		applySwap(swap);
		return false;
	}
	/*auto uncheckedServicesIDs = getDependingServices(serviceID1);
	for (ServiceID serviceID : uncheckedServicesIDs)
	{
		bool dependencyConstraintsRespected = mChecker->checkServiceDependencyConstraints(mSolution, serviceID);
		if (!dependencyConstraintsRespected)
		{
			applySwap(swap);
			return false;
		}
	}

	uncheckedServicesIDs = getDependingServices(serviceID2);
	for (ServiceID serviceID : uncheckedServicesIDs)
	{
		bool dependencyConstraintsRespected = mChecker->checkServiceDependencyConstraints(mSolution, serviceID);
		if (!dependencyConstraintsRespected)
		{
			applySwap(swap);
			return false;
		}
	}*/

	bool transientConstraintsRespected = mChecker->checkMachineTransientResourcesConstraints(mSolution, newMachineID1, mMachinesResourcesUsage[newMachineID1])
									  && mChecker->checkMachineTransientResourcesConstraints(mSolution, newMachineID2, mMachinesResourcesUsage[newMachineID2]);
	if (!transientConstraintsRespected)
	{
		applySwap(swap);
		return false;
	}

	applySwap(swap);

	return true;
}

void Solver::applySwap(const Swap& swap)
{
	const ProcessID& processID1 = swap.processID1;
	const ProcessID& processID2 = swap.processID2;

	const MachineID oldMachineID1 = mSolution[processID1];
	const MachineID oldMachineID2 = mSolution[processID2];

	const ServiceID serviceID1 = mData->getServiceID(processID1);
	const ServiceID serviceID2 = mData->getServiceID(processID2);

	// Swap them
	{
		mSolution[processID1] = oldMachineID2;
		mSolution[processID2] = oldMachineID1;
	}

	// Update auxiliary objects
	{
		auto& machineProcesses1 = mMachinesProcesses[oldMachineID1];
		auto& machineProcesses2 = mMachinesProcesses[oldMachineID2];

		machineProcesses1.erase(std::remove(machineProcesses1.begin(), machineProcesses1.end(), processID1), machineProcesses1.end());
		machineProcesses2.erase(std::remove(machineProcesses2.begin(), machineProcesses2.end(), processID2), machineProcesses2.end());

		machineProcesses1.push_back(processID2);
		machineProcesses2.push_back(processID1);

		for (ResourceID resourceID = 0; resourceID < mData->getNbResources(); ++resourceID)
		{
			mMachinesResourcesUsage[oldMachineID1][resourceID] -= mData->getResourceRequirement(processID1, resourceID);
			mMachinesResourcesUsage[oldMachineID1][resourceID] += mData->getResourceRequirement(processID2, resourceID);

			mMachinesResourcesUsage[oldMachineID2][resourceID] -= mData->getResourceRequirement(processID2, resourceID);
			mMachinesResourcesUsage[oldMachineID2][resourceID] += mData->getResourceRequirement(processID1, resourceID);
		}
	}

	const MachineID& newMachineID1 = oldMachineID2;
	const MachineID& newMachineID2 = oldMachineID1;

	// Spread
	{
		const int64 oldLocationID1 = mData->getMachineLocation(oldMachineID1);
		const int64 oldLocationID2 = mData->getMachineLocation(oldMachineID2);

		const int64& newLocationID1 = oldLocationID2;
		const int64& newLocationID2 = oldLocationID1;

		if (mServicesLocationsSpread[serviceID1][oldLocationID1] == 1)
			--mServicesSpreads[serviceID1];
		--mServicesLocationsSpread[serviceID1][oldLocationID1];

		if (mServicesLocationsSpread[serviceID1][newLocationID1] == 0)
			++mServicesSpreads[serviceID1];
		++mServicesLocationsSpread[serviceID1][newLocationID1];

		if (mServicesLocationsSpread[serviceID2][oldLocationID2] == 1)
			--mServicesSpreads[serviceID2];
		--mServicesLocationsSpread[serviceID2][oldLocationID2];

		if (mServicesLocationsSpread[serviceID2][newLocationID2] == 0)
			++mServicesSpreads[serviceID2];
		++mServicesLocationsSpread[serviceID2][newLocationID2];
	}

	// SMC
	{
		const MachineID initialMachineID1 = mData->getProcessInitialAssignment(processID1);

		/*
		A process is never swapped with another process from the same machine
		so there's no need to check newMachineID1 == initialMachineID1
		*/
		if (oldMachineID1 == initialMachineID1)
			++mServicesCost[serviceID1];
	
		// A process can come back
		if (oldMachineID1 != initialMachineID1 && newMachineID1 == initialMachineID1)
			--mServicesCost[serviceID1];
	
		// Same for the second process
		const MachineID initialMachineID2 = mData->getProcessInitialAssignment(processID2);
		if (oldMachineID2 == initialMachineID2)
			++mServicesCost[serviceID2];
	
		if (oldMachineID2 != initialMachineID2 && newMachineID2 == initialMachineID2)
			--mServicesCost[serviceID2];
	}
}

int64 Solver::getSwapProfit(const Swap& swap)
{
	const ProcessID& processID1 = swap.processID1;
	const ProcessID& processID2 = swap.processID2;

	const MachineID oldMachineID1 = mSolution[processID1];
	const MachineID oldMachineID2 = mSolution[processID2];

	const int64 oldLoadCost = mChecker->calculateMachineLoadCost(oldMachineID1, mMachinesResourcesUsage)
					  + mChecker->calculateMachineLoadCost(oldMachineID2, mMachinesResourcesUsage);
	
	const int64 oldBalanceCost = mChecker->calculateMachineBalanceCost(oldMachineID1, mMachinesResourcesUsage)
						 + mChecker->calculateMachineBalanceCost(oldMachineID2, mMachinesResourcesUsage);

	int64 oldPMC = 0;
	if (oldMachineID1 != mData->getProcessInitialAssignment(processID1))
		oldPMC += mData->getProcessMoveCost(processID1);

	if (oldMachineID2 != mData->getProcessInitialAssignment(processID2))
		oldPMC += mData->getProcessMoveCost(processID2);

	const int64 oldSMC = *std::max_element(mServicesCost.begin(), mServicesCost.end());

	int64 oldMMC = 0;
	if (oldMachineID1 != mData->getProcessInitialAssignment(processID1))
		oldMMC += mData->getMachineMoveCost(mData->getProcessInitialAssignment(processID1), mSolution[processID1]);

	if (oldMachineID2 != mData->getProcessInitialAssignment(processID2))
		oldMMC += mData->getMachineMoveCost(mData->getProcessInitialAssignment(processID2), mSolution[processID2]);

	applySwap(swap);

	const MachineID newMachineID1 = mSolution[processID1];
	const MachineID newMachineID2 = mSolution[processID2];

	const int64 newLoadCost = mChecker->calculateMachineLoadCost(newMachineID1, mMachinesResourcesUsage)
					  + mChecker->calculateMachineLoadCost(newMachineID2, mMachinesResourcesUsage);

	const int64 newBalanceCost = mChecker->calculateMachineBalanceCost(newMachineID1, mMachinesResourcesUsage)
						 + mChecker->calculateMachineBalanceCost(newMachineID2, mMachinesResourcesUsage);

	int64 newPMC = 0;
	if (newMachineID1 != mData->getProcessInitialAssignment(processID1))
		newPMC += mData->getProcessMoveCost(processID1);

	if (newMachineID2 != mData->getProcessInitialAssignment(processID2))
		newPMC += mData->getProcessMoveCost(processID2);

	const int64 newSMC = *std::max_element(mServicesCost.begin(), mServicesCost.end());
	
	int64 newMMC = 0;
	if (newMachineID1 != mData->getProcessInitialAssignment(processID1))
		newMMC += mData->getMachineMoveCost(mData->getProcessInitialAssignment(processID1), mSolution[processID1]);

	if (newMachineID2 != mData->getProcessInitialAssignment(processID2))
		newMMC += mData->getMachineMoveCost(mData->getProcessInitialAssignment(processID2), mSolution[processID2]);

	const int64 loadCostProfit = oldLoadCost - newLoadCost;
	const int64 balanceCostProfit = oldBalanceCost - newBalanceCost;
	const int64 PMCProfit = oldPMC - newPMC;
	const int64 SMCProfit = oldSMC - newSMC;
	const int64 MMCProfit = oldMMC - newMMC;

	applySwap(swap);

	return loadCostProfit + balanceCostProfit + PMCProfit + SMCProfit + MMCProfit;
}

void Solver::swapProcessesIntraServices(const std::chrono::steady_clock::time_point & startTime)
{
	for (ProcessID processID1 = 0; processID1 < mData->getNbProcesses(); ++processID1)
	{
		ServiceID serviceID1 = mData->getServiceID(processID1);
		const auto& serviceProcessesIDs = mData->getServiceProcessesIDs(serviceID1);

		for (const ProcessID& processID2 : serviceProcessesIDs)
		{
			// Because we iterate in ascending order, we only have to check for IDs bigger than processID1
			// In other words, for each swap <x, y>, we have x < y.
			// Because of this, we also don't need to check if <y, x> is already present in the swap list.
			if (processID1 <= processID2)
				continue;

			const Swap swap = { processID1, processID2 };

			if (isSwapValid(swap, SwapFlag::IntraService))
			{
				if (getSwapProfit(swap) > 0)
				{
					applySwap(swap);
				}
			}

			if (shouldStopCalculating(startTime))
				return;
		}
	}
}

void Solver::swapProcessesBruteForceAsBestFit(const std::chrono::steady_clock::time_point& startTime)
{
	for (ProcessID processID1 = 0; processID1 < mData->getNbProcesses(); ++processID1)
	{
		int64 bestProfit = 0;
		ProcessID bestProcessID = INT64_MAX;
		for (ProcessID processID2 = 0; processID2 < mData->getNbProcesses(); ++processID2)
		{
			const Swap swap = { processID1, processID2 };

			if (isSwapValid(swap))
			{
				int64 profit = getSwapProfit(swap);
				if (profit >= bestProfit)
				{
					bestProfit = profit;
					bestProcessID = processID2;
				}
			}

			if (shouldStopCalculating(startTime))
				return;
	}

	if (bestProcessID != INT64_MAX)
		applySwap({ processID1, bestProcessID });
	}
}

std::vector<ServiceID> Solver::getDependingServices(ServiceID serviceID)
{
	std::vector<ServiceID> dependingServices;

	for (ServiceID serviceID2 = 0; serviceID2 < mData->getNbServices(); ++serviceID2)
	{
		const auto& serviceDependencies = mData->getServiceDependencies(serviceID2);

		for (ServiceID dependencyID : serviceDependencies)
		{
			if (dependencyID == serviceID)
			{
				dependingServices.push_back(dependencyID);
			}
		}
	}

	return dependingServices;
}

bool shouldStopCalculating(const std::chrono::steady_clock::time_point& startTime)
{
	constexpr std::chrono::duration stopTime = std::chrono::minutes(sTimeOutMin);
	const auto currentTime = std::chrono::steady_clock::now();

	return std::chrono::duration_cast<std::chrono::minutes>(currentTime - startTime) >= stopTime;

}
