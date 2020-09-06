#pragma once

#include "Data/Data.hpp"
#include "Checker/MicroChecker.hpp"
#include "Checker/FullChecker.hpp"
#include "Solver/Swap.hpp"

#include <chrono>
#include <memory>

#define BIT(x) (1 << x)

enum SwapFlag : int64
{
	None = BIT(0),
	IntraService = BIT(1)
};

class Solver
{
public:
	Solver() = default;

	void solveInstance(const std::shared_ptr<Data>& data);

private:
	bool isSwapValid(const Swap& swap, const int flags = SwapFlag::None);
	int64 getSwapProfit(const Swap& swap);
	void applySwap(const Swap& swap);

	void swapProcessesIntraServices(const std::chrono::steady_clock::time_point& startTime);
	void swapProcessesBruteForceAsBestFit(const std::chrono::steady_clock::time_point& startTime);

	std::vector<ServiceID> getDependingServices(ServiceID serviceID);

private:
	std::shared_ptr<Data> mData;
	std::shared_ptr<MicroChecker> mChecker;
	std::shared_ptr<FullChecker> mFullChecker;

	MachinesResourcesUsage mMachinesResourcesUsage;
	MachinesProcessesIDs mMachinesProcesses;

	ServicesLocationsSpreads mServicesLocationsSpread;
	std::vector<int64> mServicesSpreads;
	std::vector<int64> mServicesCost;

	Solution mSolution;
};