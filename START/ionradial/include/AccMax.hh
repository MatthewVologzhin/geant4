#pragma once
#include <G4VAccumulable.hh>

class AccMax : public G4VAccumulable{
public:
	AccMax(){};
	virtual ~AccMax(){};

	virtual void Merge(const G4VAccumulable& other);
	virtual void Reset();
	AccMax& operator=(const AccMax& other);
	G4double GetValue();
	void SetValue(const G4double&);
	G4double fDouble;
};
