#include <TFile.h>
#include <TTree.h>
#include <cmath>
#include <random>
#include <armadillo>
#include <utility>
#include <tuple>

using namespace arma;

std::tuple<mat, mat, double> clustering(const mat, int, const int);
std::tuple<std::vector<mat>, std::vector<mat>, vec> createIndividuals(mat, const int, const int, const int);
void Selection(std::vector<mat>&, std::vector<mat>&, vec&, const double p=.05, const int verbose = 1);
void Crossover(std::vector<mat>&, std::vector<mat>&, const int verbose = 1);
void Mutation(std::vector<mat>&, const int mutationsNb = 2, const int verbose = 1);
vec Fitness(std::vector<mat>, std::vector<mat>);
void plot(mat, mat);
void ShuffleData(mat&, const int verbose=1);

std::mt19937 gen(1);

void clusteringKMGA(const std::string &path, const int K, 
					const int popSize = 10, const double p=.05, 
					const int mutationsNb = 2,
					const int verbose = 1){
	
	double eV = 1.602*1e-19;
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
	
	ShuffleData(data, verbose);
	
    auto [dataset, UVector, fitnessVec] = createIndividuals(data, K, popSize, verbose);
    int switcher = 1;
    int generation = 1;
    while (dataset.size() > 2){
		if (switcher == 1){
			switcher = 0;
			if (verbose == 1){
				std::cout << "==================== " << "Generation #" + std::to_string(generation) << " ====================" << std::endl;
			}
			Selection(dataset, UVector, fitnessVec, p, verbose);
			Crossover(dataset, UVector, verbose);
			Mutation(UVector, mutationsNb, verbose);
			++generation;
		} else {
			if (verbose == 1){
				std::cout << "==================== " << "Generation #" + std::to_string(generation) << " ====================" << std::endl;
			}
			fitnessVec = Fitness(dataset, UVector);
			Selection(dataset, UVector, fitnessVec, p, verbose);
			Crossover(dataset, UVector, verbose);
			Mutation(UVector, mutationsNb, verbose);
			++generation;
		}
	}
	mat finalU;
	mat finalData;
	if (fitnessVec.n_rows == 2){
		if (fitnessVec(0) > fitnessVec(1)){
			finalU = UVector[0];
			finalData = dataset[0];
		} else {
			finalU = UVector[1];
			finalData = dataset[1];
		}
	} else {
		finalU = UVector[0];
		finalData = dataset[0];
	}
	plot(finalData, finalU);
}

