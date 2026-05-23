#pragma once
#include <G4VAccumulable.hh>
#include <set>

class AccSet : public G4VAccumulable{
public:
	AccSet(){};
	virtual ~AccSet(){};

	virtual void Merge(const G4VAccumulable& other);
	virtual void Reset();
	AccSet& operator=(const AccSet& other);
	std::set<G4String> GetValue();
	void Insert(const std::set<G4String>&);
	void Insert(const G4String&);
	std::set<G4String> fSet;
};
