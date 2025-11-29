#include "PMDetectorConstruction.hh"

PMDetectorConstruction::PMDetectorConstruction()
{
}

PMDetectorConstruction::~PMDetectorConstruction()
{
}

G4VPhysicalVolume *PMDetectorConstruction::Construct()
{
    G4bool checkOverlaps = true;

    G4NistManager *nist = G4NistManager::Instance();
    G4Material *worldMat = nist->FindOrBuildMaterial("G4_AIR");
    G4Material *aluminumMat = nist->FindOrBuildMaterial("G4_Al");
    // OBS! Consider changing to a scintillator material later
    G4Material *detMat = nist->FindOrBuildMaterial("G4_SODIUM_IODIDE"); // Detector material: Sodium Iodide (NaI)

    G4double xWorld = 1. * m;
    G4double yWorld = 1. * m;
    G4double zWorld = 1. * m;

    G4Box *solidWorld = new G4Box("solidWorld", 0.5 * xWorld, 0.5 * yWorld, 0.5 * zWorld);
    G4LogicalVolume *logicWorld = new G4LogicalVolume(solidWorld, worldMat, "logicWorld");
    G4VPhysicalVolume *physWorld = new G4PVPlacement(0,                         // no rotation
                                                     G4ThreeVector(0., 0., 0.), // at (0,0,0)
                                                     logicWorld,                // its logical volume
                                                     "physWorld",               // its name
                                                     0,                         // its mother volume
                                                     false,                     // no boolean operation
                                                     0,                         // copy number
                                                     checkOverlaps);            // checking overlaps

    // Aluminum block
    G4double aluminumThickness = 50. * micrometer; // z length
    G4double aluminumSize = 5. * cm;               // x and y lengths
    G4Box *solidAluminum = new G4Box("solidAluminum", 0.5 * aluminumSize, 0.5 * aluminumSize, 0.5 * aluminumThickness);
    G4LogicalVolume *logicAluminum = new G4LogicalVolume(solidAluminum, aluminumMat, "logicAluminum");
    G4VPhysicalVolume *physAluminum = new G4PVPlacement(0,                              // no rotation
                                                        G4ThreeVector(0., 0., 5. * cm), // position
                                                        logicAluminum,                  // its logical volume
                                                        "physaluminum",                 // its name
                                                        logicWorld,                     // its mother volume
                                                        false,                          // no boolean operation
                                                        0,                              // copy number
                                                        checkOverlaps);                 // checking overlaps

    G4VisAttributes *aluminumVisAtt = new G4VisAttributes(G4Color(1., 0., 0., 0.5)); // Red color with 50% transparency
    aluminumVisAtt->SetForceSolid(true);
    logicAluminum->SetVisAttributes(aluminumVisAtt);

    // Detector block
    G4double detectorSize = 5. * cm; // x, y and y lengths
    G4Box *solidDetector = new G4Box("solidDetector", 0.5 * detectorSize, 0.5 * detectorSize, 0.5 * detectorSize);
    logicDetector = new G4LogicalVolume(solidDetector, detMat, "logicDetector");
    G4VPhysicalVolume *physDete = new G4PVPlacement(0,                                // no rotation
                                                    G4ThreeVector(0., 0., 10.5 * cm), // position
                                                    logicDetector,                    // its logical volume
                                                    "physDetector",                   // its name
                                                    logicWorld,                       // its mother volume
                                                    false,                            // no boolean operation
                                                    0,                                // copy number
                                                    checkOverlaps);                   // checking overlaps
    G4VisAttributes *detVisAtt = new G4VisAttributes(G4Color(1., 1., 0., 0.5));       // yellow color with 50% transparency
    detVisAtt->SetForceSolid(true);
    logicDetector->SetVisAttributes(detVisAtt);

    return physWorld;
}

void PMDetectorConstruction::ConstructSDandField()
{
    PMSensitiveDetector *sensDet = new PMSensitiveDetector("SensitiveDetector");
    logicDetector->SetSensitiveDetector(sensDet);
    G4SDManager::GetSDMpointer()->AddNewDetector(sensDet);
}