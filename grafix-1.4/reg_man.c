// reg_man.c : region_manager class && light_popup - window
//    auxiliary objects for lattice_manager

#include "lattice.h"

int sqabs(XPoint p1, XPoint p2) {
  int dx = p1.x - p2.x, dy = p1.y - p2.y;
  return (dx*dx + dy*dy);
}

// ********************** Region manager *********************

#define LOWER(x) ((x > 0) ? (x--) : 0)
#define HIGHER(x,nx) ((x < nx) ? (x++) : nx)
 
void region_manager::init_region() { 
   lm->init_region(); 
   lm->redraw();
 }

void region_manager::rise() { 
  int i; 
  int xb = lm->xspan;
  for (i=0; i< xb/10 + 1; i++) { 
    LOWER(lm->ixstart);  LOWER(lm->iystart); 
  }
  int yb = lm->yspan;
  for (i=0; i< yb/10 + 1; i++) { 
    HIGHER(lm->ixend,nx); HIGHER(lm->iyend,ny);
  }
  lm->redraw();
}

void region_manager::shrink() { 
  int xb,yb;
  xb = lm->xspan;
  zc.x = (lm->ixend + lm->ixstart)/2;
  zc.y = (lm->iyend + lm->iystart)/2;
  if (xb > 4) { // Schrumpfen auf 6/7 dh. -15%
    lm->ixstart = MAX(zc.x - (3*xb)/7, 0);  
    lm->ixend = MIN(zc.x + (3*xb)/7, nx);
  } 
  yb = lm->yspan;
  if (yb > 4) { 
    lm->iystart = MAX(zc.y - (3*yb)/7, 0);  
    lm->iyend = MIN(zc.y + (3*yb)/7, ny);
  }   
  lm->redraw();
}

region_manager::region_manager(window &parent, lattice_manager *lm) : 
  coord_window(parent,parent.width,parent.height-20,0,20,5,5,5,5), 
  nx(lm->nx), ny(lm->ny), lm(lm) {
   
    selection_mask |= ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    menu_bar *mb = new menu_bar(parent,parent.width,20,0,0,50,100,0);
  
    new instance_button <region_manager>(*mb,"complete",
					 &region_manager::init_region,this);
    new instance_button <region_manager>(*mb,">>",
					 &region_manager::rise, this);
    new instance_button <region_manager>(*mb,"<<",
					 &region_manager::shrink, this);
    new unmap_button(*mb,"close",&parent);     
    zc.x = (nx-1)/2; zc.y = (ny-1)/2;
  }

void region_manager::draw_interior() {
  xmax = nx-1; ymax = ny-1; 
  int nn = (nx > ny) ? nx : ny; // to get a quadratic grid
  define_coord(0,0,nn-1,nn-1);

  int xstart = x_window(lm->ixstart), xend = x_window(lm->ixend - 1),
      ystart = y_window(lm->iystart), yend = y_window(lm->iyend - 1);
  
  static XGCValues values = {0,0, pal_win->color_cells[80].pixel }; 
  static GC gccol = CreateGC(GCForeground, &values);
  XFillRectangle(display,pix,gccol,xstart,yend,xend-xstart,ystart-yend);
  int i, di = MAX( (4*nn)/width + 1, 1); // printf("di = %d\n",di);
  for (i=0; i<ny; i+=di) wline(0,i,xmax,i);
  for (i=0; i<nx; i+=di) wline(i,0,i,ymax);
}

// returns grid index of mouse pointer in [(xo,yo),(xo+1,yo+1)]
XPoint region_manager::raster(int x, int y) {
  int xo = (int) x_org(x), yo = (int) y_org(y); // truncated part
  XPoint pp = { (xo < 0) ? 0 : (xo > nx-2) ? nx-2 : xo, 
		(yo < 0) ? 0 : (yo > ny-2) ? ny-2 : yo };
  return pp;
}

XPoint region_manager::p_window(float x, float y) {
  XPoint pp = { x_window(x), y_window(y) }; 
  return pp; 
} 

// draws rectangle (inverting) in real world-coordinates 
// with [(x,y) (x+dx,y+dy)] directly into display-window (temporaer)
void region_manager::mark_rect(float x0, float y0, float dx, float dy) {
  static XGCValues val; 
  val.foreground = black; val.line_width = 2;
  static GC gc_thick = CreateGC(GCForeground | GCLineWidth ,  &val); 
  XPoint z1 = p_window(x0,y0+dy); // linke OBERE Ecke
  int w = int(xf*dx + 0.5), h = int(yf*dy + 0.5); 
  XDrawRectangle(display,Win,gc_thick, z1.x, z1.y, w, h);
}

