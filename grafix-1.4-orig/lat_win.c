#include "lattice.h"

float grd2rad = 180.0/M_PI; // conversion grad -> radian
static int debug = 0;

lattice_window::lattice_window(window & parent, int w, int h, int x, int y) :
coord_window(parent,w,h,x,y,2,2,2,2) { // free rand : hor, vert : 2 Pix 
  scptr = NULL;
  ncolors = 100;
  ixstart = 0; ixend = 0;
  iystart = 0; iyend = 0; rand = 0;
  external_colors = NULL; 
  nlevels = 10; // for isolines : may be changed by application
  flevel = new float[100]; // at most 100 can be set
  for (int n=0; n < nlevels; n++) flevel[n] = 0.1*(n+1); 
}

lattice_window::~lattice_window() { 
  if (scptr) delete[] scptr; 
  delete[] flevel;
}

// draw clipped lines using global clip vectors y_top, y_bot
// lastp : draw last point of line (1) or not (0)
void lattice_window::cline(XPoint a, XPoint b, int lastp) {
  int sx,sy,dx,dy,x,y,visible,ytt=0,ybb=0,up,down;
  int ex = 0, ey = 0;
  if (debug) printf(" %d %d %d %d ",a.x,a.y,b.x,b.y);
  
  dx = b.x - a.x; dy = b.y - a.y;
  sx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
  sy = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);

  x = a.x; y = a.y; 
  dx = abs(dx); dy = abs(dy);

  do { 
    up = (y > y_top[x]); down = (y < y_bot[x]); 
    if (up) ytt = y; if (down) ybb = y;
    visible = up || down; 
    int endp = (x == b.x) && (y == b.y);
    if (endp &&  ! lastp) break;

    if (visible) DrawPoint(x,y); 

    if (dy < dx) {
      if (up) y_top[x] = ytt; if (down) y_bot[x] = ybb; 
      x += sx; ey += dy;  
      if (2*ey > dx) { y += sy; ey -= dx; } 
    } else {
      y += sy; ex += dx;
      if (2*ex > dy) {  
	if (up) y_top[x] = ytt; if (down) y_bot[x] = ybb; 
	x += sx; ex -= dy; }
    }
    if (endp) break; 
  } while (1);  
}

// fills from the line (a,b) up to the boundaries y_top, y_bot
// if (lastp) including the endpoint, 
// if (dofill) really fill, else only set boundaries
void lattice_window::fill(XPoint a, XPoint b, int lastp, int dofill) {
  int sx,sy,dx,dy,x,y;
  int ex = 0, ey = 0;

  dx = b.x - a.x; dy = b.y - a.y;
  sx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
  sy = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);

  x = a.x; y = a.y; 
  dx = abs(dx); dy = abs(dy);

  if (dy < dx) { 
    do {
      int endp = (x == b.x) && (y == b.y);
      if (endp &&  ! lastp) break;

      if (y > y_top[x]) { if (dofill) line(x,y,x,y_top[x]); y_top[x] = y; }
      if (y < y_bot[x]) { if (dofill) line(x,y,x,y_bot[x]); y_bot[x] = y; }
      x += sx; ey += dy;  
      if (2*ey > dx) { y += sy; ey -= dx; } 
      if (endp) break; 
    } while (1);  
  } else {
    do {
      int endp = (x == b.x) && (y == b.y);
      if (endp &&  ! lastp) break;

      y += sy; ex += dx;
      if (2*ex > dy) {  
	if (y > y_top[x]) { if (dofill) line(x,y,x,y_top[x]); y_top[x] = y; }
        if (y < y_bot[x]) { if (dofill) line(x,y,x,y_bot[x]); y_bot[x] = y; }
	x += sx; ex -= dy; }
    
      if (endp) break; 
    } while (1);  
  }
}

