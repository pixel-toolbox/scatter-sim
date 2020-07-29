#include "SteppingAction.hpp"

#include "DetectorConstruction.hpp"

#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"

#include "G4Gamma.hh"
#include "G4Electron.hh"

#define USE_MAX true

namespace SoEiXRS {

SteppingAction::SteppingAction(){}

SteppingAction::~SteppingAction() {}

void SteppingAction::UserSteppingAction(const G4Step* step) {
	if (!fScoringVolume) {
		const DetectorConstruction* detectorConstruction =
				static_cast<const DetectorConstruction*>(G4RunManager::GetRunManager()->GetUserDetectorConstruction());
		fScoringVolume = detectorConstruction->GetScoringVolume();
	}

	G4StepPoint* thisStep = step->GetPostStepPoint();
	if (thisStep == NULL) return;
	const G4TouchableHandle postStepPoint = thisStep->GetTouchableHandle();
	G4VPhysicalVolume* touchableHandle = postStepPoint->GetVolume();
	if (touchableHandle == NULL) return;

	// get volume of the current step
	G4LogicalVolume* volume = touchableHandle->GetLogicalVolume();
	if (volume == NULL) return;

	// check if we are in scoring volume
	if (volume != fScoringVolume)
		return;

	auto energy = step->GetPreStepPoint()->GetKineticEnergy() / keV;

	if (step->GetTrack()->GetDefinition() == G4Gamma::GammaDefinition()) {
		resultPhotonPosEnergy->push_back(energy);
	} else if (step->GetTrack()->GetDefinition() == G4Electron::ElectronDefinition()) {
		resultElectronPosEnergy->push_back(energy);
	} else {
		std::cout << "???? energy: " << energy << std::endl;
	}

	step->GetTrack()->SetTrackStatus(fStopAndKill);
}

} /* namespace SoEiXRS */
