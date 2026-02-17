// window.h : basic include file for the Grafix package
// the "GNU Public Lincense" policy applies to all programs of this package
// (c) Wolfgang Koehler, wolf@first.gmd.de, Dec. 1994
//     Kiefernring 15
//     14478 Potsdam, Germany
//     0331-863238

#if !defined WINDOW_H
#define WINDOW_H

#include <X11/Xlib.h>
// #include "X11/Xutil.h"
#include <X11/keysym.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define FALSE 0
#define TRUE 1

#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

// since some dumb platforms dont have irint, or rint
#define irint(x) int((x)+0.5)

#define SimpleType 0
#define PulldownType 1

extern void error(char*, ...);

char *ext_string(char * str); // extend str for 1 + 1 space 

GC CreateGC(unsigned long mask, XGCValues * gcv);

void set_color(unsigned long color); // set color for gc_copy 
int alloc_color(unsigned red, unsigned green, unsigned blue);
int alloc_named_color(char *colname);

// draws pseudo-3D-borders, mode = up3d, flat3d (background pix), down3d
void rect3d(Window Win, int mode, short x, short y, short w, short h);

void tri3d_s(Window Win, int mode, short x, short y, short w, short h);

struct point { int x; int y; };

extern Colormap def_cmap;
extern Display * display;
extern int screen;
extern GC gc_copy, gc_but_xor, gc_clear, gc_inv, gc_rubber; 
// some often used gcs
extern GC button_fg_gc, button_br_gc, button_lw_gc; 
extern Cursor watch_cursor; // for long redraw methods eg. in coordwindows
extern Cursor hs_cursor, vs_cursor; // special cursors for scrollbars
extern XFontStruct * fixed_fn;
// some predifined simple colors : use capitals to avoid name conflicts (NR)
extern unsigned depth, black, white, Red, Green, Blue, Yellow, Violet;
extern Bool True_Color_Visual; // only for TrueColor 
                               // -> XAllocColorCells will not work then 
void handle_event(XEvent &event);

class window;

void safe_delete(window *w); // delete only, if not yet deleted !

struct win_list { // struct to manage a child list of any window
  window *child; 
  int x,y;
  struct win_list *next;
  win_list(window *ch, int x, int y, win_list *rest) : child(ch), x(x), y(y) 
    { next = rest; }
};

class main_window;
class text_popup;

class window {

protected:
  int border_width;
  GC gc,text_gc;
  text_popup * help; // ggf. help popup
  window *parentw;   // only for destructors
  long selection_mask; // default mask for SelectInput
  Bool hidden;    // if true not realize the window 
public:  
  main_window *mainw; // the main window of this window, direct child of root 
  int width, height;
  int CB_info;   // toggle debug infos on callbacks in this Window
  Window Win;
  int type;      // to distinguish simple <-> pulldown

  win_list * children;        // chained List of children
 
  window(char * DISP = NULL); // constructor for root window
  window(window & parent,
	 int w = 0, int h = 0, int x = 0, int y = 0, int bw = 1);
  // to update children list and fit the parent geometry :
  virtual void add_child(window *ch, int x, int y);
  
  void remove_child(window *ch); // remove from my children list
  virtual ~window();  // virtual destructor
  
  virtual void draw_interior() { }   // is called from Map to draw an image 
  virtual void Map(); 
  void Unmap();
  Bool toggle_map(); // to toggle mapping state : if mapped -> Unmap (avv)
  
  void set_backing_store();  // set backing_store = WhenMapped
  void set_save_under();     // save covered region when mapped

  void Realize(); // Realize the whole Window tree from this root
  void RealizeChildren();

  virtual void clear();
  virtual int eff_width() { return width; } // effective breadth minus pictures

  virtual void DrawString(int x, int y, char* string);

  void printw(int x, int y, char* fmt, ...); // called like printf, max 1000 !

  // place a string with font (fixed_fn) at x,y (0 = centered)
  void PlaceText(char * string,int x = 0, int y = 0,
		   XFontStruct * fn = fixed_fn);
  virtual void line(int x0, int y0, int x1, int y1);
  virtual void DrawPoint(int x, int y);

  void add_help(char * WMName,char * text[]);
  
  virtual void action(char*, char*) {} // action for windows, that shall
  // be managed from radio_button - BPress-CBs

