// File lat_man.c :  lattice_manager - class

#include "lattice.h"
extern window root_window;
 
// to display infos in the lattice_window with Button3 in a moving window
class display_cursor : public window {
public:
  char val[50];
  Window ll; // ein Window fuer eine simple vertikale Linie
  display_cursor(window &parent) : window(root_window,60,20,0,0,1) {
    // it must be defined as child of root to disable clipping !
    mainw = (main_window*) this; 
    XSetWindowAttributes attr; 
    attr.override_redirect = TRUE; attr.save_under = TRUE;

    XChangeWindowAttributes(display, Win, 
			    CWOverrideRedirect | CWSaveUnder, 
			    &attr);
   
    ll = XCreateSimpleWindow(display, parent.Win,0,0,2,10,0,0,1);
    XSelectInput(display, Win, ExposureMask);
  }
  void Move(XPoint p, XPoint wpos) { 
    width = 7*strlen(val); 
    XMoveResizeWindow(display,Win,p.x-width/2 + wpos.x,p.y-30 + wpos.y,
		      width, height);     
    XMoveWindow(display,ll,p.x,p.y-8); 
    XMapWindow(display,ll); 
    redraw(); XMapWindow(display,Win);
  }
  virtual void redraw() { // clear(); 
    extern GC button_fg_gc;
    XFillRectangle(display, Win, button_fg_gc, 0, 0, width, height);
    PlaceText(val);
  }
  void hide() { Unmap(); XUnmapWindow(display,ll); }
};

// *************** lattice_manager *************
// class lattice_manager : public lattice_window 
// generates a lattice_window and the buttons to manage it

void lattice_manager::set_defaults() { 
  set_angles(40/grd2rad, 55/grd2rad, 1, 0); 
  set_toggles(1,1,1,0);
  init_region();
}

void lattice_manager::draw_interior() 
{ char headline[200]; 
  // WatchCursor(); // show watch cursor here   
  alpha = fmod(alpha, 2.0*M_PI); // normalization of alpha
  make_body(nx, ny, qptr, alpha, beta, gamma, z0, opaque, dist, 
	    a_light,b_light);
  
  //  make_lattice(nx, ny, qptr, alpha, beta, gamma, z0, opaque, dist);
  // ResetCursor(); and reset it again
  sprintf(headline,"alpha = %d beta = %d gamma = %g d =%5.2f",
	  (int) (alpha*grd2rad), (int) (beta*grd2rad), gamma,dist); 
  PlaceText(headline,0,height - 10); 
  // unterer Rand,
}

// draws with-moving display-window in original[ x,y,z ]
static display_cursor *dpcur = NULL;

static int ixdp, iydp;
void lattice_manager::show_infos (XButtonEvent *ev) { 
  // back transformation for a point in the   x,y-plane
  // mouse-coordinates (px,py) -> index-coordinates (ix,iy)
  /*  float xs = x_org(ev->x), ys = y_org(ev->y)/cb,
      x = ca*xs + sa*ys + xp, y = ca*ys - sa*xs + yp;
      int ix = (int) (x+0.5) + ixstart, iy = (int) (y+0.5) + iystart;  
   */
  XPoint (*scp)[ny] = (XPoint (*)[ny]) scptr; // array of grid values
  float (*ff)[ny] = (float (*) [ny]) qptr; // array of function values

  // new : search for next grid point to cursor pos (real height)
  int ix = ixstart, iy = iystart, delta, delmin = 1000000;
  for (int ixt = ixstart; ixt < ixend; ixt++) 
    for (int iyt = iystart; iyt < iyend; iyt++) {    
      XPoint sxy = scp[ixt][iyt];
      delta = SQR(ev->x - sxy.x) + SQR(ev->y - sxy.y);
      if (delta < delmin) { delmin = delta; ix = ixt; iy = iyt; }
    }
  if (dpcur == NULL) dpcur = new display_cursor(*this);
  if (ix < ixstart || ix >= ixend || iy < iystart || iy >= iyend) return; 

  if (ix != ixdp || iy != iydp) {
    ixdp = ix; iydp = iy;
    sprintf(dpcur->val,"x: %d y: %d z: %g",ix,iy,ff[ix][iy]); 
    // wpos : absolute coords of window (rel to root)
    XPoint wpos = { ev->x_root - ev->x, ev->y_root - ev->y };
    dpcur->Move(scp[ix][iy], wpos);    
  } 
}

/* move the lattice with mouse pointer :
   an arbitrary point (to be thought in the co-ordinate plane z=0)
   can be clicked on with left button (usually the origin)
   then it can be drawn with the pointer and the new reference frame
   moves along with it.
*/
static int xctr,yctr; 
static float phi, yzs, r2; // parameter for computing new alpha, beta
static XPoint scoord[6];
static Bool clear_old;

