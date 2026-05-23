class GeomSum{
public:
	GeomSum(int n, int x): N(n), X(x){
		cout << calculation() << endl;
	}
	
private:
    long int N, X;
    
    int calculation(){
        long int sum = 0;
        for (int i=0; i<N; ++i){
        	sum += pow(X, i);
        }
    return sum;
    }   
};



void MacroFile(){
	GeomSum g = GeomSum(10, 2);
}