  void WatchCursor(); // set cursor of main_window to watch
  void ResetCursor(); // set back to normal
  virtual void redraw() {}
  virtual void resize(int, int);
protected:  
  virtual void BPress_CB(XButtonEvent) {} 
  virtual void BPress_1_CB(XButtonEvent) {}
  virtual void BPress_3_CB(XButtonEvent); // default : popup help
  virtual void BRelease_CB(XButtonEvent) {} 
  virtual void Enter_CB(XCrossingEvent) {}
  virtual void Leave_CB(XCrossingEvent) {}
  virtual void Motion_CB(XMotionEvent) {} // pointer movements
  virtual void Expose_CB(XExposeEvent);
  virtual void KeyPress_CB(XKeyEvent) {}
  virtual void Configure_CB(XConfigureEvent) {} // only main_windows
  virtual void ClientMsg_CB(XClientMessageEvent) {} // for msg from w manager

public:  
  void CallBack(XEvent &event) ;
};

// for special applications : in the main_loop use polling 
// extern Bool polling_mode; // use polling instead of XNextEvent
// extern void (*poll_handler)(); // called after each polling 

class main_window : public window {
protected:
  Bool polling_mode; // for special applications: in the main_loop use polling 
  // with the virtual function "polling_handler" instead of XNextEvent
  char *name;
public: 
  int exit_flag; // used in main_loop to break it
  main_window(char *Name, int w, int h, int fix_pos = 0, int x = 0, int y = 0);
  ~main_window();
  void do_popup(int x, int y); // Realize at absolute x,y-coords
  virtual void Configure_CB(XConfigureEvent ev);
  virtual void ClientMsg_CB(XClientMessageEvent ev);
  void set_icon(char ibits[], int iwidth, int iheight);
  void main_loop(); 
  virtual void polling_handler() {}
};

// A window that stores its content in a Pixmap, to enable a quick Expose
// Each drawing operation is done to Pixmap, Map & Expose copy pix -> Win
// 
class pixmap_window : public window {
protected:
  Pixmap pix;  // to store the image and re-map in expose events
public:
  pixmap_window(window & parent, int w = 0, int h = 0, 
	       int x = 0, int y = 0, int bw = 1);
  virtual ~pixmap_window();
  virtual void clear();
  virtual void Map(); // copy pix -> Win
  virtual void DrawString(int x, int y, char * string);
  virtual void line(int x0, int y0, int x1, int y1);
  virtual void DrawPoint(int x, int y);
  virtual void Expose_CB(XExposeEvent ev); 
  virtual void draw_interior() {}
  virtual void redraw() { clear(); Map(); }
  virtual void resize(int,int);
};

// very simple window to place text
class text_win : public window {
char *text;
public:
  text_win(window &parent, char *text, int w,int h, int x, int y, int bw = 0) :
    window(parent,w,h,x,y,bw), text(text) {}
  virtual void redraw() { PlaceText(text);  }
};

// a class for drawing an infoline in a window (max 200 chars), not flickering
class info_window : public pixmap_window {
public:
  char info[200];
  info_window(window &parent,int w, int h, int x, int y) :
    pixmap_window(parent,w,h,x,y) { info[0] = 0; }
  virtual void draw_interior() { PlaceText(info); }
  char* infoend() { return (info + strlen(info)); } // for appending to end
};

// manage position of children
class horizontal_container {
  int min_w, max_w; // minimum & maximum width for children
public:
  int xp, yp; // place position for next child, is updated from children
  int getw(int w) { return ((w > min_w) ? ((w > max_w) ? max_w : w) : min_w) ;}
  horizontal_container(int minw, int maxw) : min_w(minw), max_w(maxw) 
  { xp = yp = 0; }
};

class menu_bar : public window, public horizontal_container {
public:
  menu_bar(window& parent, int w, int h = 20, int x = 0, int y = 0, 
	   int minw = 0, int maxw = 1000, int bw = 1)
  : window(parent,w,h,x,y,bw), horizontal_container(minw,maxw) {}
  virtual void add_child(window *child, int x, int y) { 
    window::add_child(child,x,y); xp += getw(child->width); 
  }
};

class text_popup : public main_window  {
  char ** pop_text;
public: 
  text_popup(char * WMName, int w, int h, char *text[]); 
  void Expose_CB(XExposeEvent ev);
};

// Konstanten zur 3d Darstellung
#define down3d 0 
#define up3d   1 
#define flat3d 2

// a window with pseudo-3d frames
class plate : public window { 
  int mode3d;
public:
  plate(window & parent, int w, int h, int x = 0, int y = 0,int mode3d = up3d);
  virtual void redraw();  
protected:  
  void frame3d(int mode) { rect3d(Win, mode, 0, 0, width-1, height-1); }
  virtual void default_frame() { frame3d(mode3d); }
};

