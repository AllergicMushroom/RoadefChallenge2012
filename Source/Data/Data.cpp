#include "Data.hpp"

#include "Log/Log.hpp"

#include <fstream>
#include <future>

static constexpr const char* eol = "\n";

static constexpr const char* separationToken = " ";

static constexpr int64 maxNbMachines = 5000;
static constexpr int64 maxNbResource = 20;
static constexpr int64 maxNbProcesses = 50000;
static constexpr int64 maxNbServices = 5000;
static constexpr int64 maxNbNeighbourhoods = 1000;
static constexpr int64 maxNbDependencies = 5000;
static constexpr int64 maxNbLocations = 1000;
static constexpr int64 maxNbBalanceObjectives = 10;

static std::string readFileContent(const std::string& filepath);

// Vector of components, which are vectors of lines. Order: Resources, Machines, Services, Processes, Balance objectives, Weights
static const std::vector<std::vector<std::string>> tokenizeFileByComponents(const std::string& str);

static std::string readNextLine(const std::string& str, size_t& currentPosition);
static std::vector<std::string> tokenizeString(const std::string& str, const char* token);

static std::vector<ProcessID> solutionStrToVector(const std::string& solution);

Data::Data(const std::string& instancePath, const std::string& initialSolutionPath)
{
	APP_INFO("Loading data from files...");

	std::string instance = readFileContent(instancePath);
	std::string initialSolutionStr = readFileContent(initialSolutionPath);

	mInitialSolution = solutionStrToVector(initialSolutionStr);
	mSolution = mInitialSolution;

	const auto components = tokenizeFileByComponents(instance);

	// Resources
	auto componentLines = components[0];
	auto loadResources =
		[this, componentLines]()
	{
		std::string line = componentLines[0];
		int64 nbObjects = std::atoi(line.c_str());

		mResources = std::vector<Resource>(nbObjects);
		mTransientResourcesIDs = std::vector<ResourceID>(0);
		for (ResourceID resourceID = 0; resourceID < nbObjects; ++resourceID)
		{
			line = componentLines[1 + resourceID];
			auto parameters = tokenizeString(line, separationToken);

			const bool isTransient = (std::atoi(parameters[0].c_str()) == 1);
			const int64 loadCostWeight = static_cast<int64>(std::atoi(parameters[1].c_str()));

			mResources[resourceID] = Resource(isTransient, loadCostWeight);
			if (isTransient)
				mTransientResourcesIDs.push_back(resourceID);
		}

		APP_INFO("Number of resources retrieved: {0}", nbObjects);
		APP_INFO("Number of transient resources: {0}", mTransientResourcesIDs.size());

		return true;
	};

	std::future<bool> areResourcesLoaded;
	if (componentLines.size() != 0)
		areResourcesLoaded = std::async(loadResources);

	// Machines
	componentLines = components[1];
	auto loadMachines =
		[this, componentLines]()
	{
		std::string line = componentLines[0];
		int64 nbObjects = std::atoi(line.c_str());

		std::vector<int64> registeredLocations;
		std::vector<int64> registeredNeighbourhoods;

		mMachines = std::vector<Machine>(nbObjects);
		for (MachineID machineID = 0; machineID < nbObjects; ++machineID)
		{
			line = componentLines[1 + machineID];
			auto parameters = tokenizeString(line, separationToken);

			const int64 neighbourhood = std::atoi(parameters[0].c_str());
			const bool isNeighbourhoodRegistered = std::find(registeredNeighbourhoods.begin(), registeredNeighbourhoods.end(), neighbourhood) != registeredNeighbourhoods.end();

			if (!isNeighbourhoodRegistered)
				registeredNeighbourhoods.push_back(neighbourhood);

			const int64 location = std::atoi(parameters[1].c_str());
			const bool isLocationRegistered = std::find(registeredLocations.begin(), registeredLocations.end(), location) != registeredLocations.end();

			if (!isLocationRegistered)
				registeredLocations.push_back(location);

			std::vector<int64> capacities(mResources.size());
			for (int64 j = 0; j < mResources.size(); ++j)
				capacities[j] = std::atoi(parameters[2 + j].c_str());

			std::vector<int64> safetyLimits(mResources.size());
			for (int64 j = 0; j < mResources.size(); ++j)
				safetyLimits[j] = std::atoi(parameters[2 + capacities.size() + j].c_str());

			std::vector<int64> moveCosts(nbObjects);
			for (int64 j = 0; j < mMachines.size(); ++j)
				moveCosts[j] = std::atoi(parameters[2 + capacities.size() + safetyLimits.size() + j].c_str());

			mMachines[machineID] = Machine(location, neighbourhood, capacities, safetyLimits, moveCosts);
		}

		mNbLocations = static_cast<int64>(registeredLocations.size());
		mNbNeighbourhoods = static_cast<int64>(registeredNeighbourhoods.size());

		APP_INFO("Number of machines retrieved: {0}", nbObjects);
		APP_INFO("Number of locations retrieved: {0}", mNbLocations);
		APP_INFO("Number of neighbourhoods retrieved: {0}", mNbNeighbourhoods);
		return true;
	};

	std::future<bool> areMachinesLoaded;
	if (componentLines.size() != 0)
		areMachinesLoaded = std::async(loadMachines);

	// Services
	componentLines = components[2];
	int64 nbDependencies = 0;
	auto loadServices =
		[this, componentLines, &nbDependencies]()
	{
		std::string line = componentLines[0];
		int64 nbObjects = std::atoi(line.c_str());

		mServices = std::vector<Service>(nbObjects);
		for (ServiceID serviceID = 0; serviceID < nbObjects; ++serviceID)
		{
			line = componentLines[1 + serviceID];
			auto parameters = tokenizeString(line, separationToken);

			const int64 spreadMin = std::atoi(parameters[0].c_str());

			const int64 nbServiceDependencies = std::atoi(parameters[1].c_str());
			nbDependencies = nbServiceDependencies > nbDependencies ? nbServiceDependencies : nbDependencies;

			std::vector<int64> serviceDependencies(nbServiceDependencies);
			for (int64 j = 0; j < serviceDependencies.size(); ++j)
				serviceDependencies[j] = std::atoi(parameters[2 + j].c_str());

			mServices[serviceID] = Service(spreadMin, serviceDependencies);
		}

		APP_INFO("Number of services retrieved: {0}", nbObjects);
		return true;
	};

	std::future<bool> areServicesLoaded;
	if (componentLines.size() != 0)
		areServicesLoaded = std::async(loadServices);

	// Processes
	componentLines = components[3];
	auto loadProcesses =
		[this, componentLines]()
	{
		std::string line = componentLines[0];
		int64 nbObjects = std::atoi(line.c_str());

		mProcesses = std::vector<Process>(nbObjects);
		for (ProcessID processID = 0; processID < nbObjects; ++processID)
		{
			line = componentLines[1 + processID];
			auto parameters = tokenizeString(line, separationToken);

			const int64 service = std::atoi(parameters[0].c_str());
			mServices[service].addProcessID(processID);

			std::vector<int64> requirements(mResources.size());
			for (int64 j = 0; j < mResources.size(); ++j)
				requirements[j] = std::atoi(parameters[1 + j].c_str());

			const int64 moveCost = std::atoi(parameters[1 + requirements.size()].c_str());

			mProcesses[processID] = Process(service, moveCost, requirements);
		}

		APP_INFO("Number of processes retrieved: {0}", nbObjects);
		return true;
	};

	std::future<bool> areProcessesLoaded;
	if (componentLines.size() != 0)
		areProcessesLoaded = std::async(loadProcesses);

	// Balance objectives
	componentLines = components[4];
	auto loadBalanceObjectives =
		[this, componentLines]()
	{
		std::string line = componentLines[0];
		int64 nbObjects = std::atoi(line.c_str());

		mBalanceObjectives = std::vector<BalanceObjective>(nbObjects);
		for (int64 i = 0; i < nbObjects; ++i)
		{
			line = componentLines[1 + i];
			auto parameters = tokenizeString(line, separationToken);

			const int64 firstResource = std::atoi(parameters[0].c_str());
			const int64 secondResource = std::atoi(parameters[1].c_str());
			const int64 target = std::atoi(parameters[2].c_str());
			const int64 balanceCostWeight = std::atoi(parameters[3].c_str());

			std::array<int64, 3> triple = { firstResource, secondResource, target };
			mBalanceObjectives[i] = BalanceObjective(triple, balanceCostWeight);
		}

		APP_INFO("Number of balance objectives retrieved: {0}", nbObjects);
		return true;
	};

	std::future<bool> areBalanceObjectivesLoaded;
	if (componentLines.size() != 0)
		areBalanceObjectivesLoaded = std::async(loadBalanceObjectives);

	// Particular weights
	componentLines = components[5];
	auto loadWeights =
		[this, componentLines, nbDependencies]()
	{
		std::string line = componentLines[0];
		auto parameters = tokenizeString(line, separationToken);

		mPMCWeight = std::atoi(parameters[0].c_str());
		mSMCWeight = std::atoi(parameters[1].c_str());
		mMMCWeight = std::atoi(parameters[2].c_str());

		return true;
	};

	std::future<bool> areWeightsLoaded;
	if (componentLines.size() != 0)
		areWeightsLoaded = std::async(loadWeights);

	bool isEverythingLoaded = true;

	if (areResourcesLoaded.valid())			if (!areResourcesLoaded.get())			{ APP_WARN("Could not load resources.");			isEverythingLoaded = false; }
	if (areMachinesLoaded.valid())			if (!areMachinesLoaded.get())			{ APP_WARN("Could not load machines.");				isEverythingLoaded = false; }
	if (areServicesLoaded.valid())			if (!areServicesLoaded.get())			{ APP_WARN("Could not load services.");				isEverythingLoaded = false; }
	if (areProcessesLoaded.valid())			if (!areProcessesLoaded.get())			{ APP_WARN("Could not load processes.");			isEverythingLoaded = false; }
	if (areBalanceObjectivesLoaded.valid()) if (!areBalanceObjectivesLoaded.get())	{ APP_WARN("Could not load balance objectives.");	isEverythingLoaded = false; }
	if (areWeightsLoaded.valid())			if (!areWeightsLoaded.get())			{ APP_WARN("Could not load weights.");				isEverythingLoaded = false; }

	if (mResources.size() > maxNbResource)					APP_WARN("You have {0} different resources and the recommended maximum is {1}", mResources.size(), maxNbResource);
	if (mProcesses.size() > maxNbProcesses)					APP_WARN("You have {0} different processes and the recommended maximum is {1}", mProcesses.size(), maxNbProcesses);
	if (mMachines.size() > maxNbMachines)					APP_WARN("You have {0} different machines and the recommended maximum is {1}", mMachines.size(), maxNbMachines);
	if (mNbLocations > maxNbLocations)						APP_WARN("You have {0} different locations and the recommended maximum is {1}", mNbLocations, maxNbLocations);
	if (mNbNeighbourhoods > maxNbNeighbourhoods)			APP_WARN("You have {0} different neighbourhoods and the recommended maximum is {1}", mNbNeighbourhoods, maxNbNeighbourhoods);
	if (mServices.size() > maxNbServices)					APP_WARN("You have {0} different services and the recommended maximum is {1}", mServices.size(), maxNbServices);
	if (nbDependencies > maxNbDependencies)					APP_WARN("You have {0} different dependencies and the recommended maximum is {1}", nbDependencies, maxNbDependencies);
	if (mBalanceObjectives.size() > maxNbBalanceObjectives)	APP_WARN("You have {0} different balance objectives and the recommended maximum is {1}", mBalanceObjectives.size(), maxNbBalanceObjectives);

	if (mSolution.size() != mProcesses.size())			APP_WARN("Your solution assigns {0} processes when it should assign {1}.", mSolution.size(), mProcesses.size());
	if (mInitialSolution.size() != mSolution.size())	APP_WARN("The two solutions have differents sizes.");

	if (isEverythingLoaded) APP_INFO("Data successfully loaded.");

	// Precalculate things
	mMachinesInitialProcesses = calculateMachinesProcesses(mInitialSolution);
}

