// pde.c 
//                     wolf 5/94
//       test of integrations of advection partial differential eq.
//       quadv1 : Smolarkievicz method

// #include <math.h>
// #include <stdlib.h>
#include <stdio.h>

// methods and externals from "solver.c"
void smolarkievicz(int i, int j);
void simple(int i, int j);
void lax_wendroff(int x1,int x2);
void diffusion(double diff, int i, int j);
extern int nx;
extern double *u, *q, dx, dt;

char * method_list[] = { "simple upstream", "Smolarkievicz","Lax-Wendroff",0 };
void (*method[])(int,int) = { simple, smolarkievicz, lax_wendroff };

#include "window.h"   

// !! attn :
// this strings serve as pointers to compare in the action method
// ie. they have to be passed as pointers, because no strcmp takes place
char *dt_menu = "dt", *nt_menu = "nt",
     *breadth_menu = "breadth", *nx_menu = "nx",
     *method_menu = "method", *diff_menu = "diff";

// user-defined class :
// only obligatory virtual functions are : 'draw_interior' && 'action' 
class pde_window : public coord_window {
  int nt, breadth, nmeth; // method-index 
  int breadth_old, nmeth_old, nx_old; // store old values to know when to reset inits
  double dt_old, total;
  int sx, tt;
  int istart,iend; // Impuls Start & Ende
  char headline[200]; 
  double diff;
 public: 
  int show_ratio;  
  void set_inits() { 
    int i; istart = 5; iend = istart + breadth - 1;
    for (i = 0; i < nx; i++) q[i]= 1e-20; // to avoid division overflow  
    for (i = istart; i <= iend; i++) 
      q[i] = 100.0; // initial : step function
    tt = 0; total = 0; // total integrated time
  };
  
  // integrate nt time steps   
  void integrate() { 
    int i;
    for (i=0; i < nt; i++) { 
      if (diff > 0.0) diffusion(diff,0,nx);
      (*method[nmeth]) (0, nx); 

    }
    tt += nt; total += dt*nt;
  }

  pde_window(window & parent, int w, int h, int x, int y) :
    coord_window(parent,w,h,x,y)
    { set_inits();
      nx = 100; nt = 10; dt = 0.1; breadth = 10; nmeth = 1; // 
      show_ratio = 0; diff = 0.0;
    }
   
  void draw_interior() { // virtual fn must be defined : draws window content
    sx = w_eff / nx; // to make it integer
    
    define_coord(0., 0. , nx, 150.);
    draw_coords(); 
    x_ticks((nx < 200) ? 1.0 : 5.0); 
    y_ticks(10.0);
    wline(0, 100.0, nx, 100.0);

    if ((breadth != breadth_old) || (nx != nx_old) || (nmeth != nmeth_old)) 
      set_inits();

    breadth_old = breadth; dt_old = dt; nmeth_old = nmeth; nx_old = nx;

    sprintf(headline,"'%s' with dt =%5.2f nt = %d nx = %d breadth = %d"
	    "diff = %.2f tt = %d time = %.2f",
	    method_list[nmeth], dt, nt, nx, breadth, diff, tt, total);  
    
    // printf("%s\n",headline);
    PlaceText(headline,0,20);

    // display of the theoretical pulse intervall below t axis
    int x1,x2;
    float pp = u[0] * total;  		 // cumulative way
    x1 = x_window(pp + istart - 0.5) % w_eff;  // start of pulse
    x2 = x_window(pp + iend + 0.5) % w_eff;    // end of pulse
    if (x2 >= x1) rline(x1, -10, x2, -10); else	
       { rline(x1, -10, w_eff, -10); rline(0, -10, x2, -10); }

    // draw results
    graph(nx,q);
    if (show_ratio) {
      double rat[nx]; int i;
      for (i=0; i< nx;i++) { 
        rat[i] = 50.0*q[(i+1) % nx]/q[i];
        if (rat[i] > 10000) rat[i] = 0; 
      }
      graph(nx,rat);
    }
  }
  
  // main method "action" : called from radio_buttons of the radio_menus
  // comparing string pointers DIRECTLY with menu names !!
  // to find which menu was activated 

  void action(char * menu, char * value) { 
    // printf("action %s %s\n", menu,value); 
    if (menu == dt_menu) dt = atof(value); else
      if (menu == nt_menu) nt = atoi(value); else
	if (menu == breadth_menu) breadth = atoi(value); else
	  if (menu == nx_menu) nx = atoi(value); else
	    if (menu == method_menu) {
	      int i = 0; char *ms;
	      do { 
                // find method with comparing the string pointer in list
		ms = method_list[i];
		if (value == ms) nmeth = i; 
		i++;
	      } while (ms); 
	    } else 
	      if (menu == diff_menu) diff = atof(value);
		else printf("error: wrong menu %s\n", menu);
    		// nur bei Programmierfehlern !
    redraw();
  }
  void data_dump() { // print state vector to file
    FILE *fp; 
    int i; 
    char fname[20];
    sprintf(fname,"data-dump.%d",tt);
    printf("writing data dump to file '%s'\n",fname); 
    fp = fopen(fname,"w"); 
    fprintf(fp,"%s\n",headline);
    for (i=0; i<nx; i++) {       
      if (i % 8 == 0) fprintf(fp,"\n");
      fprintf(fp,"%10.6f",q[i]);
    }
    fclose(fp);
  }
};

