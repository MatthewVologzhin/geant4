#pragma once

#include "G4UImessenger.hh"
#include "globals.hh"
#include <string_view>
#include <memory>
#include <optional>

class DetectorConstruction;
class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithAnInteger;
class G4UIcmdWithADoubleAndUnit;
class G4UIcmdWithoutParameter;
class G4UIcommand;
class G4Material;

class MaterialMessenger final : public G4UImessenger
{
  public:
    explicit MaterialMessenger(DetectorConstruction*);
    ~MaterialMessenger() override = default;

    MaterialMessenger(const MaterialMessenger&) = delete;
    MaterialMessenger& operator=(const MaterialMessenger&) = delete;

    MaterialMessenger(MaterialMessenger&&) = delete;
    MaterialMessenger& operator=(MaterialMessenger&&) = delete;

    void SetNewValue(G4UIcommand*, G4String) override;

    void HandleCreateCommand(std::string_view);
    void HandleAddCommand(std::string_view);

  private:

    struct ActiveMaterialContext{
      G4Material* material{nullptr};
      int targetComponents{0};
      int addedComponents{0};
      double fraction{0.0};
    };

    DetectorConstruction* fDetector{nullptr};

    std::unique_ptr<G4UIdirectory> fMaterialDir{nullptr};

    std::unique_ptr<G4UIcommand> fCreateMaterialCmd{nullptr};
    std::unique_ptr<G4UIcommand> fAddMaterialCmd{nullptr};

    std::optional<ActiveMaterialContext> fActiveContext{std::nullopt};
};
