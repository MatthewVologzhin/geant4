#pragma once

#include "G4UImessenger.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "globals.hh"
#include <memory>

class G4UIcmdWithoutParameter;
class G4UIcmdWithAnInteger;

class RunMessenger final : public G4UImessenger
{
public:
    RunMessenger();
    ~RunMessenger() override;

    void SetNewValue(G4UIcommand*, G4String) override;

    // Planned event count for /bio/run/beamOn. CLI --events forces (and locks)
    // the value so a macro's /bio/run/setEvents cannot override it.
    static void SetPlannedEvents(G4int n) { if (!fgEventsLocked) fgEvents = n; }
    static void ForcePlannedEvents(G4int n) { fgEvents = n; fgEventsLocked = true; }
    [[nodiscard]] static G4int GetPlannedEvents() noexcept { return fgEvents; }

private:
    std::unique_ptr<G4UIdirectory>           fOutputDir;
    std::unique_ptr<G4UIcmdWithAString>      fFileNameCmd;
    std::unique_ptr<G4UIdirectory>           fRunDir;
    std::unique_ptr<G4UIcmdWithAnInteger>    fSetEventsCmd;
    std::unique_ptr<G4UIcmdWithoutParameter> fBeamOnCmd;

    static inline G4int fgEvents{0};
    static inline bool  fgEventsLocked{false};
};
