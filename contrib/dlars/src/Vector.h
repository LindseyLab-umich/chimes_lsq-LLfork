// Used by all classes to safely stop the calculation.
#ifdef USE_MKL
#include <mkl.h>
#endif

void stop_run(int stat) ;

class Vector {
public:
	double *vec ;
	int dim ;
	double shift ;

	Vector(int d1)
		{
			dim = d1 ;
			vec = new double[d1] ;
			shift = 0 ;
		}
	Vector(int d1, double val)
		{
			dim = d1 ;
			vec = new double[d1] ;
			shift = 0 ;
			for ( int j = 0 ; j < dim ; j++ ) {
				vec[j] = val ;
			}
		}
	Vector()
		{
			dim = 0 ;
			vec = NULL ;
			shift = 0 ;
		}
	~Vector()
		{
			
				delete[] vec ;
		}
	int size() const {
		return dim ;
	}
	void set(int i, double val) {
#ifdef DEBUG					
		if ( i >= dim ) {
			cout << "Vector set out of bounds" << endl ;
			exit(1) ;
		}
#endif					
		vec[i] = val ;
	}

	Vector &operator=(const Vector& in) {
		if ( dim !=in.dim ) {
			delete [] vec ;
			vec = new double [in.dim] ;
			dim = in.dim ;
		}
		for ( int j = 0 ; j < dim ; j++ ) {
			vec[j] = in.vec[j] ;
		}
		return *this ;
	}
		
	void realloc(int size) {
		// Reallocate the vector.  Maintain previous values.
		double *vec2 = new double[size] ;

		int ncopy = ( dim < size ) ? dim : size ;
		for ( int i = 0 ; i < ncopy ; i++ ) {
			vec2[i] = vec[i] ;
		}
		for ( int i = ncopy ; i < size ; i++ ) {
			vec2[i] = 0.0 ;
		}
		delete [] vec ;
		vec = vec2 ;
		dim = size ;
	}
	
	void read(ifstream &file, int dim0) 
		{
			dim = dim0 ;
			vec = new double[dim] ;
			for ( int i = 0 ; i < dim ; i++ ) {
				double val ;
				file >> val ;
				set(i, val) ;
			}
		}

	void read_sparse(ifstream &file)
		// Read a vector in sparse format.
		{
			string line ;

			clear() ;
			getline(file,line) ;

			size_t pos =  line.find('[') ;
			if ( pos == string::npos ) {
				cout << "Did not find '[' character in " + line << endl ;
				exit(1) ;
			}
			istringstream istr(line.substr(pos+1)) ;
			istr >> dim ;
			vec = new double[dim] ;
			for ( int i = 0 ; i < dim ; i++ ) {
				vec[i] = double{0} ;
			}
			int idx ;
			double val ;
			int i ;
			for ( i = 0 ; i < dim ; i++ ) {
				getline(file, line) ;
				if ( line.find(']') != string::npos ) {
					break ;
				}
				istringstream istr(line) ;
				istr >> idx >> val ;
				if ( idx < dim && idx >= 0 ) 
					set(idx, val) ;
				else {
					cout << "Error reading sparse vector " << endl ;
					cout << line ;
					exit(1) ;
				}
			}
			if ( i == dim ) {
				getline(file, line) ;				
				if ( line.find(']') == string::npos ) {
					cout << "Error read end of sparse vector " << endl ;
					cout << line ;
					exit(1) ;
				}
			}
			// cout <<  "Last line: " + line << endl ;
		}

	void normalize()
		{
			shift = 0 ;
			for ( int i = 0 ; i < dim ; i++ ) {
				shift += vec[i] ;
			}
			shift /= dim;
			for ( int i = 0 ; i < dim ; i++ ) {
				vec[i] -= shift ;
			}
		}
	void check_norm()
		{
			double test = 0.0 ;
			for ( int i = 0 ; i < dim ; i++ ) {
				test += vec[i] ;
			}
			if ( fabs(test) > 1.0e-06 ) 
			{
			
#ifdef WARN_ONLY
				if ( RANK == 0 ) 
				{
					cout << "WARNING: vector was not normalized" << endl ;
					cout << "Test yielded value: " << fabs(test) << endl;
					cout << "Success criteria:   1.0e-6" << endl;
				}
				
#else			
				if ( RANK == 0 ) 
				{
					cout << "Error: vector was not normalized" << endl ;
					cout << "Test yielded value: " << fabs(test) << endl;
					cout << "Success criteria:   1.0e-6" << endl;
				}
				exit(1) ;
#endif				
			}
		}
	double get(int idx) const 
		{
#ifdef DEBUG			
			if ( idx >= dim ) {
				cout <<  "Vector out of bounds" << endl ;
				exit(1) ;
			}
#endif			
			return vec[idx] ;
		}
	void add(int idx, double val) 
		{
			vec[idx] += val ;
		}

	void print() 
	{
		print(cout) ;
	}

	void print(ostream &of)
	// Print only non-zero values.
		{
			if ( RANK == 0 ) {
				of << "[" << endl ;
				for ( int j = 0 ; j < dim ; j++ ) {
					if ( fabs(vec[j]) > 0.0 ) 
						of << j << " " << vec[j] << endl ;
				}
				of << "]" << endl ;
			}
		}
	