// new gcc C++ extension : named return values (not in class definition)
XPoint lattice_window::screen_project(float x, float y, float z) {
  float xs,ys,yss,zss,t,dx,dy; 
  // printf(" %f %f %f \n",x,y,z);
  dx = x - xp; dy = y - yp;
  if (flat_mode) return p_window(dx,dy); // no transformation 
  xs = ca*dx - sa*dy;  ys = ca*dy + sa*dx;     // rotation x,y-plane
  zss = cb*z - sb*ys; yss = cb*ys + sb*z;      // rotation ys,z-plane
    
  // central-projection with perspective
  // exaktly : temp.y = arcsin(yss/(distance - zss)) = y-view angle
  // where "distance" is distance between viewpoint and origin

  t = (dist == 0.0) ? 1.0 : 1.0/(1.0 - dist*zss/xp); 	   
  XPoint temp = p_window(t*xs,t*yss);

  // this should not happen, it would overflow y_top, y_bot indicees
  // clip to boundary values
  if (temp.x < 0) temp.x = 0; else if (temp.x >= width) temp.x = width-1;
  return temp;
}

// for swapping the driving step variables 
static void swap_dir(int& x0, int& x1, int &dx, float &f0, float &f1) {
  int t = x0; x0 = x1; x1 = t; 
  float f = f0; f0 = f1; f1 = f;
  dx = -dx; 
}

palette_popup *pal_win = NULL; // popup window der color palette 

