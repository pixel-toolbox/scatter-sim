#include "G4RunManager.hh"

#include <iostream>
#include <vector>
#include <tuple>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

#include "include.hh"
#include "define.hh"

#include "FTFP_BERT.hh"

#include "G4VisExecutive.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4EmPenelopePhysics.hh"

using namespace SoEiXRS;

namespace SoEiXRS {
enum {
	unknownTarget = 0, energyScan, collimatorScan
} task;
}

int main(int argc, char* argv[]) {

	if (argc < 9) {
		std::cout << "Expects at least 9 arguments: " << std::endl;
		std::cout << " task: \"energyScan\" or \"collimatorScan\"" << std::endl;
		std::cout << "  output file" << std::endl;
		std::cout << "  max energy [keV] or only Energy for collimatorScan" << std::endl;
		std::cout << "  bin size [keV] or [cm] for collimatorScan" << std::endl;
		std::cout << "  distance source - filter [cm]" << std::endl;
		std::cout << "  distance filter - detector [cm]" << std::endl;
		std::cout << "  filter / collimator size [cm]" << std::endl;
		std::cout << "  detector size [cm] (assuming a square detector)" << std::endl;
		std::cout << " *Filter Material, e.g. G4_W, G4_Mo, G4_Au" << std::endl;
		std::cout << " *Filter Thickness [mm]" << std::endl;
		std::cout << "  "<< std::endl;
		return 0;
	}

	std::string strTask = argv[1];
	if (strTask == "energyScan") {
		task = energyScan;
	} else if (strTask == "collimatorScan") {
		task = collimatorScan;
	} else {
		task = unknownTarget;
	}
	std::string outputFileName = argv[2];
	double maxEnergy = std::stod(argv[3]);
	double binSize = std::stod(argv[4]);
	double distanceSourceFilter = std::stod(argv[5]);
	double distanceFilterDetect = std::stod(argv[6]);
	double filtCollSize = std::stod(argv[7]);
	double detSize = std::stod(argv[8]);

	std::vector<std::tuple<std::string, double>> filterMaterials;

	for (int i = 9; i<argc; i++) {
		std::string material = argv[i];
		i++;
		double thickness = std::stod(argv[i]);
		filterMaterials.push_back({material, thickness});
	}

	std::cout << "writing to \"" << outputFileName << "\" ..." << std::endl;
	std::cout << "#initial data:" << std::endl;
	std::cout << "#  maxEnergy="            << maxEnergy << "keV" << std::endl;
	std::cout << "#  binSize="              << binSize << "keV" << std::endl;
	std::cout << "#  distanceSourceFilter=" << distanceSourceFilter << "cm" << std::endl;
	std::cout << "#  distanceFilterDetect=" << distanceFilterDetect << "cm" << std::endl;
	std::cout << "#  filtCollSize="         << filtCollSize << "cm" << std::endl;
	std::cout << "#  detSize="              << detSize << "cm" << std::endl;
	std::cout << "# Filter Materials=[" << std::endl;
	for (auto f : filterMaterials) {
		std::cout << "#   " << std::get<0>(f) << ", " << std::get<1>(f) << "mm" << std::endl;
	}
	std::cout << "#  ]" << std::endl;

	// construct the default run manager
	G4RunManager* runManager = new G4RunManager();

	// set mandatory initialization classes
	G4VModularPhysicsList* physicsList = new FTFP_BERT;
	runManager->SetUserInitialization(physicsList);
	runManager->SetUserInitialization(new DetectorConstruction(distanceSourceFilter, distanceFilterDetect,
			filtCollSize, detSize, filterMaterials));
	ActionInitialization* actionInitialization = new ActionInitialization(1, 0.5,
			distanceSourceFilter, filtCollSize);
	runManager->SetUserInitialization(actionInitialization);

	// get the pointer to the UI manager and set verbosities
	G4UImanager* UI = G4UImanager::GetUIpointer();
	UI->ApplyCommand("/run/verbose 0");
	UI->ApplyCommand("/event/verbose 0");
	UI->ApplyCommand("/tracking/verbose 0");
	UI->ApplyCommand("/run/setCut  0.00001 mm");
	UI->ApplyCommand("/cuts/setLowEdge 250 eV");
	UI->ApplyCommand("/process/em/fluo true");

	// initialize G4 kernel
	runManager->Initialize();

	/*G4VisManager* visManager = new G4VisExecutive;
	visManager->Initialise();

	UI->ApplyCommand("/control/execute vis.mac");
	//UI->ApplyCommand("/vis/scene/endOfEventAction accumulate 1");
	UI->ApplyCommand("/vis/verbosity 2");*/

	PrimaryGeneratorAction* primaryGeneratorAction = actionInitialization->getPrimaryGeneratorAction();

	SteppingAction* steppingAction = actionInitialization->getSteppingAction();

	switch (task) {
	case energyScan: {

		std::vector<std::vector<double>> responseElectronMatrix;
		std::vector<std::vector<double>> responsePhotonMatrix;

		responseElectronMatrix.push_back(std::vector<double>(maxEnergy / binSize, 0));
		responsePhotonMatrix.push_back(std::vector<double>(maxEnergy / binSize, 0));

		for (double inputEnergy = binSize; inputEnergy < maxEnergy;
				inputEnergy += binSize) {

			std::vector<double>* resultElectronVector = new std::vector<double>();
			std::vector<double>* resultPhotonVector = new std::vector<double>();

			steppingAction->resultElectronPosEnergy = resultElectronVector;
			steppingAction->resultPhotonPosEnergy = resultPhotonVector;

			primaryGeneratorAction->energyDist = std::uniform_real_distribution<double>(
					inputEnergy - (binSize / 2), inputEnergy + (binSize / 2));

			std::cout << "current energy: " << inputEnergy << "keV"
					<< std::endl;
			int runs = 500000;
			for (int r = 0; r < runs; r++) {
				// start a run
				if (r % 1000 == 0) {
					std::cout << ".";
					std::cout.flush();
				}
				runManager->BeamOn(1);
			}

			std::vector<double> outputElectronVec(maxEnergy / binSize, 0);
			std::vector<double> outputPhotonVec(maxEnergy / binSize, 0);

			for (auto d : *resultElectronVector) {
				int pos = (d + binSize / 2.) / binSize;

				if (pos >= 0 && pos < outputElectronVec.size()) {
					outputElectronVec.at(pos) += 1;
				}
			}
			for (auto d : *resultPhotonVector) {
				int pos = (d + binSize / 2.) / binSize;

				if (pos >= 0 && pos < outputPhotonVec.size()) {
					outputPhotonVec.at(pos) += 1;
				}
			}
			responseElectronMatrix.push_back(outputElectronVec);
			responsePhotonMatrix.push_back(outputPhotonVec);

			//UI->ApplyCommand("/vis/viewer/rebuild");
			//UI->ApplyCommand("/vis/viewer/flush");
			std::cout << "  --> Electron count = " << resultElectronVector->size() << " / "
					<< (PARTICLES_IN_GUN * runs) << std::endl;
			std::cout << "  --> Photon count = " << resultPhotonVector->size() << " / "
					<< (PARTICLES_IN_GUN * runs) << std::endl;
			delete (resultElectronVector);
			delete (resultPhotonVector);
		}

		std::cout << "writing output file" << std::endl;
		std::ofstream outputFile(outputFileName + ".ele", std::ofstream::trunc);

		for (auto line : responseElectronMatrix) {
			for (auto d : line) {
				outputFile << d << " ";
			}
			outputFile << std::endl;
		}

		outputFile.close();

		std::ofstream outputFile2(outputFileName + ".pho", std::ofstream::trunc);

		for (auto line : responsePhotonMatrix) {
			for (auto d : line) {
				outputFile2 << d << " ";
			}
			outputFile2 << std::endl;
		}

		outputFile2.close();

		break;
	}
	case collimatorScan: {

		std::vector<std::vector<double>> responseMatrix;

		responseMatrix.push_back(std::vector<double>(102, 0));

		primaryGeneratorAction->energyDist = std::uniform_real_distribution<double>(
				maxEnergy - 0.001, maxEnergy + 0.001);

		for (double thisCollSize = binSize; thisCollSize < filtCollSize; thisCollSize += binSize) {

			std::vector<double>* resultVector = new std::vector<double>();

			steppingAction->resultElectronPosEnergy = resultVector;
			steppingAction->resultPhotonPosEnergy = resultVector;

			std::cout << "current collimator size: " << thisCollSize << "cm" << std::endl;

			primaryGeneratorAction->filtCollSize = thisCollSize;

			primaryGeneratorAction->directDist = std::uniform_real_distribution<double>(
					-atan(thisCollSize / (2 * distanceSourceFilter)),
					+atan(thisCollSize / (2 * distanceSourceFilter)));

			int runs = 500000;
			for (int r = 0; r < runs; r++) {
				// start a run
				if (r % 1000 == 0) {
					std::cout << ".";
					std::cout.flush();
				}
				runManager->BeamOn(1);
			}

			double energyBinSize = maxEnergy/100;

			std::vector<double> outputVec(102, 0);

			for (auto d : *resultVector) {
				int pos = (d + energyBinSize * 0.5) / energyBinSize;

				if (pos > 0 && pos < outputVec.size()) {
					outputVec.at(pos) += 1;
				}
			}
			responseMatrix.push_back(outputVec);

			//UI->ApplyCommand("/vis/viewer/rebuild");
			//UI->ApplyCommand("/vis/viewer/flush");
			std::cout << "  --> count = " << resultVector->size() << " / "
					<< (PARTICLES_IN_GUN * runs) << std::endl;
			delete (resultVector);
		}

		std::cout << "writing output file" << std::endl;
		std::ofstream outputFile(outputFileName, std::ofstream::trunc);

		for (auto line : responseMatrix) {
			for (auto d : line) {
				outputFile << d << " ";
			}
			outputFile << std::endl;
		}

		outputFile.close();

		break;
	}
	default:
		std::cout << "cannot detect task" << std::endl;
	}

	/*std::string s;

	std::cout << "done >";
	std::cout.flush();
	std::cin >> s;*/

	// job termination
	delete runManager;
	//delete visManager;

	return 0;
}
