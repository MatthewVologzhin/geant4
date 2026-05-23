#include <TFile.h>
#include <TTree.h>
#include <cmath>
#include <random>
#include <armadillo>

using namespace arma;

void clustering(const mat, int, const double, const int);
void ShuffleData(mat&, const int verbose=1);

std::mt19937 gen(1);

void clusteringKM(const std::string &path, const int K, const double minEnergy=100, const int verbose = 1){
	
	std::mt19937 gen(1);
	const double eV = 1.602*1e-19;
    //std::normal_distribution<double> rand(0.0, 1.0);  // Нормальное распределение
	TFile* f = new TFile(path.c_str());
	double energyDep, x, y, z;
	TNtuple* tupleDose = (TNtuple*)f->Get("doseData");
	tupleDose->SetBranchAddress("energy", &energyDep);
	tupleDose->SetBranchAddress("x", &x);
	tupleDose->SetBranchAddress("y", &y);
	tupleDose->SetBranchAddress("z", &z);
	int nEntries = tupleDose->GetEntries();
	std::uniform_int_distribution<int> dist(0, nEntries);
	mat data(nEntries, 4);
	for (Long64_t i=0; i<nEntries; ++i){
		tupleDose->GetEntry(i);
		data(i, 3) = energyDep/eV;
		data(i, 2) = x;
		data(i, 1) = y;
		data(i, 0) = z;
	}
	
    //int pointsNb = 10000;
    //mat data(pointsNb, 3);
    /*for (size_t i = 0; i < pointsNb; ++i) {
        data(i, 0) = rand(gen);
        data(i, 1) = rand(gen);
        data(i, 2) = rand(gen);
    }*/
	ShuffleData(data, verbose);
	clustering(data, K, minEnergy, verbose);
}

void ShuffleData(mat& data, const int verbose){
	if (verbose == 1){
		std::cout << "---------========= " << "PreStage: Data was shuffled" << " =========---------\n" << std::endl;
	}
	uint rows = data.n_rows;
	uint cols = data.n_cols;
	vec indices(rows);
	for (size_t row=0; row<rows; ++row){
		indices(row) = row;
	}
	std::shuffle(indices.begin(), indices.end(), gen);
	arma::mat shuffledData(rows, cols);
	for (size_t row = 0; row<rows; ++row) {
		shuffledData.row(row) = data.row(indices[row]);
	}
	data = std::move(shuffledData);
};

