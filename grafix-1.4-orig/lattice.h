// lattice.h

#ifndef LATTICE_H
#define LATTICE_H

#include <stdlib.h>
#include <math.h>
#include "window.h"
#include "palette.h"

#define SQR(x) ((x)*(x))

extern "C" double sqrt(double);
extern int sqabs(XPoint p1, XPoint p2);

extern float grd2rad;            // conversion grad -> radiant	
extern palette_popup *pal_win;   // the palette_window

class lattice_window : public coord_window {
  int *y_bot, *y_top;
protected:
  float ca,sa,cb,sb,dist,xp,yp,zmax,znorm;
  int *external_colors;     // if to use own colors for painting the surfaces n
                            // instead of computing them from light source   

  int ncolors;  // number of colors for body && palette
  Bool flat_mode; // no 3-dim perspective

  void pline(XPoint a, XPoint b) // draws lines uncoditionally
   { line(a.x,a.y,b.x,b.y); }

  void cline(XPoint a, XPoint b, int lastp);
  void fill(XPoint a, XPoint b, int lastp, int inside);

  void xline(XPoint a, XPoint b, int lastp) { 
    if (opaque) cline(a,b, lastp);  else pline(a,b); }
  XPoint screen_project(float x, float y, float z);

public:
  XPoint *scptr; // array of transformed grid points as  [nx][ny] 
  int ixstart,iystart,// the start indicees for the representation from qq-array
      ixend, iyend;
  int xspan, yspan;  // the max values of x,y in org.
  int opaque; 
  int rand;  // 0 : with border lines
  int body;  // toggle between lattice and body display
  int nlevels; // for isolines (default = 10)
  float *flevel; // default : 0.1*n, n = 1..10
  lattice_window(window & parent, int w, int h, int x, int y);
  virtual ~lattice_window();
 
  // alpha, beta : rotation angles (here rad!), gamma : z-factor,
  void make_body(int nx, int ny, float *FF, 
		 float alpha, float beta, float gamma,  float z0, int opaque, 
		 float distance, float al, float bl);
  // draw one isoline for spec. flevel
  void isolevel(int nx, int ny, float *FF, float flevel); 
};

// class for simple isolines : not yet completely implemented !!!
class isoline_window : public coord_window {
protected:
  int nx,ny; // grid dimension 
  float *qptr; // pointer to array of grid values
private:
  int nlevels; // # of levels
  float *levels; // level vector
  Bool raster; // draw raster lines
public:
  isoline_window(window &parent, int w, int h, int x, int y, int nx, int ny, 
	   float *qptr, int nlevels, float *levels);
  void isolevel(float flevel); // draw one isoline for spec. flevel
  virtual void draw_interior();
};

class lattice_manager;

class region_manager : public coord_window {
  int &nx,&ny;     //  references to  lm->nx, lm->ny !!
  float xmax,ymax;
public:
  lattice_manager * lm;
  XPoint zc;   // zoom centre

  void init_region();
  void rise();
  void shrink();

  region_manager(window &parent, lattice_manager *lm);
  virtual ~region_manager() {}
  virtual void draw_interior();

  XPoint raster(int x, int y);
  XPoint p_window(float x, float y) ;
  void mark_rect(float x0, float y0, float dx, float dy) ;
  void Rectangle(GC gc, XPoint p1, XPoint p2); 

  XPoint pstart, pact, pmin, pmax;
  virtual void BPress_1_CB(XButtonEvent ev);
  virtual void BRelease_CB(XButtonEvent ev);
  virtual void Motion_CB(XMotionEvent ev);
};

// generates a lattice_window and the buttons to manage it
class lattice_manager : public lattice_window {
private:
  menu_bar *mb1,*mb2; // the two menu  lines at the bottom
protected:
  main_window *light_win, // the popup for light incidence angles
              *region_win;  // popup for  region_manager 
  region_manager *region_int;  // the region_manager (for redraw-ops)
  lattice_manager * clone_lm;  // a list of clone copies (resp. NULL)

public:
  int nx, ny; // the grid dimensions
  float *qptr; // pointer to 2-d array with grid values of the function
  float alpha, beta, a_light, b_light;
  float dist, gamma, z0;

  void init_region() { ixstart = 0; ixend = nx; iystart = 0; iyend = ny; }

  void set_toggles(int pbody, int prand, int popaque, int pflat) 
    { body = pbody; rand = prand; opaque = popaque; flat_mode = pflat; } 

  void set_angles(float palhpa, float pbeta, float pgamma, float pdist)
    { alpha = palhpa; beta = pbeta; gamma = pgamma; dist = pdist; }

  void set_defaults();

  lattice_manager(window & parent, int w, int h, int x, int y, int nx, int ny,
		  float * q);
    
  virtual ~lattice_manager();

  // changing the grid 
  void respace(int nxp, int nyp) { 
    nx = nxp; ny = nyp;  
    init_region(); 
    if (clone_lm) clone_lm->respace(nxp,nyp);
  }

  void default_draw() { // for default button
     set_defaults(); redraw();
  } 

  virtual void redraw() {
    lattice_window::redraw(); region_int->redraw(); 
  }
  void redraw_clones() {
    redraw(); if (clone_lm) clone_lm->redraw_clones();
  }
  void show_infos(XButtonEvent*);
  void draw_interior();
  void action(char*, char* ) { }

  void rotate_alpha(float delta) { alpha += delta; redraw(); }

  // Callback-functions for display buttons 
  void mult_float(float *val, float increment) { *val *= increment; redraw();}
  void inc_int(int *val, int increment)   { *val += increment; redraw(); }
  void inc_float(float *val, float increment) { *val += increment; redraw(); }
  void inc_z0(float *val, float increment) // used for z0 shifting
    { *val += increment/gamma; redraw(); }

  void toggle(int *ref, int v) { *ref ^= v; redraw();}
  
  // Mouse drawing of the cordinate system, for quick change of viewpoint
  virtual void BPress_1_CB(XButtonEvent ev);
  virtual void BPress_3_CB(XButtonEvent ev);

  virtual void Motion_CB(XMotionEvent ev);
  virtual void BRelease_CB(XButtonEvent ev);

  // the arrow-keys turn the image for 5 grd
  virtual void KeyPress_CB(XKeyEvent ev);

  // produces a popup with the same parameters as actual
  lattice_manager* make_popup(char *Name, float *q);

  // produces an identical copy of the picture with same array
  void make_clone();
  void delete_clone();
};

class light_popup : public main_window {
 public:
  lattice_manager *lm; // the corresponding lattice_manager
  int immediate;
  scrollbar *sc_alpha, *sc_beta;
  light_popup(lattice_manager *latm, int w);
};


#endif // LATTICE_H