class button : public plate {
  //  parent is a pulldown menu -> other dynamic frame mode
  int in_pulldown;
protected:
  // the breadth of strings for  menu_bar (for auto-placement in container)
  int Nwidth(char *str) { return 6*strlen(str) + 6; }
public:
  char * Name;
  void add_help(char **help_text); // replaces window::add_help
  void init_button(window *parent);
  button (window & parent, char * Name, int w, int h, 
	  int x = 0, int y = 0) :
  plate (parent, w, h, x, y), Name(Name) { init_button(&parent); }

  button (window & parent, char * Name, char ** help_text, int w, int h,
	  int x = 0, int y = 0) : plate (parent, w, h, x, y), Name(Name) 
    { add_help(help_text); init_button(&parent); }
  
  // autosizing buttons
  button (menu_bar & parent, char * Name) :
    Name(Name), plate (parent, parent.getw(Nwidth(Name)),  
                       parent.height, parent.xp, parent.yp) 
    { init_button(&parent); }
  virtual ~button();
  virtual void default_frame() { frame3d(in_pulldown ? flat3d : up3d);  }
  void enter_frame() { frame3d(in_pulldown ? up3d : flat3d); }
  virtual void redraw(); 
protected: 
  virtual void Enter_CB(XCrossingEvent) { enter_frame(); }
  virtual void Leave_CB(XCrossingEvent) { default_frame(); }
  virtual void BPress_CB(XButtonEvent) { frame3d(down3d); }

  virtual void BPress_1_CB(XButtonEvent ) { 
    // printf("button press '%s' %d \n", Name, ev.button); 
  }
  virtual void BRelease_1_action() {} // hook for callbacks with Button1
  virtual void BRelease_CB(XButtonEvent ev) { 
    if (ev.state & Button1Mask) { BRelease_1_action(); default_frame(); }
  }

};

//                 ##### delete_button #####
// for ordered deleting a window and all of its children recursively
// following events for them are catched by setting the thisW-pointer to NULL
class delete_button : public button { 
window * to_del;
public: 
  delete_button(window & parent, window * to_del, int w, int h, int x, int y): 
    button(parent, "delete", w, h, x, y), to_del(to_del) {}
  delete_button(menu_bar & parent, window * to_del) :
    button(parent, "delete"), to_del(to_del) {}
private:
  void BPress_1_CB(XButtonEvent) { to_del->Unmap(); delete (to_del); }
};

//             **** "quit_button" ****
// exiting main_loop with setting the exit_flag of corr. main_window
class quit_button : public button { 
public: 
  quit_button(window & parent, int w, int h, int x, int y) : 
    button(parent, "quit", w, h, x, y) {}
  quit_button(menu_bar & parent) : button(parent, "quit") {}
private:
  void BPress_1_CB(XButtonEvent) { _exit(0); }
};

// popup_button : realize the popup menu when BPress (make it visibel) 
// if pressed again : make it unvisible 
class popup_button : public button {
protected: 
  main_window * popup_menu; 
public: 
  popup_button(window &parent, main_window * menu, char * Name, 
	       int w, int h, int x, int y) :
    button(parent, Name, w, h, x, y) { popup_menu = menu; }  

  popup_button(menu_bar &parent, main_window * menu, char * Name) :
    button(parent, Name) { popup_menu = menu; } 
protected:
  virtual void BPress_1_CB(XButtonEvent ev);
};

// popup a window with help text and a OK button
// the popup window is created in make_popup from the text array
class help_button : public popup_button {
public: 
  help_button(window & parent, int x, int y, char * text[]) :
    popup_button(parent, NULL, "help", 60, 20, x, y) { make_popup(text); }

  help_button(menu_bar & parent, char * Name, char * text[]) :
    popup_button(parent,NULL, Name) { make_popup(text); }
private:
  void make_popup(char * text[]);
};

// *** "callback_button" class ****
// a button attached with a callback function on BRelease !! event
class callback_button : public button {
  void (*callback) ();
public:
  callback_button (window & parent, char * Name, void (*cb)(), 
		   int w, int h, int x = 0, int y = 0) : 
  button (parent, Name, w, h, x, y), callback(cb) { }
  
  callback_button (menu_bar & parent, char * Name, void (*cb)()) : 
  button (parent, Name), callback(cb) { }

protected:
  virtual void BPress_1_CB(XButtonEvent) { } // do nothing
  virtual void BRelease_1_action()  { (*callback)(); } 
};

// *** "template_button" class ****
// invokes any member function (void) of an instance of class T, or
// a simple function that has T as argument
// ie. the callback-function is of type "void cb(T)"
// example:  void dist(float x) {ww->z +=x;..} 
//            template_button <float> (mb,"Name", dist, 2);

