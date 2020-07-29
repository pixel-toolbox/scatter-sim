#ifndef SoEiXRS_CLASSES_SteppingAction_HPP_
#define SoEiXRS_CLASSES_SteppingAction_HPP_ 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"

#include <fstream>
#include <vector>

class B1EventAction;

class G4LogicalVolume;

namespace SoEiXRS {

class SteppingAction : public G4UserSteppingAction {

	public:
		SteppingAction();
		virtual ~SteppingAction();

		virtual void UserSteppingAction(const G4Step*);

	    std::vector<double>* resultElectronPosEnergy = NULL;
	    std::vector<double>* resultPhotonPosEnergy = NULL;

	private:
	    G4LogicalVolume* fScoringVolume = NULL;
};

} /* namespace SoEiXRS */

#endif /* SoEiXRS_CLASSES_SteppingAction_HPP_ */