void Data::attachSolution(const std::string& solutionPath)
{
	std::string solutionStr = readFileContent(solutionPath);
	mSolution = solutionStrToVector(solutionStr);
}

void Data::attachSolution(const std::vector<int64>& solution)
{
	mSolution = solution;
}

void Data::saveNewSolutionToFile(const std::string& filepath)
{
	std::ofstream solutionFile;
	solutionFile.open(filepath);

	for (const auto& assignment : mSolution)
		solutionFile << std::to_string(assignment) + " ";

	solutionFile << eol;

	solutionFile.close();
}

MachinesResourcesUsage Data::calculateMachinesResourcesUsage(const Solution& solution) const
{
 	MachinesResourcesUsage machinesResourcesUsages(mMachines.size(), std::vector<int64>(mResources.size(), 0));

	for (ProcessID processID = 0; processID < mProcesses.size(); ++processID)
	{
		for (ResourceID resourceID = 0; resourceID < mResources.size(); ++resourceID)
			machinesResourcesUsages[solution[processID]][resourceID] += mProcesses[processID].getResourceRequirement(resourceID);
	}

	return machinesResourcesUsages;
}

MachinesProcessesIDs Data::calculateMachinesProcesses(const Solution& solution) const
{
	MachinesProcessesIDs machinesProcesses(mMachines.size(), std::vector<ProcessID>(0));

	for (ProcessID processID = 0; processID < mProcesses.size(); ++processID)
		machinesProcesses[solution[processID]].push_back(processID);

	return machinesProcesses;
}

