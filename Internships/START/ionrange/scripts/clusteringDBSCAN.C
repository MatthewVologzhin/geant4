#include <TFile.h>
#include <TTree.h>
#include <cmath>
#include <random>
#include <armadillo>
#include <utility>
#include <tuple>

using namespace arma;

void Plot(mat);
void ShuffleData(mat&, const int verbose = 1);
void DBSCAN(mat&, const double, const double, const double, const double, const size_t, const int verbose = 1);
void SparseData(mat&, const double, const int verbose = 1);
void EnergySample(mat&, const double, const double, const int verbose = 1);
std::vector<size_t> FindNeighbors(mat, const size_t, const double);
rowvec ComputeCentroid(mat, const size_t, const double);
void MergeCentroids(mat&, const double, const int verbose = 1);

std::mt19937 gen(time(nullptr));
std::uniform_real_distribution<> urd (.0, 1.);
void clusteringDBSCAN(const std::string &path, 
					  const double epsilon = 3.2, // nm
					  const size_t minPts = 2,
					  const double sPointProb = 0.16,
					  const double minE = 5,
					  const double maxE = 37.5,
					  const int verbose = 1){
	
	double eV = 1/1.602*1e19;
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
		data(i, 3) = energyDep*eV;
		data(i, 2) = x;
		data(i, 1) = y;
		data(i, 0) = z;
	}
	
	/* Shuffle the data */
	ShuffleData(data, verbose);
	
	DBSCAN(data, sPointProb, minE, maxE, epsilon, minPts, verbose);
	//MergeCentroids(data, epsilon, verbose);
	Plot(data);
}

void DBSCAN(mat& data, const double sPointProb, const double minE, const double maxE, const double epsilon, const size_t minPts, const int verbose=1){
	SparseData(data, sPointProb, verbose);
	EnergySample(data, minE, maxE, verbose);
	const size_t rows = data.n_rows;
	const size_t cols = data.n_cols;
	
	if (verbose == 1){
		std::cout << "---------========= " << "Stage III: DBSCAN" << " =========---------" << std::endl;
	}
	vec idVec(rows);
	idVec.fill(-1.);
	data = join_horiz(data, idVec);
	size_t clusterID = 0;
	for (size_t i=0; i<rows; ++i){
		double &pointID = data(i, cols); 
		if (pointID != -1.) continue;
		std::vector<size_t> neighbors = FindNeighbors(data, i, epsilon);
		std::cout << "Number of neighbors for point " + std::to_string(i+1) + ": " << neighbors.size() << std::endl;
		if (neighbors.size() < minPts){
			pointID = -1.;
			continue;
		} else {
			
			++clusterID;
			std::queue<size_t> q;
			for (size_t n : neighbors){
				data(n, cols) = clusterID;
				q.push(n);
			}
			
			while(!q.empty()){
				size_t current = q.front(); q.pop();
				auto currentNeighbors = FindNeighbors(data, current, epsilon);
				for (size_t n : currentNeighbors){
					if (currentNeighbors.size() >= minPts){
						double &pointID = data(n, cols); 
						if (pointID == -1. || pointID == 0.){
							pointID = clusterID;
							q.push(n);
						}
					}
				}
			}
		}
	}	
	/* Output */
	/*mat U(0,6);
	for (size_t i=0; i<clusters.n_rows ; ++i){
		mat cluster = clusters.row(i);
		size_t r = cluster.n_rows;
		size_t c = cluster.n_cols;
		vec idx(r);
		idx.fill(i+1);
		vec strands(r);
		for (size_t j=0; j<r; j++){
			strands(j) = rand()%2+1;
		}
		cluster = join_horiz(cluster, idx);
		cluster = join_horiz(cluster, strands);
		U = join_vert(U, cluster);
	}
	std::cout << U << std::endl;*/
	
}

