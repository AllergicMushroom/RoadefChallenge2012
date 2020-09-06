#pragma once

#include "Core.hpp"

#include <array>

class BalanceObjective
{
public:
	BalanceObjective() = default;
	BalanceObjective(std::array<int64, 3> triple, int64 balanceCostWeight) : mTriple(triple), mBalanceCostWeight(balanceCostWeight) {}

	inline ResourceID	getFirstResourceID()	const { return mTriple[0]; }
	inline ResourceID	getSecondResourceID()	const { return mTriple[1]; }
	inline int64		getTargetRatio()		const { return mTriple[2]; }
	inline int64		getBalanceCostWeight()	const { return mBalanceCostWeight; }

private:
	std::array<int64, 3> mTriple; // <resource 1, resource 2, target ratio>

	int64 mBalanceCostWeight = 1;
};