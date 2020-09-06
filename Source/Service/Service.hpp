#pragma once

#include "Core.hpp"

#include <vector>

class Service
{
public:
	Service() = default;
	Service(int64 spreadMin, const std::vector<int64>& dependencies) : mSpreadMin(spreadMin), mProcesses(0), mDependencies(dependencies) {}

	inline const std::vector<int64>& getDependencies() const { return mDependencies; }
	inline int64 getSpreadMin() const { return mSpreadMin; }

	inline const std::vector<ProcessID>& getProcessIDs() const	{ return mProcesses; }
	inline void	addProcessID(ProcessID processID) { mProcesses.push_back(processID); }

private:
	int64 mSpreadMin = 0;

	std::vector<ProcessID> mProcesses;
	std::vector<ServiceID> mDependencies;
};