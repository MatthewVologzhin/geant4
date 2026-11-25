// EnergyLossMessenger.cc
#include "EnergyLossMessenger.hh"

#include "G4EmCalculator.hh"
#include "G4Exception.hh"
#include "G4IonTable.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4ParticleDefinition.hh"
#include "G4Region.hh"
#include "G4RegionStore.hh"
#include "G4SystemOfUnits.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcommand.hh"
#include "G4UIdirectory.hh"
#include "G4UnitsTable.hh"

#include <cfloat>
#include <sstream>
#include <string>

namespace {

const G4Material* FindMaterial(const G4String& name)
{
    const G4Material* mat = G4Material::GetMaterial(name, false);
    if (!mat) {
        mat = G4NistManager::Instance()->FindOrBuildMaterial(name, false);
    }
    return mat;
}

void PrintDedx(const G4Material* mat, const G4ParticleDefinition* ion,
               G4double energy, G4double thickness, const G4String& regionName)
{
    G4EmCalculator calc;

    const G4double dedxE = calc.ComputeElectronicDEDX(energy, ion, mat, DBL_MAX);
    const G4double dedxN = calc.ComputeNuclearDEDX(energy, ion, mat);
    const G4double dedxT = calc.ComputeTotalDEDX(energy, ion, mat, DBL_MAX);

    auto toKeVperUm = [](G4double dedx) { return dedx * mm / (keV / um); };

    G4cout << "=== [expLRB] G4EmCalculator dE/dx ===" << G4endl;
    G4cout << "  particle: " << ion->GetParticleName()
           << "  (Z=" << ion->GetAtomicNumber() << ", A=" << ion->GetAtomicMass()
           << ", q=" << ion->GetPDGCharge() / eplus << "e)" << G4endl;
    G4cout << "  material: " << mat->GetName()
           << "  rho=" << mat->GetDensity() / (g / cm3) << " g/cm3" << G4endl;
    G4cout << "  kinetic energy: " << energy / MeV << " MeV" << G4endl;
    G4cout << "  electronic dE/dx: " << toKeVperUm(dedxE) << " keV/um" << G4endl;
    G4cout << "  nuclear    dE/dx: " << toKeVperUm(dedxN) << " keV/um" << G4endl;
    G4cout << "  total      dE/dx: " << toKeVperUm(dedxT) << " keV/um" << G4endl;

    if (!regionName.empty()) {
        const G4Region* reg = G4RegionStore::GetInstance()->GetRegion(regionName, false);
        if (reg) {
            const G4double dedxReg = calc.GetDEDX(energy, ion, mat, reg);
            G4cout << "  region " << regionName << " (table) dE/dx: "
                   << toKeVperUm(dedxReg) << " keV/um" << G4endl;
        } else {
            G4cout << "  region " << regionName << ": NOT FOUND" << G4endl;
        }
    }

    if (thickness > 0.0) {
        // Numerically integrate the total stopping power along the slab.
        const G4double dx = thickness / 1000.0;
        G4double E = energy;
        G4double remaining = thickness;
        G4double deposit = 0.0;
        while (remaining > 0.0 && E > 0.0) {
            const G4double step = std::min(dx, remaining);
            const G4double dedx = calc.ComputeTotalDEDX(E, ion, mat, DBL_MAX);
            const G4double dE = std::min(dedx * step, E);
            deposit += dE;
            E -= dE;
            remaining -= step;
        }
        G4cout << "  integrated deposit over " << thickness / um << " um: "
               << deposit / MeV << " MeV  (exit E = " << E / MeV << " MeV)" << G4endl;
    }
    G4cout << "=== end G4EmCalculator ===" << G4endl;
}

}  // namespace

EnergyLossMessenger::EnergyLossMessenger()
{
    fDir = std::make_unique<G4UIdirectory>("/expLRB/dedx/");
    fDir->SetGuidance("Read-only analytic stopping power diagnostics (G4EmCalculator).");

    fPrintCmd = std::make_unique<G4UIcmdWithAString>("/expLRB/dedx/print", this);
    fPrintCmd->SetGuidance("Print analytic EM dE/dx for an ion.");
    fPrintCmd->SetGuidance("usage: /expLRB/dedx/print <material> <Z> <A> <energy> <eUnit> [thickness tUnit]");
    fPrintCmd->SetGuidance("example: /expLRB/dedx/print G4_WATER 10 20 800 MeV 10 um");
    fPrintCmd->SetParameterName("spec", false);
    fPrintCmd->AvailableForStates(G4State_Idle);
}

EnergyLossMessenger::~EnergyLossMessenger() = default;

void EnergyLossMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
    if (command != fPrintCmd.get()) return;

    std::istringstream ss(newValue);
    std::string materialName, eUnit, tUnit;
    G4int Z = 0, A = 0;
    G4double eRaw = 0.0, tRaw = 0.0;

    if (!(ss >> materialName >> Z >> A >> eRaw >> eUnit)) {
        G4Exception("EnergyLossMessenger::SetNewValue", "BadArguments", FatalException,
                    "Expected: <material> <Z> <A> <energy> <eUnit> [thickness tUnit] [region name]");
        return;
    }

    G4double thickness = 0.0;
    G4String regionName;
    std::string token;
    while (ss >> token) {
        if (token == "region") {
            ss >> regionName;
        } else if (ss >> tUnit) {
            thickness = G4UIcommand::ConvertToDouble(token.c_str()) *
                        G4UIcommand::ValueOf(tUnit.c_str());
        }
    }

    const G4double energy = eRaw * G4UIcommand::ValueOf(eUnit.c_str());
    if (energy <= 0.0) {
        G4Exception("EnergyLossMessenger::SetNewValue", "BadEnergy", FatalException,
                    "Energy must be positive.");
        return;
    }

    const G4Material* mat = FindMaterial(materialName);
    if (!mat) {
        G4Exception("EnergyLossMessenger::SetNewValue", "NoMaterial", FatalException,
                    ("Unknown material: " + materialName).c_str());
        return;
    }

    const G4ParticleDefinition* ion =
        G4IonTable::GetIonTable()->GetIon(Z, A, 0.0, '\0');
    if (!ion) {
        G4Exception("EnergyLossMessenger::SetNewValue", "NoIon", FatalException,
                    "Failed to build the requested ion.");
        return;
    }

    PrintDedx(mat, ion, energy, thickness, regionName);
}
