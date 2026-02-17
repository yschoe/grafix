// files.h
#ifndef FILES_H
#define FILES_H

#define MODE_READ 0  // only read
#define MODE_WRITE 1 // test if already exists
#define MODE_OVWRT 2 // always overwrite old file, no test

FILE *open_file_interactive(char *defname, char *name_ret, int mode);
FILE *fopen_test(char *fina);

#define CONFIRM_UNDEF 0
#define CONFIRM_YES 1
#define CONFIRM_NO 2

// open a confirmbox, returns CONFIRM_YES or  CONFIRM_NO
int yes_no_box(char *title, char **text, int w = 200, int h = 120);

// simply set a variable of type T to value 
template <class T>
class variable_button : public button {
  T value;
  T* variable;
public:
  variable_button(window &parent, char *Name, T *variable, T value, 
		  int w, int h, int x, int y) :
    button(parent,Name,w,h,x,y), value(value), variable(variable) { }
  void BRelease_CB(XButtonEvent) { *variable = value; }
};

class window;
class text_scrollbar;
class text_window;

// text_window with an attached scrollbar
class text_viewer : public window {
  text_scrollbar *vs;
  text_window *tw;
public: 
  int slider_height(int zz);
  void reset(char *buf, int nc);
  text_viewer(window &parent, int w, int h, int x, int y, char *buf, 
	      int nc, int bw = 0);
  virtual void KeyPress_CB(XKeyEvent ev);
};

//  ************* vert_scrollbar ***********
//  vertical scrollbar with re-definable behaviour :
//  defines virtual functions for move (button 2) and jmp (button 1 &3)
//  should actually shift to window.h

class vert_scrollbar : public plate {
  plate *bar;
protected:
  int sw,sh,sx,sy,yspan; // span width for movement y = [0..yspan]
public:
  int yact; // actual position (0..yspan)
  void set_vars();
  // sh : height of slider
  vert_scrollbar(window &parent, int w, int h, int x, int y, int sh);
protected:
  virtual void move_callback(int y) { // called from draw (Button 2)
    set_slider(y - sy); 
  }
  virtual void jmp_callback(Bool up) { // called from pg (Button 1 & 3)
    set_slider( yact + (up) ? (-sh) : sh); // jmp +/- one sh
  } 
  // jump on mouse click
  virtual void BPress_CB(XButtonEvent ev);
  // draw if button 2 pressed
  virtual void Motion_CB(XMotionEvent ev);
public: 
  void set_slider(int y);
  // set slider in [0.0 .. 1.0] : 
  void set_slider_rel(float z) { set_slider(irint(z * yspan));}
  // called from resize && others 
  void slider_adapt(int shp); // adaptate to new slider height
  void resize(int w, int h);
  void adapt(int h, int shp);
};

class select_scrollbar;
class select_item;

typedef void (*VPCP)(void *, char *);

// class for entering filename masks calls enter_cb 
class mask_edit : public edit_window {
  VPCP cb;
  void *wptr;
public: 
  mask_edit(window &parent, VPCP enter_cb, void *inst, 
	    int w,int h, int x, int y) : 
    edit_window(parent,"",w,h,x,y) { cb = enter_cb; wptr = inst;}
  virtual void enter() { cb(wptr,value); } 
};

// plate for filename display (or similiar string)
class file_display : public plate { 
public:
  char *val;
  file_display(window &parent, int w, int h, int x, int y) :
    plate (parent, w,h,x,y, down3d) { val = NULL; }
  virtual void redraw();
  void set_val(char *x);
};

// makes a plate with (max) nits select_items -> selection box
// not all are neseccarily drawn
// this should be completed with a scrollbar
class selector : public plate {
  select_item **pits;
  char **itptr;
  int itot; // total number of items in **pits
  int ishift; // index of first displayed item
  window *interior; // the inner part w/o boundary
  select_scrollbar *vs; // the attached scrollbar
public:
  int itdsp; // number of actually displayable items 
  selector(window &parent, int nits, VPCP show_cb, VPCP act_cb,
           int w, int h, int x, int y, int hi);
  void set_items(char **items, int nits);
  
  ~selector() { delete[] pits; }

  void redraw();
  void resize(int w, int h);

  void shift(int ip);
  void shift_rel(int di);   // shift rel to current pos
  // intervall for ishift
  int ispan() { return (itot - itdsp); } 

  void KeyPress_CB(XKeyEvent ev);
};

#endif // FILES_H