template <class T>
class template_button : public button {
  void (*callback) (T);
  T value;
public:
  template_button (window & parent, char * Name, void (*cb)(T), T val,
		   int w, int h, int x = 0, int y = 0) : 
  button (parent, Name, w, h, x, y), callback(cb) {value = val; }
  
  template_button (menu_bar & parent, char * Name, void (*cb)(T), 
		  T val) : 
  button (parent, Name), callback(cb) { value = val; }

protected:
  virtual void BPress_1_CB(XButtonEvent) { } // do nothing
  virtual void BRelease_1_action()  { (*callback)(value); }
};

//   ***** instance_button : template button for member-functions of class
template <class T>
class instance_button : public button {
  void (T::*member)();
  T *instance;
public:
  instance_button(window &parent, char *Name, void (T::*mem)(), T *inst,
		  int w, int h, int x, int y) :
    button(parent,Name,w,h,x,y) { member = mem; instance = inst; }

  instance_button(menu_bar &parent, char *Name, void (T::*mem)(), T *inst) :
    button(parent,Name) { member = mem; instance = inst; }  
protected:
  virtual void BPress_1_CB(XButtonEvent) { (instance->*member)(); }
  virtual void BRelease_1_action() {} 
};

// *** "function_button" class **** : yet another variant of callbacks
// a button attached with a callback function on BRelease !! events
// the callback_function gets all arguments of the  ellipses
// (pointer or int); max 10 
// default arguments are not prohibited !

#include <stdarg.h>

typedef void (*CB)(...); // type of the callback fn

class function_button : public button {
  CB callback;
  void *values[10]; // the passed values
public:
  function_button (window & parent, char * Name,  
                   int w, int h, int x, int y, CB cb,  ...);

  function_button (menu_bar & parent, char * Name, CB cb, ...);
protected:
  virtual void BPress_1_CB(XButtonEvent) { } // do nothing
  virtual void BRelease_1_action() {
    (*callback)(values[0],values[1],values[2],values[3],values[4],values[5],
		values[6],values[7],values[8],values[9]);} 
};

typedef void (*VVP)(void *); // typecast for callbacks with void* argument
extern void switch_dummy(void *); // default dummy fn : does nothing

// ****** class switch_button *****

// a button with 2 states of display, which switch on click
// the 2. string should be <= the first (= initial)
class switch_button : public button {
  char *Narr[2]; // the two strings for display
  VVP callbck; // the callback to call with instance ptr
  void *instptr;
  Bool *toggle; // the pointer to toggled value
public:  
  switch_button (menu_bar & parent, char *Name1, char *Name2, Bool *toggle,
		 VVP inf = switch_dummy , void * toinf = NULL);

  switch_button (window & parent, char *Name1, char *Name2, Bool *toggle,
		 VVP inf, void * toinf, int w, int h, int x, int y);

  void switch_it();
protected:
  virtual void BPress_1_CB(XButtonEvent) { } // do nothing
  virtual void BRelease_1_action() { switch_it(); }
};


//     ****** class toggle_button : with display of state ****

class toggle_button : public button {
  int *vptr; // the pointer to  toggle-value
  int xright; // place for picture on the right side
  virtual int eff_width() { return (width - xright); } // for PlaceText
public:
  toggle_button(menu_bar &parent, char *Name, int *ref) : 
      button(parent,ext_string(Name)) { vptr = ref ; xright = 12; }
  toggle_button(window &parent, char *Name, int *ref, int w, int h, 
		 int x, int y ) :
      button(parent,Name,w,h,x,y) { vptr = ref; xright = 12; }
  void picture() { // 8x8 Pixel gross
    int offs = (height-8)/2; // distance to top- and bottom border
    rect3d(Win,(*vptr) ? down3d : up3d, width-xright,offs,8,8); }
protected:
  virtual void toggle_cb (int) {}  // can be overloded
  virtual void BRelease_1_action() { 
    *vptr = ! *vptr; picture(); toggle_cb(*vptr); }
  virtual void redraw() { button::redraw(); picture(); }
};

//   ************  frequently used : toggle and redraw a window
class toggle_redraw_button : public toggle_button {
window *win;
public:
  toggle_redraw_button(menu_bar &parent, char * Name, int *ref, window *win):
     toggle_button(parent,Name,ref), win(win) { }
  virtual void toggle_cb(int) { win->redraw(); }
};

// *********************************************************************
//                   PULLDOWN windows
// pulldown window : a window child from root_window, not yet visible, 
// not managed from WM, position is determined from the button at popup time
class radio_button;
class pulldown_window : public main_window {
  radio_button *rold; // the last selected radio_button
public:
  pulldown_window (int w, int h);
  inline void Release_CB(XButtonEvent) { Unmap();}
  void toggle(radio_button *rb);
};

