// wave.c : wolf 7/95
//   demo program for three-dim.c : solution of threedim wave eq.

#include "three-nxnynz.h"

double t[NZP2][NYP2][NXP2], tp[NZP2][NYP2][NXP2], dt[NZP2][NYP2][NXP2];

double gamma;
double deltat = 10;
double cdx = 0.001; // paramter == c*dx

double sqr(double x) { return x*x; }

void wave_init(int *tptr) {
  int x,y,z;
  for (x=0; x<NXP2; x++) for (y=0; y<NYP2; y++) for (z=0; z<NZP2; z++) {
    t[z][y][x] = 0.; 
    tp[z][y][x] = 0.;
  }
  for (x=4; x<7; x++) for (y=4; y<7; y++) for (z=0; z<3; z++) 
   t[z][y][x] = 0.02; // initial impuls
  *tptr = 0;
  gamma = sqr(cdx*deltat);
  //  printf("init %x\n",&t);
}

void wave_step(int *tptr) { 
  int x,y,z;
  for (x=0; x<NXP2; x++) for (y=0; y<NYP2; y++) for (z=0; z<NZP2; z++) {

    double tt = t[z][y][x], dt2 = 6*tt;
    // assumes values beyond boundaries = 0
    if (x > 0)      dt2 -= t[z][y][x-1]; 
    if (x < NXP2-1) dt2 -= t[z][y][x+1];
    if (y > 0)      dt2 -= t[z][y-1][x]; 
    if (y < NYP2-1) dt2 -= t[z][y+1][x];
    if (z > 0)      dt2 -= t[z-1][y][x]; 
    if (z < NZP2-1) dt2 -= t[z+1][y][x];
       
	t[z][y][x] =  2*tt - tp[z][y][x] - gamma*dt2;
    dt[z][y][x] = tt -  tp[z][y][x];
    tp[z][y][x] = tt;
  }
  *tptr += (int) deltat;

}

void wave_exit() {
}


 
