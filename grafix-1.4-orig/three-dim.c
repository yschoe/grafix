// three-dim.c :                 wolf 7/95
// demo-program for the integrator and video functionality of grafix
// invokes "wave.c" (threedim wave function integrator) interactively
// may be considered as frame for hooking own applications 
// the saved files may be reviewed with program "replay"
// if a command line argument is given it is used as base-filename, 
// else the progname is used to build a name "base.video"
//

#include "animator.h"

// interface description for three-dim
// for linking with other integration sources :
// replace wave_* with your function names

#include "three-nxnynz.h"

extern void wave_init(int*); // the initialization part of the integrator
extern void wave_step(int*); // integration time step of 
extern void wave_exit();     // for cleanup

extern double t[NZP2][NYP2][NXP2];

char *three_help[] = {
  "'three-dim' serves as reference program for binding other integration",
  "programs (even Fortran programs) with the Grafix user interface.","",
  "Please start the integration (with '>>') and let it run for some seconds.",
  "It produces the output file 'three-dim.video' which can be viewed with 'replay'",
  "","The other buttons :",
  "'rotate' lets the view rotate around z-axis, to stop press it again",
  "'layer' selects a z-array to display","'reinit' resets all to t = 0",
  "'>' makes one integration step","'||' stops the running integration",
  "'spawn' opens another window to enable viewing several z-layers simultaneously",0 };
  
class three_integrator : public integrator {
public:
  three_integrator(char *fina, char *video_file) :
    integrator(fina,video_file,NXP2,NYP2,100) { // every 100 sec write video
    d2disp *tdisp = new d2disp[NZP2]; 
    int i, nz =  MIN(5, NZP2); // write only max. 5 arrays to save disk space
    for (i=0; i < nz; i++) { 
      tdisp[i].name = new char[20]; sprintf(tdisp[i].name,"t layer %d",i);
      tdisp[i].array.darr = (double*) t[i]; 
      tdisp[i].gamma = 1.0; tdisp[i].z0 = 0;
      tdisp[i].array_mode = ARRAY_NY_NX | ARRAY_DOUBLE;
    }
    // there might be up to 10 columns added to the menu (and saved)
    add_column("layer", nz , tdisp); 
    sprintf(header,"three-dim demo");
    new help_button(*mb,"help",three_help);
    start();
  }
  // the virtual functions to define, itime is the actual time in seconds
  // to be updated from the step-function (wave_step)
  virtual void init_solver() { wave_init(&itime); }
  virtual void step_solver() { wave_step(&itime); }
  virtual void exit_solver() { wave_exit(); }
};


int main(int argc, char *argv[]) {
  // if a command line argument is given it is used as base-filename for 
  // the video file else the progname is used
  char *fina = (argc > 1) ? argv[1] : argv[0]; 
  char video_file[100]; // the name of the file to save video to 
  sprintf(video_file,"%s.video",fina); // is named eg. "three_dim.video"

  new three_integrator(fina, video_file);

}
