/*
 * TransferBase.cpp
 *
 *  Created on: 23 Mar 2015
 *      Author: erik
 */

#include "Transferbase.h"

#include <exception>
#include <deque>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include "Asteroid.h"

#if TRANSFERBASE_OUTPUT
#include <iostream>
#endif

namespace model {

Transferbase::Transferbase(const std::string& filename, const std::vector<Asteroid*> &asteroids) {
	this->filename = filename;
	missionStart = missionDuration = nelderMeadEpsilon = density =  -1;
	nelderMeadDeltaV = nelderMeadResolution = asteroidCount = -1;

	id_int i=0;
	for (Asteroid* asteroid : asteroids) {
		asteroidIndex.emplace(asteroid, i);
		idIndex.emplace(i, asteroid);
		i++;
	}
}

Transferbase::Transferbase(const std::string& filename, const std::vector<Asteroid*> &asteroids, double missionStart, double missionDuration, int nelderMeadDeltaV,
		int nelderMeadResolution, double nelderMeadEpsilon, int asteroidCount, double density) : Transferbase(filename, asteroids) {

	this->missionStart = missionStart;
	this->missionDuration = missionDuration;
	this->nelderMeadDeltaV = nelderMeadDeltaV;
	this->nelderMeadResolution = nelderMeadResolution;
	this->nelderMeadEpsilon = nelderMeadEpsilon;
	this->asteroidCount = asteroidCount;
	this->density = density;
}

bool Transferbase::fileExists() {
	// see if file exists
	filestream.open(filename.c_str(), std::ios::in | std::ios::binary);
	if (filestream.good()) {
		// can be opened
		filestream.close();

		return true;
	} else {
		return false;
	}
}

void Transferbase::create() {

	filestream.open(filename.c_str(), std::ios::out | std::ios::binary);
	writeInt(version);
	writeDouble(missionStart);
	writeDouble(missionDuration);
	writeInt(nelderMeadDeltaV);
	writeInt(nelderMeadResolution);
	writeDouble(nelderMeadEpsilon);
	writeInt(asteroidCount);
	writeDouble(density);

	writeInt(static_cast<int>(asteroidIndex.size()));
	for (auto& asteroidId : asteroidIndex) {
		writeString(asteroidId.first->getName());
		writeInt(asteroidId.second);
	}

	filestream.flush();
}

void Transferbase::finalizeTransferbase(Asteroid* startingPoint, std::vector<Asteroid*> asteroids, int spacecraftDeltaVlimit, const std::string &inputFilename, const std::string &outputFilename) {

	using std::unordered_map;
	using std::string;

	// load original
	Transferbase original(inputFilename, asteroids);
	original.load(false);

	// remove junk data
	std::cout << "remove non origins" << std::endl;
	original.removeNonOrigins();
	std::cout << "remove time" << std::endl;
	original.removeJunkTime(startingPoint);
	std::cout << "remove cost" << std::endl;
	original.removeJunkCost(startingPoint, spacecraftDeltaVlimit);

	// find all asteroid ids used in finalized transferbase
	std::unordered_set<Asteroid*> includedAsteroids;
	for (auto& originMap : original.transferWindowIndex) {
		includedAsteroids.emplace(originMap.first);
		Asteroid *previous = nullptr;
		for (TransferWindow& tw : originMap.second) {
			if (tw.target != previous) {
				previous = tw.target;
				includedAsteroids.emplace(tw.target);
			}
		}
	}

	// create trimmed Asteroid* vector for creating new Transferbase with trimmed index
	std::unordered_map<Asteroid*, id_int> trimmedIndex;
	for (auto& entry : original.asteroidIndex) {
		Asteroid *asteroid = entry.first;
		id_int id = entry.second;
		if (includedAsteroids.find(asteroid) != includedAsteroids.end()) {
			trimmedIndex.emplace(asteroid, id);
		}
	}

	// create initial Transferbase file
	Transferbase finalized(outputFilename, std::vector<Asteroid*>(), original.missionStart, original.missionDuration, original.nelderMeadDeltaV, original.nelderMeadResolution,
			original.nelderMeadEpsilon, original.asteroidCount,	original.density);
	finalized.asteroidIndex = trimmedIndex;
	finalized.create();

	// write data to file
	finalized.writeFromTransferbase(original.transferWindowIndex);

}

void Transferbase::removeNonOrigins() {
#if TRANSFERBASE_OUTPUT
	// count in data
	std::unordered_set<Asteroid*> dataAsteroids;
	size_t dataTransfers = 0;
	for (auto& originMap: transferWindowIndex) {
		dataAsteroids.emplace(originMap.first);
		dataTransfers += originMap.second.size();
		Asteroid* previous = nullptr;
		for (TransferWindow& tw : originMap.second) {
			if (previous != tw.target) {
				previous = tw.target;
				dataAsteroids.emplace(tw.target);
			}
		}
	}
#endif

	for (auto originIt = transferWindowIndex.begin(); originIt != transferWindowIndex.end();) {
		// keep only windows to asteroids which are also origins
		std::deque<TransferWindow> windowsToKeep;
		Asteroid* previous = nullptr;
		bool keep = false;
		for (TransferWindow &tw : originIt->second) {
			if (tw.target != previous) {
				previous = tw.target;
				keep = transferWindowIndex.find(tw.target) != transferWindowIndex.end();
			}
			if (keep) {
				windowsToKeep.push_back(tw);
			}
		}

		originIt->second.swap(windowsToKeep);
		++originIt;
	}

	for (auto originIt = transferWindowIndex.begin(); originIt != transferWindowIndex.end();) {
		if (originIt->second.size() == 0) {
			originIt = transferWindowIndex.erase(originIt);
		} else {
			++originIt;
		}
	}


#if TRANSFERBASE_OUTPUT
	// count in newData
	std::unordered_set<Asteroid*> newDataAsteroids;
	size_t newDataTransfers = 0;
	for (auto& originMap: transferWindowIndex) {
		newDataAsteroids.emplace(originMap.first);
		newDataTransfers += originMap.second.size();
		Asteroid* previous = nullptr;
		for (TransferWindow& tw : originMap.second) {
			if (previous != tw.target) {
				previous = tw.target;
				newDataAsteroids.emplace(tw.target);
			}
		}
	}

	std::cout << "Remaining after destination pruning:" << "\n\t"
			<< dataTransfers << " - " << dataTransfers - newDataTransfers  << " = " << newDataTransfers << " transfers and" << "\n\t"
			<< dataAsteroids.size() << " - " << dataAsteroids.size() - newDataAsteroids.size() << " = " << newDataAsteroids.size() << " asteroids"
			<< std::endl;
#endif

}

void Transferbase::removeJunkTime(Asteroid* startingPoint) {
	using std::unordered_map;

	// initialize structures
	unordered_map<Asteroid*, double> earliestArrival;										// structure specifying remaining delta v
	std::deque<Asteroid*> queue;															// queue of asteroids with changed remainingDeltaV
	earliestArrival[startingPoint] = 0;
	queue.push_back(startingPoint);
	double timeOfMissionEnd = missionStart + missionDuration;

	// iteratively improve earliest arrival time (bf-search)
	while (not queue.empty()) {
		Asteroid *origin = queue.front();
		queue.pop_front();

		double selfArrival = earliestArrival.at(origin);
		// if current asteroid is a leaf (not present as origin), it does not have out-bound transfers
		auto iter = transferWindowIndex.find(origin);
		if (iter != transferWindowIndex.end()) {
			// go through all transfers with current as origin and attempt to improve estimate for them
			Asteroid *previous = nullptr;
			for (TransferWindow& tw : iter->second) {
				// calculate time of arrival at destination
				int toa = tw.tod + tw.tof;
				if (tw.tod >= selfArrival and toa <= timeOfMissionEnd) {
					Asteroid *destination = tw.target;
					// if destination has not yet been reached, initialize
					auto destinationIter = earliestArrival.find(destination);
					if (destinationIter == earliestArrival.end()) {
						earliestArrival.emplace(destination, timeOfMissionEnd +1);
						destinationIter = earliestArrival.find(destination);
					}
					// check that transfer improves previous and that it is feasible
					if (toa < destinationIter->second) {
						destinationIter->second = toa;
						if (previous != destination) {
							queue.push_back(destination);
							previous = destination;
						}
					}
				}
			}
		}
	}


#if TRANSFERBASE_OUTPUT
	// count in data
	std::unordered_set<Asteroid*> dataAsteroids;
	size_t dataTransfers = 0;
	for (auto& originMap: transferWindowIndex) {
		dataAsteroids.emplace(originMap.first);
		dataTransfers += originMap.second.size();
		Asteroid* previous = nullptr;
		for (TransferWindow& tw : originMap.second) {
			if (previous != tw.target) {
				previous = tw.target;
				dataAsteroids.emplace(tw.target);
			}
		}
	}
#endif

	// extract asteroids and transfers to keep;
	for (auto it = transferWindowIndex.begin(); it != transferWindowIndex.end();) {
		Asteroid *origin = it->first;
		std::deque<TransferWindow> windowsToKeep;

		// proceed if origin could be reached (exists in structure)
		auto originIter = earliestArrival.find(origin);
		if (originIter != earliestArrival.end()) {
			double arrival = originIter->second;
			// only proceed if origin can be reached before mission end
			if (arrival <= timeOfMissionEnd) {
				for (TransferWindow &tw : it->second) {
					if (tw.tod >= arrival) {
						windowsToKeep.push_back(tw);
					}
				}
			}
		}

		if (windowsToKeep.size() > 0) {
			it->second.swap(windowsToKeep);
			++it;
		} else {
			it = transferWindowIndex.erase(it);
		}
	}


#if TRANSFERBASE_OUTPUT
	// count in newData
	std::unordered_set<Asteroid*> newDataAsteroids;
	size_t newDataTransfers = 0;
	for (auto& originMap: transferWindowIndex) {
		newDataAsteroids.emplace(originMap.first);
		newDataTransfers += originMap.second.size();
		Asteroid* previous = nullptr;
		for (TransferWindow tw : originMap.second) {
			if (previous != tw.target) {
				previous = tw.target;
				newDataAsteroids.emplace(tw.target);
			}
		}
	}

	std::cout << "Remaining after time pruning:" << "\n\t"
			<< dataTransfers << " - " << dataTransfers - newDataTransfers  << " = " << newDataTransfers << " transfers and" << "\n\t"
			<< dataAsteroids.size() << " - " << dataAsteroids.size() - newDataAsteroids.size() << " = " << newDataAsteroids.size() << " asteroids"
			<< std::endl;
#endif
}

void Transferbase::removeJunkCost(Asteroid *startingPoint, int spacecraftDeltaVlimit) {
	using std::unordered_map;
	static const int MAXIMUM_TRANSFER_COST = 1000000000;

	// perform search in original's data starting at requested starting point, never exceeding delta V limit

	// initialize structures
	unordered_map<Asteroid*, unordered_map<Asteroid*, int>> cheapestTransferCosts;			// structure containing entries of the form <origin name, destination name, cost of cheapest transfer>

	// fill in cheapest transfer costs
	for (auto& originMaps : transferWindowIndex) {
		// inserts origin
		auto& originCheapest = cheapestTransferCosts[originMaps.first];

		auto previousIter = originCheapest.end();
		Asteroid* previous = nullptr;
		for (TransferWindow& tw : originMaps.second) {
			if (tw.target != previous) {
				previous = tw.target;
				previousIter = originCheapest.find(tw.target);
				if (previousIter == originCheapest.end()) {
					previousIter = originCheapest.emplace(tw.target,MAXIMUM_TRANSFER_COST).first;
				}
			}

			if (previousIter->second > tw.deltaV) {
				previousIter->second = tw.deltaV;
			}
		}
	}

	std::cout << "filled " << std::endl;

	// iteratively improve remaining delta v (bf-search)
	unordered_map<Asteroid*, int> remainingDeltaVs;											// structure specifying remaining delta v
	std::deque<Asteroid*> queue;															// queue of asteroids with changed remainingDeltaV
	remainingDeltaVs[startingPoint] = spacecraftDeltaVlimit;
	queue.push_back(startingPoint);
	while (not queue.empty()) {
		Asteroid *asteroid = queue.front();
		queue.pop_front();

		int selfRemaining = remainingDeltaVs.at(asteroid);

		// current asteroid may be a leaf node (only present as destination) and not have out-bound transfers
		auto originCheapestIter = cheapestTransferCosts.find(asteroid);
		if (originCheapestIter != cheapestTransferCosts.end()) {
			// go through all destinations where current is origin and attempt to improve estimate for them
			for (auto& destinationMap : originCheapestIter->second) {
				int newRemaining = selfRemaining - destinationMap.second;
				if (newRemaining >= 0) {
					// retrieve previous best and initialize to default of int (0) if no previous was set
					int &previousRemaining = remainingDeltaVs[destinationMap.first];
					if (newRemaining > previousRemaining) {
						previousRemaining = newRemaining;
						queue.push_back(destinationMap.first);
					}
				}

			}
		}
	}


#if TRANSFERBASE_OUTPUT
	// count in data
	std::unordered_set<Asteroid*> dataAsteroids;
	size_t dataTransfers = 0;
	for (auto& originMap: transferWindowIndex) {
		dataAsteroids.emplace(originMap.first);
		dataTransfers += originMap.second.size();
		Asteroid* previous = nullptr;
		for (TransferWindow tw : originMap.second) {
			if (previous != tw.target) {
				previous = tw.target;
				dataAsteroids.emplace(tw.target);
			}
		}
	}
#endif

	for (auto it = transferWindowIndex.begin(); it != transferWindowIndex.end();) {
		Asteroid *origin = it->first;
		std::deque<TransferWindow> transfersToKeep;

		// proceed if origin could be reached (exists in structure)
		auto deltaVIter = remainingDeltaVs.find(origin);
		if (deltaVIter != remainingDeltaVs.end()) {

			int remainingDeltaV = deltaVIter->second;
			auto& originCheapest = cheapestTransferCosts.at(origin);

			// include all transfers within delta v limit
			for (TransferWindow& tw : it->second) {
				// do not consider destination if cheapest transfer is not cheap enough
				if (originCheapest.at(tw.target) <= remainingDeltaV) {
					transfersToKeep.push_back(tw);
				}
			}
		}

		if (transfersToKeep.size() > 0) {
			it->second.swap(transfersToKeep);
			++it;
		} else {
			it = transferWindowIndex.erase(it);
		}
	}


#if TRANSFERBASE_OUTPUT
	// count in newData
	std::unordered_set<Asteroid*> newDataAsteroids;
	size_t newDataTransfers = 0;
	for (auto& originMap: transferWindowIndex) {
		newDataAsteroids.emplace(originMap.first);
		newDataTransfers += originMap.second.size();
		Asteroid* previous = nullptr;
		for (TransferWindow tw : originMap.second) {
			if (previous != tw.target) {
				previous = tw.target;
				newDataAsteroids.emplace(tw.target);
			}
		}
	}

	std::cout << "Remaining after delta v pruning:" << "\n\t"
			<< dataTransfers << " - " << dataTransfers - newDataTransfers  << " = " << newDataTransfers << " transfers and" << "\n\t"
			<< dataAsteroids.size() << " - " << dataAsteroids.size() - newDataAsteroids.size() << " = " << newDataAsteroids.size() << " asteroids"
			<< std::endl;
#endif
}



void Transferbase::load(bool parameterCheck) {
	// see if file exists
	if (fileExists()) {
		filestream.open(filename.c_str(), std::ios::in | std::ios::binary);
		if (not filestream.good()) {
			throw std::runtime_error("Could not open file " + filename);
		}

	#if TRANSFERBASE_OUTPUT
		std::cout << "Loading TransferBase from " << filename << '\n';
	#endif

		// load mission parameters
		int version = readInt();
		double missionStart = readDouble();
		double missionDuration = readDouble();
		int nelderMeadDeltaV = readInt();
		int nelderMeadResolution = readInt();
		double nelderMeadEpsilon = readDouble();
		int asteroidCount = readInt();
		double density = readDouble();

	#if TRANSFERBASE_OUTPUT
		std::cout << "file version=" << version << '\n';
		std::cout << "missionStart=" << missionStart << '\n';
		std::cout << "missionDuration=" << missionDuration << '\n';
		std::cout << "nelderMeadDeltaV=" << nelderMeadDeltaV << '\n';
		std::cout << "nelderMeadResolution=" << nelderMeadResolution << '\n';
		std::cout << "nelderMeadEpsilon=" << nelderMeadEpsilon << '\n';
		std::cout << "asteroidCount=" << asteroidCount << '\n';
		std::cout << "density=" << density << '\n';
	#endif


		if (parameterCheck) {
			if (this->missionStart != missionStart or this->missionDuration != missionDuration or this->nelderMeadDeltaV != nelderMeadDeltaV or
					this->nelderMeadResolution != nelderMeadResolution or this->nelderMeadEpsilon != nelderMeadEpsilon or
					this->asteroidCount != asteroidCount or this->density != density
			) {
				filestream.close();
				throw std::invalid_argument("Could not load " + filename + " because database's parameters do not match current settings.");
			}
		} else {
			this->missionStart = missionStart;
			this->missionDuration = missionDuration;
			this->nelderMeadDeltaV = nelderMeadDeltaV;
			this->nelderMeadResolution = nelderMeadResolution;
			this->nelderMeadEpsilon = nelderMeadEpsilon;
			this->asteroidCount = asteroidCount;
			this->density = density;
		}


		// ignore index
		int count = readInt();
		for (int i=0; i < count; ++i) {
			readName();
			readInt();
		}

		transferWindowIndex.rehash(expected_number_of_origin_asteroids);
		// see if there is another entry in file
		while (filestream.peek() != EOF) {

			id_int fromId = readInt();
			int toCount = readInt();

			Asteroid *from = idIndex.at(fromId);

			std::deque<TransferWindow> &transferWindows = transferWindowIndex[from];

			for(int i=0; i < toCount; ++i) {

				id_int toId = readInt();
				Asteroid *to = idIndex.at(toId);

				int transferCount = readChar();

				for(int j=0; j < transferCount; ++j) {
					transferWindows.push_back(TransferWindow(readShort(), readShort(), readShort(), to));
				}
			}
		}

	#if TRANSFERBASE_OUTPUT
		std::cout << "index count=" << count << '\n';
		std::cout << "origin count=" << transferWindowIndex.size() << '\n';
		size_t transferCount = 0;
		for (auto& originMap: transferWindowIndex) {
			transferCount += originMap.second.size();
		}
		std::cout << "transfer count=" << transferCount << '\n';
		std::cout << std::endl;
	#endif

		filestream.close();
		filestream.open(filename.c_str(), std::ios::out | std::ios::app | std::ios::binary);


	} else {
		throw std::invalid_argument("Transferbase cannot be loaded - file does not exist");
	}

}

void Transferbase::writeFromTransferbase(const TransferWindowIndex &transferWindowIndex) {
	for (auto& originMap : transferWindowIndex) {
		write(originMap.first, originMap.second);
	}
}

void Transferbase::write(Asteroid* from, const std::deque<TransferWindow> &transferWindowMap) {
	std::unordered_map<Asteroid*, std::deque<TransferWindow>> destinationTransfersMap;

	Asteroid* previous = nullptr;
	std::deque<TransferWindow> *previousTransfers = nullptr;
	for (const TransferWindow &tw : transferWindowMap) {
		if (tw.target != previous) {
			previousTransfers = &destinationTransfersMap[tw.target];
			previous = tw.target;
		}
		previousTransfers->push_back(tw);
	}


	id_int fromId = asteroidIndex.at(from);
	writeInt(fromId);
	// number of destinations
	writeInt(static_cast<int>(destinationTransfersMap.size()));
	for (auto& destinationTransfers : destinationTransfersMap) {
		// write name of to asteroid
		int toId = asteroidIndex.at(destinationTransfers.first);
		writeInt(toId);

		// write number of transfers
		writeChar(static_cast<char>(destinationTransfers.second.size()));

		// write transfers
		for (const TransferWindow &transfer : destinationTransfers.second) {
			writeShort(static_cast<short>(transfer.deltaV));
			writeShort(static_cast<short>(transfer.tod));
			writeShort(static_cast<short>(transfer.tof));
		}
	}

	filestream.flush();
}

std::deque<TransferWindow> Transferbase::getTransferWindows(Asteroid *from) {
	auto iter = transferWindowIndex.find(from);
	std::deque<TransferWindow> windows = iter->second;
	transferWindowIndex.erase(iter);
	return windows;
}

bool Transferbase::lookup(const Asteroid* from) const {
	if (transferWindowIndex.find(const_cast<Asteroid*>(from)) != transferWindowIndex.end()) {
		return true;
	} else {
		return false;
	}
}

void printNumberFormatted(long t) {
	static const std::vector<std::pair<std::string,long>> magnitudes = {
			std::make_pair("P", std::pow(10,15)),
			std::make_pair("T", std::pow(10,12)),
			std::make_pair("G", std::pow(10,9)),
			std::make_pair("M", std::pow(10,6)),
			std::make_pair("K", std::pow(10,3)),
			std::make_pair("",  std::pow(10,0))
	};

	for (size_t i=0; i < magnitudes.size(); ++i) {
		if (t >= magnitudes[i].second) {
			double value = static_cast<double>(t) / static_cast<double>(magnitudes[i].second);
			// if at least two digits
			if (value >= 10) {
				std::cout << static_cast<int>(std::round(value)) << magnitudes[i].first;
			} else {
				std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(1) << value << magnitudes[i].first;
			}
			break;
		}
	}
}
void Transferbase::printTransferbase(Asteroid *start, short initialDeltaV, long printEveryN, size_t chunkSize) {

	struct levelInfo {
		long transfers = 0;
		long terminalTransfers = 0;
		std::unordered_set<Asteroid*> asteroids;
	};

	// holds one arrival event
	struct arrival {
		Asteroid* asteroid;
		short dvLeft;
		short toa;
		short level;
	};

	// holds one transfer window
	struct window {
		Asteroid* asteroid;
		short dvCost;
		short tod;
		short toa;
	};

	// restructure transfer index to better suit this algorithm
	std::unordered_map<Asteroid*, std::vector<window>> windowIndex;

	for (auto& originMap : transferWindowIndex) {
		std::vector<window> originWindows;
		originWindows.reserve(originMap.second.size());

		for (TransferWindow tw : originMap.second) {
			originWindows.push_back(window {tw.target, tw.deltaV, tw.tod, static_cast<short>(tw.tod+tw.tof)});
		}
		// sort according to tod in descending order, required to skip transfers with tod < origin.toa
		std::sort(originWindows.begin(), originWindows.end(), [] (const window &w1, const window &w2) { return w1.tod > w2.tod; });
		windowIndex.emplace(originMap.first, originWindows);
	}

	std::deque<arrival> stack;

	// contains output information
	std::vector<levelInfo> levelInfos;

	if (windowIndex.find(start) != windowIndex.end()) {
		stack.push_back(arrival{start, initialDeltaV, 0, 0});
	} else {
		throw std::runtime_error("Could not find \"" + start->getName() + "\" in Transfer Map");
	}

	long working = 0;					// number of threads working
	long totalCount = 0;				// number of arrival events pulled from the stack
	long incrementalCount = 0;			// number of arrival events pulled form the stack since last console print

	// begin measuring time for output
	std::chrono::high_resolution_clock::time_point timeStart = std::chrono::high_resolution_clock::now();

#pragma omp parallel
	{

		std::vector<arrival> scheduledTasks;			// local list of arrivals from stack
		std::vector<arrival> newTasksCache;				// local list of arrivals to put on stack
		scheduledTasks.reserve(chunkSize);

		while (true) {

			// result is set to
			// 		0 while work is to be fetched,
			// 		1 if work has been fetched and
			//		-1 when finished
			int result = 0;

			do {
#pragma omp critical (state)
				{
					if (stack.size() > 0) {
						result = 1;
						++working;

						size_t chunk = std::min(chunkSize, stack.size()/2+1);
						scheduledTasks.insert(scheduledTasks.end(), stack.end() - chunk, stack.end());
						stack.erase(stack.end() - chunk, stack.end());

						incrementalCount += chunk;
						if (incrementalCount >= printEveryN) {
							totalCount += printEveryN;
							incrementalCount -= printEveryN;
#pragma omp critical (output)
							{
								for (size_t i=1; i < levelInfos.size(); ++i) {
									auto& li = levelInfos[i];
									std::cout << "[";
									printNumberFormatted(li.transfers - li.terminalTransfers);
									std::cout << ",";
									printNumberFormatted(li.terminalTransfers);
									std::cout << ',';
									printNumberFormatted(li.asteroids.size());
									std::cout << "] ";
								}
								printNumberFormatted(totalCount);
								std::cout << std::endl;
							}
						}
					} else if (working == 0) {
						// finished
						result = -1;
					}
				}
			} while(result == 0);

			// check quit
			if (result == -1) {
				// no more work
				break;
			}

			// perform work
			for (arrival& task : scheduledTasks) {
				const auto it = windowIndex.find(task.asteroid);
				size_t initialSize = newTasksCache.size();

				// check if outgoing transfers exist
				if (it != windowIndex.end()) {
					short nextLevel = static_cast<short>(task.level + 1);
					for (window& win : it->second) {
						if (win.tod > task.toa) {
							short ddv = static_cast<short>(task.dvLeft - win.dvCost);
							if (ddv >= 0) {
								newTasksCache.push_back( arrival{ win.asteroid, ddv, win.toa, nextLevel});
							}
						} else {
							break;
						}
					}
				}
				if (newTasksCache.size() == initialSize)
#pragma omp critical (output)
				{
					while (levelInfos.size() <= static_cast<size_t>(task.level)) {
						levelInfos.push_back(levelInfo());
					}
					levelInfos[task.level].terminalTransfers++;
				}

			}
			scheduledTasks.clear();
#pragma omp critical (state)
			{
				working--;
				stack.insert(stack.end(), newTasksCache.begin(), newTasksCache.end());
			}
#pragma omp critical (output)
			{
				for (arrival& newTask : newTasksCache) {
					while (levelInfos.size() <= static_cast<size_t>(newTask.level)) {
						levelInfos.push_back(levelInfo());
					}
					levelInfos[newTask.level].transfers++;
					levelInfos[newTask.level].asteroids.emplace(newTask.asteroid);
				}
			}
			newTasksCache.clear();

		}
	}

	std::cout << '\n';

	// end time measurement
	std::chrono::high_resolution_clock::time_point timeEnd = std::chrono::high_resolution_clock::now();

	std::cout << "transfers away from start, terminal routes, non-terminal routes, asteroids, cumulative asteroids" << std::endl;

	std::unordered_set<Asteroid*> cumulativeAsteroids;
	for (size_t i=1; i < levelInfos.size(); ++i) {
		auto& li = levelInfos[i];
		cumulativeAsteroids.insert(li.asteroids.begin(), li.asteroids.end());
		std::cout << i << ", " << li.terminalTransfers << ", " << li.transfers - li.terminalTransfers << ", " << li.asteroids.size() << ", " << cumulativeAsteroids.size() << '\n';
	}

	std::cout << '\n';

	totalCount += incrementalCount;
	std::cout << "Total transfers:\t" << totalCount << '\n';
	long seconds = std::chrono::duration_cast<std::chrono::seconds>(timeEnd - timeStart).count();
	std::cout << "Took " << seconds << "s." << '\n';
	std::cout << "Million transfers / s:\t" << std::setiosflags(std::ios::fixed) << std::setprecision(4) << static_cast<double>(totalCount) / static_cast<double>(seconds) / 10e6 << '\n';

	std::cout.flush();
}

Transferbase::~Transferbase() {
	// do nothing, and to NOT store to file (modified by convert)
}

} /* namespace model */
