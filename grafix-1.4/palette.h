struct palstr { char* Name; int limits[2][3]; };

class palette_popup : public main_window {
public:
  XColor *color_cells;
  scrollbar *smin[3], *smax[3];
  int ncolors;
  window *redraw_win; // the window to redraw when palette changes
  // 1. constructor allocates new ColorCells
  palette_popup(int ncol, palstr* pinit = 0);  

  // second constructor : use it if ColorCells are already allocated
  palette_popup(int ncol, long unsigned *pixels); 
  void paint();
  void set_pal(palstr *ps);
private:
  unsigned palind(float min, float max, int i);
  void init_palette(int ncol);
};
