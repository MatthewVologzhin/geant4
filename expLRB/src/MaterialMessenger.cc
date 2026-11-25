#include "G4Material.hh"
#include "G4NistManager.hh"

#include "MaterialMessenger.hh"

MaterialMessenger::MaterialMessenger(DetectorConstruction* detector) : fDetector(detector)
{
    fMaterialDir = std::make_unique<G4UIdirectory>("/expLRB/material/");
    fMaterialDir->SetGuidance("Create a new complex material.");

    fCreateMaterialCmd = std::make_unique<G4UIcommand>("/expLRB/material/create", this);
    fCreateMaterialCmd->SetGuidance("Initialize a new compound/mixture.");
    fCreateMaterialCmd->SetParameter(new G4UIparameter("name", 's', false));
    fCreateMaterialCmd->SetParameter(new G4UIparameter("density", 'd', false));
    fCreateMaterialCmd->SetParameter(new G4UIparameter("unit", 's', false));
    fCreateMaterialCmd->SetParameter(new G4UIparameter("components", 'i', false));
    fCreateMaterialCmd->AvailableForStates(G4State_Idle, G4State_PreInit);

    fAddMaterialCmd = std::make_unique<G4UIcommand>("/expLRB/material/add", this);
    fAddMaterialCmd->SetGuidance("Adding a new component/element in initialisated material.");
    fAddMaterialCmd->SetParameter(new G4UIparameter("compName", 's', false));
    fAddMaterialCmd->SetParameter(new G4UIparameter("frac", 'd', false));
    fAddMaterialCmd->AvailableForStates(G4State_Idle, G4State_PreInit);
}

void MaterialMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
    if (command == fCreateMaterialCmd.get()) HandleCreateCommand(newValue);
    else if (command == fAddMaterialCmd.get()) HandleAddCommand(newValue);
}

void MaterialMessenger::HandleCreateCommand(std::string_view args)
{
    if (fActiveContext.has_value())
    {
        G4Exception("MaterialMessenger::HandleCreateCommand", "MatInProgress", 
                    FatalException, 
                    "Previous material was not finished! Complete adding components first.");
        return;
    }

    std::istringstream stream{std::string(args)};
    std::string name;
    double density{0.0};
    std::string unit;
    int nComponents{0};

    if (!(stream >> name >> density >> unit >> nComponents) || nComponents <= 0)
    {
        G4Exception("MaterialMessenger::HandleCreateCommand", "InvalidArgs",
                    FatalException,
                    "Syntax error. Usage: /expLRB/material/create <name> <density> <unit> <nComponents>\n Also check that nComponents > 0!");
        return;
    }
    
    const double unitScale = G4UIcommand::ValueOf(unit.c_str());
    if (unitScale <= 0.0)
    {
        G4Exception("MaterialMessenger::HandleCreateCommand", "InvalidUnits",
                    FatalException,
                    "Unknown or zero density unit!");
        return;
    }

    double finalDensity = density * unitScale;
    G4Material* newMaterial = new G4Material(name, finalDensity, nComponents);

    fActiveContext.emplace(ActiveMaterialContext{
        newMaterial,
        nComponents,
        0,
        0.0
    });

    G4cout << "[MaterialMessenger] " << name << " was initialized! It awaiting n=" 
    << nComponents << " components!" << G4endl;
}

void MaterialMessenger::HandleAddCommand(std::string_view args)
{
    if (!fActiveContext.has_value())
    {
        G4Exception("MaterialMessenger::HandleAddCommand", "NoActiveContext",
                    FatalException, "Material isn't created!\nThe first create it: /expLRB/material/create <name> <density> <unit> <nComps>");
        return;
    }

    std::istringstream stream{std::string(args)};

    std::string compName;
    double fraction{0.0};

    if (!(stream >> compName >> fraction) || fraction <= 0 || fraction > 1)
    {
        G4Exception("MaterialMessenger::HandleAddCommand", "InvalidArgs",
                    FatalException, "Syntax error. Usage: /expLRB/material/add <name> <frac>\n Also check that 0 < fraction <= 1 !");
        return;
    }

    ActiveMaterialContext& ctx = *fActiveContext;
    G4NistManager* nist = G4NistManager::Instance();

    if (auto* subMat = nist->FindOrBuildMaterial(compName); subMat != nullptr)
    {
        ctx.material->AddMaterial(subMat, fraction);
    }
    else if (auto* subEl = nist->FindOrBuildElement(compName); subEl != nullptr)
    {
        ctx.material->AddElement(subEl, fraction);
    }
    else
    {
        const std::string err = "The components with the name - \"" + compName + "\" doesn't exist!";
        G4Exception("MaterialMessenger::HandleAddCommand", "ComponentNotFound",
                    FatalException, err.c_str());
    }

    ctx.addedComponents++;
    ctx.fraction += fraction;

    G4cout << "  -> Added " << compName << " (" << fraction << ") [" 
           << ctx.addedComponents << "/" << ctx.targetComponents << "]" << G4endl;
    
    if (ctx.addedComponents == ctx.targetComponents)
    {
        if (std::fabs(ctx.fraction - 1.0) > ctx.targetComponents * DBL_EPSILON)
        {
            G4Exception("MaterialMessenger::HandleAddCommand", "InvalidFraction",
                        JustWarning, "Summary fraction isn't equal 1.0!");
        }
        G4cout << "[MaterialMessenger] Material '" << ctx.material->GetName() 
               << "' constructed successfully!" << G4endl;
        fActiveContext.reset();
    }
};