void lattice_manager::BPress_1_CB(XButtonEvent ev) {  
  xctr = width/2; yctr = height/2; // centred co-oordinates, start values
  int xs = ev.x - xctr, ys = ev.y - yctr; 
  float xsf = xs/float(width); // norming because aspect-verzerrung
  yzs = ys/cb/height; r2 = SQR(yzs) + SQR(xsf);
  phi = alpha + atan2(yzs,xsf); 
  clear_old = False;
}

void lattice_manager::BPress_3_CB(XButtonEvent ev) {
  ixdp = -1; // make sure it is new point
  show_infos(&ev);
}

void lattice_manager::BRelease_CB(XButtonEvent ev) 
{ if (ev.state & Button1Mask) redraw(); else
    if (ev.state & Button3Mask) dpcur->hide();
}

// draws the pulled co-ordinate system 
void temp_coord(Window Win, int /* body */, XPoint scoord[]) {
  int i;
  for (i=0; i<3; i++) XDrawLines(display,Win,gc_rubber,
				 scoord + 2*i, 2, CoordModeOrigin);
}

void lattice_manager::Motion_CB(XMotionEvent ev) { 
 if (ev.state & Button1Mask) { // button 1 is pressed
   int xss = ev.x - xctr, yss = ev.y - yctr;
   if (SQR(xss) + SQR(yss) < 10) return; // too indifferent
   float xsf = xss/float(width), ysf = yss/float(height);
   float nom = r2 - SQR(ysf) - SQR(xsf);
   if (nom < 0) return; // oder : beta = 0;
   beta = atan2(sqrt(nom), fabs(ysf)); 
   // if ysf is used instead of fabs(ysf) -> the bottom view is shown
   // instead of rear view
   alpha = phi  - atan2(ysf/cos(beta),xsf); 
   
   if (clear_old) temp_coord(Win,body,scoord); // delete old frame
   clear_old = True;
   ca = cos(alpha); sa = sin(alpha); cb = cos(beta); sb = sin(beta);
     
   scoord[0] = scoord[2] = scoord[4] = screen_project(0,0,0);
   scoord[1] = screen_project(0,0,zmax); 
   scoord[3] = screen_project(xspan,0,0);
   scoord[5] = screen_project(0,yspan,0);
   temp_coord(Win,body,scoord);
 } else if (ev.state & Button3Mask) show_infos((XButtonEvent*) &ev); 
}

// the arrow-keys turn the image for 5 grd
void lattice_manager::KeyPress_CB(XKeyEvent ev) 
{ KeySym keysym = XLookupKeysym(&ev,0);
  /*char * sym = XKeysymToString(keysym);
    printf("0x%x sym = '%s'\n", keysym,sym);
    */
  int change = TRUE;
  switch (keysym) {
  case XK_Left:  alpha -= 0.1; break;
  case XK_Right: alpha += 0.1; break;  
  case XK_Up:    beta += 0.1; break;
  case XK_Down:  beta -= 0.1; break;
  default: change = FALSE;
  }
  if (change) redraw();
}

// makes button with special callback-function for changing a T-value 
// with the pointer "T *ref" and the passed value "T val"
template <class T>
class modifier_button : public button { 
lattice_manager * lat; 
void (lattice_manager::*callback)(T*,T);
T *v1, v2;
public:
  modifier_button(menu_bar &parent,char * Name, 
		  void (lattice_manager::*cb)(T*,T), 
		  lattice_manager *lm, T *ref, T val) :
    button(parent,Name) { callback = cb; lat = lm; v1 = ref; v2 = val;}
  virtual void BRelease_1_action() { (lat->*callback)(v1,v2); }
};


static char *lattice_help[] = { 
 "* To toggle between the grid- and body-mode use the 'body' button", 
 "* To change the viewpoint use the mouse pointer :",
 "  By clicking an arbitrary point in the xy-plane (test first with origin)",
 "  with the left mouse button and pulling it, a temporary co-ordinate system",
 "  is shown which will become the new cs when the button is released",
 "* The right mouse button is used to display values on the grid points","",
 "* The other buttons in the two bottom rows are used to change the view :",
 "  a+/-  : rotate the x,y-plane (horizontal) for 5 grd",
 "  b+/-  : rotate the y,z-plene (vertical 'tilting') for 5 grd",
 "* Additionally the 4 arrow keys can be used to rotate the lattice","", 
 "  z+/-  : enlarges/reduces height for 25 percent",
 "  z0+/- : shifting the zero level of the displayed lattice",
 "  d+/-  : shift perspective projection distance for +/- 1%","",
 "* Toggle Buttons ",
 "  rand     : show the lines on the border faces",
 "             in isoline-mode show grid lines",
 "  opaque   : toggle to show hidden lines (in grid-mode)",
 "             and toggle illumination mode (in body-mode)", 
 "  body     : toggle between the grid- and filled-body-mode",
 "  isolin   : toggle between 'isolines' (flat-mode) and 3d-perspective",
 "  dump     : makes X-Window dump-file 'lattice.dump' of the actual picture",
 "  hardcopy : analogue dump directly to lpr","",
 "  region   : pops up a window to define an displayed part (zooming)",
 "  palette  : popup for setting the color palette (for body-mode)",
 "  light    : popup for setting the angle of light source (body-mode)",
 "  clone    : make a copy of the lattice which maps the same array, and thus",
 "             will be also be updated (if part of an integration)", 
 0 };

