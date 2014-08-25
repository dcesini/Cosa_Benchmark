#include <math.h>
#include <iostream>
#include <iomanip>
#include <complex>
#include <fftw3.h>

float h(int ii) {
   
   return sin(6.0 * M_PI * ii) + cos(M_PI/3.0 * ii);
};

float sin(int ii) {
   return std::sin(ii);
};


int main() {

int const N = pow(2,7);
fftw_complex *in, *out;
fftw_plan p;
in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

// create the N sample on t
for (int i = 0 ; i < N; i++) {
   in[i][0] = h(i);
   //in[i][0] = sin(i);
   in[i][1] = 0.0;
};


fftw_execute(p); /* repeat as needed */



//for (int i = 0 ; i < N; i++) {
//   std::cout << std::setprecision(10) << "( " << in[i][0] << " , " << in[i][1] << " )" << std::endl;
//}
std::cout << std::endl;
std::cout << std::endl;

for (int i = 0 ; i < N; i++) {
   std::cout << std::setprecision(10) << float(i) / N << " - " << "( " << out[i][0] << " , " << out[i][1] << " )" << std::endl;
}
std::cout << std::endl;


fftw_destroy_plan(p);
fftw_free(in); fftw_free(out);

return 0;

}