//     **** "pulldown_button" class *****
// map the window (menu) on root when button is activated (BPress)
// the window is mapped to absolute co-ordinates !
class pulldown_button : public button { 
  pulldown_window  * pulldown_menu; 
  int xright;
  virtual int eff_width() { return (width-xright); }
public:
  pulldown_button (window & parent, pulldown_window * menu, char * Name, 
		   int w, int h, int x, int y);
  pulldown_button(menu_bar & parent, pulldown_window * menu, 
		  char * Name);
  pulldown_button (window & parent, pulldown_window * menu, char * Name, 
		   char ** help_text, int w, int h, int x, int y);
  void picture();
protected:
  virtual void BPress_1_CB(XButtonEvent ev);   
  virtual void Leave_CB(XCrossingEvent ev) { // if B1 was not pressed
    if (!(ev.state & Button1Mask)) default_frame(); }
  virtual void redraw() { button::redraw(); picture(); }
};


// creates single button entry with Name and value in a radio_menu
class radio_button : public toggle_button {
  char * value;  // the special value
  char * menu_name ; // the name of the button which pulled this radio_menu
  window * action_win;
public:  
  Bool status; // managed by the parent pulldown_window
  radio_button(pulldown_window &parent, char * MName, 
	       int w, int h, int x, int y, char * val, window * action) 
       : toggle_button(parent,val,&status,w,h,x,y), value(val) {
    menu_name = MName; // Name of menu_button; is passed to action
    action_win = action;
  }
protected:
  virtual void BRelease_1_action() { 
    if (action_win) action_win->action(menu_name, value); 
    ((pulldown_window *) parentw)->toggle(this); // inform parent which is set 
  }
};

// makes a radio-menu (toggle) with  named buttons (name = value : string)
// list : consists of char* : { "value1", "value2",..., 0 }
// action_win : window which "action" method is to call after toggle
// the Name (as pointer) is passed also -> to know which menu was activated
// trick : if w == 0 parent must be menu_bar -> use autoplacement
pulldown_button * make_radio_menu(window &parent, char *Name,
				  char **blist,  window * action_win = 0,
				  int w= 0, int h= 0, int x= 0, int y= 0,
				  int inival = -1);

// inival is the index of the radio_button that is initially set
pulldown_button * make_radio_menu(menu_bar &parent, char *Name,
				  char **blist,  window * action_win = 0,
				  int inival = -1);

// analogous : with help popup
pulldown_button * make_radio_menu(window &parent, char *Name, char **list,
				  char ** help_text, window * action_win = 0,
				  int w= 0, int h= 0, int x= 0, int y= 0,
				  int inival = -1); 

// for callback_buttons : name string, callback function
struct but_cb { char * bname; void (*cb) (); };

// makes pulldown_menu with a list of callback_buttons
// und einem pulldown_button (im parent) der es aktiviert
pulldown_button * make_pulldown_menu(window &parent, char *Name, 
				     int nbuttons, struct but_cb list[],
				     int w= 0, int h= 0 , int x= 0, int y= 0);

// ---------------------- POPUP WINDOWS ------------------------------------

// unmap_button : a special button to Unmap the parent
//                 on BPress, esp. usefull for popup menus
class unmap_button : public button { 
  window * to_unmap;
public: 
  unmap_button(window & parent, char * string,
	       int w, int h, int x, int y) : 
  button(parent,string, w, h, x, y) { to_unmap = &parent;}  
  // 2. constructor, used for unmap_buttons on menu_bars
  unmap_button(menu_bar & parent, char * string, window * unmap) :
    button(parent,string) { to_unmap = unmap;}
private:
  virtual void BPress_1_CB(XButtonEvent) { to_unmap->Unmap(); }
};

// computes for string array: max length of strings && number of strings 
void compute_text_size(char *text[], int &cols, int &lines);

struct f_point { 
  float x, y; 
  f_point() {}
  f_point(float x, float y) : x(x), y(y) {}
};

// define a co-ordinate system in a window 
class coord_window : public pixmap_window {
protected:
  int x0,y0; // window-coordinates of origin
  int w_eff,h_eff; // effective	width and height of coord-sys
  int w_diff,h_diff,rxl,ryd;
  float xl,yd, // WC of left, bottom edge (origin, ia: 0,0)
        xr,yu, // WC of right, top edge (ia: xmax, ymax)
        xf,yf; // transformation factors
public:

  void define_coord(float x1, float y1, float x2, float y2);
  virtual void resize(int, int);

  // compute total window-coordinates from world-values
  int x_window(float x);
  int y_window(float y);
  XPoint p_window(float x, float y); // returns same as XPoint
 