lattice_manager::lattice_manager(window & parent, int w, int h, int x, int y,
				 int nxp, int nyp, float * q) : 
lattice_window(parent,w,h-40,x,y), nx(nxp), ny(nyp), qptr(q)
{ 
  selection_mask |= ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                    KeyPressMask;  // enable Keypress events
  set_defaults(); z0 = 0;
  int bh = 20;
  
  // the menu-bars are placed on the lattice-window (bottom)
  mb1 = new menu_bar(parent, w, bh, x, y+h-40, 30, 100, 0);
  mb2 = new menu_bar(parent, w, bh, x, y+h-20, 30, 100, 0);
  // there is only one global palette also if there are many lattice manager
  if (pal_win == NULL) 
    pal_win = new palette_popup(ncolors); // popup for palette
  light_win = new light_popup(this,240);
  region_win = new main_window("region manager",200,222);
  region_int = new region_manager(*region_win,this);
  clone_lm = NULL;

  new modifier_button <float> (*mb1,"a+", inc_float,this,&alpha,0.1);
  new modifier_button <float> (*mb2,"a-", inc_float,this,&alpha,-0.1);

  new modifier_button <float> (*mb1,"b+", inc_float,this,&beta,0.1);
  new modifier_button <float> (*mb2,"b-", inc_float,this,&beta,-0.1); 

  new modifier_button <float> (*mb1,"z+", mult_float, this, &gamma, 1.25); 
  new modifier_button <float> (*mb2,"z-", mult_float, this, &gamma, 0.80); 
  
  new modifier_button <float> (*mb1,"z0+", inc_z0, this, &z0, -0.1); 
  new modifier_button <float> (*mb2,"z0-", inc_z0, this, &z0, 0.1); 
    
  new modifier_button <float> (*mb1,"d+", inc_float, this, &dist, 0.01);   
  new modifier_button <float> (*mb2,"d-", inc_float, this, &dist, -0.01);  

  new toggle_redraw_button(*mb1," rand ",&rand,this);
  new toggle_redraw_button(*mb2,"opaque",&opaque,this);
    
  new toggle_redraw_button(*mb1," body ",&body,this); 
  new toggle_redraw_button(*mb2,"isolin",&flat_mode,this);

  // hardcopy of the inner window !
  new xwd_button(*mb1,"  dump  ","-out lattice.dump",this);
  new xwd_button(*mb2,"hardcopy"," | xpr | lpr",this);
 
  new popup_button (*mb1,  pal_win,"palette");
  new popup_button (*mb2,light_win," light ");
  new instance_button <lattice_manager> (*mb1,"default",
  					 &lattice_manager::default_draw,this);
  // new instance_button <lattice_manager> (*mb1,"rotate",
  //                                        &lattice_manager::rotate,this);
  
  new instance_button <lattice_manager> (*mb2," clone ",
					 &lattice_manager::make_clone, this);
  new popup_button (*mb1,region_win,"region");
  new help_button (*mb2,            " help ",lattice_help);
 
}

lattice_manager::~lattice_manager() { 
  safe_delete(light_win); // safe_delete checks first if the windows are deleted
  safe_delete(region_win); // already implicitely from root_window before del.
  if (clone_lm) safe_delete(clone_lm);
}

lattice_manager* lattice_manager::make_popup(char * Name,float *q) {
  main_window *pp = new main_window(Name,width,height+20);
  lattice_manager* lm = new lattice_manager(*pp,width,height,0,20,nx,ny,q);
  lm->set_toggles(body, rand, opaque, flat_mode);
  lm->set_angles(alpha, beta, gamma, dist);  
  return lm;
}

void lattice_manager::delete_clone() {
  clone_lm->mainw->Unmap();
  delete clone_lm->mainw;
  clone_lm = NULL;
}  

void lattice_manager::make_clone() { 
  if (clone_lm) return; // only one clone possible, but it may be chained
  clone_lm = make_popup("clone",qptr);  
  main_window *pp = clone_lm->mainw;  // for accessing the popup-Window

  // the button for deleting the clone is on the popup of the clone 
  new instance_button <lattice_manager> (*pp, "delete",
		      &lattice_manager::delete_clone, this, 100, 20,0,0);
  pp->do_popup(0,0);
}

