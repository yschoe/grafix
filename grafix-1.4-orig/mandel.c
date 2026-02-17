// mandel.c : demo for the Mandelbrot set
// wolf 1/97

#include "window.h"
#undef Complex
#include <Complex.h>

#define ITMAX 255

inline int iterate(Complex& c) {
  int it = 0;
  Complex z = 0;
  do { z = z*z + c  ; if (++it == ITMAX) break; } while (norm(z) < 1e4);
  return it;
}

main_window *newman(double x1, double y1, double x2, double y2, Bool top=False);

class mandel_win : public coord_window {
  info_window *iw;
public:
  mandel_win(window &parent, int nx, int ny, info_window *iw) :
  coord_window(parent,nx,ny,0,0), iw(iw) {
    selection_mask |= ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
  }
  XPoint p1,p2;
  void BPress_1_CB(XButtonEvent ev) {
    p1.x = ev.x; p1.y = ev.y;
    p2 = p1;
  }
  void BRelease_CB(XButtonEvent ) {  
    if (fabs(p2.x-p1.x) + fabs(p2.y-p1.y) > 20) {
      main_window *mw= newman(x_org(p1.x),x_org(p2.x),y_org(p2.y),y_org(p1.y));
      mw->RealizeChildren();
    }
  }
  void Motion_CB(XMotionEvent ev) { 
    if (ev.state & Button1Mask) { 
      // clear old rect
      XDrawRectangle(display,Win,gc_rubber,p1.x-1, p1.y-1,  
		     p2.x - p1.x + 2, p2.y - p1.y + 2);
      p2.x = ev.x; p2.y = ev.y;
      XDrawRectangle(display,Win,gc_rubber,p1.x-1, p1.y-1,  
		     p2.x - p1.x + 2, p2.y - p1.y + 2);
    }
    else {
      sprintf(iw->info,"%g %g",x_org(ev.x),y_org(ev.y));
      iw->redraw();
    }
  }
  void draw_interior() {
    int nx = width, ny = height;
    for (int y = 0; y < ny; y++) {
      for (int x = 0; x < nx; x++) {
	Complex c(x_org(x),y_org(y));
	int it = iterate(c);
	set_color(it);
	DrawPoint(x,y);
      } // to show the progress :
      XCopyArea(display,pix, Win, gc_copy, 0, y, width, 1, 0, y);
      XMapWindow(display, Win); 
    } 
    set_color(black);
  }
};

main_window * newman(double x1, double x2, double y1, double y2, Bool top) {
  int nx = 400, ny = 320; 
  char str[200]; sprintf(str,"x [%g %g] y [%g %g]",x1,x2,y1,y2);
  main_window *mw = new main_window(str,nx,ny);
  info_window *iw = new info_window(*mw,nx,20,0,ny-20);
  mandel_win *cw = new mandel_win(*mw,nx,ny-20,iw);
  cw->define_coord(x1,y1,x2,y2);
  if (top) new quit_button(*mw,40,20,0,0);
  return mw;
}

main() {
  main_window *mw = newman(-2,0.5,-1,1,True);    
  mw->main_loop();
}