  // back transformation : window coords to world-values
  float x_org(int xw);
  float y_org(int yw);

  // rx : free rand left/right & ry : bottom
  coord_window(window & parent, int w, int h, int x, int y, 
	       int rxl = 5, int rxr = 5, int ryd = 5, int ryu = 5);
 
  // draws line with normed window coordinates
  // ie. origin => (0,0), y grows upwards !!
  void rline(int xp, int yp, int xq, int yq) {
    line(x0 + xp, y0 - yp, x0 + xq, y0 - yq); }
  
  void draw_coords(); // draw coordinate system 
  // draw x-ticks from xl..xo in dx-steps, max n ticks
  void x_ticks(float dx, int n = 1000);
  void y_ticks(float dy, int n = 1000);

  void wline(float x1, float y1, float x2, float y2) {
    line(x_window(x1),y_window(y1),x_window(x2),y_window(y2)); 
  }
  void f_line(f_point p1, f_point p2) { // a line in world coords
    wline(p1.x, p1.y, p2.x, p2.y);
  }
  void rectangle(float x1, float y1, float x2, float y2) {
    wline(x1,y1,x2,y1); wline(x1,y1,x1,y2); 
    wline(x2,y1,x2,y2); wline(x1,y2,x2,y2);
  }

  void graph(int nx, double f[]); 
  void draw_interior() {}

};

// invokes system-call with cmdline-string as argument
class system_button : public button {
char * cmdline;
public: 
  system_button(window &parent,char *Name, char *cmdline, 
	      int w, int h, int x, int y) : 
    button(parent,Name,w,h,x,y), cmdline(cmdline) {}
  virtual void BPress_1_CB(XButtonEvent);
};

// calls 'system("xwd -in wid " + arg)' and produces X-Window-dump of the
// window 'dumpw'; 
// with "arg" the output can be controlled
// eg. "-out -nobdr dump.file" or " | xpr | lpr " as hardcopy 
// with "xwud -in dump.file" or "xpr" to display or print
// eg:  'xwd_button du1(mb,"dump","-nobdrs -out dump.file",pwindow);'
class xwd_button : public button { 
window * dumpw;
char * arg; 
public: 
  xwd_button(window &parent, char *Name, char *arg, window *dumpw,
	      int w, int h, int x, int y) : 
    button(parent,Name,w,h,x,y), dumpw(dumpw), arg(arg) {}
  
  xwd_button(menu_bar &parent,char *Name, char *arg, window *dumpw ) : 
    button(parent,Name), dumpw(dumpw), arg(arg) {}

  void BPress_1_CB(XButtonEvent);
};
 
// **************************************************************
//               SCROLLBARS

// sliders as movable plate for scrollbars
class slider : public plate { 
public: 
  slider(window &parent, int w, int h, int x, int y); 
  virtual void redraw(); // called from Expose_CB
};

// a simple scrollbar without display of the value (should not be used)
// if the 1. constructor (without xanf) is used, the slider must explicitely 
// be set !!

class pure_scrollbar : public plate {
  slider * bar;
protected:
  int sw,sh,sy,xoff,xmax,xspan,xact;
public:
  int nticks; // the number of tick lines (def = 0)
  void set_slider(int x); // fuer Anfang: Setzen des sliders nach x
  void init (); // Setzen der Elemente
  // 1. constructor: without initial value
  pure_scrollbar(window &parent, int w, int h, int x, int y) :
    plate (parent,w,h,x,y,down3d) { init(); }
  // 2. constructor : with initial value 
  pure_scrollbar(window &parent, int w, int h, int x, int y, int xanf) :
    plate (parent,w,h,x,y,down3d) { init(); set_slider(xanf);}

protected:
  // virtual function "callbck" is called from move
  virtual void callbck(int x) { printf(" %d",x); fflush(stdout); } 

  void move(int x); // Bewegen des sliders (x = Pointer) 
  void move_cb(int x); // Bewegen des sliders (x = Pointer) und callbck  

  virtual void redraw();

  // Springen bei Maus-Click button1
  virtual void BPress_1_CB(XButtonEvent ev) { move_cb(ev.x - xoff); }

  // Ziehen, wenn Button1 gedrueckt
  virtual void Motion_CB(XMotionEvent ev) { 
    if (ev.state & Button1Mask) { move_cb(ev.x - xoff); }
  } 
  virtual void resize(int, int);
};

// to display the actual value of a slider
class display_window : public plate { 
char * val; 
public: 
  display_window(window &parent, char *def, int w, int h, int x, int y, 
		 int mode3d) :
    plate(parent,w,h,x,y,mode3d) { val = def; }
  void draw_val() { PlaceText(val); }
  void set_text_mode(int mode); // mode 0 : Loeschen, 1 : Schreiben
  virtual void redraw() { plate::redraw(); draw_val(); }

};