// draws a perspective picture of the gridwise defined function FF
// either with shading algorithm (body = True) or as a lattice
void lattice_window::make_body(int nx, int ny, float *FF, 
			       float alpha, float beta, float gamma, float z0, 
			       int opaque, 
			       float distance, float al, float bl) {
  float (*ff)[ny] = (float (*) [ny]) FF;
  y_bot = new int[width]; y_top = new int[width]; 
  int x,y; 
  for (x=0; x < width; x++) { y_top[x] = 0; y_bot[x] = height; }

  // eigentlich koennen hier alle Projektionen und clippings entfallen !
  if (flat_mode) { alpha = beta = distance = 0; }

  ca = cos(alpha); sa = sin(alpha); cb = cos(beta); sb = sin(beta);
  dist = distance;
  // printf(" %d %d %f %f %f\n",nx,ny,alpha,beta,gamma);

  if (ixend == 0) ixend = nx; // am Anfang mit 0 initialisiert
  if (iyend == 0) iyend = ny;
  xspan = ixend - ixstart - 1; yspan = iyend - iystart -1;
  xp = 0.5*xspan; yp = 0.5*yspan; // Drehpunkt der x,y-Ebene

  float xmax,ymax;
  if (flat_mode) { xmax = xp; ymax= yp; } else
  ymax = xmax = 0.5*sqrt(xspan*xspan + yspan*yspan); 

  define_coord(-xmax, -ymax, xmax, ymax); 

  int ix,iy;
  float zh,xh,yh;
  zmax = -1E30;
  if (scptr) delete[] scptr; 
  scptr = new XPoint[nx*ny]; 
  // all  nx*ny grid-points after trafo, addressed as scp[nx][ny]
  XPoint (*scp)[ny] = (XPoint (*) [ny]) scptr; 
  znorm = gamma * xmax;
  for (ix = ixstart, xh = .0; ix < ixend; ix++, xh += 1.0) { 
    for (iy = iystart, yh = .0; iy < iyend; iy++, yh += 1.0 ) {
      zh = znorm * (ff[ix][iy]-z0); if (zh > zmax) zmax = zh;
      // if (flat_mode) { 
      // scp[ix][iy] = p_window(xh - xp,yh - yp);
      // } else
      scp[ix][iy] = screen_project(xh, yh, zh);
    }
  }  

  int colors[nx][ny];

  if (body && ! external_colors) { 
    // else : given colors for areas -> no computation               
    // computing of reflections coefficients of the grid points
    // subtraction of rotation angles -> means fixed (constant) light source
    // otherwise : co-moving light source (moves with frame)
    // disadvantage for fixed light source :
    // upon rotating the orientation is bad 
    float e_alpha = al /* -alpha */ , e_beta = bl /* -beta */ ;
    float ex = cos(e_alpha)*cos(e_beta), ey = sin(e_alpha)*cos(e_beta), 
    ez = sin(e_beta); // (ex,ey,ez) = normal of incident ray

    float rpp[nx][ny];
    float rmax = -1e30, rmin = 1e30, mu = 1/SQR(gamma); 
    for (ix = ixstart+1; ix < ixend; ix++) { 
      for (iy = iystart+1; iy < iyend; iy++) {
	float z00 = ff[ix][iy]; // z0 shift not needed here
	float z10 = ff[ix-1][iy], z01 = ff[ix][iy-1], z11 = ff[ix-1][iy-1];
	// computation of normals  to (x,y) (x+1,y) (x,y+1) :
	// n = (zx, zy, 1)/|N|
	float zxp = z01 - z11,  zyp = z10 - z11; // growing differences
	float zxm = z00 - z10,  zym = z00 - z01; // falling  differences
	// fuer Quadrate benutze mittlere Ableitung
         zxp += zxm; zyp += zym; 
	//            scalar product n * E
	rpp[ix][iy]= opaque ? (zxp*ex+zyp*ey+ez)/sqrt(zxp*zxp + zyp*zyp + mu) 
	                    : - (z00+z01+z10+z11); // prop. z
	rmax = MAX(rpp[ix][iy], rmax);  // for norming of scalar products
	rmin = MIN(rpp[ix][iy], rmin);
      }
    } 
    // printf(" %g %g\n",rmin,rmax);
    for (ix = ixstart+1; ix < ixend; ix++)  
      for (iy = iystart+1; iy < iyend; iy++)
	// norming  of scalar prod. s = [rmin..rmax] -> [0..ncolors-1]
	colors[ix][iy] = (int) ((ncolors-1)*(rpp[ix][iy]-rmin)/(rmax-rmin));
    // norming in -1..+1 (for normals adapted) :
    // colors[ix][iy] = (int) ((ncolors-1)*(0.5*rpp[ix][iy] + 0.5));
  }

  // Determination of parameters of the for-loops, in such a way that 
  // the drawing always starts at the front edge (towards view-point)        
  // xs,ys  are the offsets between  ix,iy and the 
  // index of the scalarprodukt-arrays, which count from 1,1
  // examples  : ixstart = 0, ixend = 10, 
  //                	x0   	x1    xe     dx     xs
  // alpha = 0..90   	0    	1      9      1      0
  // alpha = -90..0  	9    	8      0     -1      1
  
  int x0 = ixstart, x1 = ixstart+1, xe = ixend-1, dx = 1, 
      xx = ixend-ixstart-1, xs = 0;  
  int y0 = iystart, y1 = iystart+1, ye = iyend-1, dy = 1, 
      yy = iyend-iystart-1, ys = 0;

  float xfront = 0, yfront = 0, xback = xspan, yback = yspan, xff, yff;
  if (sa < 0.) { swap_dir(x0,xe,dx,xfront,xback); x1 = ixend-2; xs = 1; }
  if (ca < 0.) { swap_dir(y0,ye,dy,yfront,yback); y1 = iyend-2; ys = 1; }

  // draw the x and y axes (front lines)
  XPoint sff = screen_project(xfront,yfront,0),
         sbf = screen_project(xback,yfront,0), 
         sfb = screen_project(xfront,yback,0);
  cline(sff,sbf,0); // x axis
  cline(sff,sfb,0); // y axis

  if (rand) { // draw the lines from axes to first grid line
    for (y = y0, yff = yfront; ; y += dy, yff += dy) {
      cline(screen_project(xfront,yff,0),scp[x0][y],0);
      if (y == ye) break;
    }
    for (x = x0, xff = xfront; ; x+= dx, xff += dx) {
      cline(screen_project(xff,yfront,0),scp[x][y0],0);  
      if (x == xe) break;
    }
  }

  if (body) { // draw coloured faces 
    for (ix=x1, x=0; x < xx; ix+=dx, x++) 
      cline(scp[ix][y0],scp[ix-dx][y0],1);
    // or : fill(scp[ix][y0],scp[ix-dx][y0],1,0); -> without drawing
    for (iy=y1, y=0; y < yy; iy+=dy, y++)
      cline(scp[x0][iy],scp[x0][iy-dy],1);

    for (ix=x1, x=0; x < xx; ix+=dx, x++) {
      for (iy=y1, y=0; y < yy; iy+=dy, y++) { 
	XPoint p00 = scp[ix][iy], p10 = scp[ix-dx][iy], p01 = scp[ix][iy-dy];

	int color;
	if (external_colors) color = external_colors[(ix+xs)*ny + iy+ys];
	else {
	  int pixr = colors[ix+xs][iy+ys];
	  color = pal_win->color_cells[pixr].pixel;
	} 
	set_color(color);  
	fill(p00, p10, 0, 1); fill(p00, p01, 0, 1);  
      }
    }
    set_color(black); // reset for gc_copy !  
  } else { // ! body draw threedim lattice
    if (! flat_mode)
      for (y = y0; ; y += dy) {
	for (x = x0; ; x+= dx) { 
	  XPoint sc = scp[x][y];
	  if (x != xe) xline(sc,scp[x+dx][y],1);
	  if (y != ye) xline(sc,scp[x][y+dy],0); 
	  if (x == xe) break;
	} 
	if (y == ye) break;
      }
  } // ! body 
  if (flat_mode) { // make isolines
    // use 5 colors for isolines
    unsigned cols[] = { Red, Green, Blue, Yellow, Violet };
    for (int lev = 0; lev < nlevels; lev++) {
      set_color(cols[lev % 5]); 
      char text[40]; sprintf(text,"level %g",flevel[lev]);
      DrawString(width-100,(lev+1)*12+4, text);
      isolevel(nx,ny,FF,flevel[lev]*1.00001); // use epsilon !! 
    } 
    set_color(black);
    
    if (rand) { // here : draw raster lines  
      for (y = y0;; y += dy) { pline(scp[x0][y],scp[xe][y]); if (y==ye) break;}
      for (x = x0;; x += dx) { pline(scp[x][y0],scp[x][ye]); if (x==xe) break;}
    }				    
  } 
  
  if (! flat_mode) {
    // draw z axis with ticks and numbers
    XPoint sc00 = screen_project(0,0,0);       // the origin
    XPoint sczm = screen_project(0,0,zmax); 
    pline(sc00,sczm);     // z-axis to zmax
    int i, dzy = sczm.y - sc00.y, nti;  // draw z-ticks with numbers
    nti = (abs(dzy) > 60) ? 5 : (abs(dzy) / 15 + 1); // number of ticks 
    for (i=0; i< nti+1; i++) { 
      float zi = i/float(nti);
      int x = sc00.x, y = sc00.y + int(zi*dzy); 
      line(x,y,x-4,y); // z tick
      char ticks[20]; sprintf(ticks,"%g",zi*zmax/znorm + z0);
      int xs = (x < width/2) ? x+3 : x - 6*strlen(ticks) - 6; //string Position
      DrawString(xs,y+5,ticks);
    }
  }
  delete[] y_top; delete[] y_bot;
}

