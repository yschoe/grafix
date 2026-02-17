// hardcopy.c : print hardcopy of pointer-selected window on printer /dev/lp1
//              specialized for epson 9-needle printer LX 400 or similiar
//              and only for 16bpp True Color now (rgb = 565)
// choose optimal x-resolution (if possible : plotter mode, else shrink)

static char NLN[] = { 27, 74, 24, 0}; // Zeilenvorschub 24/216 = 1/8 Zoll
static char InitSeq[] = { 27, 64, 0}; // initialize printer 

#include "window.h"

// macros for rgb = 565
#define b565(c) ((c) & 31)
#define g565(c) (((c) >> 5) & 63)
#define r565(c) (((c) >> 11) & 31)
#define lum565(c) (b565(c) + g565(c) + r565(c))

extern "C" XDestroyImage(XImage *);

void scan(Bool top);  

Cursor cross_curs = XCreateFontCursor(display,130); // cross
Window root = DefaultRootWindow(display);

// display selected window as b/w image with options to modify
class hardc_win : public main_window {
  XImage *xi;
  pixmap_window *pm;
  int nx,ny,xoff;
  unsigned short *src;
  unsigned short rth,gth,bth; // color thresholds 0..31/63/31
  FILE *lp;
  // choose a mask for print colors
  // here : print 0xeff (10. raster) but not 0xfff (1-raster)
  Bool dot(unsigned short pix) { 
    return ((pix & 0xfff) < 0xfff); 
    // return (r565(pix) < rth && g565(pix) < gth && b565(pix) < bth); 
  }

public:  
  hardc_win(Bool top, XImage *xi) : 
  main_window("view", xi->width, xi->height+20),xi(xi) { 
    rth = 32; gth = 31; bth = 32;
    src = (unsigned short *) xi->data;
    nx = xi->width; ny = xi->height; xoff = xi->bytes_per_line/2; 
    printf("window size = %d x %d\n",nx,ny);
    pm = new pixmap_window(*this,nx,ny,0,20);
  
    for (int y = 0; y < ny; y++) {
      for (int x = 0; x < nx; x++) // choose a mask for print colors
	if (dot(src[xoff*y + x])) pm->DrawPoint(x,y);
    }  
    menu_bar *mb = new menu_bar(*this,nx,20,0,0,0,nx/3); // maxw = nx/3
    new template_button <int> (*mb,"new",scan,0);
    new instance_button <hardc_win> (*mb,"print",print,this);
    if (top) {
      new quit_button(*mb);
      main_loop(); 
    } else { 
      new delete_button(*mb,this);
      RealizeChildren();
    }
  }
 
  void lpr(char *str) { fprintf(lp,"%s",str); }

  void print() {
    // copy window to image, works only in 16bpp TrueColor mode !!
    lp = fopen("/dev/lp1","w");
    if (lp == 0) {
      printf("\nerror opening %s\n",strerror(errno));
      exit(1);
    }  
    lpr("\n"); // important : Papier spannen
    lpr(InitSeq);	
    int grmod=5; // 0..7 , 5 = 72 dpi (1:1 Plotter)(580p) 4 = 80dpi (644p)
    if (nx > 725) grmod = 2; else 
      if (nx > 644) grmod = 6; else 
	if (nx > 580) grmod = 4;

    unsigned char lbuf[nx];  bzero(lbuf,nx);
    unsigned char pin = 0x80; // 8. bit
    for (int y = 0; y < ny; y++) {
      int x; 
      for (x = 0; x < nx; x++) if (dot(src[xoff*y + x])) lbuf[x] |= pin; 
      if (pin > 1 && y < ny-1) pin = pin >> 1; 
      else { // line finished
	// empty pixels at the end may be omitted for speed
	int nxx = nx; while (nxx > 0 && lbuf[nxx-1] == 0) nxx--;
	// init graph mode 
	fprintf(lp,"%c%c%c%c%c%c",13, 27, 42, grmod, nxx % 256, nxx / 256);
	for (x = 0; x < nxx; x++) fprintf(lp,"%c",lbuf[x]);
	lpr(NLN);
	pin = 0x80; bzero(lbuf,nx);
      }
    }
    lpr(InitSeq); fclose(lp);
  }

  ~hardc_win() { XDestroyImage(xi); }  
};

// interactively select a window to hardcopy and display it as b/w picture
void scan(Bool top) {
  printf("\nclick on the window for hardcopy %c\n",7);
  XGrabPointer(display,root,False,ButtonPress,
	       GrabModeAsync,GrabModeAsync,root,cross_curs,0);

  while (1) { 
    XEvent event;   
    XNextEvent(display, &event);
    if (event.type == ButtonPress) break;
  }
  XUngrabPointer(display,0);
  Window rr,dumpw;
  int rx,ry,wx,wy;
  unsigned mask;
  XQueryPointer(display,root,&rr,&dumpw,&rx,&ry,&wx,&wy,&mask);

  XWindowAttributes watt;
  XGetWindowAttributes(display,dumpw,&watt);
  // copy window to image, works only in 16bpp TrueColor mode !!
  XImage *xi = XGetImage(display,dumpw,0,0,
			 watt.width,watt.height,AllPlanes,ZPixmap);
  new hardc_win(top,xi);
}

main() {
  scan(True);
}

