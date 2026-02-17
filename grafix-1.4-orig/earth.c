// earth-map.C
//    wolfk 4/97
// interactive display of the earth shape with many nice features !

#include <stdio.h>
#include "window.h"
#include <complex>
#undef Complex

#define MAP_DATA_SCALE (30000)
extern short map_data[]; // the data

typedef std::complex<double> Complex;

double grd2rad = 180./M_PI;
extern int button_fg_pix; // from window.c
struct pos3 { // 3-dim position
  double x,y,z;
  Complex latlon() { // compute latitude, longitude as complex number
    double phi = asin(y); // Breite
    double lambda = atan2(z,x); // Hoehe
    return Complex(phi,lambda);
  }
};

struct poly { // closed polygon with some characteristics
  int npts, val; // val = 1/-1 -> land/water indicator
  pos3 *pos; // the polygon
  Complex llmin,llmax; // enclosing bigcircles 
  double area; // integrated !
  poly() { area = 0; llmin = Complex(0,0); }
};

pos3 *extract_curve(int npts, short *data) {
  pos3 *rslt = new pos3[npts];
  int x = 0, y = 0, z = 0;
  // the coords are normed to 1.0 (unity sphere)
  double scale = 1.0 / MAP_DATA_SCALE;
  for (int i=0; i < npts; i++) {
    pos3 *pos = rslt+i;
    x += data[0]; y += data[1]; z += data[2];
    pos->x = x * scale; pos->y = y * scale; pos->z = z * scale;
    data += 3; 
  }
  return rslt;
}

int scan_curves(poly *pp) {
  short *raw = map_data;
  int cidx = 0, ntot = 0; // poly-index, total # of points
  int npts; 
  while ((npts = raw[0]) != 0) {
    // printf("cidx = %d %d ",cidx,npts);
    pp[cidx].val = raw[1]; 
    raw += 2;
    pp[cidx].pos = extract_curve(npts,raw);
    pp[cidx].npts = npts;
    cidx++; ntot += npts;
    raw += 3*npts;  
    
  }
  printf("%d polys with %d points\n",cidx, ntot);
  return cidx;
}

inline void drehe(double &x, double &y, double c, double s) {
  double xt = c*x + s*y; y = c*y - s*x; x = xt;
}

char *help1[] = {
  "The toggle button 'rot' lets the Earth rotate in an animation mode",
  "The 'coor' button toggles the display of the latitude and longitude circles",
  "The 'hide' button toggles between an opaque and transparent mode","",
  "The 'z+' 'z-' buttons enable zooming and un-zooming by a constant factor",
  "The 'back' button toggles background between black/white","",
  "You may also use the mouse to rotate the Earth freely in any direction",
  "by pressing the left mouse button and dragging",
  "(disabled in rotation mode)",
  0};

class earth_win : public coord_window {
  int np;
  poly *pp;
  Bool use_cols, circum, mmode, hide, cocirc, black_mode;
  Bool *polling_mode;
  double phi; // projection angle for (x,z)-plane -> around y-axe (= North-pole)
  double theta; // projection angle for (y,z)-plane -> 
  double csp, ssp, cst, sst; // projection sinus and cosinus
  double scale;
  GC gcx;
  int fg_col,bg_col; // forground and background colors
public:
  earth_win(window *parent, int w, int h, int x, int y, int np, poly *pp, 
	   menu_bar *mb, int *polling_mode) :
  coord_window(*parent,w,h,x,y), np(np), pp(pp), polling_mode(polling_mode) {
    scale = 1.3; rescale();
    phi = 0; theta = 23.5/grd2rad;
    use_cols = True; new toggle_redraw_button(*mb,"cols",&use_cols,this);
    // circum = False; new toggle_redraw_button(*mb,"circ",&circum,this);
    cocirc = True; new toggle_redraw_button(*mb,"coor",&cocirc,this);
    mmode = False;
    *polling_mode = True;
    hide = True; new toggle_redraw_button(*mb,"hide",&hide,this);
    new instance_button <earth_win> (*mb,"phi+",&step_phi,this);  
    new instance_button <earth_win> (*mb,"the+",&step_theta,this);
    new instance_button <earth_win> (*mb,"z+",&zoom,this);
    new instance_button <earth_win> (*mb,"z-",&unzoom,this);
    new instance_button <earth_win> (*mb,"back",&back_toggle,this);
    new help_button(*mb,"help",help1);

    selection_mask |= PointerMotionMask | ButtonPressMask;  
    black_mode = True;
    set_fgbg();
    clear(); // else white background 
  }
  void set_fgbg() {
    if (black_mode) { fg_col = white; bg_col = black; }
    else { fg_col = black; bg_col = white; }
    XSetForeground(display, gc_clear, bg_col);
    XSetForeground(display, gc, fg_col);
  }
  void back_toggle() { // toggle white/black background
    black_mode = ! black_mode;
    set_fgbg();
    redraw();
  }

