// two-dim.c : two dimensional compution of advection equation
// using different integration methods a
// and graphic display with the lattice class

#include "lattice.h"
#include "smolark.h"
#include <math.h>

#define Pi 3.14159265358979323846

char * method_list[] = { "smolark", "smolark_wrap","simple",0 };
void (*method[]) (float *qp, float *up, float *vp, float dt, int nx, int ny, 
		  int,int,int,int) = { smolark, smolark, simple };

char *dt_menu = "dt", *nt_menu = "nt",
     *breadth_menu = "breadth", *nx_menu = "nx",  *ny_menu = "ny", 
     *dir_menu = "direction", *method_menu = "method", *run_menu = "n steps";

class two_dim;
// to display the headline : use pixmap_window to avoid flickering
class head : public pixmap_window { 
  two_dim  * td;
public:
  head(window &parent,two_dim  * td, int w, int h, int x, int y);
  virtual void draw_interior() ;
};
				
class two_dim : public lattice_manager {
  head * headline;
  char headstr[200];
  float *qp, *up, *vp; 
  int istart,iend,jstart,jend,tt,nt,nq,breadth,nmeth;
  int breadth_old,nmeth_old,nx_old;

  float dt,total; // time step, total elapsed time
  float xctr,yctr; // coordinates of pulse
  int  direction; // angle of wind flow (in degrees 0..360)

  float &u(int i, int j) { return *(up + ny*i + j); }
  float &v(int i, int j) { return *(vp + ny*i + j); }
  float &q(int i, int j) { return *(qp + ny*i + j); }

public:
  char * headprint() {
    sprintf(headstr,"%s dir = %d dt= %5.2f nt = %d tt= %d time = %7.2f",
	    method_list[nmeth],direction,dt,nt,tt,total); return headstr;
  }
  void set_inits() { // set q to initial values for restart
    int i,j; 
    istart = nx/10; jstart = ny/10; 
    iend = istart + breadth+1; jend = jstart + breadth+1;
    for (i=0; i<nx; i++) for (j=0; j<ny; j++) { 
      // initial step function computed from global (xstart, xend)
      // step value proportional to nx/2, only for lattice-display form
      q(i,j) = (i>istart && i<iend && j>jstart && j<jend) ? 1 : 0.1;
    }
    total = 0.0; // total integrated time
    tt = 0;
    // alpha = (direction - 90)/grd2rad; // viewpoint for lattice from the side
    alpha = 30/grd2rad; beta = 60/grd2rad; // starting values for projektion.
    xctr = istart; yctr = jstart;  
  }
  void init_uv() { // set stream field according to angle, |u*u + v*v| == 1.0
    int i,j; 
    float a = cos(direction/grd2rad), b = sin(direction/grd2rad);
    for (i=0; i<nx; i++) for (j=0; j<ny; j++) {
      u(i,j) = a; v(i,j) = b; }
  }  

  void new_arrays() { // create new arrays on the heap and initialize them   
    qp = new float[nx*ny]; 
    up = new float[nx*ny]; vp = new float[nx*ny];
    init_uv();
    set_inits();
    qptr = qp; 
  }

  two_dim(window &parent, int w, int h, int x, int y, int nx, int ny)
    : lattice_manager(parent, w, h-20,x, y+20, nx, ny, NULL) {
    headline = new head(parent,this,w,20,x,y);
    dt = 0.1; nt = 1; breadth = 5; direction = 45; 
    new_arrays();
    nmeth = 0; // Start with smolarkievicz as default
    // headline->CB_info = 1;
  }

  void redraw() {
    lattice_manager::redraw();
    region_int->mark_rect(xctr,yctr,breadth,breadth);  
  }

  void integrate() { 
    WatchCursor();
    int i; for (i=0; i<nt; i++) (*method[nmeth])(qp,up,vp,dt,nx,ny,0,nx,0,ny); 
    tt += nt; total += dt*nt;
    xctr += u(0,0)*dt*nt; yctr += v(0,0)*dt*nt; // nur fuer konstante u,v 
    ResetCursor();
  }
  
  // main method "action" : called from radio_buttons of the radio_menus
  // comparing string pointers DIRECTLY with menu names !!
  // to find which menu was activated 

  void action(char * menu, char * value) { 
    //   printf("action menu = %s value = %s\n", menu,value); 
    if (menu == dt_menu) dt = atof(value); else
      if (menu == nt_menu) nt = atoi(value); else
	if (menu == breadth_menu) breadth = atoi(value); else
	  if (menu == nx_menu || menu == ny_menu) { 
	    if (menu == nx_menu) nx = atoi(value); else ny = atoi(value);  
	    delete qp; delete up; delete vp;
	    new_arrays();
	    respace(nx,ny);
	  }
	  else
	    if (menu == method_menu) {
	      int i = 0; char *ms;
	      do { 
		// find method with comparing the string pointer in list
		ms = method_list[i];
		if (value == ms) nmeth = i; 
		i++; }  while (ms); 
	    } else 
	      if (menu == dir_menu) 
		{ direction = atoi(value); init_uv(); }
	      else
		if (menu == run_menu)
		  { int i,n = atoi(value);
		    for(i=0;i<n;i++) { integrate(); redraw_all(); } 
		    return; } 
		else 
		  printf("error: wrong menu %s\n", menu);
    if ((menu == breadth_menu) || (menu == method_menu)) set_inits(); 
    redraw_all();
  }