		void print_sparse(ofstream &of)
		// Print only non-zero values.
		{
			of << "[ " << dim << endl ;
			for ( int j = 0 ; j < dim ; j++ ) {
				if ( fabs(vec[j]) > 0.0 ) 
					of << j << " " << vec[j] << endl ;
			}
			of << "]" << endl ;
		}

	void print_norm(const string filename) const
	// Print shift used in normalizing the vector.
	{
		ofstream outfile(filename) ;
		if ( ! outfile.good() ) {
			cout << "Error: could not open " << filename << endl ;
			stop_run(1) ;
		}
		outfile.precision(14) ;
		outfile << "# Shift " << endl ;
		outfile << std::scientific << std::setprecision(12) << shift << endl ;
		outfile.close() ;
	}
	
	void print_all(ostream &of)
	// Print all values.
		{
			if ( RANK == 0 ) {
				for ( int j = 0 ; j < dim ; j++ ) {
					of << j << " " << vec[j] << endl ;
				}
			}
		}
	double dot(const Vector &vec2) {
		if ( vec2.dim != dim ) {
			cout << "Error: dot product with unmatched dimensions\n" ;
			exit(1) ;
		}
#ifdef USE_BLAS
		double val = cblas_ddot(dim, vec2.vec, 1, vec, 1) ;
#else		
		double val = 0.0 ;
		for ( int j = 0 ; j < dim ; j++ ) {
			val += vec2.get(j) * vec[j] ;
		}
#endif		
		return (val) ;
	}
	void scale(Vector &out, double val) 
	// Scale the vector by the given value, put result in Out.
		{
#ifdef USE_BLAS
			cblas_dcopy(dim, vec, 1, out.vec, 1) ;
			cblas_dscal(dim, val, out.vec, 1) ;
#else			
			for ( int j = 0 ; j < dim ; j++ ) {
				out.set(j, val * vec[j] ) ;
			}
#endif			
		}
	void scale(Vector &out, const Vector &vals) 
	// Multiply the vector by the given array of values, put result in Out.
		{
#ifdef USE_MKL
			vdMul(dim, vec, vals.vec, vec) ;
#else			
			for ( int j = 0 ; j < dim ; j++ ) {
				out.set(j, vals.get(j) * vec[j] ) ;
			}
#endif			
		}
	double l1norm()
	// Returns L1 norm (sum of abs values).
		{
#ifdef USE_BLAS
			double norm = cblas_dasum(dim, vec, 1) ;
#else			
			double norm = 0 ;
			for ( int i = 0 ; i < dim ; i++ ) {
				norm += fabs(vec[i]) ;
			}
#endif			
			return norm ;
		}
	void remove(int idx)
	// Remove the specified index from the vector.
		{
			if ( idx < 0 || idx >= dim ) {
				cout << "Error: bad index to remove from vector: " << idx << endl ;
			}
			for ( int i = idx ; i < dim - 1 ; i++ ) {
				vec[i] = vec[i+1] ;
			}
			--dim ;
		}

	void push(double val)
		// Add the value to the end of the vector.
	{
		double *newv = new double[dim+1] ;
		for ( int j = 0 ; j < dim ; j++ ) {
			newv[j] = vec[j] ;
		}
		newv[dim] = val ;
		delete [] vec ;
		vec = newv ;
		++dim ;
	}
	void add(const Vector &in)
	// Add the vector to vec.
	{
		if ( in.dim != dim ) {
					cout << "Error in add: dim mismatch\n" ;
			exit(1) ;
		}
#ifdef USE_MKL
		vdAdd(dim, vec, in.vec, vec) ;
#else		
		for ( int k = 0 ; k < dim ; k++ ) {
			vec[k] += in.get(k) ;
		}
#endif		
	}
	void subtract(const Vector &in)
	// Subtract the vector from vec.
	{
		if ( in.dim != dim ) {
			cout << "Error in add: dim mismatch\n" ;
			exit(1) ;
		}
#ifdef USE_MKL
		vdSub(dim, vec, in.vec, vec) ;
#else		
		for ( int k = 0 ; k < dim ; k++ ) {
			vec[k] -= in.get(k) ;
		}
#endif		
	}
	void add_mult(const Vector &in, double factor)
		// Set out = out + factor * in
	{
		if ( in.dim != dim ) {
			cout << "Error in add_mult: dim mismatch\n" ;
			exit(1) ;
		}
#ifdef USE_BLAS
		cblas_daxpy(dim, factor, in.vec, 1, vec, 1) ;
#else		
		for ( int k = 0 ; k < dim ; k++ ) {
			vec[k] += factor * in.get(k) ;
		}
#endif		
	}
	void assign_mult(const Vector &in1, const Vector &in2, double factor)
		// Set vec = in1 + factor * in2
	{
		if ( in1.dim != dim || in2.dim != dim ) {
			cout << "Error in add_mult: dim mismatch\n" ;
			exit(1) ;
		}
#ifdef USE_BLAS
		cblas_dcopy(dim, in1.vec, 1, vec, 1) ;
		cblas_daxpy(dim, factor, in2.vec, 1, vec, 1) ;
#else		
		for ( int k = 0 ; k < dim ; k++ ) {
			vec[k] = in1.get(k) + factor * in2.get(k) ;
		}
#endif		
	}
	
	void clear()
	// Clear all entries of the vector.
	{
		if ( dim > 0 ) 
			delete [] vec ;
		vec = NULL ;
		dim = 0 ;
	}
} ;
