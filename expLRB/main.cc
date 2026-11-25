#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "RunAction.hh"
#include "RunMessenger.hh"
#include "ScoringProfile.hh"

#include "G4RunManagerFactory.hh"
#include "G4RunManager.hh"
#include "G4SteppingVerbose.hh"
#include "G4Types.hh"
#include "globals.hh"
#include "G4UImanager.hh"
#include "G4UIcommand.hh"
#include "Randomize.hh"
#ifdef EXP_LRB_WITH_UIVIS
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#endif

#include <cstdlib>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool HasDisplay() {
    const char* display = std::getenv("DISPLAY");
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    return (display && *display) || (wayland && *wayland);
}

bool IsUnsignedInteger(const std::string& s) {
    return !s.empty() && s.find_first_not_of("0123456789") == std::string::npos;
}

// Last "/bio/score/profile <name>" directive in a macro file (empty if none).
// The profile must be known before the RunManager builds its user actions, so
// main() reads it here instead of relying on the command being executed later.
std::string ScanMacroProfile(const std::string& path) {
    std::ifstream in(path);
    if (!in) return {};
    const std::string key = "/bio/score/profile";
    std::string line, found;
    while (std::getline(in, line)) {
        const auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        if (line.compare(first, key.size(), key) != 0) continue;
        std::istringstream ss(line.substr(first + key.size()));
        std::string token;
        if (ss >> token) found = token;
    }
    return found;
}

}  // namespace

int main(int argc, char** argv)
{
    std::string macroFile;
    std::string outFile;
    std::string profileName;
    long        seedValue{0};
    bool        seedProvided{false};
    long        eventsValue{0};
    bool        eventsProvided{false};
    int         nThreads{0};
    std::vector<std::string> positional;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto nextValue = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                G4cerr << "=== [expLRB] Missing value for " << name << " ===" << G4endl;
                std::exit(1);
            }
            return std::string(argv[++i]);
        };

        if (arg == "--mac" || arg == "-m")          macroFile  = nextValue("--mac");
        else if (arg == "--out" || arg == "-o")     outFile    = nextValue("--out");
        else if (arg == "--profile" || arg == "-p") profileName = nextValue("--profile");
        else if (arg == "--threads" || arg == "-t") nThreads   = G4UIcommand::ConvertToInt(nextValue("--threads").c_str());
        else if (arg == "--seed" || arg == "-s")  { seedValue  = std::stol(nextValue("--seed")); seedProvided = true; }
        else if (arg == "--events" || arg == "-n") {
            eventsValue = std::stol(nextValue("--events"));
            if (eventsValue <= 0) {
                G4cerr << "=== [expLRB] --events must be a positive integer ===" << G4endl;
                std::exit(1);
            }
            eventsProvided = true;
        }
        else if (!arg.empty() && arg[0] == '-') {
            G4cerr << "=== [expLRB] Unknown option: " << arg << " ===" << G4endl;
            std::exit(1);
        }
        else positional.push_back(arg);
    }

    // Обратная совместимость: позиционные аргументы <macro> [threads]
    if (macroFile.empty() && !positional.empty()) macroFile = positional[0];
    if (nThreads <= 0 && positional.size() >= 2 && IsUnsignedInteger(positional[1])) {
        nThreads = G4UIcommand::ConvertToInt(positional[1].c_str());
    }

    // Автоматический независимый RNG-сид (или явный --seed)
    const long seed = seedProvided
        ? seedValue
        : [] {
              std::random_device rd;
              std::uniform_int_distribution<long> dist(1, 2000000000L);
              return dist(rd);
          }();
    G4Random::setTheSeed(seed);
    G4cout << "=== [expLRB] Engine Seed automatically set to: " << seed << " ===" << G4endl;

    G4int precision = 4;
    G4SteppingVerbose::UseBestUnit(precision);

    // Scoring profile must be resolved BEFORE the RunManager builds its user
    // actions: G4MTRunManager::SetUserInitialization() calls BuildForMaster()
    // immediately, creating the master ntuple/histogram schema before any
    // macro command runs. CLI --profile wins; otherwise scan the macro for
    // /bio/score/profile so config-driven macros stay the source of truth.
    std::string resolvedProfile = profileName;
    if (resolvedProfile.empty() && !macroFile.empty()) {
        resolvedProfile = ScanMacroProfile(macroFile);
    }
    if (!resolvedProfile.empty()) {
        if (!ScoringProfileExists(resolvedProfile)) {
            G4cerr << "=== [expLRB] Unknown scoring profile: " << resolvedProfile
                   << " (valid:";
            for (const auto& n : ScoringProfileNames()) G4cerr << ' ' << n;
            G4cerr << ") ===" << G4endl;
            return 1;
        }
        ForceActiveScoringProfile(resolvedProfile);
    }

    // --out приоритетнее команды /bio/output/fileName в макросе
    if (!outFile.empty()) {
        RunAction::ForceOutputFileName(outFile);
    }

    // --events приоритетнее команды /bio/run/setEvents в макросе
    if (eventsProvided) {
        RunMessenger::ForcePlannedEvents(static_cast<G4int>(eventsValue));
    }

    // Force the requested thread count before the RunManager is built, so it is not
    // overridden by /run/numberOfThreads in the macro (CLI --threads wins). Geant4's
    // G4FORCENUMBEROFTHREADS takes precedence over the UI command.
    if (nThreads > 0) {
        setenv("G4FORCENUMBEROFTHREADS", std::to_string(nThreads).c_str(), 1);
    }

    auto* runManager = G4RunManagerFactory::CreateRunManager();

    if (nThreads > 0) {
        runManager->SetNumberOfThreads(nThreads);
        G4cout << "===== Simulation run with " << runManager->GetNumberOfThreads()
               << " threads =====" << G4endl;
    }

    auto* detector = new DetectorConstruction();
    runManager->SetUserInitialization(detector);
    runManager->SetUserInitialization(new PhysicsList());
    runManager->SetUserInitialization(new ActionInitialization(detector));

    auto* UImanager = G4UImanager::GetUIpointer();

    // Batch, если задан макрос ИЛИ графическое окружение недоступно
    const bool batch = !macroFile.empty() || !HasDisplay();

    if (!batch) {
#ifdef EXP_LRB_WITH_UIVIS
        auto* ui = new G4UIExecutive(argc, argv);
        auto* visManager = new G4VisExecutive("Quiet");
        visManager->Initialize();

        UImanager->ApplyCommand("/control/execute vis.mac");
        ui->SessionStart();

        delete ui;
        delete visManager;
#else
        G4cerr << "=== [expLRB] UI/Vis were disabled at build time; interactive mode unavailable. ===" << G4endl;
#endif
    }
    else {
        if (macroFile.empty()) {
            G4cerr << "=== [expLRB] Batch mode requested but no macro given. Use --mac <file.mac>. ===" << G4endl;
            delete runManager;
            return 1;
        }
        G4cout << "=== [expLRB] Batch mode: executing " << macroFile << " ===" << G4endl;
        UImanager->ApplyCommand("/control/execute " + G4String(macroFile));
    }

    delete runManager;
    return 0;
}