void ShuffleData(mat& data, const int verbose=1){
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

void plot(mat data, mat U){
	double xmin = min(data.col(0)), xmax = max(data.col(0));
	double ymin = min(data.col(1)), ymax = max(data.col(1));
	double zmin = min(data.col(2)), zmax = max(data.col(2));
	TCanvas *c = new TCanvas("c", "K-means with Genetic algorithm 3D Clusters", 1920, 1080);			
	TH3D *h3d = new TH3D("h3d", "K-means with Genetic algorithm 3D Clusters", 1, xmin, xmax, 1, ymin, ymax, 1, zmin, zmax);
	h3d->SetStats(0);
	h3d->Draw();
	const int K = U.n_cols;
	const int N = U.n_rows;
	mat centroids;
	for (size_t i=0; i<K; ++i){
		TPolyMarker3D *pm = new TPolyMarker3D();				
		/* Every points */
		/*for (size_t row = 0; row < N; ++row) {
			if (U(row, i) != 0) {
				pm->SetNextPoint(data(row, 0), data(row, 1), data(row, 2));
			}
		}*/
		
		/* Obtain centroids */
		centroids = zeros(K, 4);
		for (size_t i = 0; i < K; ++i) {
			uvec indices;
			for (size_t row = 0; row < U.n_rows; ++row) {
				if (U(row, i) != 0) {
					indices.insert_rows(indices.n_rows, uvec({row}));
				}
			}
			for (auto index : indices){
				centroids.row(i) += data.row(index)/indices.n_rows;
			}
			//newCentroids.row(i) /= indices.n_rows;
			centroids(i, 3) *= indices.n_rows;
		}
		
		/* Check for points beyound the TFrame */
		if (centroids(i, 0) < xmin || centroids(i, 0) > xmax){
			std::cout << "The centroid is outside of the Frame: X-axis" << std::endl;
		} else if (centroids(i, 1) < ymin || centroids(i, 1) > ymax) {
			std::cout << "The centroid is outside of the Frame: Y-axis" << std::endl;
		} else if (centroids(i, 2) < zmin || centroids(i, 2) > zmax) {
			std::cout << "The centroid is outside of the Frame: Z-axis" << std::endl;
		}	
		
		/* Only centroids */
		pm->SetPoint(i, centroids(i, 0), centroids(i, 1), centroids(i, 2));
		pm->SetMarkerSize(centroids(i,3)*centroids.n_rows*2e-5);
		pm->SetMarkerStyle(20);  // Стиль маркера (круг)
		
		int color = (i % 9) + 1;  // ROOT цвета: 1=черный, 2=красный, 3=синий, etc.
		pm->SetMarkerColor(color);
		pm->Draw("P SAME");
	}
	//gErrorIgnoreLevel = kWarning;
	gPad->RedrawAxis();
	c->Update();
	
	TCanvas* canvas2 = new TCanvas("c2", "Number of the clusters fraction", 1920, 1080);
	TGraph* graph = new TGraph();
	size_t ptsCounter = 0;
	double step = 0.01;
	int nbSteps = 100000;
	double minEnergy = 100;
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
	//canvas2->SaveAs(canvas2Name.c_str());
}

vec Fitness(std::vector<mat> dataset, std::vector<mat> UVector){
	vec fitnessVec = zeros(dataset.size());
	for (size_t j=0; j < dataset.size(); ++j){
		mat data = dataset[j];
		mat U = UVector[j];
		
		const int K = U.n_cols;
		const int N = U.n_rows;
		mat centroids = zeros(K, 4);
		for (size_t i = 0; i < K; ++i) {
			uvec indices;
			for (size_t row = 0; row < U.n_rows; ++row) {
				if (U(row, i) != 0) {
					indices.insert_rows(indices.n_rows, uvec({row}));
				}
			}
			for (auto index : indices){
				centroids.row(i) += data.row(index)/indices.n_rows;
			}
			//newCentroids.row(i) /= indices.n_rows;
			centroids(i, 3) *= indices.n_rows;
		}
		
		double fitnessValue = .0;
		for (size_t i = 0; i < K; ++i) {
			uvec indices;
			for (size_t row = 0; row < N; ++row) {
				if (U(row, i) != 0) {
					indices.insert_rows(indices.n_rows, uvec({row}));
				}
			}
			for (auto index : indices){
				fitnessValue += norm(centroids.submat(i,0,i,2)-data.submat(index,0,index,2));
			}
		
		}
		fitnessVec(j) = fitnessValue;
	}
	return fitnessVec;
}

std::tuple<std::vector<mat>, std::vector<mat>, vec> createIndividuals(mat data, const int K, const int popSize, const int verbose=1){
	int batch = data.n_rows/popSize;
	std::vector<mat> dataset(popSize);
	std::vector<mat> UVector(popSize);
	for (size_t i=0 ; i < popSize; ++i){
		dataset[i] = data;
	}	
	
	vec fitnessVec = zeros(popSize);
	std::vector<mat> populations;
	for (size_t i = 0; i < popSize; ++i) {
		if (verbose > 0){
			std::cout << "--------------------- Individual #" + std::to_string(i+1) + " ---------------------"   << std::endl;
		}
		auto [pop, U, fit] = clustering(dataset[i], K, verbose);
		UVector[i] = U;
		fitnessVec(i) = fit;
		populations.push_back(pop);
	};

	if (verbose > 0){
		std::cout << "---------========= " << "Stage I: Individuals" << " =========---------" << std::endl;
		for (size_t i=0 ; i < popSize; ++i){
			std::cout << "----------------------- Individual #" << i+1 << " -----------------------" << std::endl; 
			std::cout << "Fitness value: " << fitnessVec(i) << std::endl;
			std::cout << "Number of centroids: " << populations[i].n_rows << std::endl;
			std::cout << "Centroids: " << std::endl;
			std::cout << populations[i] << std::endl;
		};
	};
	
	return std::make_tuple(dataset, UVector, fitnessVec);
};

void Selection(std::vector<mat>& dataset, std::vector<mat>& UVector, vec& fitnessVec, const double p, const int verbose){
	vec probabilities(fitnessVec.n_rows);
	std::vector<double> newFitnessVec;
	std::vector<mat> newDataset, newUVector;	
	
	double minFitness = fitnessVec.min();
	double denominator = .0;
	for (size_t i=0; i < dataset.size(); ++i){
		denominator += fitnessVec(i) - minFitness;
	}
	
	for (size_t i=0; i < dataset.size(); ++i){
		probabilities(i) = (fitnessVec(i) - minFitness)/denominator;
		if (probabilities(i) > p){
			newUVector.push_back(UVector[i]); 
			newDataset.push_back(dataset[i]);
			newFitnessVec.push_back(fitnessVec(i));
		} 
	}
	
	if (verbose > 0){
		std::cout << "---------========= " << "Stage II: Selection" << " =========---------" << std::endl;
		std::cout << "\t\t\tp = " << p << std::endl;
		std::cout << "--------------------- Probabilities ---------------------\n" << probabilities << std::endl;
	}
	
	/* Put the minimal fitness value at the end: it's help crossover drop the weakest individual */
	
	size_t idx;
	if (!newFitnessVec.empty()) {
		auto max_it = std::min_element(newFitnessVec.begin(), newFitnessVec.end());
		idx = max_it - newFitnessVec.begin();
	}
	
	std::vector<mat> interDataset, interUVector;
	vec interFitnessVec(newDataset.size());
	for (size_t i=0; i<newDataset.size(); ++i){
		if (i != idx){
			interDataset.push_back(newDataset[i]);
			interUVector.push_back(newUVector[i]);
			interFitnessVec(i) = newFitnessVec[i];
		}
	}
	
	interDataset.push_back(newDataset[idx]);
	interUVector.push_back(newUVector[idx]);
	interFitnessVec.back() = newFitnessVec[idx];
	
	dataset = interDataset;
	UVector = interUVector;
	fitnessVec = interFitnessVec;
}

void Crossover(std::vector<mat>& dataset, std::vector<mat>& UVector, const int verbose){
	std::vector<mat> crossoveredDataset;
	std::vector<mat> crossoveredUVector;
	
	for (size_t i=0; i<dataset.size()/2; ++i){
		mat U1 		= UVector[i];
		mat U2 		= UVector[2*i];
		mat data1 	= dataset[i];
		mat data2 	= dataset[2*i];
		
		
		mat newU1 	= zeros(U1.n_rows, U1.n_cols);
		mat newU2 	= zeros(U2.n_rows, U2.n_cols);
		mat newData1= zeros(data1.n_rows, data1.n_cols);
		mat newData2= zeros(data2.n_rows, data2.n_cols);
		int crossPoint = rand() % data1.n_rows;
		int chromosomeLength = data1.n_rows;
		for (size_t j=0; j<chromosomeLength; ++j){
			if (j < crossPoint){
				newU1.row(j) = U2.row(j);
				newU2.row(j) = U1.row(j);
				newData1.row(j) = data2.row(j);
				newData2.row(j) = data1.row(j);
			} else {
				newU1.row(j) = U1.row(j);
				newU2.row(j) = U2.row(j);
				newData1.row(j) = data1.row(j);
				newData2.row(j) = data2.row(j);
			}
		}
		crossoveredUVector.push_back(newU1);
		crossoveredUVector.push_back(newU2);
		crossoveredDataset.push_back(newData1);
		crossoveredDataset.push_back(newData2);
	}
	dataset = crossoveredDataset;
	UVector = crossoveredUVector;
	
	/* We kill the last person */

	if (verbose > 0){
		std::cout << "---------========= " << "Stage III: Crossover" << " =========---------" << std::endl;
		std::cout << "----------------- Crossovered population -----------------"   << std::endl;
	}
};

void Mutation(std::vector<mat>& UVector, const int mutationsNb, const int verbose){
	for (mat &U : UVector){
		const int K = U.n_cols;
		const int N = U.n_rows;
		vec 	mutationIndices(mutationsNb);
		mat 	mutationMat = zeros(mutationsNb, K);
		for (size_t i=0; i<mutationsNb; ++i){
			mutationIndices(i) = rand() % N;
			int col = rand() % K;
			mutationMat(i, col) = 1;
		}
		int idx;
		for (size_t i=0; i<mutationsNb; ++i){
			idx = mutationIndices(i);
			U.row(idx) = mutationMat.row(i);
		}
	}
	if (verbose > 0){
		std::cout << "---------========== " << "Stage IV: Mutation" << " ==========---------" << std::endl;
		std::cout << "------------------- It was " << mutationsNb << " mutations -------------------"   << std::endl;
	}
};

std::tuple<mat, mat, double> clustering(mat data, const int K, const int verbose){
	std::mt19937 gen(std::time(nullptr));
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
	
	// Making of U-matrix
	int iterations = 0;
	double fitnessValue;
	mat U(N, K);
	while (true){
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
		
		mat newCentroids = zeros(K, 4);
		fitnessValue = .0;
		for (size_t i = 0; i < K; ++i) {
			uvec indices;
			for (size_t row = 0; row < U.n_rows; ++row) {
				if (U(row, i) != 0) {
					indices.insert_rows(indices.n_rows, uvec({row}));
				}
			}
			for (auto index : indices){
				newCentroids.row(i) += data.row(index);
				fitnessValue += norm(centroids.submat(i,0,i,2)-data.submat(index,0,index,2));
			}
			if (indices.n_rows == 0){
				int idx = randI(gen);
				newCentroids.row(i) = data.row(idx); 
				std::cout << "Number of indices size is zero!" << std::endl;
			} else {
				newCentroids.row(i) /= indices.n_rows;
				newCentroids(i, 3) *= indices.n_rows;
			}
		}
		
		iterations += 1;
		bool areEqual = all(all(newCentroids == centroids));
		if (verbose > 0){
			std::cout << "Convergence #" << iterations << ": \t" << abs(accu(centroids) - accu(newCentroids)) << std::endl;
		}
		if (!areEqual){	
			centroids = newCentroids;
		} else {
			centroids = newCentroids;
			std::cout << "Iterations number: " << iterations << std::endl;
			
            double xmin = min(data.col(0)), xmax = max(data.col(0));
            double ymin = min(data.col(1)), ymax = max(data.col(1));
            double zmin = min(data.col(2)), zmax = max(data.col(2));
            
			break;
		}
		
	}
	return std::make_tuple(centroids, U, fitnessValue);
	
};
