#include "lattice.h"

int nx, ny;
float *qexp,*qsin,*qigl;

void compute() {
  float (*aexp)[ny] = (float (*)[ny]) qexp;
  float (*asin)[ny] = (float (*)[ny]) qsin; 
  float (*aigl)[ny] = (float (*)[ny]) qigl;
  int x,y;
  for (x = 0; x < nx; x++) { 
    float xp = ((float) x)/nx - 0.5;  // Intervall [-0.5 .. +0.5]
    for (y = 0; y < ny; y++) { 
      float yp = ((float) y)/ny - 0.5;
      float r = sqrt(SQR(xp) + SQR(yp));    
      aexp[x][y] = exp (-10*r*r); // normed to 1.0
      asin[x][y] = .25 * (1.0 + cos(15*r)); // normed to 0.5
      float rad = 1 - SQR(xp*2) - SQR(yp*3);
      aigl[x][y] = (rad < 0) ? 0 : sqrt(rad);
    } 
  }
}

lattice_manager *lm;

void selexp() { lm->qptr = qexp; lm->redraw();}
void selsin() { lm->qptr = qsin; lm->redraw();}
void seligl() { lm->qptr = qigl; lm->redraw();}

// needed to have an action method for the radio_menu "nx" and rotate mode
class main_action_window : public main_window { 
public:
  main_action_window(char *Name, int w, int h) : main_window(Name,w,h) {
    int bh = 20;
    nx = ny = 30; // initial values
    qexp = new float[10000]; // >= max(nx)*max(ny);
    qsin = new float[10000];     
    qigl = new float[10000]; 
    compute();

    menu_bar *mb = new menu_bar(*this,w,bh,0,0,70,100,0);
    lm = new lattice_manager(*this,w,h-bh,0,bh,nx,ny,qsin);

    struct but_cb m_list[] = {{"sin",selsin}, {"gauss",selexp}, 
			      {"iglo",seligl}};
    make_pulldown_menu(*mb,"menu",3,m_list);
    char *nx_list[] = {"3","5","10","15","20","25","30","40","50","60","80",0};
    make_radio_menu(*mb,"nx",nx_list,mainw);
    new toggle_button(*mb,"rotate",&polling_mode); 
    new quit_button(*mb);
    main_loop(); // loop forever ....
  }

  virtual void action(char* /*menu*/, char *value) {
    nx = ny = atoi(value); 
    compute();
    lm->respace(nx, ny); // redraws the lattice
    lm->redraw_clones();
  } 
  virtual void polling_handler() { lm->rotate_alpha(0.05); }
};

int main (int /*argc*/, char * argv[]) {
  new main_action_window(argv[0], 450, 500);
}
