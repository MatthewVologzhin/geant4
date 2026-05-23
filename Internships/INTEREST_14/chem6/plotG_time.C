#include <TFile.h>
#include <TTree.h>
#include <TGraphErrors.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TAxis.h>
#include <iostream>
#include <vector>
#include <map>

// Структуры оставляем без изменений для совместимости данных
struct SpeciesInfoAOS {
   SpeciesInfoAOS() : fNEvent(0), fNumber(0), fG(0.), fG2(0.) {}
   Int_t fNEvent;
   Int_t fNumber;
   Double_t fG;
   Double_t fG2;
   string fName;
};

struct SpeciesInfoSOA {
   SpeciesInfoSOA() : fRelatErr(0) {}
   std::vector<Double_t> fG;
   std::vector<Double_t> fGerr;
   std::vector<Double_t> fTime;
   Double_t fRelatErr;
   string fName;
};

void plotG_time()
{
   // Настройка стилей (без изменений)
   gROOT->SetStyle("Plain");
   gStyle->SetPalette(1);
   gStyle->SetPadTickX(1);
   gStyle->SetPadTickY(1);

   Double_t timeA, sumG, sumG2;
   Int_t speciesID, number, nEvent;
   char speciesName[500];

   TFile* file = TFile::Open("chem6_ion-12_12-MeV_G4EmDNAPhysics_option8_G4EmDNAChemistry_option3_SBS_3463_22_0.root");
   if (!file || file->IsZombie()) {
      cout << "Error: Species0.root not found!" << endl;
      return;
   }

   TTree* tree = (TTree*)file->Get("species");
   if (!tree) {
      cout << "Error: Tree 'species' not found!" << endl;
      return;
   }

   tree->SetBranchAddress("speciesID", &speciesID);
   tree->SetBranchAddress("number", &number);
   tree->SetBranchAddress("nEvent", &nEvent);
   tree->SetBranchAddress("speciesName", &speciesName);
   tree->SetBranchAddress("time", &timeA);
   tree->SetBranchAddress("sumG", &sumG);
   tree->SetBranchAddress("sumG2", &sumG2);

   Long64_t nentries = tree->GetEntries();
   if(nentries == 0) {
      cout << "No entries found!" << endl;
      return;
   }

   // 1. Сбор данных (без изменений)
   std::map<int, std::map<double, SpeciesInfoAOS>> speciesTimeInfo;
   for (Int_t j=0; j < nentries; j++) {
     tree->GetEntry(j);
     SpeciesInfoAOS& infoAOS = speciesTimeInfo[speciesID][timeA];
     infoAOS.fNumber += number;
     infoAOS.fG += sumG;
     infoAOS.fG2 += sumG2;
     infoAOS.fNEvent += nEvent;
     infoAOS.fName = speciesName;
   }

   // 2. Подготовка холста и файла
   TCanvas *c1 = new TCanvas("c1", "Plots", 800, 600);
   const char* outputPDF = "G_vs_Time_All.pdf";
   
   // Открываем PDF файл (символ [ )
   c1->Print(Form("%s[", outputPDF));

   auto it_SOA = speciesTimeInfo.begin();
   auto end_SOA = speciesTimeInfo.end();

   for (; it_SOA != end_SOA; ++it_SOA) {
      const Int_t _speciesID = it_SOA->first;
      
      auto it2 = it_SOA->second.begin();
      auto end2 = it_SOA->second.end();

      std::vector<Double_t> vG, vGerr, vTime;
      string currentName = it2->second.fName;

      for (; it2 != end2; ++it2) {
         SpeciesInfoAOS& infoAOS = it2->second;
         Double_t _SumG2 = infoAOS.fG2;
         Double_t _MeanG = infoAOS.fG/infoAOS.fNEvent;
         Double_t _Gerr = (infoAOS.fNEvent > 1) ? sqrt(abs((_SumG2/infoAOS.fNEvent - pow(_MeanG,2))
                                                       /(infoAOS.fNEvent-1) )) : 0.;
         vG.push_back(_MeanG);
         vGerr.push_back(_Gerr);
         vTime.push_back(it2->first);
      }

      // Создаем график
      TGraphErrors* gSpecies = new TGraphErrors(vG.size(), vTime.data(), vG.data(), 0, vGerr.data());

      Int_t color = (2 + _speciesID) % TColor::GetNumberOfColors();
      if(color == 5 || color == 10 || color == 0) ++color;

      gSpecies->SetTitle(currentName.c_str());
      gSpecies->SetMarkerStyle(20 + _speciesID % 10);
      gSpecies->SetMarkerColor(color);
      gSpecies->SetLineColor(color);
      gSpecies->GetXaxis()->SetTitle("Time (ns)");
      gSpecies->GetYaxis()->SetTitle("G value (molecules/100 eV)");

      // 3. Рисование и сохранение страницы
      c1->Clear();
      c1->SetLogx(); // Устанавливаем логарифмическую шкалу как в оригинале
      gSpecies->Draw("ALP");
      
      // Добавляем текущий вид холста в PDF
      c1->Print(outputPDF);

      delete gSpecies; // Очищаем память
   }

   // 4. Закрываем PDF файл (символ ] )
   c1->Print(Form("%s]", outputPDF));

   cout << "Success: All plots saved to " << outputPDF << endl;
   file->Close();
}