#pragma once

#include "Core.hpp"

#include "BalanceObjective/BalanceObjective.hpp"
#include "Location/Location.hpp"
#include "Machine/Machine.hpp"
#include "Process/Process.hpp"
#include "Resource/Resource.hpp"
#include "Service/Service.hpp"

#include <string>
#include <unordered_map>

class Data
{
public:
	Data() = delete;
	Data(const std::string& instancePath, const std::string& initialSolutionPath);

	void attachSolution(const std::string& solutionPath);
	void attachSolution(const std::vector<MachineID>& solution);

	void saveNewSolutionToFile(const std::string& filepath);

	inline int64 getNbProcesses() const { return mProcesses.size(); }
	inline int64 getNbMachines() const { return mMachines.size(); }
	inline int64 getNbResources() const { return mResources.size(); }
	inline int64 getNbServices() const { return mServices.size(); }
	inline int64 getNbBalanceObjectives() const { return mBalanceObjectives.size(); }
	inline int64 getNbLocations() const { return mNbLocations; }
	inline int64 getNbNeighbourhoods() const { return mNbNeighbourhoods; }

	inline MachineID getProcessInitialAssignment(ProcessID processID) const { return mInitialSolution[processID]; }
	inline int64 getProcessMoveCost(ProcessID processID) const { return mProcesses[processID].getMoveCost(); }

	inline int64 getMachineMoveCost(MachineID oldMachineID, MachineID newMachineID)	const { return mMachines[oldMachineID].getMoveCost(newMachineID); }
	inline int64 getMachineLocation(MachineID machineID) const { return mMachines[machineID].getLocation(); }
	inline int64 getMachineNeighbourhood(MachineID machineID) const { return mMachines[machineID].getNeighbourhood(); }

	inline const std::vector<ResourceID>& getTransientResourcesIDs() const { return mTransientResourcesIDs; }
	inline int64 getResourceRequirement(ProcessID processID, ResourceID resourceID) const { return mProcesses[processID].getResourceRequirement(resourceID); }
	inline int64 getResourceSafetyLimit(MachineID machineID, ResourceID resourceID)	const { return mMachines[machineID].getSafetyLimit(resourceID); }
	inline int64 getResourceCapacity(MachineID machineID, ResourceID resourceID) const { return mMachines[machineID].getCapacity(resourceID); }
	inline int64 getResourceLoadCostWeight(ResourceID resourceID) const { return mResources[resourceID].getLoadCostWeight(); }

	inline ServiceID getServiceID(ProcessID processID) const { return mProcesses[processID].getService(); }
	inline const std::vector<ProcessID>& getServiceProcessesIDs(ServiceID serviceID) const { return mServices[serviceID].getProcessIDs(); }
	inline const std::vector<ServiceID>& getServiceDependencies(ServiceID serviceID) const { return mServices[serviceID].getDependencies(); }
	inline int64 getServiceSpreadMin(ServiceID serviceID) const { return mServices[serviceID].getSpreadMin(); }

	inline ResourceID getBalanceObjectiveFirstResource(BalanceObjectiveID balanceObjectiveID) const { return mBalanceObjectives[balanceObjectiveID].getFirstResourceID(); }
	inline ResourceID getBalanceObjectiveSecondResource(BalanceObjectiveID balanceObjectiveID) const { return mBalanceObjectives[balanceObjectiveID].getSecondResourceID(); }
	inline int64 getBalanceObjectiveTargetRatio(BalanceObjectiveID balanceObjectiveID) const { return mBalanceObjectives[balanceObjectiveID].getTargetRatio(); }
	inline int64 getBalanceObjectiveCostWeight(BalanceObjectiveID balanceObjectiveID) const { return mBalanceObjectives[balanceObjectiveID].getBalanceCostWeight(); }
	
	inline int64 getPMCWeight() const { return mPMCWeight; }
	inline int64 getSMCWeight() const { return mSMCWeight; }
	inline int64 getMMCWeight() const { return mMMCWeight; }
	
	inline const Solution& getInitialSolution()	const { return mInitialSolution; }
	inline const Solution& getSolution() const { return mSolution; }

	MachinesResourcesUsage calculateMachinesResourcesUsage(const Solution& solution) const;

	const std::vector<ProcessID>& getMachineInitialProcessesIDs(MachineID machineID) const { return mMachinesInitialProcesses[machineID]; }
	MachinesProcessesIDs calculateMachinesProcesses(const Solution& solution) const;

	ServicesLocationsSpreads calculateServicesLocationsSpreads(const Solution& solution) const;

private:
	std::vector<Resource> mResources;
	std::vector<ResourceID> mTransientResourcesIDs;

	std::vector<Process> mProcesses;
	std::vector<Machine> mMachines;
	std::vector<Service> mServices;
	std::vector<BalanceObjective> mBalanceObjectives;

	int64 mNbLocations = 0;
	int64 mNbNeighbourhoods = 0;

	int64 mPMCWeight = 1;
	int64 mSMCWeight = 1;
	int64 mMMCWeight = 1;

	Solution mInitialSolution;
	MachinesProcessesIDs mMachinesInitialProcesses;

	Solution mSolution;
};