  void redraw_all() { 
    headline->redraw(); redraw_clones();
  }

  // ein popup-Window mit dem aktuellen Funktionsbild erzeugen -> snapshot
  void spawn() {
    // das popup hat dieselbe Groesse wie das momentane und den Kopf als Name
    float *qspawn = new float[nx*ny]; 
    int i; // Erzeugen einer Kopie des Feldes : 
    for (i=0; i<nx*ny; i++) qspawn[i] = qptr[i];
    lattice_manager *snap = make_popup(headstr,qspawn);
    main_window *pp = snap->mainw;
    // der delete_button sollte auch noch "delete [] qspawn" ausloesen ! 
    new delete_button (*pp,pp,100,20,0,0);
    pp->do_popup(0,0); // realize it
  }

};

// definition of methods follows here, since declaration of two_dim is used
head::head(window &parent, two_dim *td, int w, int h, int x, int y):
      pixmap_window(parent,w,h,x,y,0), td(td) { }	
// compute new string and display it 
void head::draw_interior() { PlaceText( td->headprint() ); }
	
two_dim *two;

void re_init() { two->set_inits(); two->redraw_all(); }
void step() { two->integrate(); two->redraw_all();}

void snapshot() { two->spawn(); }

// derive a special class to use polling handler for running 
class two_main : public main_window {
public:
  int *pmode; // pointer to protected variable
  two_main(char *name, int w, int h) : main_window(name,w,h) { 
    pmode = &polling_mode; 
  }
  virtual void polling_handler() { step(); }
};

int main (int /*argc*/, char * argv[]) {
  int ww = 560, wh = 500, bh = 20;
  
  two_main mainw(argv[0], ww, wh);
  menu_bar mb(mainw,ww,bh,0,0,20,80,0);

  callback_button reinit(mb,"re-init",re_init);
  two = new two_dim(mainw,ww,wh-bh,0,bh+2,20,20);

  char *dt_list[] = { "0.01","0.02","0.05","0.1","0.2","0.4","0.5",
		      "0.6","0.7","0.8","0.9","1.0",0};
  char *dt_help[] = { "time step value [s] for a single step of integration",
                     "equals 'alpha', since dx = v = 1.0 is used",0};

  make_radio_menu(mb, dt_menu, dt_list, dt_help, two);

  char *nt_list[] = {"1","2","3","5","8","10","20","30","50","80","100",0};
  char *nt_help[] = {"number of single step to integrate,", 
		     "befor the graph is drawn again","",0};
  make_radio_menu(mb, nt_menu, nt_list, nt_help, two);
  
  char *nx_list[] = {"10","20","30","50","80","100","200","300",0};
  char *nx_help[] = {"number of discretization intervalls for x",
		     "if the value is changed the integration is restarted",
		     "automatically", 0};
  make_radio_menu(mb, nx_menu, nx_list, nx_help, two); 
  char *ny_list[] = {"3","5","10","15","20","30","50","80","100","200",0};
  make_radio_menu(mb, ny_menu, ny_list, nx_help, two);

  char *breadth_list[] = {"1","2","3","4","5","7","10","20","30","50","80",0};
  char *breadth_help[] = {"the breadth of the initial step function",
			  "given in count of dx steps",
			  "changing forces restart",0};

  make_radio_menu(mb,breadth_menu, breadth_list, 
		  breadth_help, two);
  char *dir_list[] = {"0","5","10","15","20","30","40","45",0};
  char *dir_help[] = {"angle of wind direction in grd", 
		      "0 : in x-direction, 90 : in y-direction",0};
  make_radio_menu(mb, dir_menu, dir_list, dir_help, two);
  
  char *method_help[] = {"selection of integration method ",
			 " * simple upstream",
			 " * Smolarkievicz",
			 0}; 
  make_radio_menu(mb,method_menu,method_list, method_help, two);
   
  new toggle_button(mb,"run",mainw.pmode); // toggles between run/stop

  char *run_list[] = {"5","10","20","30","50","80","100","150","200",0};
  char *run_help[] = {"integrates the given number of time steps",0};
  make_radio_menu(mb, run_menu, run_list, run_help, two);
  
  callback_button snapshot(mb,"snapshot",&snapshot); 
  char *snap_help[] = {"'snapshot' spawnes a copy of the actual picture ",
		       "as permanent lattice_manager window, ",
		       "that will not be integrated further",0};    
  snapshot.add_help(snap_help);

  char * help[] = {
    "Changing of parameters with the pulldown-menues in this menue-line ",
    "that are activated with the left mouse button","",
    "Button 'run' starts integration, press it again to stop.","",
    "To get explanations for the individual menues,",
    "press the right mouse button on the field.",0};

  help_button hb(mb,"help",help);
  quit_button qq(mb);

  mainw.main_loop();

}