void ShuffleData(mat& data, const int verbose=1){
	if (verbose == 1){
		std::cout << "---------========= " << "PreStage: Data was shuffled" << " =========---------" << std::endl;
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

void SparseData(mat& data, const double sPointProb, const int verbose=1){
	if (verbose == 1){
		std::cout << "---------========= " << "Stage I: Sparsing the data" << " =========---------" << std::endl;
		std::cout << "Event number before the Sparse: " << data.n_rows << std::endl;
	}
	
	uint rows = data.n_rows;
	uint cols = data.n_cols;
	uint rowsCounter = 0;
	double rVal; 
	mat newData(rows, cols);
	for (size_t row=0; row<rows; row++){
		rVal = urd(gen);
		if (rVal < sPointProb){
			newData.row(rowsCounter) = data.row(row);
			++rowsCounter;
		}
	}
	newData = newData.submat(0, 0, rowsCounter-1, 3);
	data = std::move(newData);
	if (verbose == 1){
		std::cout << "Event number after the Sparse: " << data.n_rows << std::endl;
	}
}

void EnergySample(mat& data, const double minE, const double maxE, const int verbose=1){
	if (verbose == 1){
		std::cout << "---------========= " << "Stage II: Energy sampling" << " =========---------" << std::endl;
		std::cout << "Event number before the Energy sampling: " << data.n_rows << std::endl;
	}
	double probability;
	double energy;
	double randomProb;
	size_t rows = data.n_rows;
	size_t cols = data.n_cols;
	size_t rowsCounter = 0;
	mat newData(rows, cols);
	for (size_t row=0; row<rows; ++row){
		energy = data(row, 3);
		probability = (energy-minE)/(maxE-minE);
		randomProb = urd(gen);
		if (randomProb < probability){
			newData.row(rowsCounter) = data.row(row);
			++rowsCounter;
		}
	}
	newData = newData.submat(0, 0, rowsCounter-1, 3);
	data = std::move(newData);
	if (verbose == 1){
		std::cout << "Event number after the Energy sampling: " << data.n_rows << std::endl;
	}
	
}

std::vector<size_t> FindNeighbors(mat data, const size_t idx, const double epsilon){
	const size_t rows = data.n_rows;
	std::vector<size_t> neighbors;
	for (size_t i=0; i<rows; i++){
		double distance = norm(data.submat(i,0,i,2)-data.submat(idx,0,idx,2));
		if (distance <= epsilon && i!=idx){
			neighbors.push_back(i);
		}
	}
	return neighbors;
}

rowvec ComputeCentroid(mat data, const size_t idx, const double epsilon){
	double sumX = .0;
	double sumY = .0;
	double sumZ = .0;
	double sumE = .0;
	const size_t rows = data.n_rows;
	const size_t cols = data.n_cols;
	size_t elements = 0;
	for (size_t row=0; row<rows; row++){
		double pointID = data(row, cols-1);
		if (pointID == idx){
			++elements;
			sumX += data(row, 2);
			sumY += data(row, 1);
			sumZ += data(row, 0);
			sumE += data(row, 3);
		}
	}
	rowvec centroid = {sumZ/elements, sumY/elements, sumX/elements, sumE, static_cast<double>(idx)};
	return centroid;
	
}

void MergeCentroids(mat& data, const double epsilon, const int verbose=1){
	const size_t rows = data.n_rows;
	const size_t cols = data.n_cols;
	unordered_set<size_t> clusterIDs;
	for (size_t row=0; row<rows; row++){
		double pointID = data(row, 4);
		if (pointID > 0) clusterIDs.insert(pointID);
	}
	
	size_t clustersNb = clusterIDs.size();
	mat centroids(0,5);
	for (size_t idx=1; idx<=clustersNb; idx++){
		rowvec centroid = ComputeCentroid(data, idx, epsilon);
		centroids = join_vert(centroids, centroid);
	}
	
	for (size_t i=0; i<centroids.n_rows; i++){
		for (size_t j=i+1; j<centroids.n_rows; j++){
			double distance = norm(centroids.submat(i,0,i,2)-centroids.submat(j,0,j,2));
			if (distance <= epsilon){
				double oldID = centroids(j, 4);
				double newID = centroids(i, 4);
				for (size_t row=0; row<rows; row++){
					double &pointID = data(row, 4);
					if (pointID == oldID){
						pointID = newID;
					}
				}
				std::cout << "A merger has occurred" << std::endl;
				mat newCentroids(0, centroids.n_cols);
				newCentroids = join_vert(newCentroids, centroids.submat(0, 4, j-1, 4));
				newCentroids = join_vert(newCentroids, centroids.submat(j+1, 4, centroids.n_rows-1, 4));
                centroids = newCentroids;
                j--;
			}
		}
	}
}

void Plot(mat data){
	double xmin = min(data.col(0)), xmax = max(data.col(0));
	double ymin = min(data.col(1)), ymax = max(data.col(1));
	double zmin = min(data.col(2)), zmax = max(data.col(2));
	TCanvas *c = new TCanvas("c", "DBSCAN 3D Clusters", 1920, 1080);			
	TH3D *h3d = new TH3D("h3d", "DBSCAN 3D Clusters", 1, xmin, xmax, 1, ymin, ymax, 1, zmin, zmax);
	h3d->SetStats(0);
	h3d->Draw();
	h3d->GetXaxis()->SetRangeUser(-100,100);
	h3d->GetXaxis()->SetLimits(-100,100);
	h3d->GetYaxis()->SetRangeUser(-100,100);
	h3d->GetYaxis()->SetLimits(-100,100);
	h3d->GetZaxis()->SetRangeUser(-100,100);
	h3d->GetZaxis()->SetLimits(-100,100);
	const int cols = data.n_cols;
	const int rows = data.n_rows;				
	for (size_t row=0; row<rows; ++row){
		/* Every points */
		/*TPolyMarker3D *pm = new TPolyMarker3D();
		pm->SetNextPoint(data(row, 0), data(row, 1), data(row, 2));
		pm->SetMarkerSize(data(row, 3)*1e-2);
		int color = (static_cast<int>(data(row,4)) % 9);  // ROOT цвета: 1=черный, 2=красный, 3=синий, etc.
		if (color == -1){
			color = 1;
		} else {
			++color;
		}
		pm->SetMarkerColor(color);
		pm->Draw("P SAME");
		pm->SetMarkerStyle(20);*/
		
		/* Coloring by DNA Breaks */
		TPolyMarker3D *pm = new TPolyMarker3D();
		
		int color;
		size_t count = accu(data.col(4) == data(row, 4));
		if (count > 2 and data(row,4) != -1){
			pm->SetNextPoint(data(row, 0), data(row, 1), data(row, 2));
			pm->SetMarkerSize(data(row, 3)*0.25e-1);
			color = 4;
		} else if (count == 2 and data(row,4) != -1) {
			pm->SetNextPoint(data(row, 0), data(row, 1), data(row, 2));
			pm->SetMarkerSize(data(row, 3)*0.25e-1);
			color = 2;
		} else if (data(row,4) == -1) {
			pm->SetNextPoint(data(row, 0), data(row, 1), data(row, 2));
			pm->SetMarkerSize(data(row, 3)*0.25e-1);
			color = 1;
		} else {
			//std::cout << "Error with cluster number in Plot!" << std::endl;
		}
		pm->SetMarkerColor(color);
		pm->Draw("P SAME");
		pm->SetMarkerStyle(20);
		
		
		
		
		/* Obtain centroids */
		/*mat centroids = zeros(K, 4);
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
		}*/
		
		/* Only centroids */
		/*for (size_t row = 0; row < N; ++row) {
			if (U(row, i) != 0) {
				sumEnergy += data(i, 3);
			}
		}
		pm->SetPoint(i, centroids(i, 0), centroids(i, 1), centroids(i, 2));
		pm->SetMarkerSize(sumEnergy*1e1);
		pm->SetMarkerStyle(20);  // Стиль маркера (круг)
		
		int color = (i % 9) + 1;  // ROOT цвета: 1=черный, 2=красный, 3=синий, etc.
		pm->SetMarkerColor(color);
		pm->Draw("P SAME");*/
	}
	//gErrorIgnoreLevel = kWarning;
	gPad->RedrawAxis();
	c->Update();
}
