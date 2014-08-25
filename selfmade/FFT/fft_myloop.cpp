#include <math.h>
#include <iostream>
#include <iomanip>
#include <complex>

double h(int ii) {
   return sin(6.0 * M_PI * ii) + cos(M_PI/3.0 * ii);
};

double sin(int ii) {
   return std::sin(ii);
};


int main() {

int const N = pow(2,7);
std::complex<double> data[N];
std::complex<double> FT[N];

// function h(t) = t^3 + 1/(1 + t)

// create the N sample on t

for (int i = 0 ; i < N; i++) {
   data[i] = std::complex<double>(h(i),0.0);
   //data[i] = std::complex<double>(sin(i),0.0);
};

//Computing the FT using FT(k) = sum(h(k)*exp(2*PI * img * k * n / N )) pe k = 0..N-1
// This should go as O(N^2)
//
//

double pi(M_PI);
std::complex<double> img(0.0,1.0);
std::cout << std::setprecision(10) << "N = " << N << std::endl;
std::cout << std::setprecision(10) << "pi = " << pi << std::endl;
std::cout << std::setprecision(10) << "img = " << img << std::endl;

std::complex<double> W = std::exp( std::complex<double>(-2.0 / N) * pi * img );
std::cout << std::setprecision(10) << "W = " << W << std::endl;
std::complex<double> Wnk(0.0,0.0);

int ii = 0;
for (int n = -N / 2; n < N / 2 ; n++) {
//for (int n = 0; n < N -1 ; n++) {
   FT[ii] = std::complex<double>(0.0,0.0);
   for (int k = 0; k < N - 1; k++) {
      Wnk =  std::pow(W , n * k );
      FT[ii] = FT[ii] + data[k] * Wnk;
   }
   ii++;
}

//for (int i = 0 ; i < N; i++) {
//   std::cout << std::setprecision(10) << data[i] << "   " ;
//}
std::cout << std::endl;
std::cout << std::endl;

for (int i = 0 ; i < N; i++) {
   std::cout << std::setprecision(10) << (-1.0 * N/2 + i ) / N << " - " << FT[i] << std::endl ;
}
std::cout << std::endl;

return 0;

}
