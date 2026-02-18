#include <TApplication.h>
#include <TH3D.h>
#include <TRandom3.h>

void test(){
	
	TCanvas* c = new TCanvas("c", "Test title", 1280, 720);
	
	TH3D* h3d = new TH3D("test", "Test plots",
						 20, -10, 10,
						 20, -10, 10,
						 20, -10, 10);
	
	TRandom randomGen(0);
	
	double x,y,z;
	double mean = 0;
	double sigma = 1;
	const int n = 10000;
	for (size_t i=0; i <= n; i++){
		x = randomGen.Gaus(mean, sigma);
		y = randomGen.Gaus(mean, sigma);
		z = randomGen.Gaus(mean, sigma);
		h3d->Fill(x,y,z);
	}
	h3d->Draw("GLISO");
};
