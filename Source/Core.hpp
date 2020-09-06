#pragma once

// -------- BUILD CONFIGURATIONS SETTINGS ---- //

#ifdef MODE_DEBUG
#define ENABLE_ASSERTS

#ifdef PLATFORM_WINDOWS
#include <vld.h>
#endif

#elif defined MODE_DEBUGOPTON
#define ENABLE_ASSERTS

#ifdef PLATFORM_WINDOWS
#include <vld.h>
#endif

#elif defined MODE_RELEASE

#endif

// ------------------------------------------- //

// -------- USEFUL MACROS -------------------- //

#ifdef ENABLE_ASSERTS
#define APP_ASSERT(x, error, ...)												\
	{																			\
		if (!(x))																\
		{																		\
			std::string errorStr = "Assertion failed: " + std::string(error);	\
			APP_ERROR(error, __VA_ARGS__);										\
		}																		\
	}

#else
#define APP_ASSERT(x, error, ...)
#endif

// ------------------------------------------- //

// -------- TYPES ---------------------------- //

typedef long long int64;

typedef int64 MachineID;
typedef int64 ProcessID;
typedef int64 ResourceID;
typedef int64 ServiceID;
typedef int64 BalanceObjectiveID;
typedef int64 LocationID;

#include <vector>
typedef std::vector<std::vector<ProcessID>>	MachinesProcessesIDs;		// [MachineID]
typedef std::vector<std::vector<int64>>		MachinesResourcesUsage;		// [MachineID][ResourceID]
typedef std::vector<std::vector<int64>>		ServicesLocationsSpreads;	// [ServiceID][LocationID]

typedef std::vector<MachineID> Solution;

// ------------------------------------------- //