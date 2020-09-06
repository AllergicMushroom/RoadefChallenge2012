#pragma once

#include "Core.hpp"

#include <vector>

class Location
{
public:
	Location() = default;

	inline const std::vector<MachineID>& getMachineIDs() const { return mMachines; }
	inline void addMachineID(unsigned int machineID) { mMachines.push_back(machineID); }

private:
	std::vector<MachineID> mMachines;
};