// draw one isoline for spec. flevel
void lattice_window::isolevel(int /*nx*/, int ny, float *FF, float flevel) { 
  float (*ff)[ny] = (float (*) [ny]) FF;   
  float xh,yh; int x,y;
  for (y = iystart, yh = 0.; y < iyend-1; y++, yh += 1.) {
    for (x = ixstart, xh = 0.; x < ixend-1; x++, xh += 1.) {
      float f00,f01,f10,f11,dx0,dy0,dx1,dy1;
      f00 = ff[x][y]; f01 = ff[x][y+1]; 
      f10 = ff[x+1][y]; f11 = ff[x+1][y+1];
      dx0 = f10 - f00; dy0 = f01 - f00;
      dx1 = f11 - f01; dy1 = f11 - f10;
      /*
	 
       f01    dx1    f11
	  ----------
	 !          !
	 !          !
      dy0!          ! dy1
	 !          !
	 !          !
	  ----------
       f00    dx0    f10         

       */
      float d00 = flevel - f00, d01 = flevel - f01,
            d10 = flevel - f10, d11 = flevel - f11;
      int np = 0; // number of crosses
      XPoint px[4]; // up to 4 cross-points on 4 edges
      if (d00*d10 < 0) // cuts dx0
	px[np++] = screen_project(xh + d00/dx0, yh, 0); 
      if (d00*d01 < 0) // cuts dy0
	px[np++] = screen_project(xh, yh + d00/dy0, 0); 
      if (d11*d01 < 0) // cuts dx1
	px[np++] = screen_project(xh + d01/dx1, yh + 1., 0); 
      if (d11*d10 < 0) // cuts dy1
	px[np++] = screen_project(xh + 1., yh + d10/dy1, 0); 
      if (np == 1 || np == 3) { // can happen for special cases (ignored)
	// only if f == level on some nodes 
	// printf("np = %d %d %d %.2f %g %g %g %g\n",np,x,y,flevel,
	//           d00,d01,d10,d11); pause();
      }
      if (np == 2) pline(px[0], px[1]);
      if (np == 4) { 
	// discriminante for function f(x,y) = a + b*x + c*y + d*x*y
	double D = dx0*dy0 + d00*(f11-f01-f10+f00);
	// rectangle(x,y,x+1,y+1); // show the critical rectangles 
	// printf("%g %d %d %g \n",flevel,x,y,D);
	
	if (D > 0) { // either 0-1, 2-3 (means \\) or 0-3, 1-2 (means //)
	  pline(px[0], px[1]); pline(px[2],px[3]);
	} else {
	  pline(px[0], px[3]); pline(px[1],px[2]);
	}
      }
    }
  }
}   


