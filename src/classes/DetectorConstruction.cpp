#include "DetectorConstruction.hpp"

namespace SoEiXRS {

DetectorConstruction::DetectorConstruction(double distanceSourceFilter,
		double distanceFilterDetect, double filtCollSize, double detSize,
		std::vector<std::tuple<std::string, double>> filterMaterials) :
		G4VUserDetectorConstruction(), nist(G4NistManager::Instance()), world_sizeXY(
				std::max(filtCollSize * 5 * cm, detSize * 20 * cm)), world_sizeZ(
				std::max(distanceSourceFilter, distanceFilterDetect) * 2.1 * cm) {

	/// https://books.google.de/books?id=8REhrrTKZKwC&pg=PA25#v=onepage&q&f=false

	double a = 1.01 * g / mole;
	G4Element* elH = new G4Element("Hydrogen", "H", 1., a);
	a = 12.01 * g / mole;
	G4Element* elC = new G4Element("Carbon", "C", 6., a);
	a = 14.01 * g / mole;
	G4Element* elN = new G4Element("Nitrogen", "N", 7., a);
	a = 16.00 * g / mole;
	G4Element* elO = new G4Element("Oxygen", "O", 8., a);

	double density = 1.290 * mg / cm3;
	G4Material* air = new G4Material("Air", density, 2);
	air->AddElement(elN, 78.084 * perCent);
	air->AddElement(elO, 20.942 * perCent);

	density = 1.19 * g / cm3;
	G4Material* Acrylic = new G4Material("PMMA", density, 3);
	Acrylic->AddElement(elC, 5);
	Acrylic->AddElement(elH, 8);
	Acrylic->AddElement(elO, 2);

	density = 1.290 * mg / cm3;
	G4Material* vac = new G4Material("CustomVacuum", density, 2, kStateGas,
			293.15 * kelvin, atmosphere / ((double) pow(10, (5+3))));
	vac->AddElement(elN, 78.084 * perCent);
	vac->AddElement(elO, 20.942 * perCent);

	G4Box* solidWorld = new G4Box("WorldBox", 0.5 * world_sizeXY,
			0.5 * world_sizeXY, 0.5 * world_sizeZ);
	G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, air,
			"WorldVolume");
	physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld,
			"WorldPhysVolume", 0, false, 0, true);

	double current_z_pos = 0 * mm;
	/// build the filter
	for (auto t : filterMaterials) {
		auto thickness = std::get<1>(t) * mm;
		current_z_pos += thickness / 1.9;

		auto prefix = "Target_" + std::get<0>(t) + "_"
				+ std::to_string(thickness) + "_";

		G4Box* target_box = new G4Box(prefix + "box", filtCollSize * 2 * cm,
				filtCollSize * 2 * cm, thickness / 2);

		std::string material = std::get<0>(t);
		G4Material* target_mat;

		if (material == "pmma") {
			target_mat = Acrylic;
		} else {
			target_mat = nist->FindOrBuildMaterial(material);
		}

		G4LogicalVolume* target_vol = new G4LogicalVolume(target_box,
				target_mat, prefix + "vol");
		G4VPhysicalVolume* target_physvol = new G4PVPlacement(0,
				G4ThreeVector(0, 0, current_z_pos), target_vol,
				prefix + "physVol", logicWorld, false, 0, true);
		current_z_pos += thickness / 1.9;
	}

	/// build the vacuum chamber
	G4Box* vac_box = new G4Box("VacChamberBox", world_sizeXY *0.99,
			world_sizeXY *0.99, distanceSourceFilter * cm);
	G4LogicalVolume* vac_vol = new G4LogicalVolume(vac_box, vac,
			"VacChamberVolume");
	G4VPhysicalVolume* vac_physvol = new G4PVPlacement(0,
			G4ThreeVector(0, 0, -distanceSourceFilter * cm), vac_vol,
			"VacChamberPhysVolume", logicWorld, false, 0, true);

	/// build detector
	G4Box* det_box = new G4Box("DetectorBox", (detSize / 2) * cm,
			(detSize / 2) * cm, .5 * mm);
	G4LogicalVolume* det_vol = new G4LogicalVolume(det_box,
			nist->FindOrBuildMaterial("G4_Si"), "DetectorVolume");
	G4VPhysicalVolume* det_physvol = new G4PVPlacement(0,
			G4ThreeVector(0, 0, distanceFilterDetect * cm), det_vol,
			"DetectorPhysVolume", logicWorld, false, 0, true);

	scoringVolume = det_vol;
}

G4VPhysicalVolume* DetectorConstruction::Construct() {
	return physWorld;
}

DetectorConstruction::~DetectorConstruction() {
	// TODO Auto-generated destructor stub
}

} /* namespace SoEiXRS */
