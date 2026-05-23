#include <G4VAccumulable.hh>
#include "AccSet.hh"

void AccSet::Merge(const G4VAccumulable& other){
	const AccSet& otherAccSet = static_cast<const AccSet&>(other);
	this->Insert(otherAccSet.fSet);
}

void AccSet::Reset(){
	this->fSet.clear();
}

 AccSet& AccSet::operator=(const AccSet& other){
        // Проверка на самоприсваивание
        if (this == &other) {
            return *this; // Возвращаем текущий объект
        }

        // Копируем данные из другого объекта
        this->fSet = other.fSet;

        return *this; // Возвращаем ссылку на текущий объект
    }
    
std::set<G4String> AccSet::GetValue(){
	return fSet;
}

void AccSet::Insert(const std::set<G4String>& newValue){
    for (const auto& str : newValue) {
		if (newValue.empty()) {
			G4cout << "AccSet::Insert: Inserting an empty string." << G4endl;
		}
        fSet.insert(str);
    }
}

void AccSet::Insert(const G4String& newValue){
	if (newValue.empty()) {
        G4cout << "AccSet::Insert: Inserting an empty string." << G4endl;
    }
	fSet.insert(newValue);
}