void clustering(mat data, const int K, const double minEnergy=100, const int verbose=1){
	std::mt19937 gen(1);
	mat centroids(0, 4);
	Long64_t N = data.n_rows;
	std::uniform_int_distribution<int> randI(0, N-1);
	
	// Initialisation of Сentroid vectors 
	std::vector<int> indices;
	while (centroids.n_rows != K){
		int idx = randI(gen);
		auto it = std::find(indices.begin(), indices.end(), idx);
		if (it == indices.end()){
			centroids = join_vert(centroids, data.row(idx));
			indices.push_back(idx);
		}
	}
	if (verbose == 1){
		std::cout << "---------========= " << "Stage I: First centroids were created" << " =========---------\n" << std::endl;
	}	

	// Making of U-matrix
	int iterations = 0;
	std::cout << "---------========= " << "Stage II: K-Means is starting" << " =========---------" << std::endl;
	while (true){
		mat U(N, K);
		for (size_t i=0; i<N; ++i){
			rowvec uRow(K);
			for (size_t j=0; j<K; ++j){
				uRow(j) = norm(centroids.submat(j,0,j,2)-data.submat(i,0,i,2));
			}
			size_t minIndex = uRow.index_min();
			uRow = zeros<rowvec>(K);
			uRow(minIndex) = 1; 
			U.row(i) = uRow;
		}

		mat newCentroids(K,4);
		for (size_t i = 0; i < K; ++i) {
			uvec indices;
			for (size_t row = 0; row < N; ++row) {
				if (U(row, i) != 0) {
					indices.insert_rows(indices.n_rows, uvec({row}));
				}
			}
			for (auto index : indices){
				newCentroids.row(i) += data.row(index); 
			}
			if (indices.n_rows == 0){
				int idx = randI(gen);
				newCentroids.row(i) = data.row(idx); 
				std::cout << "Number of indices size is zero!" << std::endl;
			} else {
				newCentroids.row(i) /= indices.n_rows;
				newCentroids(i, 3) *= indices.n_rows;
			}
			/*std::cout << "Old: " << centroids << std::endl;
			std::cout << "Normalized new: " << newCentroids << std::endl;*/
		}
		iterations += 1;
		std::cout << "Convergence #" << iterations << ": \t" << abs(accu(centroids) - accu(newCentroids)) << std::endl;
		bool areEqual = all(all(newCentroids == centroids));
		if (!areEqual){	
			centroids = newCentroids;
		} else {
			std::cout << "---------=================================================---------\n" << std::endl;
			centroids = newCentroids;
			
			double xmin = min(data.col(0)), xmax = max(data.col(0));
			double ymin = min(data.col(1)), ymax = max(data.col(1));
			double zmin = min(data.col(2)), zmax = max(data.col(2));
            
            
            if (verbose > 0){
				TCanvas *c = new TCanvas("c", "K-means 3D Clusters", 1920, 1080);
				TH3D *h3d = new TH3D("h3d", "K-means 3D Clusters", 100, xmin, xmax, 100, ymin, ymax, 100, zmin, zmax);
				
				h3d->SetStats(0);
				h3d->Draw();				
				for (size_t i=0; i<K; ++i){
					TPolyMarker3D *pm = new TPolyMarker3D();
					pm->SetPoint(i, centroids(i, 0), centroids(i, 1), centroids(i, 2));
					pm->SetMarkerSize(centroids(i, 3)*centroids.n_rows*2e-5);
					pm->SetMarkerStyle(20);  // Стиль маркера (круг)
					
					/*double z1 = newCentroids(i, 0);
					double y1 = newCentroids(i, 1);
					double x1 = newCentroids(i, 2);
					if (z1 < xmin || z1 > xmax){
						std::cout << "Problem with Z" << std::endl;
					} else if (y1 < ymin || y1 > ymax) {
						std::cout << "Problem with Y" << std::endl;
					} else if (x1 < zmin || x1 > zmax) {
						std::cout << "Problem with X" << std::endl;	
					}*/
					
					int color = (i % 9) + 1;  // ROOT цвета: 1=черный, 2=красный, 3=синий, etc.
					pm->SetMarkerColor(color);
					pm->Draw("P SAME");
				}
				//gErrorIgnoreLevel = kWarning;
				gPad->RedrawAxis();
				c->Modified();
				c->Update();
				c->SaveAs(("plots/clustering K-means(" + std::to_string(K) + ").png").c_str());
				
				TCanvas* canvas2 = new TCanvas("c2", "Number of the clusters fraction", 1920, 1080);
				TGraph* graph = new TGraph();
				size_t ptsCounter = 0;
				double step = 0.01;
				int nbSteps = 100000;
				for (double k=1; k<nbSteps; ++k){
					double effectiveRadius = step*k;
					size_t nbIon = 0;
					size_t nbDelta = 0;
					for (Long64_t i=0; i<centroids.n_rows; i++){
						double energy = centroids(i,3);
						if (energy >= minEnergy){
							double radius = norm(centroids.submat(i,1,i,2)); 
							if (radius <= effectiveRadius){
								++nbIon;
							} else {
								++nbDelta;
							}
						}
					}
					if (nbIon != 0){
						double fraction = static_cast<double>(nbDelta)/nbIon;
						graph->SetPoint(ptsCounter, effectiveRadius, fraction);
						++ptsCounter;
					}
				}
				gPad->SetLogx();
				gPad->SetLogy();
				graph->SetLineWidth(2);
				graph->SetLineColor(kBlack);
				TAxis *Xaxis = graph->GetXaxis();
				TAxis *Yaxis = graph->GetYaxis();
				Xaxis->SetTitle("Effective radius [nm]");
				Yaxis->SetTitle("The fraction N^{#delta}/N^{I}");
				graph->SetTitle(("At energy E=" + std::to_string(minEnergy) + " eV & " + std::to_string(centroids.n_rows) + " clusters").c_str());
				Xaxis->CenterTitle(true); Xaxis->SetTitleOffset(1.2);
				Yaxis->CenterTitle(true); 
				graph->Draw("APL");
				gPad->SetFrameBorderMode(0);
				
				canvas2->Update();
				gPad->GetFrame()->SetBorderSize(0); // убирает рамку вокруг холста
				gPad->GetFrame()->SetLineColor(kWhite);
				const std::string canvas2Name = "plots/Clusters fraction.png";
				canvas2->SaveAs(canvas2Name.c_str());
				/*if (!gEve) {
					gEve = TEveManager::Create();
				}
				for (size_t row = 0; row < K; ++row) {
					TEvePointSet* clusterPoints = new TEvePointSet(Form("Cluster_%zu", row));
					clusterPoints->SetMarkerStyle(20);  // Стиль маркера (круг)
					
					// Добавляем одну точку для этого кластера
					clusterPoints->SetNextPoint(newCentroids(row, 0), newCentroids(row, 1), newCentroids(row, 2));
					
					// Устанавливаем индивидуальный размер (пропорционально энергии)
					double size = newCentroids(row, 3) * 1e-1;  // Масштабируем для видимости
					clusterPoints->SetMarkerSize(size);
					
					// Устанавливаем индивидуальный цвет
					int color = (row % 9) + 1;  // ROOT цвета: 1=чёрный, 2=красный, etc.
					clusterPoints->SetMarkerColor(color);
					
					// Добавляем в 3D-визуализатор
					gEve->AddElement(clusterPoints);
				}
				
				// Обновляем сцену
				gEve->GetDefaultGLViewer()->SetCurrentCamera(TGLViewer::kCameraPerspXOY);
				gEve->GetDefaultGLViewer()->CurrentCamera().Reset();
				gEve->GetDefaultGLViewer()->UpdateScene();
				gEve->Redraw3D(kTRUE);*/
			}
			break;
		}
		
	}
	
};