  void rot_phi(double dphi) { phi += dphi; redraw(); } 
  void step_phi() { rot_phi(0.1); }
  void step_theta() { theta += 0.1; redraw(); }

  void rescale() { define_coord(-scale,-scale,scale,scale); } 
  void zoom() { scale /= 1.3; rescale(); redraw(); }
  void unzoom() { scale *= 1.3; rescale(); redraw(); }

  int xprev, yprev; // previous values for mouse motion events
  virtual void BPress_CB(XButtonEvent ev) { xprev = ev.x; yprev = ev.y; }

  virtual void Motion_CB(XMotionEvent ev) {
    if (*polling_mode) return;
    if (ev.state & Button1Mask) {
      theta += 0.01*(ev.y - yprev);
      rot_phi((ev.x-xprev)*0.01); 
      // printf("%d %f ",ev.x, phi); fflush(stdout);
      xprev = ev.x; yprev = ev.y;
    }
  }

  inline void projection(double &x, double &y, double &z) {
    drehe(x,z,csp,ssp); // first rot phi around y-axis (North-pole)
    drehe(z,y,cst,sst); // then theta around new x
  }
  // direct mapping of xyz -> screen coords, not very fast
  XPoint pmap(double x, double y, double z) {
    projection(x,y,z); return p_window(x,y);
  }
  void pline(XPoint p1, XPoint p2) { line(p1.x,p1.y,p2.x,p2.y); }

  // aux function : ctr = center of ellipse, xr, yr half axes, 
  // phi0 measured from y-axis, in radians
  void arc(XPoint ctr, int xr, int yr, double phi0, double dphi) {
    static double fact = 64*grd2rad;
    if (yr < 0) { phi0 += M_PI; yr = -yr; }    
    // terriffic errors if xr or yr are negative :
    // even server crashes !!!
    xr = abs(xr);
    int arc1 = irint(fact * (phi0 - M_PI /2));
    XDrawArc(display,pix,gc,ctr.x - xr,ctr.y - yr, 2*xr, 2*yr,
	     arc1,irint(fact*dphi));
  }

