// BioDetectorConstruction.cc
#include "BioDetectorConstruction.hh"
#include "G4UIdirectory.hh"
#include "G4UIcommand.hh"
#include "G4UIparameter.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "ScoringProfile.hh"
#include "G4Box.hh"
#include "G4Orb.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4EmParameters.hh"
#include "G4NistManager.hh"
#include "G4ProductionCuts.hh"
#include "G4Region.hh"
#include "CellPacker.hh"
#include "DetectorConstruction.hh"
#include <sstream>

BioDetectorConstruction::BioDetectorConstruction(DetectorConstruction* detector)
    : fDetector(detector), fNist(G4NistManager::Instance())
{
    fBioDirectory = std::make_unique<G4UIdirectory>("/bio/");
    fBioDirectory->SetGuidance("Description of the cells and biolayers.");

    fCellDirectory = std::make_unique<G4UIdirectory>("/bio/cell/");
    fCellDirectory->SetGuidance("Description of the cell geometry and chemical composition.");

    fCellGeometryCmd = std::make_unique<G4UIcommand>("/bio/cell/geometry", this);
    fCellGeometryCmd->SetGuidance("Set geometry of the cell (nucleus, cytoplasm, membrane): <r_nucleus> <thickness_c> <thickness_m> <unit_length>");
    fCellGeometryCmd->SetParameter(new G4UIparameter("radius_nucleus", 'd', false));
    fCellGeometryCmd->SetParameter(new G4UIparameter("thickness_cytoplasm", 'd', false));
    fCellGeometryCmd->SetParameter(new G4UIparameter("thickness_membrane", 'd', false));
    auto* cellGeometryUnitParameter = new G4UIparameter("unit_length", 's', false);
    cellGeometryUnitParameter->SetDefaultValue("um");
    fCellGeometryCmd->SetParameter(cellGeometryUnitParameter);
    fCellGeometryCmd->AvailableForStates(G4State_PreInit);

    fCellMaterialsCmd = std::make_unique<G4UIcommand>("/bio/cell/materials", this);
    fCellMaterialsCmd->SetGuidance("Set chemical composition of the cell: <material_n> <material_c> <material_m>");
    fCellMaterialsCmd->SetParameter(new G4UIparameter("material_nucleus", 's', false));
    fCellMaterialsCmd->SetParameter(new G4UIparameter("material_cytoplasm", 's', false));
    fCellMaterialsCmd->SetParameter(new G4UIparameter("material_membrane", 's', false));
    fCellMaterialsCmd->AvailableForStates(G4State_PreInit);

    fCellDensitiesCmd = std::make_unique<G4UIcommand>("/bio/cell/densities", this);
    fCellDensitiesCmd->SetGuidance("Set density of the cell: <density_n> <density_c> <density_m> <unit_density>");
    fCellDensitiesCmd->SetParameter(new G4UIparameter("density_nucleus", 'd', false));
    fCellDensitiesCmd->SetParameter(new G4UIparameter("density_cytoplasm", 'd', false));
    fCellDensitiesCmd->SetParameter(new G4UIparameter("density_membrane", 'd', false));
    auto* densityUnitParameter = new G4UIparameter("unit_density", 's', false);
    densityUnitParameter->SetDefaultValue("g/cm3");
    fCellDensitiesCmd->SetParameter(densityUnitParameter);
    fCellDensitiesCmd->AvailableForStates(G4State_PreInit);

    fPhysicsCellCmd = std::make_unique<G4UIcommand>("/bio/cell/physics", this);
    fPhysicsCellCmd->SetGuidance("Set physics constructor inside cells.");
    auto* physicsCellParameter = new G4UIparameter("physics", 's', false);
    physicsCellParameter->SetDefaultValue("DNA_Opt8");
    physicsCellParameter->SetParameterCandidates("none DNA_Opt0 DNA_Opt2 DNA_Opt4 DNA_Opt6 DNA_Opt8");
    fPhysicsCellCmd->SetParameter(physicsCellParameter);
    fPhysicsCellCmd->AvailableForStates(G4State_PreInit);

    fCellCutCmd = std::make_unique<G4UIcmdWithADoubleAndUnit>("/bio/cell/setCut", this);
    fCellCutCmd->SetGuidance("Set production cut in CellRegion (default: 10 nm).");
    fCellCutCmd->SetParameterName("cellCut", false);
    fCellCutCmd->SetRange("cellCut>0.");
    fCellCutCmd->SetUnitCategory("Length");
    fCellCutCmd->SetDefaultUnit("nm");
    fCellCutCmd->AvailableForStates(G4State_PreInit);

    fBioLayerDirectory = std::make_unique<G4UIdirectory>("/bio/layer/");
    fBioLayerDirectory->SetGuidance("Description of the biolayer: material, cell count, sizes and position.");

    fCreateBioLayerCmd = std::make_unique<G4UIcommand>("/bio/layer/create", this);
    fCreateBioLayerCmd->SetGuidance("Set medium material and cell count: <material> <number_cell>");
    fCreateBioLayerCmd->SetParameter(new G4UIparameter("material", 's', false));
    fCreateBioLayerCmd->SetParameter(new G4UIparameter("number_cell", 'i', false));
    fCreateBioLayerCmd->AvailableForStates(G4State_PreInit);

    fSizeBioLayerCmd = std::make_unique<G4UIcommand>("/bio/layer/size", this);
    fSizeBioLayerCmd->SetGuidance("Set sizes of the biolayer: <size_x> <size_y> <size_z> <unit>");
    fSizeBioLayerCmd->SetParameter(new G4UIparameter("size_x", 'd', false));
    fSizeBioLayerCmd->SetParameter(new G4UIparameter("size_y", 'd', false));
    fSizeBioLayerCmd->SetParameter(new G4UIparameter("size_z", 'd', false));
    auto* sizeUnitParameter = new G4UIparameter("size_unit", 's', false);
    sizeUnitParameter->SetDefaultValue("mm");
    fSizeBioLayerCmd->SetParameter(sizeUnitParameter);
    fSizeBioLayerCmd->AvailableForStates(G4State_PreInit);

    fPosBioLayerCmd = std::make_unique<G4UIcommand>("/bio/layer/pos", this);
    fPosBioLayerCmd->SetGuidance("Set position of the biolayer: <pos_x> <pos_y> <pos_z> <unit>");
    fPosBioLayerCmd->SetParameter(new G4UIparameter("pos_x", 'd', false));
    fPosBioLayerCmd->SetParameter(new G4UIparameter("pos_y", 'd', false));
    fPosBioLayerCmd->SetParameter(new G4UIparameter("pos_z", 'd', false));
    auto* posUnitParameter = new G4UIparameter("pos_unit", 's', false);
    posUnitParameter->SetDefaultValue("mm");
    fPosBioLayerCmd->SetParameter(posUnitParameter);
    fPosBioLayerCmd->AvailableForStates(G4State_PreInit);

    fPhysicsBioLayerCmd = std::make_unique<G4UIcommand>("/bio/layer/physics", this);
    fPhysicsBioLayerCmd->SetGuidance("Set physics constructor for the biolayer medium.");
    auto* physicsParameter = new G4UIparameter("physics", 's', false);
    physicsParameter->SetDefaultValue("none");
    physicsParameter->SetParameterCandidates("none DNA_Opt0 DNA_Opt2 DNA_Opt4 DNA_Opt6 DNA_Opt8");
    fPhysicsBioLayerCmd->SetParameter(physicsParameter);
    fPhysicsBioLayerCmd->AvailableForStates(G4State_PreInit);

    fBioLayerCutCmd = std::make_unique<G4UIcmdWithADoubleAndUnit>("/bio/layer/setCut", this);
    fBioLayerCutCmd->SetGuidance("Set production cut in BioRegion medium (default: 0.5 um).");
    fBioLayerCutCmd->SetParameterName("bioCut", false);
    fBioLayerCutCmd->SetRange("bioCut>0.");
    fBioLayerCutCmd->SetUnitCategory("Length");
    fBioLayerCutCmd->SetDefaultUnit("um");
    fBioLayerCutCmd->AvailableForStates(G4State_PreInit);

    fScoreDirectory = std::make_unique<G4UIdirectory>("/bio/score/");
    fScoreDirectory->SetGuidance("Configuration of the sensitive scoring volume.");

    fScoreTargetCmd = std::make_unique<G4UIcmdWithAString>("/bio/score/target", this);
    fScoreTargetCmd->SetGuidance("Select target volume for ICSD, dose and entrance spectrum scoring: nucleus, cytoplasm, membrane, cell, medium");
    fScoreTargetCmd->SetParameterName("target", false);
    fScoreTargetCmd->SetCandidates("nucleus cytoplasm membrane cell medium");
    fScoreTargetCmd->SetDefaultValue("nucleus");
    fScoreTargetCmd->AvailableForStates(G4State_PreInit);

    fScoreProfileCmd = std::make_unique<G4UIcmdWithAString>("/bio/score/profile", this);
    fScoreProfileCmd->SetGuidance("Select the scoring profile (output groups): minimal, standard, full.");
    fScoreProfileCmd->SetParameterName("profile", false);
    {
        std::string candidates;
        for (const auto& n : ScoringProfileNames()) {
            if (!candidates.empty()) candidates += ' ';
            candidates += n;
        }
        fScoreProfileCmd->SetCandidates(candidates.c_str());
    }
    fScoreProfileCmd->SetDefaultValue("standard");
    fScoreProfileCmd->AvailableForStates(G4State_PreInit);

    fScoreListCmd = std::make_unique<G4UIcommand>("/bio/score/list", this);
    fScoreListCmd->SetGuidance("List the available scoring profiles and mark the active one.");
    fScoreListCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

BioDetectorConstruction::~BioDetectorConstruction() = default;

void BioDetectorConstruction::Construct(G4LogicalVolume* logicalMotherVolume)
{
    if (!HasBioLayer()) {
        G4Exception("BioDetectorConstruction::Construct", "NonAssignmentLayer",
            FatalException, "BioLayer doesn't exist! Create it via '/bio/layer/create' and '/bio/layer/size'.");
        return;
    }
    if (!logicalMotherVolume) {
        G4Exception("BioDetectorConstruction::Construct", "NullMotherVolume",
            FatalException, "Mother volume pointer passed to BioDetectorConstruction is null!");
        return;
    }

    fNist->FindOrBuildMaterial("G4_WATER");

    const auto& layer = fBioLayerConfig;
    const auto& cell  = fCellParameters;
    const bool buildCells = (*layer.nCells > 0);

    // Клеточные сферы нужны только при nCells > 0; в режиме без клеток
    // (nCells == 0) параметры '/bio/cell/*' не требуются.
    if (buildCells && !cell.IsReady()) {
        G4Exception("BioDetectorConstruction::Construct", "IncompleteCellParameters",
            FatalException, "Cell parameters incomplete! Define '/bio/cell/geometry', '/bio/cell/materials' and '/bio/cell/densities'.");
        return;
    }

    // 1. Объем питательного раствора (создается всегда)
    auto* solidMedium = new G4Box("solid_biomedium", 0.5 * layer.size->x(), 0.5 * layer.size->y(), 0.5 * layer.size->z());
    G4Material* matLayer = *layer.material;

    if (layer.physics.has_value() && layer.physics != "none") {
        matLayer = fNist->BuildMaterialWithNewDensity("G4_WATER_LAYER", "G4_WATER", matLayer->GetDensity());
    }
    fLogicMedium = new G4LogicalVolume(solidMedium, matLayer, "logic_biomedium");
    new G4PVPlacement(nullptr, *layer.pos, fLogicMedium, "biomedium", logicalMotherVolume, false, 0, false);

    if (buildCells) {
        // 2. Геометрия составной клетки
        auto* solidNucleus   = new G4Orb("solid_nucleus", *cell.radius_nucleus);
        auto* solidCytoplasm = new G4Orb("solid_cytoplasm", *cell.radius_cytoplasm);
        auto* solidCell      = new G4Orb("solid_membrane", *cell.radius_membrane);

        G4Material* matN = *cell.material_nucleus;
        G4Material* matC = *cell.material_cytoplasm;
        G4Material* matM = *cell.material_membrane;

        if (cell.physics.has_value() && cell.physics != "none") {
            matN = fNist->BuildMaterialWithNewDensity("G4_WATER_N", "G4_WATER", cell.density_nucleus.value_or(matN->GetDensity()));
            matC = fNist->BuildMaterialWithNewDensity("G4_WATER_C", "G4_WATER", cell.density_cytoplasm.value_or(matC->GetDensity()));
            matM = fNist->BuildMaterialWithNewDensity("G4_WATER_M", "G4_WATER", cell.density_membrane.value_or(matM->GetDensity()));
        }

        fLogicNucleus   = new G4LogicalVolume(solidNucleus, matN, "logic_nucleus");
        fLogicCytoplasm = new G4LogicalVolume(solidCytoplasm, matC, "logic_cytoplasm");
        fLogicCell      = new G4LogicalVolume(solidCell, matM, "logic_cell");

        new G4PVPlacement(nullptr, G4ThreeVector(), fLogicNucleus, "phys_nucleus", fLogicCytoplasm, false, 0, false);
        new G4PVPlacement(nullptr, G4ThreeVector(), fLogicCytoplasm, "phys_cytoplasm", fLogicCell, false, 0, false);

        const double cellRadius = static_cast<G4Orb*>(fLogicCell->GetSolid())->GetRadius();
        auto positions = CellPacker::GeneratePositions(*layer.size, cellRadius, *layer.nCells, 5000);
        fActualCellCount = positions.size();

        if (fActualCellCount < static_cast<size_t>(*layer.nCells)) {
            G4ExceptionDescription ed;
            ed << "CellPacker placed only " << fActualCellCount << " of " << *layer.nCells
               << " requested cells. Dose normalization will use " << fActualCellCount << ".";
            G4Exception("BioDetectorConstruction::Construct", "PartialCellPacking", JustWarning, ed);
        } else {
            G4cout << "### [BioDetectorConstruction] Packed cells: " << fActualCellCount << G4endl;
        }

        for (size_t i = 0; i < fActualCellCount; ++i) {
            new G4PVPlacement(nullptr, positions[i], fLogicCell, "phys_cell_in_media", fLogicMedium, false, static_cast<G4int>(i), false);
        }

        // 3. Регион клеток для физики и cut'ов
        auto* cellRegion = new G4Region("CellRegion");
        fLogicCell->SetRegion(cellRegion);
        cellRegion->AddRootLogicalVolume(fLogicCell);
        auto* cutsCell = new G4ProductionCuts();
        cutsCell->SetProductionCut(fCellCut);
        cellRegion->SetProductionCuts(cutsCell);

        if (cell.physics.has_value() && cell.physics != "none") {
            G4EmParameters::Instance()->AddDNA("CellRegion", *cell.physics);
            G4cout << "### [BioDetectorConstruction] Geant4-DNA [" << *cell.physics
                   << "] registered for CellRegion (cut = " << fCellCut / nm << " nm)." << G4endl;
        }
    } else {
        // Режим без клеток: сплошная биосреда без клеточных сфер.
        fActualCellCount = 0;
        if (fScoreTarget != DetectorRole::BioMedium) {
            G4cout << "### [BioDetectorConstruction] nCells = 0: no cells packed; "
                      "scoring target is switched to 'medium'." << G4endl;
            fScoreTarget = DetectorRole::BioMedium;
        } else {
            G4cout << "### [BioDetectorConstruction] nCells = 0: continuous medium only." << G4endl;
        }
    }

    // 4. Регион биосреды для физики и cut'ов (создается всегда)
    auto* bioRegion = new G4Region("BioRegion");
    fLogicMedium->SetRegion(bioRegion);
    bioRegion->AddRootLogicalVolume(fLogicMedium);
    auto* cutsBio = new G4ProductionCuts();
    cutsBio->SetProductionCut(fBioLayerCut);
    bioRegion->SetProductionCuts(cutsBio);

    if (layer.physics.has_value() && layer.physics != "none") {
        G4EmParameters::Instance()->AddDNA("BioRegion", *layer.physics);
        G4cout << "### [BioDetectorConstruction] Geant4-DNA [" << *layer.physics
               << "] registered for BioRegion (cut = " << fBioLayerCut / um << " um)." << G4endl;
    }
}

std::vector<G4LogicalVolume*> BioDetectorConstruction::GetScoringLogicalVolumes() const
{
    std::vector<G4LogicalVolume*> volumes;
    switch (fScoreTarget) {
        case DetectorRole::Nucleus:
            volumes.push_back(fLogicNucleus ? fLogicNucleus : fLogicMedium);
            break;
        case DetectorRole::Cytoplasm:
            volumes.push_back(fLogicCytoplasm ? fLogicCytoplasm : fLogicMedium);
            break;
        case DetectorRole::Membrane:
            volumes.push_back(fLogicCell ? fLogicCell : fLogicMedium);
            break;
        case DetectorRole::WholeCell:
            if (fLogicNucleus)   volumes.push_back(fLogicNucleus);
            if (fLogicCytoplasm) volumes.push_back(fLogicCytoplasm);
            if (fLogicCell)      volumes.push_back(fLogicCell);
            if (volumes.empty()) volumes.push_back(fLogicMedium);
            break;
        case DetectorRole::BioMedium:
            volumes.push_back(fLogicMedium);
            break;
    }
    return volumes;
}

G4double BioDetectorConstruction::GetScoringMass() const
{
    const G4double mediumMass = fLogicMedium ? fLogicMedium->GetMass(false, false) : 0.0;
    switch (fScoreTarget) {
        case DetectorRole::Nucleus:
            return fLogicNucleus ? static_cast<G4double>(fActualCellCount) * fLogicNucleus->GetMass(false, false) : mediumMass;
        case DetectorRole::Cytoplasm:
            return fLogicCytoplasm ? static_cast<G4double>(fActualCellCount) * fLogicCytoplasm->GetMass(false, false) : mediumMass;
        case DetectorRole::Membrane:
            return fLogicCell ? static_cast<G4double>(fActualCellCount) * fLogicCell->GetMass(false, false) : mediumMass;
        case DetectorRole::WholeCell:
            return fLogicCell ? static_cast<G4double>(fActualCellCount) * fLogicCell->GetMass(false, true) : mediumMass;
        case DetectorRole::BioMedium:
            return mediumMass;
    }
    return 0.0;
}

G4String BioDetectorConstruction::GetHitsCollectionName() const noexcept
{
    return G4String(ToHitsCollectionName(fScoreTarget));
}

void BioDetectorConstruction::SetNewValue(G4UIcommand* command, G4String newValue)
{
    if (command == fCellGeometryCmd.get())         SetCellGeometryCmd(newValue);
    else if (command == fCellMaterialsCmd.get())   SetCellMaterialCmd(newValue);
    else if (command == fCellDensitiesCmd.get())   SetCellDensityCmd(newValue);
    else if (command == fCreateBioLayerCmd.get())  CreateBioLayerCmd(newValue);
    else if (command == fSizeBioLayerCmd.get())    SetBioLayerSizeCmd(newValue);
    else if (command == fPosBioLayerCmd.get())     SetBioLayerPosCmd(newValue);
    else if (command == fPhysicsBioLayerCmd.get()) SetPhysicsBioLayer(newValue);
    else if (command == fPhysicsCellCmd.get())     SetPhysicsCell(newValue);
    else if (command == fScoreTargetCmd.get())     SetScoreTargetCmd(newValue);
    else if (command == fScoreProfileCmd.get())    SetScoreProfileCmd(newValue);
    else if (command == fScoreListCmd.get()) {
        G4cout << "[bio/score] Available scoring profiles:" << G4endl;
        for (const auto& n : ScoringProfileNames()) {
            G4cout << "  " << ScoringProfileDescription(n)
                   << (n == GetActiveScoringProfile().name ? "  <-- active" : "") << G4endl;
        }
    }
    else if (command == fCellCutCmd.get())         fCellCut = fCellCutCmd->GetNewDoubleValue(newValue);
    else if (command == fBioLayerCutCmd.get())     fBioLayerCut = fBioLayerCutCmd->GetNewDoubleValue(newValue);
}

void BioDetectorConstruction::SetScoreTargetCmd(std::string_view arg)
{
    if (arg == "nucleus")        fScoreTarget = DetectorRole::Nucleus;
    else if (arg == "cytoplasm") fScoreTarget = DetectorRole::Cytoplasm;
    else if (arg == "membrane")  fScoreTarget = DetectorRole::Membrane;
    else if (arg == "cell")      fScoreTarget = DetectorRole::WholeCell;
    else if (arg == "medium")    fScoreTarget = DetectorRole::BioMedium;
    else {
        G4Exception("BioDetectorConstruction::SetScoreTargetCmd", "InvalidTarget",
                    FatalException, ("Unknown scoring target: " + std::string(arg) + ". Valid: nucleus, cytoplasm, membrane, cell, medium").c_str());
    }
}

void BioDetectorConstruction::SetScoreProfileCmd(std::string_view arg)
{
    SetActiveScoringProfile(arg);
}

void BioDetectorConstruction::SetCellGeometryCmd(std::string_view arg)
{
    if (fCellParameters.radius_nucleus.has_value() &&
        fCellParameters.radius_cytoplasm.has_value() &&
        fCellParameters.radius_membrane.has_value())
    {
        G4Exception("BioDetectorConstruction::SetCellGeometryCmd", "NewGeometryCell",
                    JustWarning, "Cell's geometry is already fixed.");
    }

    std::istringstream stream{std::string(arg)};
    double radius_n{0.}, thickness_c{0.}, thickness_m{0.};
    std::string unit_length;

    if (!(stream >> radius_n >> thickness_c >> thickness_m >> unit_length)
        || radius_n < 0. || thickness_c < 0. || thickness_m < 0.)
    {
        G4Exception("BioDetectorConstruction::SetCellGeometryCmd", "NonPhysicalValues",
                    FatalException, "Radius or thickness cannot be negative or invalidly formatted!");
        return;
    }

    const double length_scale = G4UIcommand::ValueOf(unit_length.c_str());
    if (length_scale <= 0.0) {
        G4Exception("BioDetectorConstruction::SetCellGeometryCmd", "InvalidUnits",
                    FatalException, "Unknown or zero length unit specified!");
        return;
    }

    const double radius_nucleus   = radius_n * length_scale;
    const double radius_cytoplasm = radius_nucleus + thickness_c * length_scale;
    const double radius_cell      = radius_cytoplasm + thickness_m * length_scale;

    fCellParameters.radius_nucleus   = radius_nucleus;
    fCellParameters.radius_cytoplasm = radius_cytoplasm;
    fCellParameters.radius_membrane  = radius_cell;
}

void BioDetectorConstruction::SetCellDensityCmd(std::string_view arg)
{
    if (fCellParameters.density_nucleus.has_value() &&
        fCellParameters.density_cytoplasm.has_value() &&
        fCellParameters.density_membrane.has_value())
    {
        G4Exception("BioDetectorConstruction::SetCellDensityCmd", "NewDensityCell",
                    JustWarning, "Cell's densities are already fixed.");
    }

    std::istringstream stream{std::string(arg)};
    double density_n{0.}, density_c{0.}, density_m{0.};
    std::string unit_density;

    if (!(stream >> density_n >> density_c >> density_m >> unit_density)
        || density_n < 0. || density_c < 0. || density_m < 0.)
    {
        G4Exception("BioDetectorConstruction::SetCellDensityCmd", "NonPhysicalValues",
                    FatalException, "Cell densities cannot be negative!");
        return;
    }

    const double density_scale = G4UIcommand::ValueOf(unit_density.c_str());
    if (density_scale <= 0.0) {
        G4Exception("BioDetectorConstruction::SetCellDensityCmd", "InvalidUnits",
                    FatalException, "Unknown or zero density unit specified!");
        return;
    }

    fCellParameters.density_nucleus   = density_n * density_scale;
    fCellParameters.density_cytoplasm = density_c * density_scale;
    fCellParameters.density_membrane  = density_m * density_scale;
}

void BioDetectorConstruction::SetCellMaterialCmd(std::string_view arg)
{
    if (fCellParameters.material_nucleus.has_value() &&
        fCellParameters.material_cytoplasm.has_value() &&
        fCellParameters.material_membrane.has_value())
    {
        G4Exception("BioDetectorConstruction::SetCellMaterialCmd", "NewMaterialCell",
                    JustWarning, "Cell's materials are already fixed.");
    }

    std::istringstream stream{std::string(arg)};
    std::string material_name_n, material_name_c, material_name_m;

    if (!(stream >> material_name_n >> material_name_c >> material_name_m)) {
        G4Exception("BioDetectorConstruction::SetCellMaterialCmd", "MaterialNames",
                    FatalException, "Syntax error reading cell material names!");
        return;
    }

    G4Material* material_n = fNist->FindOrBuildMaterial(material_name_n);
    G4Material* material_c = fNist->FindOrBuildMaterial(material_name_c);
    G4Material* material_m = fNist->FindOrBuildMaterial(material_name_m);

    if (!(material_n && material_c && material_m)) {
        G4Exception("BioDetectorConstruction::SetCellMaterialCmd", "InvalidBioMaterial",
                    FatalException, "One of the specified cell materials doesn't exist in the NIST/materials table!");
        return;
    }

    fCellParameters.material_nucleus   = material_n;
    fCellParameters.material_cytoplasm = material_c;
    fCellParameters.material_membrane  = material_m;
}

void BioDetectorConstruction::CreateBioLayerCmd(std::string_view arg)
{
    if (fBioLayerConfig.material.has_value() && fBioLayerConfig.nCells.has_value())
    {
        G4Exception("BioDetectorConstruction::CreateBioLayerCmd", "BioParam",
                    JustWarning, "Biolayer parameters are already fixed.");
    }

    std::istringstream stream{std::string(arg)};
    std::string material_name;
    long long int number_cells{0};

    if (!(stream >> material_name >> number_cells) || number_cells < 0) {
        G4Exception("BioDetectorConstruction::CreateBioLayerCmd", "BioLayerInit",
                    FatalException, "Syntax error or negative cell count! Usage: /bio/layer/create <material> <number_cell>");
        return;
    }

    G4Material* material = fNist->FindOrBuildMaterial(material_name);
    if (!material) {
        G4Exception("BioDetectorConstruction::CreateBioLayerCmd", "InvalidBioMaterial",
                    FatalException, ("Material '" + material_name + "' for the biolayer doesn't exist!").c_str());
        return;
    }

    fBioLayerConfig.material = material;
    fBioLayerConfig.nCells   = number_cells;
}

void BioDetectorConstruction::SetBioLayerSizeCmd(std::string_view arg)
{
    if (fBioLayerConfig.size.has_value()) {
        G4Exception("BioDetectorConstruction::SetBioLayerSizeCmd", "BioSizeExist",
                    JustWarning, "Biolayer sizes are already configured.");
    }

    std::istringstream stream{std::string(arg)};
    double size_x{0.}, size_y{0.}, size_z{0.};
    std::string size_unit;

    if (!(stream >> size_x >> size_y >> size_z >> size_unit) ||
          size_x < 0. || size_y < 0. || size_z < 0.)
    {
        G4Exception("BioDetectorConstruction::SetBioLayerSizeCmd", "BioSize",
                    FatalException, "Biolayer sizes cannot be negative or invalidly formatted!");
        return;
    }

    const double size_scale = G4UIcommand::ValueOf(size_unit.c_str());
    if (size_scale <= 0.0) {
        G4Exception("BioDetectorConstruction::SetBioLayerSizeCmd", "InvalidUnits",
                    FatalException, "Unknown or zero size unit specified for biolayer!");
        return;
    }

    fBioLayerConfig.size = G4ThreeVector(size_x * size_scale, size_y * size_scale, size_z * size_scale);
}

void BioDetectorConstruction::SetBioLayerPosCmd(std::string_view arg)
{
    if (fBioLayerConfig.pos.has_value()) {
        G4Exception("BioDetectorConstruction::SetBioLayerPosCmd", "BioPosExist",
                    JustWarning, "Biolayer position is already configured.");
    }

    std::istringstream stream{std::string(arg)};
    double pos_x{0.}, pos_y{0.}, pos_z{0.};
    std::string pos_unit;

    if (!(stream >> pos_x >> pos_y >> pos_z >> pos_unit)) {
        G4Exception("BioDetectorConstruction::SetBioLayerPosCmd", "BioPos",
                    FatalException, "Syntax error reading biolayer position!");
        return;
    }

    const double pos_scale = G4UIcommand::ValueOf(pos_unit.c_str());
    if (pos_scale <= 0.0) {
        G4Exception("BioDetectorConstruction::SetBioLayerPosCmd", "InvalidUnits",
                    FatalException, "Unknown or zero position unit specified for biolayer!");
        return;
    }

    fBioLayerConfig.pos = G4ThreeVector(pos_x * pos_scale, pos_y * pos_scale, pos_z * pos_scale);
}

void BioDetectorConstruction::SetPhysicsBioLayer(std::string_view arg)
{
    if (fBioLayerConfig.physics.has_value()) {
        G4Exception("BioDetectorConstruction::SetPhysicsBioLayer", "PhysListExist",
                    JustWarning, "Biolayer physics constructor is already set.");
    }

    std::istringstream stream{std::string(arg)};
    std::string physics;

    if (!(stream >> physics)) {
        G4Exception("BioDetectorConstruction::SetPhysicsBioLayer", "PhysListRead",
                    FatalException, "Syntax error reading physics constructor name for biolayer!");
        return;
    }

    fBioLayerConfig.physics = physics;
}

void BioDetectorConstruction::SetPhysicsCell(std::string_view arg)
{
    if (fCellParameters.physics.has_value()) {
        G4Exception("BioDetectorConstruction::SetPhysicsCell", "PhysListExistCell",
                    JustWarning, "Cell physics constructor is already set.");
    }

    std::istringstream stream{std::string(arg)};
    std::string physics;

    if (!(stream >> physics)) {
        G4Exception("BioDetectorConstruction::SetPhysicsCell", "PhysListReadCell",
                    FatalException, "Syntax error reading physics constructor name for cell!");
        return;
    }

    fCellParameters.physics = physics;
}