#pragma once

#include <unordered_map>

class Machine
{
public:
	Machine() = default;
	Machine(int64 location, int64 neighbourhood, const std::vector<int64>& capacities, const std::vector<int64>& safetyLimits, const std::vector<int64>& moveCosts)
		: mLocation(location), mNeighbourhood(neighbourhood), mCapacities(capacities), mSafetyLimits(safetyLimits), mMoveCosts(moveCosts) {}

	inline int64 getCapacity(ResourceID resourceID)	const { return mCapacities[resourceID]; }
	inline int64 getSafetyLimit(ResourceID resourceID) const { return mSafetyLimits[resourceID]; }
	inline int64 getMoveCost(MachineID machineID) const { return mMoveCosts[machineID]; }

	inline int64 getLocation() const { return mLocation; }
	inline int64 getNeighbourhood() const { return mNeighbourhood; }

private:
	int64 mLocation;
	int64 mNeighbourhood;

	std::vector<int64> mCapacities;
	std::vector<int64> mSafetyLimits;

	std::vector<int64> mMoveCosts;
};