//   ********* class scrollbar ********
// the union "fn_union" is used to access the function pointer 
// for the 2 types of info function, that occur as callbacks
union fn_union {  
  void (*empty)();     // der parameterlose call mode (Form 1)
  void (*vptr)(void*); // der callmode mit void* (Form 2)
  void *value;         // fuer Test auf NULL
};

// 2 constructors : with different callback-forms   
//      1. callback inf() as function without parameters  (old version )
//      2. callback inf(void *), gets the (void*) argument "to_inf" 
//         with this form memberfunctions of other classes can be called 
// "minp, maxp" are the limit values of the scrollbar, which are mapped to
//          [0..xspan] pixels
//          if maxp == 0 (default) the width of the slider is adopted
// "format" serves as transformation value
// "inf"    is an callback, which is called upon each move of the slider 
//          (without arguments), the actual value can be querried from
//          "scrollbar.value" 
// "xstart" is the starting value of the slider

class scrollbar : public pure_scrollbar {
  display_window * disp_win;
  pure_scrollbar * ps;
  char str[80];
  void *to_inform; // pointer for 2nd form
  fn_union inffn;
protected:
  char * format; // the format string for display, like in printf: "Wert = %4x"
  double factor;   // the conversion factor x into pixels = 0..w -> min..max 
public:
  float min, max, value;
private:
  void setval(float x) { // update string and value
    value = x; sprintf(str,format,x); 
  }
  int pwidth(int w) { return (w-60); } // the eff. width of pure_scrollbar
  float pix_to_val(int pix) { return (pix*factor + min); }
  int val_to_pix(float x) { return (irint((x-min)/factor)); }
public:
  void init(window &parent, int w, int h, int x, int y,
	    float minp, float maxp, float xstart);
  // 1. construktor
  scrollbar(window &parent, void (*inf)(), int w, int h, int x, int y, 
	    float minp=0, float maxp=0, float xstart = 0,char *format= "%.0f");
  // 2. construktor
  scrollbar(window &parent, void (*inf)(void*), void * to_inf,
	    int w, int h, int x, int y, 
	    float minp=0, float maxp=0, float xstart=0, char *format= "%.0f");

  // explicite setting of slider and display 
  void change(float x) { move( val_to_pix(x) ); callbck_val(x);} 
protected:
  virtual void callbck(int pix) {  // called from move_cb (mouse events)
    callbck_val( pix_to_val(pix) );
  }
  virtual void callbck_val(float x);    // called from change
  virtual void resize(int, int);
};

// **** class tick_scrollbar ***** : scrollbar with ticks and numbers
// has fixed height (20 + 15 = 35) with ticks and value displays 

#define MAXTCKS 20 // max number of ticks+2 

class tick_scrollbar : public scrollbar {
  text_win *valtxt[MAXTCKS+2]; // display windows for : max 20 ticks !!
  char *strvec; // holds all tick numbers as C-strings 
  char *valstr[MAXTCKS+2]; // pointer to individual tick-string into strvec
  void tickstr();
public:
  tick_scrollbar(window &parent, void (*inf)(void*), void * to_inf,
		 int w, int x, int y, int n_ticks, float minp = 0, 
		 float maxp = 0, float xstart = 0, char *format= "%.0f");
  ~tick_scrollbar();
  void adapt(int maxp, int xstart);
};
  
// container class for interaction between play_scrollbar and 
// a lattice drawer class 
// methods must be overwritten from derived classes,
// a pointer of this class is given to play_scrollbar as argument
class stepper {
public:
  virtual void seek_step(float v) = 0; // seek and perform actual drawing
  virtual Bool switch_play_mode(VVP, void*) = 0; // returns actual mode
};

// **** class play_scrollbar ***
// tick_scrollbar, that adds playback- and stepping buttons left and right  
class play_scrollbar : public tick_scrollbar {
  stepper *stp;
  Bool play_mode;
  // called when setting the slider, or indirectly from step_xxx
  void updated() { stp->seek_step(value); }

  // button callback functions :
  void step_fwd() { change(value+1); } 
  void step_bck() { change(value-1); } 
  void reset() { change(min); }

  void switch_play() {
    play_mode = stp->switch_play_mode((VVP)&play_scrollbar::step_fwd,this);
  }
public:
  play_scrollbar(window &parent, int w, int x, int y, int n_ticks, stepper *stp,
		 int minp = 0, int maxp = 0, int xstart = 0);
};