// ************ class isoline_window *******
isoline_window::isoline_window(window &parent, int w, int h, int x, int y, 
			       int nx, int ny, 
			       float *qptr, int nlevels, float *levels) :
  coord_window(parent,w,h,x,y) , nx(nx), ny(ny), qptr(qptr), 
  nlevels(nlevels), levels(levels) {
    define_coord(0,0,nx-1,ny-1);
    raster = False; // True;
  }

// draw one isoline for spec. flevel
void isoline_window::isolevel(float flevel) { 
  float (*ff)[ny] = (float (*) [ny]) qptr;
   
  for (int y = 0; y < ny-1; y++) {
    for (int x = 0; x < nx-1; x++) {
      float f00,f01,f10,f11,dx0,dy0,dx1,dy1;
      f00 = ff[x][y]; f01 = ff[x][y+1]; 
      f10 = ff[x+1][y]; f11 = ff[x+1][y+1];
      dx0 = f10 - f00; dy0 = f01 - f00;
      dx1 = f11 - f01; dy1 = f11 - f10;
      /*
	 
       f01    dx1    f11
	  ----------
	 !          !
	 !          !
      dy0!          ! dy1
	 !          !
	 !          !
	  ----------
       f00    dx0    f10         

       */
      float d00 = flevel - f00, d01 = flevel - f01,
            d10 = flevel - f10, d11 = flevel - f11;
      int np = 0; f_point pc[4]; // up to 4 cross-points on 4 edges
      if (d00*d10 < 0) pc[np++] = f_point(x + d00/dx0,y); // cuts dx0
      if (d00*d01 < 0) pc[np++] = f_point(x, y + d00/dy0); // cuts dy0
      if (d11*d01 < 0) pc[np++] = f_point(x + d01/dx1, y+1);  // cuts dx1
      if (d11*d10 < 0) pc[np++] = f_point(x+1, y + d10/dy1);  // cuts dy1
      if (np == 1 || np == 3) error("isolvel"); // can never happen ??!!
      if (np == 2) f_line(pc[0], pc[1]);
      if (np == 4) { 
	// discriminante for function f(x,y) = a + b*x + c*y + d*x*y
	double D = dx0*dy0 + d00*(f11-f01-f10+f00);
	// rectangle(x,y,x+1,y+1); // show the critical rectangles 
	// printf("%g %d %d %g \n",flevel,x,y,D);
	
	if (D > 0) { // either 0-1, 2-3 (means \\) or 0-3, 1-2 (means //)
	  f_line(pc[0], pc[1]); f_line(pc[2],pc[3]);
	} else {
	  f_line(pc[0], pc[3]); f_line(pc[1],pc[2]);
	}
      }
    }
  }
}   

void isoline_window::draw_interior() {
  draw_coords(); // x,y coord lines
    
  rline(0,h_eff,w_eff,h_eff); rline(w_eff,0,w_eff,h_eff); // other limits
  x_ticks(1); y_ticks(1);

  if (raster) {
    set_color(31); // color for drawing raster lines
    for (int x = 1; x < nx; x++) wline(x,0,x,ny-1);
    for (int y = 1; y < ny; y++) wline(0,y,nx-1,y);
  }
  int lcols[4] = { 0, 31, 32*63, 2048*31 };  
  for (int l = 0; l < nlevels; l++) {
    set_color(lcols[l]);
    float flevel = levels[l] + 0.00001; 
    // addition of 0.0001 : trick to avoid isolines cross nodes
    char text[40]; sprintf(text,"isoline %g",flevel);
    DrawString(width-100,(l+1)*12+4, text);
    isolevel(flevel);
  }
  set_color(black);
}
