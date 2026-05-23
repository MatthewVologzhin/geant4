#include "EventAction.hh"
#include "RunAction.hh"

EventAction::EventAction(RunAction* runAction) : fRunAction(runAction)
{};

EventAction::~EventAction()
{};

void EventAction::BeginOfEventAction(const G4Event* event)
{};

void EventAction::EndOfEventAction(const G4Event* event)
{};
