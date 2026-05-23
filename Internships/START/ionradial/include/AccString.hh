#pragma once
#include <G4VAccumulable.hh>

class AccString : public G4VAccumulable{
public:
	AccString(){};
	virtual ~AccString(){};

	virtual void Merge(const G4VAccumulable& other);
	virtual void Reset();
	AccString& operator=(const AccString& other);
	G4String GetValue();
	void SetValue(const G4String&);
	G4String fString;
};
