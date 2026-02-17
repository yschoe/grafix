
#include "window.h"
#include "palette.h"

unsigned palette_popup::palind(float min, float max, int i) {
  if (ncolors == 0) return 0;
  float f = (i * (max-min)) / ncolors;  
  int xi =  (int) (min + f);
  if (xi < 0) return 0;
  if (xi > 0xffff) return 0xffff; else return xi;
}

// not very efficient : if all scrollbars are set, this is called 3 times !!
void palette_popup::paint() { 
  for (int i = 0; i < ncolors; i++) { 
    XColor *xcol = &color_cells[i];
    unsigned r = palind(smin[0]->value,smax[0]->value,i);
    unsigned b = palind(smin[1]->value,smax[1]->value,i);
    unsigned g = palind(smin[2]->value,smax[2]->value,i);
    if (True_Color_Visual) xcol->pixel = alloc_color(r,g,b); 
    else {
      xcol->red = r; xcol->blue = b; xcol->green = g;
      XStoreColor(display,def_cmap,xcol);
    }
  }
  if (True_Color_Visual && redraw_win)  { // this should be really a list !
    redraw_win->redraw(); // printf("%x\n",redraw_win);
  }
}

// a window to display color bars
class palette_display : public window {
  GC gc;  
  XGCValues values; 
  palette_popup *pp;
public:
  palette_display(window &parent, int w, int h, int x, int y, 
		  palette_popup *pp) : 
  window(parent,w,h,x,y), pp(pp) { 
    values.foreground = 0;
    gc = CreateGC(GCForeground, &values); 
    pp->redraw_win = this; // I will be redrawed !!
  }
  virtual void redraw() { 
    int i, x = 0, nc = pp->ncolors; if (nc == 0) return;
    int dx = width/nc + 1;
    for (i = 0; i < nc; i++) { 
      values.foreground = pp->color_cells[i].pixel; 
      XChangeGC(display,gc, GCForeground, &values); 
      x = int(i*width/float(nc));
      XFillRectangle(display,Win,gc,x,0,dx,height); 
    }
  }
};

char * colstr[] = { "red", "blue", "green"};

class pal_text_win : public window {
public: 
  pal_text_win(window &parent, int h, int x, int y) : 
    window(parent, 70,h ,x,y,0) {}
  virtual void redraw() { 
    int x = 5, y = 15, i;
    for (i = 0; i<3; i++) { 
      DrawString(x+30,y+12,colstr[i]);
      DrawString(x,y,"xmin"); y+= 25; 
      DrawString(x,y,"xmax"); y+= 40; 
    } }
};
 
// must be static !
static
struct palstr fbar[] = { { "brown ", { {21, 4, 0 }, { 65, 65, 55} } },
			 { "rainbow", { { 0, 65, 0}, { 65, 0 , 22} } },
			 { "yellow", { {0,0,0}, { 65, 0, 65} } },
			 { "steel",  { {0,0,0}, {0,65,65} } },
			 { "violet", { {37,37,0}, {65,65,65} } },
			 { "gray", { {0,0,0}, {65,65,65} } }
                       };

static void pal_paint(palette_popup * pp) { pp->paint(); };
static void pal_set_cb(palette_popup *pp, palstr *ps) { pp->set_pal(ps); }

// 1. constructor for palette_popup : creates a color palette and
// a popup for displaying and manipulate the palettes   
palette_popup::palette_popup(int ncol, palstr* pinit) 
: main_window("palette",375,330) {
  redraw_win = NULL; // will be set in init_palette
  init_palette(ncol);
  long unsigned plane_mask; 
  if (! True_Color_Visual)
    for (int i = 0; i < ncol; i++) {
      // printf(" %d",i); fflush(stdout);
      if (0 == XAllocColorCells(display,def_cmap,TRUE,&plane_mask,0,
				&(color_cells[i].pixel),1)) {
	printf("warning : only %d color cells of %d free on display\n",i,ncol);
	ncolors = i; break;
      }
      color_cells[i].flags = DoRed | DoGreen | DoBlue;
      // cout << color_cells[i].pixel <<" "; 
    } 
  if (pinit == NULL) pinit = fbar+0; // def. = first element
  set_pal(pinit);  // initialization  
}

// 2. constructor : the ColorCells are already present  (pixels)
palette_popup::palette_popup(int ncol, long unsigned *pixels) : 
  main_window("palette",375,330) {
  init_palette(ncol);
  for (int i = 0; i < ncolors; i++) {
    color_cells[i].pixel = pixels[i];
    color_cells[i].flags = DoRed | DoGreen | DoBlue;
  }
}

// used by the constructors     
void palette_popup::init_palette(int ncol) {
  ncolors = ncol;

  int ci,i,y = 5, w = 300, x= 5; 
  new pal_text_win(*this,180,w+x,y);
  for (ci = 0; ci<3; ci++) {
    smin[ci] = new scrollbar(*this,pal_paint,this,w,20,x,y,0,0xffff); y+= 25;
    smax[ci] = new scrollbar(*this,pal_paint,this,w,20,x,y,0,0xffff); y+= 40;
  }

  menu_bar *mb = new menu_bar(*this,370,20,x,y,50,100,0); y+= 25;
  int nbar = sizeof(fbar)/sizeof(palstr); // Elementzahl von fbar
  
  for (i=0; i < nbar; i++)
    new function_button(*mb,fbar[i].Name,(CB) pal_set_cb, this, fbar+i);

  new palette_display(*this,370,50,x,y,this); y+= 60;
  new unmap_button(*this,"close",100,20,130,y);
  color_cells = new XColor[ncolors];   
}

void palette_popup::set_pal(palstr *ps) { 
  for (int i=0; i< 3; i++) { 
   smin[i]->change(ps->limits[0][i]*1000); 
   smax[i]->change(ps->limits[1][i]*1000); }
}