// class for temporarily set and reset cursor for a window 
// using constructors & destructors
class TempCursor {
 window * win;
 public:
  TempCursor(window *w) {
    win = w;
    XDefineCursor(display, win->Win, watch_cursor);
   }
  ~TempCursor() { XUndefineCursor(display, win->Win); }
};

// ***************************************
// class "edit_window" for editing  of strings, max 200 chars long 
// two virtual fn should be newly defined in derived classes: 
//   enter - action on completion
//   escape - aborting
class edit_window : public plate {
  int cp; // text cursor position in string (0..strlen)
  int xs; // x-pos of string start
protected:
  void mark_cursor();
  virtual void Enter_CB(XCrossingEvent ev);
  virtual void Leave_CB(XCrossingEvent ev);

  void del_char();
  void ins_char(char x);
public:
  char value[200];  
  edit_window(window &parent, char *str, int w,int h, int x, int y);
  virtual void redraw();
  virtual void enter() { printf("%s\n",value); } // to overwrite
  virtual void escape() { memset(value,0,80); cp = 0; } // clears string
  virtual void format_error() { XBell(display,10); }
  void KeyPress_CB(XKeyEvent ev);
};

// class to display a simple analogue clock bound to an external time variable
class clock_win : public pixmap_window {
public:
  int* tptr; // pointer to the time variable (in seconds), 
             // to be updated externally !
  int xc,yc; // centre co-ordinates
  float rh,rm; // rx = radius of minute/hour arrows
  int d; // diameter of the clock
    // *tptr is the timer
  clock_win(window &parent, int *tptr,int w, int h, int x, int y);
  void arrow(double phi, float r);  // display a centered arrow with radius r
  virtual void draw_interior();
  virtual void resize(int w, int h);
  void init();
};

//  ************* twodim_input ***********
// a class used to get a twodim value from a mouse pointer driven slider

class twodim_input;

typedef void (*CBHOOK)(void *,twodim_input *); // the correct type for cbhook 

class twodim_input : public plate {
  plate *bar;
  void *vptr; // the first arg for cbhook -> should be a this-pointer
protected:
  int sw,sh; // slider size
  int sx,sy; // slider offset to borders
public:
  int xact,yact; // actual position (0..*span)
  int xspan,yspan; // span width for movement x,y = [0..zspan]
  // the cbhook should be used to respond on input 
  void (*cbhook)(void*, twodim_input *);

  // swp, shp : width/height of slider; if = 0 : confined in this direction 
  // vptr : passed as 1. argument to cbhook
  twodim_input(window &parent, int w, int h, int x, int y, int swp, int shp,
	       void *vptr = NULL);
protected:
  void set_vars();
  void move(int x, int y);  // called from Events
  virtual void BPress_CB(XButtonEvent ev);
  virtual void Motion_CB(XMotionEvent ev);

public: 
  void set_slider(int x, int y);
  void slider_adapt(int swp,int shp); // called from resize && others 
  void resize(int w, int h);
  void configure(int w, int h, int swp, int shp, int x, int y);
};

// two simple convinience derivations from twodim_input
class horizontal_shifter : public twodim_input {
public:
  horizontal_shifter(window &parent, int w, int h, int x, int y, int sw, 
		     CBHOOK cbh, void *vptr = NULL);
};

class vertical_shifter : public twodim_input {
public:
  vertical_shifter(window &parent, int w, int h, int x, int y, int sh,
		   CBHOOK cbh, void *vptr = NULL); 
};

// scrolled_window has a virtual window which can be shifted with shifters
// arguments are : real width/height, virtual width/height
// (the first window with correct resizing)
// the virtual window must be derived from class virtual_window !!!
class scrolled_window : public window {
  int xp, yp; // actual position
  int hvis,wvis; // visible window size
public:
  // the hook called from shifters : shift drawing_area
  void cbhook(twodim_input *ts); 
  twodim_input *vs, *hs;
  window *clip; // the clipped region
  window *virt_win; // the virtual window
  int hvirt, wvirt; // the virtual size

  scrolled_window(window &parent, int w, int h, int vw, int vh, 
	       int x = 0, int y = 0);
  virtual void resize(int w, int h);
};

// class to enable a convinient linking of virtual window for scrolled_window
// *** an instance must be defined for every scrolled_window *** !!!
class virtual_window  : public window {
public:
  virtual_window(scrolled_window *scr) : 
  window(*(scr->clip), scr->wvirt, scr->hvirt,0,0,0) { // no border
    scr->virt_win = this; 
  }
  virtual void redraw() = 0; // if not defined : useless !!
  virtual void resize(int, int) {} // do nothing !!
};

#endif // WINDOW_H
