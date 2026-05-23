#include <G4VAccumulable.hh>
#include "AccMax.hh"

void AccMax::Merge(const G4VAccumulable& other){
	const AccMax& otherAccMax = static_cast<const AccMax&>(other);
	if (otherAccMax.fDouble > this->fDouble){
		this->SetValue(otherAccMax.fDouble);
	}
}

void AccMax::Reset(){
	this->fDouble = .0;
}

 AccMax& AccMax::operator=(const AccMax& other){
        // Проверка на самоприсваивание
        if (this == &other) {
            return *this; // Возвращаем текущий объект
        }

        // Копируем данные из другого объекта
        this->fDouble = other.fDouble;

        return *this; // Возвращаем ссылку на текущий объект
    }
    
G4double AccMax::GetValue(){
	return fDouble;
}

void AccMax::SetValue(const G4double& newValue){
	this->fDouble = newValue;
}
