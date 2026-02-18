#include <G4VAccumulable.hh>
#include "AccString.hh"

void AccString::Merge(const G4VAccumulable& other){
	const AccString& otherAccString = static_cast<const AccString&>(other);
	this->SetValue(otherAccString.fString);
}

void AccString::Reset(){}

 AccString& AccString::operator=(const AccString& other){
        // Проверка на самоприсваивание
        if (this == &other) {
            return *this; // Возвращаем текущий объект
        }

        // Копируем данные из другого объекта
        this->fString = other.fString;

        return *this; // Возвращаем ссылку на текущий объект
    }
    
G4String AccString::GetValue(){
	return fString;
}

void AccString::SetValue(const G4String& newValue){
	this->fString = newValue;
}
