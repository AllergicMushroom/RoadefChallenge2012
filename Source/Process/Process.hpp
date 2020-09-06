#pragma once

#include "Core.hpp"
#include "Resource/Resource.hpp"

#include <vector>

class Process
{
public:
	Process() = default;
	Process(int64 service, int64 moveCost, const std::vector<int64>& requirements) : mService(service), mMoveCost(moveCost), mRequirements(requirements) {}

	inline int64 getService() const { return mService; }
	inline int64 getMoveCost() const { return mMoveCost; }
	inline int64 getResourceRequirement(ResourceID resourceID) const { return mRequirements[resourceID]; }

private:
	ServiceID mService;

	int64 mMoveCost;

	std::vector<int64> mRequirements;
};