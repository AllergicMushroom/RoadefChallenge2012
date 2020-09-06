#pragma once

class Resource
{
public:
	Resource() = default;
    Resource(bool isTransient, int64 loadCostWeight) : mIsTransient(isTransient), mLoadCostWeight(loadCostWeight) {}

    inline bool	isTransient() const { return mIsTransient; }
	inline int64 getLoadCostWeight() const { return mLoadCostWeight; }

private:
    bool mIsTransient;

	int64 mLoadCostWeight;
};