ServicesLocationsSpreads Data::calculateServicesLocationsSpreads(const Solution & solution) const
{
	ServicesLocationsSpreads servicesLocationsSpreads(mServices.size(), std::vector<int64>(mNbLocations, 0));

	for (ProcessID processID = 0; processID < mProcesses.size(); ++processID)
		++servicesLocationsSpreads[getServiceID(processID)][getMachineLocation(solution[processID])];

	return servicesLocationsSpreads;
}

static std::string readFileContent(const std::string& filepath)
{
	std::string contents;
	std::ifstream instanceFile(filepath, std::ios::in | std::ios::binary);
	if (instanceFile)
	{
		instanceFile.seekg(0, std::ios::end);
		contents.resize(instanceFile.tellg());

		instanceFile.seekg(0, std::ios::beg);
		instanceFile.read(&contents[0], contents.size());

		instanceFile.close();
	}
	else
		APP_ERROR("Could not open instance file ({0})", filepath);

	return contents;
}

static const std::vector<std::vector<std::string>> tokenizeFileByComponents(const std::string& str)
{
	bool hasFirstLineBeenRead = false;

	size_t position = 0;
	constexpr int64 nbComponents = 4; // Balance objectives and Weights are done differently.

	std::vector<std::vector<std::string>> components(0);
	for (int64 i = 0; i < nbComponents; ++i)
	{
		std::string line = hasFirstLineBeenRead ? readNextLine(str, position) : str.substr(0, str.find_first_of(eol));
		hasFirstLineBeenRead = true;

		int64 nbObjects = std::atoi(line.c_str());

		std::vector<std::string> componentLines(0);
		componentLines.push_back(line);
		for (int64 lineNbr = 0; lineNbr < nbObjects; ++lineNbr)
		{
			line = readNextLine(str, position);

			componentLines.push_back(line);
		}

		components.push_back(componentLines);
	}

	// Balance Objectives
	std::string line = readNextLine(str, position);
	int64 nbObjects = std::atoi(line.c_str());

	std::vector<std::string> balanceObjectives(0);
	balanceObjectives.push_back(line);
	for (int64 i = 0; i < nbObjects; ++i)
	{
		line = readNextLine(str, position);
		auto parameters = tokenizeString(line, separationToken);

		std::string balanceObjective = "";
		for (int64 j = 0; j < parameters.size(); ++j)
			balanceObjective += parameters[j] + " ";

		line = readNextLine(str, position); // This is the only part where a parameter is on a new line. Google please stop joking
		balanceObjective += line;

		balanceObjectives.push_back(balanceObjective);
	}

	components.push_back(balanceObjectives);

	// Weights
	line = readNextLine(str, position);
	std::vector<std::string> weights = { line };

	components.push_back(weights);
	return components;
}