extern "C" usleep(unsigned usecs); // sleep mikrosecs 

pde_window * pwindow;

void quit() { exit(0); }
void integrate() {   
   pwindow->WatchCursor(); 
   pwindow->integrate(); pwindow->redraw(); 
   pwindow->ResetCursor();
}
void re_init() { pwindow->set_inits(); pwindow->redraw(); }
void data_dump() { pwindow->data_dump(); }
void toggle_ratio() { pwindow->show_ratio ^= 1; pwindow->redraw(); }

void animate() { 
  int i;
  for (i=0; i<100; i++) { integrate(); /* usleep(1000); */ } 
}

// derive a special class to use polling handler for running 
class one_main : public main_window {
public:
  int *pmode; // pointer to protected variable
  one_main(char *name, int w, int h) : main_window(name,w,h) { 
    pmode = &polling_mode; 
  }
  virtual void polling_handler() { integrate(); }
};

int main (int /*argc*/, char * argv[]) { 
  int ww = 650, wh = 300, bh = 24;

  one_main mainw(argv[0], ww, wh);

  menu_bar mb(mainw, ww, bh , 0, 0, 50, 100, 0); // to place all buttons here
				// minw = maxw : equal sized buttons
  struct but_cb pd_list[] = { {"re-init",re_init }, {"one step", integrate},
			      {"100 steps",animate},  {"quit", quit } };
  button * menu = make_pulldown_menu(mb, "  menu  ", 4, pd_list); 
  char * mhelp[] = {
    "menu serves to select some actions :",
    "'re-init'  : resets to starting values of integration",
    "'one step' : integrates 'nt' time steps",
    "'100 steps': integrates 100 * nt time steps, ",
    "'quit'     : terminates the program", 0}; 

  menu->add_help(mhelp);
  
  int i;

  double u0;
  int nx_max = 1000; 

  q = new double[nx_max]; // global result vector
  u = new double[nx_max]; // global velocity vector
  u0 = 1.0;  dx = 1.0; 
 
  for (i=0; i < nx_max; i++) u[i] = u0;

  pwindow = new pde_window (mainw, ww, wh-bh, 0, bh + 2);

  // Menu zur dt-Umschaltung
  char *dt_list[] = { "0.01","0.02","0.05","0.1","0.2","0.4","0.5",
		      "0.6","0.7","0.8","0.9","1.0","1.1",0};
  char *dt_help[] = { "time step value [s] for a single step of integration",
		      "equals 'alpha', since dx = v = 1.0 is used",0};
  make_radio_menu(mb, dt_menu, dt_list, dt_help, pwindow);

  char *nt_list[] = {"1","2","3","5","10","30","100","1000","10000",0};
  char *nt_help[] = {"number of single step to integrate,", 
		     "befor the graph is drawn again","",0};
  make_radio_menu(mb, nt_menu, nt_list, nt_help, pwindow);
  
  char *nx_list[] = {"10","20","30","50","80","100","200","300","500",0};
  char *nx_help[] = {"number of discretization intervalls for x",
		     "if the value is changed the integration is restarted",
		     "automatically", 0}; 
  make_radio_menu(mb, nx_menu, nx_list, nx_help, pwindow);
 
  char *breadth_list[] = {"1","2","3","4","5","7","10","20","30","50","80","100",0};
  char *breadth_help[] = {"the breadth of the initial step function",
			  "given in count of dx steps",
			  "changing forces restart",0};
  make_radio_menu(mb,breadth_menu, breadth_list, 
		  breadth_help, pwindow);
    
  char *diff_list[] = { "0","0.01","0.02","0.05","0.1","0.2","0.4","0.5",
		        "0.6","0.7","0.8","1.0","2.0",0};
  char *diff_help[] = { "diffusion coefficient", 0};
  make_radio_menu(mb, diff_menu, diff_list, diff_help, pwindow);

  char *method_help[] = {"selection of integration method ",
			 " * simple upstream",
			 " * Smolarkievicz",
			 " * Lax-Wendroff",
			 0}; 
  make_radio_menu(mb,method_menu,method_list, method_help, pwindow);
   
  new toggle_button(mb,"run",mainw.pmode); // toggles between run/stop

  callback_button data(mb,"data dump",data_dump);
  callback_button ratio(mb,"ratio",toggle_ratio);

  char * help[] = {
    "Changing of parameters with the pulldown-menues in this menue-line ",
    "that are activated with the left mouse button","",
    "Button 'run' starts integration, press it again to stop.","",
    "To get explanations for the individual menues,",
    "press the right mouse button on the field.",0};

  help_button hb(mb,"help",help);
  quit_button qb(mb);

  mainw.main_loop();
}
  
