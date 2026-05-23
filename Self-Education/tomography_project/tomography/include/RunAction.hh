#include <G4UserRunAction.hh>
#include <G4RootAnalysisManager.hh>
#include <G4Run.hh>

class RunAction : public G4UserRunAction{
public:
	RunAction();
	~RunAction() override;
	void BeginOfRunAction(const G4Run*) override;
	void EndOfRunAction(const G4Run*) override;	

private:
	void Book();

private:
	G4RootAnalysisManager* fAnalysisManager = nullptr;
};