static std::string readNextLine(const std::string& str, size_t& currentPosition)
{
	size_t nextLinePosition = str.find(eol, currentPosition) + strlen(eol);
	size_t endOfLine = str.find(eol, nextLinePosition) + strlen(eol);

	currentPosition = nextLinePosition;
	return str.substr(nextLinePosition, endOfLine - nextLinePosition);
}

static std::vector<std::string> tokenizeString(const std::string& str, const char* token)
{
	size_t begin = 0;
	size_t position = begin;
	size_t end = str.length();

	std::vector<std::string> tokens(0);
	while (position != end)
	{
		position = str.find(token, begin);
		if (position == std::string::npos)
			position = end;

		tokens.push_back(str.substr(begin, position - begin)); // substr expects count as second parameters, not a position

		begin = str.find(token, position) + 1;
	}

	return tokens;
}

static std::vector<ProcessID> solutionStrToVector(const std::string& solution)
{
	auto tokens = tokenizeString(solution, " ");

	auto solutionVector = std::vector<int64>(tokens.size());

	for (int64 i = 0; i < solutionVector.size(); ++i)
		solutionVector[i] = std::atoi(tokens[i].c_str());

	solutionVector.pop_back(); // Google trolling yet again by adding a space at the end of the file
	solutionVector.shrink_to_fit();

	return solutionVector;
}