  // the main task :
  void draw_interior() {
    unsigned cols[6] = { fg_col, Red, Green, Blue, Yellow, Violet}; 
    
    XPoint topleft = p_window(-1.,1.), center = p_window(0.,0.); 
    int dx = center.x - topleft.x, dy = center.y - topleft.y;

    // a simple circle is the shape of the earth
    // the color of the body of the Earth
    GC gcs = (black_mode) ? button_lw_gc : button_br_gc;
    XFillArc(display,pix,gcs,topleft.x,topleft.y,2*dx,2*dy,0,360*64);

    csp = cos(phi); ssp = sin(phi);
    cst = cos(theta); sst = sin(theta);
    int nx; // nx = 1 : use all points, 2 = every 2nd,...
    nx = MAX(1, irint(scale*2000/width)); // d ~= 25km ~= 1/4grd ~= 1/300 
    // printf("%d %f ",nx, phi); fflush(stdout);
    // Poles
    set_color(button_fg_pix); // a nicer color for coord-lines & ticks
    XPoint NP = pmap(0,1,0), SP = pmap(0,-1,0); // North- and South-Pole
    // North-Pole-tick
    if (sst >= 0) pline(NP, pmap(0,1.1,0)); 
    else pline(SP,pmap(0,-1.1,0));

    if (cocirc) { // draw coordinate circles
      // draw latitude circles (could be computed once, since indep. of phi)
      int nlat = 90/15; // every 15 grd
      if (scale < 1.0) nlat = irint(scale*nlat);
      for (int il = -nlat+1; il < nlat; il++) {
	double lat = il*M_PI/(2*nlat), y = sin(lat), yp = y*cos(theta);
	XPoint cyp = p_window(0,yp); // Projektion der y-Achse auf screen-y
	// -> center of latitude circle 
	int xr = irint(xf*cos(lat)),       // Halbachsen
	  yr = irint(yf*sin(theta)*cos(lat)); 
	
	double p0 = 0, delp = 2*M_PI; 
	if (hide) {
	  double th = - tan(theta)*tan(lat);
	  // discriminator for range of phi that is visible 
	  delp = (th < -1.0) ? M_PI : ((th < 1.0) ? acos(th) : 0);
	  if (cos(theta)*cos(lat) < 0) {  p0 = delp; delp = M_PI - delp; }
	  else p0 = -delp;
	  delp *= 2.0;
	}
	if (delp > 0) arc(cyp,xr,yr,p0,delp);
      }

      // draw longitude circles
      int nlon = 8; // number of long circles to display
      double dlon = 2*M_PI/nlon; // angle between circles
      Complex clon(1,0), // = exp(i*lon)  
              dcl(cos(dlon), sin(dlon)); // increment factor = exp(i*dlon)
      for (int l=0; l < nlon; l++) {
	double cl = clon.real(), sl = clon.imag();
	double dphi; 
	if (hide) {
	  double clp = cl*csp - sl*ssp; // = cos(lon+phi);
	  dphi = atan2(sst,clp*cst); // bow length from pole 
	  // if (l==0) printw(300,20,"%.2f",dphi);
	} else dphi = -M_PI;
	clon *= dcl; // complex factor (next long circle) -> lon += dlon!
	     
	// now approx the circle with a polygon with np points
	int nd = irint(sqrt(xf/8)*fabs(dphi) + 1) + 1; // needed approximation
	// xf is the radius of the Earth in pixels
	dphi /= (nd-1); // increment for lat 
	XPoint plon[nd]; 

	Complex ddp(cos(dphi), sin(dphi)),  // increment factor for lat
	        clat(0,1); // = exp(i*lat) : lat = Pi/2 => NP
	if (sst < 0) clat = Complex(0,-1);  // south pole
	plon[0] = (sst > 0) ? NP : SP; // longit circle starts at visible pole 

	for (int i = 1; i < nd; i++) {
	  clat *= ddp; // complex multiplication !
	  double cp = clat.real(), sp = clat.imag();
	  plon[i] = pmap(cp*sl,sp,cp*cl);
	} 
	XDrawLines(display,pix,gc,plon,nd,CoordModeOrigin);
      }
    }
    set_color(fg_col);
    for (int ip=0; ip < np; ip++) {
      poly *pi = pp+ip; 
      int npts =  pi->npts;
      if (npts <= 2*nx) break; // only 2 points to draw
      if (use_cols) set_color(cols[ip % 6]);
      int nd = 0; 
      XPoint pmap[npts+1]; // at most npts+1 poly-points
      for (int n = 0; n < npts; n += nx) {
	double x = pi->pos[n].x, y = pi->pos[n].y, z = pi->pos[n].z;
	projection(x,y,z);
	if (hide) { // use hiding algorithm (simple zp)
	  if (z > 0.) 
	    pmap[nd++] = p_window(x, y);
	  else { // an hidden point 
	    if (nd > 0) // draw polygon up to last point 
	      XDrawLines(display,pix,gc,pmap,nd,CoordModeOrigin);
	    nd = 0; // and start a new poly
	  }
	} else pmap[nd++] = p_window(x, y);
	
      }
      if (nd > 0) {
	if (! hide) 
	  pmap[nd++] = pmap[0]; // close the poly ?!
	else ; // ??????
	
	XDrawLines(display,pix,gc,pmap,nd,CoordModeOrigin);
	//XFillPolygon(display,pix,gc_copy,pmap,npts+1,Convex,CoordModeOrigin);
      }
    }
    set_color(fg_col);    
    
    printw(2,20,"%.1f %.1f %.1f %.1f %d",phi*grd2rad,theta*grd2rad,1/scale,xf,nx);
    set_color(black);
  }
};

class earth_main : public main_window {
  earth_win *pw;
public:
  earth_main(int w, int h) : main_window("xearth",w,h) {
    poly pp[1000];
    int np = scan_curves(pp);

    menu_bar *mb = new menu_bar(*this,w,20,0,0);
    new quit_button(*mb); 
    new toggle_button(*mb,"rot",&polling_mode);
    polling_mode = True;
    pw = new earth_win(this,w,h-20,0,20,np,pp,mb,&polling_mode);
    main_loop();
  }
  void polling_handler() {
    pw->rot_phi(0.03);
  }
};

int main() {
  new earth_main(400,420);
}