// draws rectanle in integer world-coordinates (temporaer)
void region_manager::Rectangle(GC gc, XPoint p1, XPoint p2) {
  XPoint z1 = p_window(p1.x,p1.y), z2 = p_window(p2.x,p2.y);
  XDrawRectangle(display,Win,gc,z1.x-1, z2.y-1,  
		 z2.x - z1.x + 2, z1.y - z2.y + 2);
  /*  Anzeige des Ausschnitts im Originalbild : */
  // if (! lm->body) return; // not yet ready !!
  int xn = p2.x-p1.x+1, yn = p2.y-p1.y+1;
  int i,x,y,pi;
  // das Feld der Gitterwerte
  XPoint (*scp)[lm->ny] = (XPoint (*)[lm->ny]) lm->scptr;
  XPoint p[xn + yn]; 
  int xb[2] = {MAX(p1.x, lm->ixstart), MIN(p2.x, lm->ixend-1)}, 
      yb[2] = {MAX(p1.y, lm->iystart), MIN(p2.y, lm->iyend-1)};
  // printf(" %d %d %d %d \n",xb[0],xb[1],yb[0],yb[1]);
  for (i = 0; i< 2; i++) {
    y = yb[i]; 
    if (y >= lm->iystart &&  y < lm->iyend) {
      for (x= xb[0], pi=0; x <= xb[1]; x++, pi++) p[pi] = scp[x][y];
      if (pi>0) XDrawLines(display,lm->Win,gc,p,pi,0); 
    } 
    x = xb[i]; 
    if (x >= lm->ixstart &&  x < lm->ixend) {
      for (y= yb[0], pi=0; y <= yb[1]; y++, pi++) p[pi] = scp[x][y];
      if (pi>0) XDrawLines(display,lm->Win,gc,p,pi,0); 
    }
  }
}

void region_manager::BPress_1_CB(XButtonEvent ev) {
  pstart = raster(ev.x, ev.y); 
  pmin = pstart; pmax.x = pmin.x+1; pmax.y = pmin.y+1;
  Rectangle(gc_rubber,pmin,pmax);
  pact = pstart;
}

void region_manager::BRelease_CB(XButtonEvent) {    
  Rectangle(gc_rubber,pmin,pmax);
  if ( 100 * sqabs(pmax,pmin) < 800 + SQR(lm->xspan) + SQR(lm->yspan)) return; 
  // region too small if not 10% + 2x2 grids
  lm->ixstart = pmin.x; lm->ixend = pmax.x+1;
  lm->iystart = pmin.y; lm->iyend = pmax.y+1;
  redraw(); lm->redraw();
}

void region_manager::Motion_CB(XMotionEvent ev) { 
  if (ev.state & Button1Mask) { // Button 1 gedrueckt
    XPoint pp = raster(ev.x,ev.y); 
    if (pp.x != pact.x || pp.y != pact.y) {
      pact = pp; 
      Rectangle(gc_rubber,pmin,pmax);
      pmin.x = MIN(pstart.x,pp.x); pmin.y = MIN(pstart.y,pp.y);
      pmax.x = MAX(pstart.x,pp.x) + 1; pmax.y = MAX(pstart.y,pp.y) + 1;
      Rectangle(gc_rubber,pmin,pmax);
    }
  }
}

// ********** the light popup window *************
// setting the incidence angle of the light source

static void update_light(light_popup * lp) { 
  lp->lm->a_light = lp->sc_alpha->value/grd2rad;
  lp->lm->b_light = lp->sc_beta->value/grd2rad;
  if (lp->immediate) lp->lm->redraw();
}

light_popup::light_popup(lattice_manager *latm, int w) 
: main_window("light",w+10,100) {    
  lm = latm; 
  immediate = FALSE;
  int y = 5;
  sc_alpha = new scrollbar(*this, update_light, this, w,20,5,y,0,360,30); 
  y+= 30;
  sc_beta  = new scrollbar(*this, update_light, this, w,20,5,y,0,180,5); 
  y+= 40;
  int x = 5, wb = w/3; 
  
  new instance_button <pixmap_window> 
    (*this,"redraw",&pixmap_window::redraw,lm,wb,20,x,y);  
  x+= wb;
  new toggle_button(*this,"immediate",&immediate,wb,20,x,y); x+= wb;
  new unmap_button(*this,"close",wb,20,x,y);
  update_light(this); // init values
}


