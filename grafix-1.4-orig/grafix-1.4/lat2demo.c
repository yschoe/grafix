/* lat2demo.c : wolf 10/94
   demonstrates the use of lattice_manager class
   generates pictures of two functions inside one main window, each having
   its own lattice-manager functionality buttons for manipulating
*/
#include "lattice.h"

// computes the function values as twodim. arrays for two function types
void compute(int typ, float *q, int nx, int ny) {
  float (*qq)[ny] = (float (*)[ny]) q; // type cast from float* -> float(*)[ny]
  int x,y;
  for (x = 0; x < nx; x++)  
    for (y = 0; y < ny; y++) { 
      float r = sqrt(SQR(x-nx/2) + SQR(y-10));    
      switch (typ) {
      case 1:  
	qq[x][y] = exp (-r*r/10); break;
      case 2:
	qq[x][y] = 0.3*(1 + sin(r)); break;
      }
    } 
}

int main (int /*argc*/, char * argv[]) {
  lattice_manager *lm1,*lm2;
  int ww = 905, wh = 400;

  main_window *mainw = new main_window(argv[0], ww, wh);
  
  int nx = 20, ny = 20;
  float *q1 = new float[nx*ny];
  compute(1,q1,nx,ny);
  lm1 = new lattice_manager(*mainw,ww/2-2,wh-2,0,0,nx,ny,q1);
  new quit_button(*lm1,60,20,0,0);

  float *q2 = new float[nx*ny];
  compute(2,q2,nx,ny);
  lm2 = new lattice_manager(*mainw,ww/2-2,wh-2,ww/2,0,nx,ny,q2);
  lm2->body = False;  // set to grid-mode for this function initially

  mainw->main_loop();
  delete[] q1;  delete[] q2; 
}
