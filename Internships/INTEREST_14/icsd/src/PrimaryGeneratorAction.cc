//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
// This example is provided by the Geant4-DNA collaboration
// Any report or published results obtained using the Geant4-DNA software
// shall cite the following Geant4-DNA collaboration publication:
// Med. Phys. 37 (2010) 4692-4708
// J. Comput. Phys. 274 (2014) 841-882
// The Geant4-DNA web site is available at http://geant4-dna.org
//
//
/// \file PrimaryGeneratorAction.cc
/// \brief Implementation of the PrimaryGeneratorAction class

#include "PrimaryGeneratorAction.hh"

#include "G4LogicalVolumeStore.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4Orb.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

PrimaryGeneratorAction::PrimaryGeneratorAction() : G4VUserPrimaryGeneratorAction()
{
  G4int n_particle = 1;
  fpParticleGun = new G4ParticleGun(n_particle);

  // default gun parameters
  fpParticleGun->SetParticleEnergy(500. * eV);
  fpParticleGun->SetParticleMomentumDirection(G4ThreeVector(1., 0., 0.));
  fpParticleGun->SetParticlePosition(G4ThreeVector(-1.15 * nanometer, 0., 0));
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fpParticleGun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  auto* targetLogical = G4LogicalVolumeStore::GetInstance()->GetVolume("Target");
  if (!targetLogical) {
      G4Exception("PrimaryGenerator", "001", FatalException, "Target volume not found!");
  }

  G4VSolid* targetSolid = targetLogical->GetSolid();
  G4double radius = 0;
  if (auto* orb = dynamic_cast<G4Orb*>(targetSolid)) {
      radius = orb->GetRadius();
  } else if (auto* tubs = dynamic_cast<G4Tubs*>(targetSolid)) {
      radius = tubs->GetOuterRadius();
  }

  G4double xStart = -(radius + 15.0*nm); 
  fpParticleGun->SetParticlePosition(G4ThreeVector(xStart, 0., 0.));
  fpParticleGun->SetParticleMomentumDirection(G4ThreeVector(1., 0., 0.));
  fpParticleGun->GeneratePrimaryVertex(anEvent);
}
