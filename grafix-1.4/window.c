// window.c : basic source file for the Grafix package
// the "GNU Public Lincense" policy applies to all programs of this package
// (c) Wolfgang Koehler, wolf@first.gmd.de, Dec. 1994 - Nov. 1995
//     Kiefernring 15
//     14478 Potsdam, Germany
//     0331-863238

#include <assert.h>
#include "window.h"
#include "X11/Xutil.h"
#include "X11/cursorfont.h"

#include "eventnames.h" // from Xlib Manual

void error(char* fmt, ...) {
  va_list args; va_start(args,fmt); 
  fprintf(stderr,"fatal error: "); 
  vfprintf(stderr, fmt, args);
  fprintf(stderr,"\n");
  va_end(args);
  _exit(1); // exit(1) results in seg fault !
}

// linked list with pair (XID, window *)
class xlist {
public:
  unsigned XW; // the XWindow ID = Win member of window
  window *tw;  // the this pointer of associated window
  xlist *next;
  // constructor pushes a new entry in front of the rest list
  xlist(unsigned XW, window *tw, xlist *rest) : XW(XW), tw(tw) { next = rest; }
};

// hash table mechanism used to map XWindow IDs to window pointers
// is needed to get a fast and flexible even handling
// implemented as a vector of xlist* (xltab)
// which is indexed with (WID % TABSIZE)
// each index is a simple linked list that stores the this pointer
// of the associated window 

// class for hash table with xlist entries
// constructor takes tabsize as argument; but size should not matter 
// unless we have really huge numbers of windows (event handling could be slow)
// public methods : xhadd, xhget, xhdel
class xhash {
  xlist **xltab; // the actual hash table
  unsigned tabsize;
  unsigned Wind(unsigned Win) { return (Win % tabsize); }
public:
  xhash(unsigned size) : tabsize(size) {
    xltab = new xlist* [tabsize];
    for (unsigned i = 0; i < tabsize; i++) xltab[i] = NULL;
  }
  ~xhash() { delete[] xltab; }

  // add a new entry in the hash table
  void xhadd(unsigned Win, window *tw) {
    // printf(" new win %x %p\n",Win,tw);
    xltab[Wind(Win)] = new xlist(Win, tw, xltab[Wind(Win)]);
  }
  
  // get the xlist pointer for Win
  xlist *xhget(unsigned Win) { 
    xlist *xptr;
    for (xptr = xltab[Wind(Win)]; xptr; xptr = xptr->next)
      if (xptr->XW == Win) break;
    return xptr; // returns NULL if not found (usually deleted)
  }

  // delete the entry in the hash table : remove the cell by relinking
  void xhdel(unsigned Win) { 
    // printf("delete %x \n",Win);
    xlist *xptr,*xxp = NULL; // the previous xlist pointer (points to -> xptr)
    for (xptr = xltab[Wind(Win)]; xptr; xptr = xptr->next) {
      if (xptr->XW == Win) break; xxp = xptr;
    }
    if (xxp) xxp->next = xptr->next; // relink the list (exclude xptr)
    else xltab[Wind(Win)] = xptr->next; // it was the first entry to remove
    delete (xptr); // the deleted entry 
  }
  // only for tests : read the hash table
  xlist *xhread(int i) { return (xltab[i]); }

};

static xhash *xhatab; // the one and only xhash table for event handling
  
// eg. for menu_bars : make place for picture in the Name-string !
char * ext_string(char * Name) { 
  // printf("%s %d -> ",Name,strlen(Name)); fflush(stdout);
  char * nn = new char[strlen(Name) + 3]; // delete not needed, marginal
  sprintf(nn," %s ",Name); 
  return nn; // _Name_
}

Display * display;
int screen;
GC gc_copy, gc_but_xor, gc_clear, gc_inv, gc_rubber; // some often used gcs
XFontStruct * fixed_fn;       // and fonts
Cursor watch_cursor;
Cursor text_cursor;  // for edit_windows
Cursor hs_cursor, vs_cursor; // special cursors for scrollbars

unsigned depth;
unsigned black, white, Red, Green, Blue, Yellow, Violet;

// convinience functions
GC CreateGC(unsigned long mask, XGCValues * gcv)
{ return XCreateGC(display, DefaultRootWindow(display), mask, gcv); }

// set default gc for line drawing etc.
void set_color(unsigned long color) {
   static XGCValues values; 
   values.foreground = color;
   XChangeGC(display,gc_copy, GCForeground, &values);
}

Colormap def_cmap;

int alloc_color(unsigned red, unsigned green, unsigned blue) {
  // allocation of a cmap-entry for given color -> returns pixel-value
  // -1 failure : no free entry
  XColor col = { 0, red, green, blue };
  if (XAllocColor(display,def_cmap,&col) == 0) {   
    printf(" Warning : Color map full (%x,%x,%x) \n",red,green,blue); 
    return(-1); }
  return col.pixel;
}

int alloc_named_color(char *colname) {
  XColor col;
  if (XAllocNamedColor(display,def_cmap,colname,&col,&col) == 0) 
    error(" Color map full"); 
  return col.pixel;
}

// the GCs for the button-representation
GC button_fg_gc, button_br_gc, button_lw_gc; 
int button_fg_pix, button_br_pix, button_lw_pix; // the button colors

Bool True_Color_Visual; // only for TrueColor -> no XAllocColorCells will work

// init global used screen values
void init_globals(char * DISP) { 
  if (DISP == NULL) DISP = getenv("DISPLAY");
  display = XOpenDisplay(DISP);   
  if (display == NULL) error("cannot open display '%s'",DISP);

  Visual *visual = DefaultVisual(display,screen);
  int cl = visual->c_class;
  True_Color_Visual = (cl == TrueColor);
  // printf("Visual Class = %d  True_Col = %d\n",cl,  True_Color_Visual );
  
  screen = DefaultScreen(display);    
  depth = XDefaultDepth(display,screen);
  def_cmap = DefaultColormap(display,screen);
  black = BlackPixel(display,screen);
  white = WhitePixel(display,screen);
  Red = alloc_color(0xffff,0,0); 
  Green = alloc_color(0,0xffff,0);
  Blue = alloc_color(0,0,0xffff);
  Violet = alloc_color(0xe000,0,0xe000);
  Yellow = alloc_color(0xd000,0xd000,0);
  button_fg_pix = alloc_color(0x6180,0xa290,0xc300); // buttons inner part 
  button_br_pix = alloc_color(0x8a20,0xe380,0xffff); // bright rim
  button_lw_pix = alloc_color(0x30c0,0x5144,0x6180); // dark rim

  XGCValues gcv; gcv.function = GXcopy ;     
  gcv.foreground = black; gcv.background = white; 
  gc_copy = CreateGC(GCFunction | GCForeground , &gcv);
  
  gcv.foreground = button_fg_pix ^ black; 
  gcv.function = GXxor;
  gc_but_xor = CreateGC(GCFunction | GCForeground , &gcv); 
  // for xor-ing with button color (used in edit_window) 

  gcv.function = GXcopy; 
  gc_clear= CreateGC(GCFunction, &gcv);
  // use white as *background* for clear !!!
  // should be changed to custom color !
  XSetForeground(display, gc_clear, white);
  
  gcv.function = GXinvert;
  gc_inv = CreateGC(GCFunction, &gcv);

 //XGCValues={function,plane_mask,foreground,background,linewidth,linestyle,..}
  XGCValues val_rubber = { GXinvert, AllPlanes,0,0,2, LineOnOffDash}; 

  // rubberband for temp. coordinate lines
  gc_rubber= CreateGC(GCFunction | GCPlaneMask | GCLineWidth | GCLineStyle, 
		      &val_rubber); 

  fixed_fn = XLoadQueryFont(display,"fixed");
  XSetFont(display, gc_copy, fixed_fn->fid); // default font 

  XGCValues values;
  values.foreground = button_fg_pix;
  button_fg_gc = CreateGC(GCForeground, &values);
  values.foreground = button_br_pix;
  button_br_gc =  CreateGC(GCForeground, &values);
  values.foreground = button_lw_pix;
  button_lw_gc =  CreateGC(GCForeground, &values);

  watch_cursor = XCreateFontCursor(display,XC_watch);
  text_cursor = XCreateFontCursor(display,XC_xterm);
  hs_cursor = XCreateFontCursor(display, XC_sb_h_double_arrow); 
  vs_cursor = XCreateFontCursor(display, XC_sb_v_double_arrow); 

}

#define TABSIZE 100 // the tabsize is really arbitrary ( > 0)
// if you think the buttons react too slowly you could increase this 
 
window::window(char * DISP) { // constructor for root window
  init_globals(DISP);
  width = DisplayWidth(display,screen);
  height = DisplayHeight(display,screen);
  Win = DefaultRootWindow(display);  
  children = NULL; // own children List = empty
  parentw = NULL; // has no parent
  xhatab = new xhash(TABSIZE);  
  xhatab->xhadd(Win,this); // for safety reasons ; should not be needed
  // printf(" root Window %x %d %d \n",Win, width, height);
};

int debug_create = False; // not defined in window.h (private)

window::window(window & parent, int w, int h, int x, int y, int bw) { 
  width = (w ? w : parent.width) - 2*bw;
  height = (h ? h : parent.height) - 2*bw;
  border_width = bw;
  parentw = &parent;
  hidden = False;  CB_info = False;
  mainw = parent.mainw; // simply pass through
  children = NULL; // own children List = empty

  parent.add_child(this,x,y);
  help = NULL;
  type = SimpleType;
  Win = XCreateSimpleWindow(display, parent.Win, x,y, width, height,
			    border_width,black,white); 
 
  if (debug_create) printf(" Window %lx %d %d \n",Win,width, height);
  
  gc = gc_copy; // default gc
  text_gc = gc_copy; // gc for DrawString, PlaceText; temoprarily changed

  xhatab->xhadd(Win,this); // set hash table entry

  //  default mask, modified by subclasses if needed
  selection_mask =  StructureNotifyMask | ExposureMask ;
};
  
// to update children list (and possibly fit the parent geometry) :
void window::add_child(window *ch, int x, int y) { 
  children = new win_list(ch,x,y,children);
}

void window::remove_child(window *chrem) { // remove from my children list
  win_list *chpp = NULL, *chp;
  for (chp = children; chp; chp = chp->next) {
    if (chrem == chp->child) break; chpp = chp;
  } 
  assert(chp); // loop completed -> child not in parent list, 
  // this should never happen, else list is corrupted
  
  // now re-link the pointers in the children list : 
  if (chpp) chpp->next = chp->next; else children = chp->next;
  delete (chp); // delete old win_list structure
}

window root_window; // **** definition ****

void safe_delete(window *w) { // delete only, if not yet deleted !
  // look children list of root_window -> only main_windows are catched !
  for (win_list *ch = root_window.children; ch; ch = ch->next) 
    if (ch->child == w) { delete (w); return; }
}

Bool debug_delete = False; // debugging printf for destructors

window::~window() {
  if (debug_delete) printf("destr %lx %p\n",Win,this); 

  xhatab->xhdel(Win);  // remove from the hash table

  // correct new version (11/95) :
  // first : search parents child list for my own entry and remove it
  if (this != &root_window) // for root parentw is not defined
    parentw->remove_child(this);

  // now delete my own children :
  // *** attn : this is called recursively ***
  // ie. the list - itself - is destructed from the children 
  // via above mechanism; after this it must be empty !
  for (win_list *ch = children; ch;) { 
    win_list *next = ch->next; // store here because ch is deleted implicitely
    delete (ch->child); 
    ch = next;
  }
  assert(children == NULL); // list must be empty now !!

  if (debug_delete && this == &root_window) {
    for (int i= 0; i < TABSIZE ; i++) printf("%p ",xhatab->xhread(i));
    printf("\n");
  } 
}

void window::Map() { 
  if (hidden) return; 
  draw_interior();  
  XMapWindow(display, Win);
}

/* remarks to ResizeRequest : 
   if the bit in the SelectMask is set the window manager does no further
   process XResizeRequestEvents because they are managed from 
   application window !
*/
void window::Realize() {  
  // printf("Realize %x %d %d\n",Win,width,height);
  XSelectInput(display, Win, selection_mask);
  Map(); // the proper sequence (Select, Map) is essential !!
};

void window::Unmap() { XUnmapWindow(display,Win); };

// to toggle mapping state : if mapped -> Unmap (avv) ; return new map state
Bool window::toggle_map() {
  XWindowAttributes watt;
  XGetWindowAttributes(display,Win,&watt);
  Bool state = (watt.map_state == IsViewable);
  if (state) Unmap(); else Map(); 
  return !state; // it switched
}

// Realize the whole Window tree from this root
void window::RealizeChildren() { 
  Realize();
  for (win_list *cc = children; cc; cc = cc->next) { 
    window *ch = cc->child;
    ch->RealizeChildren();
  }
}

void window::set_backing_store() {
  XSetWindowAttributes attr; attr.backing_store = WhenMapped;
  XChangeWindowAttributes(display,Win,CWBackingStore,&attr);
}

void window::set_save_under() {
  XSetWindowAttributes attr; attr.save_under = TRUE;
  XChangeWindowAttributes(display,Win,CWSaveUnder,&attr);
}

void window::clear() { 
  XFillRectangle(display, Win, gc_clear, 0,0,width,height); 
}

void window::DrawString(int x, int y, char * string) { 
  XDrawString(display,Win,text_gc,x,y, string, strlen(string)); 
}

void window::printw(int x, int y, char* fmt, ...) {
  va_list args; va_start(args,fmt); 
  char str[1000]; 
  vsprintf(str, fmt, args);
  DrawString(x,y,str);
  va_end(args);
}

void window::PlaceText(char * string, int x, int y, XFontStruct * fn) { 
  XSetFont(display, text_gc, fn->fid);
  int tw = XTextWidth(fn, string, strlen(string));   
  int th = fn->ascent; // + fn->descent;
  if (x == 0) x = (eff_width() - tw) / 2;  // horizontal centred
  if (y == 0) y = (height + th) / 2; // vertical centred
  DrawString(x,y,string); 
  XSetFont(display, text_gc, fixed_fn->fid);
}

void window::line(int x0, int y0, int x1, int y1) {
  XDrawLine(display, Win, gc_copy, x0, y0, x1, y1); 
}

void window::DrawPoint(int x, int y) {
  XDrawPoint(display, Win, gc_copy, x, y); 
}

void window::BPress_3_CB(XButtonEvent ev) { 
  if (help) help->do_popup(ev.x_root,ev.y_root + 20);
  //  popup below cursor pos. else use:   help->RealizeChildren();
}

void window::Expose_CB(XExposeEvent ev) { 
  if (CB_info) 
    printf("expose %d %d %d %d %d\n",ev.count,ev.x,ev.y,ev.width,ev.height);
  if (ev.count == 0) redraw();
}

/* definitions from /usr/include/X11/X.h : 
   see -> eventnames.h 
   KeyPress		2   KeyRelease		3
   ButtonPress		4   ButtonRelease	5
   MotionNotify		6   EnterNotify		7
   LeaveNotify		8   FocusIn		9
   FocusOut		10  KeymapNotify	11
   Expose		12  GraphicsExpose	13
   NoExpose		14  VisibilityNotify	15
   CreateNotify		16  DestroyNotify	17
   UnmapNotify		18  MapNotify		19
   MapRequest		20  ReparentNotify	21
   ConfigureNotify	22  ConfigureRequest	23
   GravityNotify	24  ResizeRequest	25
   CirculateNotify	26  CirculateRequest	27
   PropertyNotify	28  SelectionClear	29
   SelectionRequest	30  SelectionNotify	31
   ColormapNotify	32  ClientMessage	33
   MappingNotify	34  LASTEvent		35
*/

// the main callback function
pulldown_button * active_button = NULL;

// global list : used to destroy an pulldown window 
// with the next button release 
// in the CallBack function; not very nice though
window * pulldown_mapped = NULL; 

void window::CallBack(XEvent &event) 
{ if (CB_info) printf("Event %s (%d) in Win %lx\n",event_names[event.type],
		       event.type,event.xany.window);
  if (hidden) return;
  if ((event.type == ButtonRelease))
    { if (pulldown_mapped) // hat ein pulldown menu gemapped -> unmap it
	{ pulldown_mapped->Unmap(); pulldown_mapped = NULL; }  
      
      if (active_button) // restore invoker button
	{ active_button->default_frame(); active_button = NULL;}
    }
  switch (event.type) 
    { 
    case ButtonPress:    
        BPress_CB(event.xbutton); // allgemeiner BPress-handler, zB. buttons   
	switch (event.xbutton.button) {
	  case 1: BPress_1_CB(event.xbutton); break;
	  case 3: BPress_3_CB(event.xbutton); break;
	}
        break;
    case ButtonRelease:
        BRelease_CB(event.xbutton);  break;
    case EnterNotify: 
	Enter_CB(event.xcrossing); break;
    case LeaveNotify:
	Leave_CB(event.xcrossing); break;
    case Expose:
	Expose_CB(event.xexpose); break;
    case KeyPress:
	KeyPress_CB(event.xkey);  break;
    case MotionNotify: 
	Motion_CB(event.xmotion); break;
    case ConfigureNotify:
        Configure_CB(event.xconfigure); break;
    case ClientMessage:
	ClientMsg_CB(event.xclient); break;
    default: break;
    }; 
}

void window::add_help(char * WMName,char * text[]) { 
  int lines, cols;
  compute_text_size(text,cols,lines);
  help = new text_popup(WMName,6*cols + 10, 20*lines + 30,text);
}

void window::WatchCursor() { // set main window to watch
  XDefineCursor(display,mainw->Win,watch_cursor); 
  XFlush(display);
  //  watch_main = mainw;
}

void window::ResetCursor() { // temporarily set, in event-loop reset
  XUndefineCursor(display,mainw->Win);
}

// static float xf_res,yf_res; // resize-factors: either global (in CB) or 
// computed locally inside resize itself

void window::resize(int w, int h) {
  // printf("resize %x: w,h = %d %d (%d %d)\n",Win,w,h,width,height); 
  if (width == w && height == h) return; // evtl. only move Events
  float xf_res = float(w)/width, yf_res = float(h)/height;
  width = w; height = h;  
  XResizeWindow(display,Win,w,h);
  win_list *ch = children; 
  while (ch) { 
    window * cw = ch->child;
    int xn = irint(xf_res * ch->x), yn = irint(yf_res * ch->y);
    ch->x = xn; ch->y = yn;  // new x,y - coordinatec of the child-window
    XMoveWindow(display,cw->Win, xn, yn);
    cw->resize(irint(xf_res * cw->width), irint(yf_res * cw->height)); 
    ch = ch->next; 
  } 
} 

// ##################     main_window class    ###########################
#include "icon.h"

main_window *top_main = 0; // used to handle client messages : 
/* it becomes the first created main_window
   for this window will the close button (from the window manager)
   exit the application. Other main_windows will only be unmapped
   Therefore the main application window should be created first
*/

main_window::main_window(char * WMName, int w, int h,int fix_pos,int x,int y) 
  : window(root_window, w, h, x, y, 0) { // no border
  mainw = this; // its me myself !
  name = WMName; // mainly for debugging used
  polling_mode = False;     // default : use XNextEvent in main_loop

  XStoreName(display, Win, WMName);  // set name in WM-frame

  Cursor main_cursor = XCreateFontCursor(display,XC_left_ptr);
  // printf("1.main %s %d\n", WMName, fix_pos);
  XDefineCursor(display, Win, main_cursor);
  set_icon(icon_bits,icon_width,icon_height);
 
  switch (fix_pos) {       
  case 2: 
    x = (DisplayWidth(display,screen) - w)/2;
    y = (DisplayHeight(display,screen)- h)/2;  // centre window
    XMoveWindow(display,Win, x, y); // XSetWMNormalHints doesnt work here ?!
    // thats why the explicit move is inserted
    // no break;  
  case 1:  // x,y have been given explicitely
    XSizeHints *sz_hints = XAllocSizeHints(); 
    sz_hints->flags = PPosition;
    sz_hints->x = x;
    sz_hints->y = y;
    XSetWMNormalHints(display,Win,sz_hints);
    XFree(sz_hints);      
    break;
  }
  // this protocoll is used from the window manager (mwm) when the 
  // close button is pushed (f.kill). If not set the whole application
  // is killed when any main window is closed !
  Atom p = XInternAtom(display,"WM_DELETE_WINDOW",0); 
  XSetWMProtocols(display,Win,&p,1);

  if (top_main == 0) top_main = this; // the first created should be the app.
}

// up to now only invoked from window manager upon "close"
void main_window::ClientMsg_CB(XClientMessageEvent) {
  if (this == top_main) _exit(0); else Unmap();
}

main_window::~main_window() {
  XDestroyWindow(display,Win); // also destroys the Subwindows 
  if (debug_delete) printf("main_window '%s' -> ", name);
  // window::~window(); 
  // explicite destr. for basis class not needed
}

// only for popup windows : show them at given x,y : old version :
// 1. make it small before moving it, 2. realize, 3. move to x,y-position
// not quite convinient
// this version -> does not work !
void main_window::do_popup(int x , int y)
{ XMoveWindow(display, Win, x,y); // -> does not work here
  RealizeChildren(); 
  // XMoveResizeWindow(display, Win, x, y, width, height); // -> too slow 
}
	
void main_window::Configure_CB(XConfigureEvent ev) {  
  // printf("config %d %d %d \n",ev.type,ev.width,ev.height);
  // xf_res = float(ev.width)/width; yf_res = float(ev.height)/height;
  resize(ev.width, ev.height); 
}

// the event loop :
// from the window-id the this-pointer of the window is computed (table)
// then the "CallBack" function invokes the virtual handler to the 
// corresponding derived class from window

void handle_event(XEvent &event) {
  unsigned wid = event.xany.window;

  xlist *xw = xhatab->xhget(wid);
  if (xw) xw->tw->CallBack(event); 
  // else : the window is deleted, but some events can occur nevertheless 
  //  eg. LeaveNotify-Events (type = 8) 
}

// for event loop in polling_mode
static Bool predicate(Display *, XEvent *, char *) { return TRUE; } 

void main_window::main_loop() {
  exit_flag = False;
  RealizeChildren(); 
  while (1) { 
    XEvent event;   
    if (polling_mode) { // polling : used only for special applications
      if (! XCheckIfEvent(display,&event,predicate,"loop")) {
	polling_handler(); 
	continue;  // no event handling needed
      }
    } else XNextEvent(display, &event);
    handle_event(event);
    // in case of the queue has filled during handling -> flush events 
    while (XCheckMaskEvent(display, KeyPressMask | KeyReleaseMask 
			   | PointerMotionMask,&event));
    if (exit_flag) break; // is set by quit buttons
  }
  Unmap();
  // printf("main_loop exited\n");
}

void main_window::set_icon(char *ibits, int iwidth, int iheight) {
  // printf("set_icon %x %d %d %x %x\n",ibits,iwidth,iheight,display,Win); 
  Pixmap icon_pixmap = XCreateBitmapFromData(display,Win,ibits,iwidth,iheight);
  // printf("set_icon %x \n",icon_pixmap); 
  XWMHints *wm_hints = XAllocWMHints();
  // printf("set_icon %x \n",wm_hints); 
  wm_hints->icon_pixmap = icon_pixmap;
  wm_hints->flags = IconPixmapHint;

  XSetWMHints(display,Win,wm_hints);
  XFree((void *) wm_hints);
}

// ####################      pixmap_window    ############################

pixmap_window::pixmap_window(window & parent,int w,int h,int x,int y,int bw) : 
window(parent, w, h,x, y, bw) {
  pix = XCreatePixmap(display, Win, width, height, depth);
  clear();
}

pixmap_window::~pixmap_window() {
  XFreePixmap(display,pix);
}  

void pixmap_window::clear() { 
  XFillRectangle(display, pix, gc_clear, 0,0, width, height); 
}
 
void pixmap_window::Map() { 
  draw_interior(); 
  XCopyArea(display,pix, Win, gc_copy, 0, 0, width, height, 0, 0);
  XMapWindow(display, Win); 
}

void pixmap_window::DrawString(int x, int y, char * string) {
  XDrawString(display,pix,gc_copy,x,y, string, strlen(string)); }

void pixmap_window::line(int x0, int y0, int x1, int y1) {
  XDrawLine(display, pix, gc_copy, x0, y0, x1, y1); }

void pixmap_window::DrawPoint(int x, int y) {
    XDrawPoint(display, pix, gc_copy, x, y); 
  }

void pixmap_window::Expose_CB(XExposeEvent ev) { 
  // restore the exposed area from pixmap onto screen
  XCopyArea(display, pix, Win,gc_copy,ev.x,ev.y,ev.width,ev.height,ev.x,ev.y);
}

void pixmap_window::resize(int w, int h) {
  // printf("resize %dx%d -> %dx%d\n",width,height,w,h);
  if (width == w && height == h) return; // evtl. nur move Events
  XFreePixmap(display,pix);
  pix = XCreatePixmap(display,Win,w,h,depth);
  window::resize(w,h);
  clear(); // erst hier (braucht width, height !)
  Map();
}

// ****************** 3d-shapes ********  
// draw rectangle in 3d-look with two distinct GCs
// mode = up3d, flat3d (background pix), down3d
void rect3d(Window Win, int mode, short x, short y, short w, short h) {
  int i, thickness = 2;  
  GC left_top,right_bot; // colors for both border parts
  switch (mode) { 
  case up3d:   left_top = button_br_gc; right_bot = button_lw_gc; break;
  case down3d: left_top = button_lw_gc; right_bot = button_br_gc; break;
  case flat3d: 
  default: left_top = button_fg_gc; right_bot = button_fg_gc; break;
  }
  for (i = 0; i < thickness; i++) {
    short xa = x+i, ya = y+i, xe = x+w-i, ye = y+h-i ; 
    XPoint xp[5] = { {xa,ye}, {xa,ya}, {xe,ya}, {xe,ye}, {xa,ye}}; // short x,y
    XDrawLines(display,Win, left_top, xp, 3, CoordModeOrigin);
    XDrawLines(display,Win, right_bot, xp+2, 3, CoordModeOrigin);
  }
}

// analog triangle, points downwards, x,y = left edge
// mode = up3d, flat3d (background pix), down3d
void tri3d_s(Window Win, int mode, short x, short y, short w, short h) {
  int i, thickness = 2;  
  GC left_top,right_bot; 
  switch (mode) { 
  case up3d:   left_top = button_br_gc; right_bot = button_lw_gc; break;
  case down3d: left_top = button_lw_gc; right_bot = button_br_gc; break;
  case flat3d: 
  default: left_top = button_fg_gc; right_bot = button_fg_gc; break;
  }
  for (i = 0; i < thickness; i++) {
    short xa = x+i, ya = y+i, xe = x+w-i, ye = y+h-i, xm = x+w/2; 
    XPoint xp[4] = { {xm,ye}, {xa,ya}, {xe,ya}, {xm,ye}}; 
    XDrawLines(display,Win, left_top, xp, 3, CoordModeOrigin); 
    XDrawLines(display,Win, right_bot, xp+2, 2, CoordModeOrigin);
  }
}

// plates are pseudo-3d-windows
plate::plate(window & parent, int w, int h, int x, int y,int mode3d)  
: window(parent, w, h, x, y, 0), mode3d(mode3d) { 
  selection_mask |= EnterWindowMask | LeaveWindowMask; 
}
void plate::redraw() {
  XFillRectangle(display,Win,button_fg_gc,0,0,width,height); 
  default_frame(); // virtual function !
}

// --------------------------- BUTTONS  -------------------------------
// ##################         button class      ###########################

void button::init_button(window *parent) {  
  in_pulldown = (parent->type == PulldownType);
  selection_mask |= ButtonPressMask | ButtonReleaseMask;
} 
void button::redraw() {
  plate::redraw(); 
  PlaceText(Name); 
}

button::~button() {
  if (debug_delete) printf("button '%s' -> ",Name); 
}

void button::add_help(char **help_text) {
    char * WMName = new char[strlen(Name) + 12]; 
    sprintf(WMName,"Help : '%s'",Name);
    window::add_help(WMName,help_text); }

// ##################      popup_button        #########################
// realize the popup menu when BPress (make it visibel) 
// if pressed again : make it unvisible 

void popup_button::BPress_1_CB(XButtonEvent ev) { 
  XWindowAttributes attr;
  XGetWindowAttributes(display,popup_menu->Win,&attr);
  if (attr.map_state == IsViewable) popup_menu->Unmap(); 
  else popup_menu->do_popup(ev.x_root + 10,ev.y_root + 20); 
}  

// ##################     help_button class    ############################

void help_button::make_popup(char *text[]) {
  // Berechnung der noetigen Windowgroesse fuer popup
 int ln, cols; 
 compute_text_size(text,cols,ln);
 popup_menu = new text_popup("help", 6*cols + 10, ln*15 + 30, text); 
 // leave room for OK button (bottom)!
}

// ##################   function_button class    ############################

function_button::function_button (window & parent, char * Name,  
                                 int w, int h, int x, int y, CB cb,  ...) : 
      button (parent, Name, w, h, x, y), callback(cb) {
    int i; va_list ap; va_start(ap,cb);
    for (i=0;i<10;i++) values[i] = va_arg(ap,void*);
    va_end(ap);
  }

function_button::function_button (menu_bar & parent, char * Name, CB cb, ...) :
  button (parent, Name), callback(cb) { 
    int i; va_list ap; va_start(ap,cb);
    for (i=0;i<10;i++) values[i] = va_arg(ap,void*);
    va_end(ap);
  }

// --------------  switch_button ------------------  
// a button with 2 states of display, which switch on click
// the 2. string should be <= the first (= initial)

void switch_dummy(void *) {} // default dummy fn : does nothing

switch_button::switch_button(menu_bar & parent, char *Name1, char *Name2, 
			     Bool *toggle,
		 VVP inf, void * toinf) : button(parent,Name1), toggle(toggle) 
{ callbck = inf; instptr = toinf; Narr[0] = Name1; Narr[1] = Name2; }

switch_button::switch_button(window & parent, char *Name1, char *Name2, Bool *toggle,
		 VVP inf, void * toinf, int w, int h, int x, int y) : 
		 button(parent,Name1,w,h,x,y) ,toggle(toggle) 
    { callbck = inf; instptr = toinf; Narr[0] = Name1; Narr[1] = Name2; }

void switch_button::switch_it() {
    *toggle = ! *toggle;  // switch
    Name = Narr[*toggle]; // use as index ! (0,1)
    redraw(); // display new string
    (*callbck)(instptr); 
  }
 
// ---------------------------- PULLDOWN --------------------------------

// ##################     pulldown_window class    #######################
// a window child from root_window, not yet visible, not managed from WM,
// position is determined from the button at popup time

// special cursor for pulldown menus 
Cursor pulldown_cursor = XCreateFontCursor(display,XC_right_ptr);

pulldown_window::pulldown_window (int w, int h) : main_window("",w,h) {
  XSetWindowAttributes attr; 
  attr.override_redirect = TRUE; attr.save_under = TRUE;
  attr.cursor = pulldown_cursor; 
  XChangeWindowAttributes(display, Win, 
			  CWOverrideRedirect | CWCursor | CWSaveUnder, 
			  &attr);
  type = PulldownType;
  rold = NULL;
}
  
void pulldown_window::toggle(radio_button *rb) {
  if (rold) rold->status = False;
  // printf("%p %p %p\n",this,rold,rb);
  rold = rb; rb->status = True;
}
// ##################     pulldown_button class    ########################
// map the window (menu) on root when button is activated (BPress)
// the window is mapped to absolute co-ordinates !

pulldown_button::pulldown_button (window & parent, pulldown_window * menu, 
				  char * Name, int w, int h, int x, int y) 
: button(parent, Name, w, h, x, y) { pulldown_menu = menu; xright = 12;}

pulldown_button::pulldown_button(menu_bar & parent, pulldown_window * menu, 
				 char * Name) 
: button(parent, ext_string(Name)) { pulldown_menu = menu; xright = 12; }

pulldown_button::pulldown_button (window & parent, pulldown_window * menu, 
				  char * Name, char ** help_text,
				  int w, int h, int x, int y) 
: button(parent, Name, help_text, w, h, x, y) 
         { pulldown_menu = menu; xright = 12;}
 
void pulldown_button::picture() { 
   int offs = (height-8)/2; 
   tri3d_s(Win,up3d, width-xright,offs,8,8);
}

void pulldown_button::BPress_1_CB(XButtonEvent ev) 
{ 
  unsigned x, y; // *absolut*  position of pulldown menu
  // determine the absolute coord of mouse pointer
  x = ev.x_root - ev.x; // + (width-pulldown_menu->width)/2; horizontal centr.
  y = ev.y_root - ev.y + height + 1;

  XMoveWindow(display, pulldown_menu->Win, x, y);
 
  pulldown_menu->RealizeChildren();
  XRaiseWindow(display,pulldown_menu->Win); 

  XGrabPointer(display,pulldown_menu->Win,TRUE,
	       Button1MotionMask | PointerMotionMask |
	       EnterWindowMask | LeaveWindowMask 
	       , GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
  pulldown_mapped = pulldown_menu; 
  active_button = this;
  // with this trick the next BRelease causes the unmapping of the pulldown
  // and restores the default_frame (this)
  // not to solve with class method since BRelease is invoked for another win
  // should be replaced by push/pop method, to allow multiple popups
} 

// if w == 0 parent must be menu_bar -> use autoplacement
pulldown_button* make_radio_menu(window &parent, char *Name, char **list,
				 window * action_win,
				 int w, int h, int x, int y, int inival) {
  int wm, hm;      // size of pulldown_menu
  int nbuttons;   // number of buttons on pulldown
  char * value;
  // first parse blist to determine size of pulldown menu
  compute_text_size(list,wm,nbuttons);
  wm = MAX(40, wm * 6 + 18);   //  6 = width of char
  hm = nbuttons * 20;       // 20 = height of button
  pulldown_window * pd_wi = new pulldown_window(wm,hm);
  pulldown_button * pd_bt;
  if (w > 0) pd_bt = new pulldown_button(parent,pd_wi,Name,w,h,x,y); else
     pd_bt = new pulldown_button((menu_bar&) parent, pd_wi, Name);
  int yp = 0, ind = 0;  
  while (*list) { 
    value = *list++;
    radio_button *rb =
      new radio_button(*pd_wi,Name,wm,20,0,yp,value, action_win);
    Bool status = (ind++ == inival);
    rb->status = status; // the first button is set True, others False
    if (status) pd_wi->toggle(rb);
    yp += 20;
  };
  return pd_bt;
}

pulldown_button* make_radio_menu(menu_bar &parent, char *Name,
				  char **blist,  window * action_win,
				  int inival) {
  pulldown_button* r_menu = make_radio_menu(parent, Name, blist,
					    action_win, 0, 0, 0, 0, inival );
  return r_menu;
}

// analogous : with help popup
pulldown_button* make_radio_menu(window &parent, char *Name, char **list,
				  char ** help_text, window * action_win,
				  int w, int h, int x, int y, int inival) {
  pulldown_button* r_menu = make_radio_menu(parent, Name, list,
					    action_win, w, h, x, y, inival );
  r_menu->add_help(help_text);
  return r_menu;
}

// list : { {"button1", &callback1}, {"button2", &callback} }
pulldown_button * make_pulldown_menu(window &parent, char *Name, 
				     int nbuttons, struct but_cb list[],
				     int w, int h, int x, int y) {
  int wm = 0, hm;     // size of pulldown_menu
  int i;
  // first parse blist to determine size of pulldown menu
  for (i=0; i < nbuttons; i++) { 
    int ll;
    ll = strlen(list[i].bname); if (ll > wm) wm = ll;
  };
  wm = wm * 6 + 10 ;            //  6 = length of char
  hm = nbuttons * 20;           // 20 = height of button
  pulldown_window * pd = new pulldown_window(wm, hm);
  pulldown_button * pb;
  if (w > 0) pb = new pulldown_button(parent,pd,Name,w,h,x,y); else
     pb = new pulldown_button((menu_bar&) parent, pd, Name); 

  int yp = 0; 
  for (i=0; i < nbuttons; i++) { 
    new callback_button(*pd,list[i].bname,list[i].cb,wm,20,0, yp);
    yp += 20;
  }
  return pb;
}


// ###################### text_popup #######################
text_popup::text_popup(char * WMName, int w, int h, char *text[]) :
main_window (WMName, w, h) { 
  pop_text = text; 
  new unmap_button (*this,"OK",50,20, (width-50)/2,height-23); 
  // a button to close the popup window
}

void text_popup::Expose_CB(XExposeEvent) { // write the text
    int ln = 0, y = 0; 
    while (pop_text[ln]) {   
      y+= 15;
      PlaceText(pop_text[ln++], 4, y);
    }
  }

// String-Arrays: char *x[] = {"xyzuvw","XXXX",...,0} 
// max length of a  string  && number of  strings 
void compute_text_size(char *text[], int &cols, int &lines) {
  int ll = 0;
  lines = 0; cols = 0;
  while (text[lines]) { 
    ll = strlen(text[lines++]);
    if (ll > cols) cols = ll;
    if (ll > 1000 || cols > 1000) error("text-array has no NULL-termination");
  }
}

// ##################### coord_window ##########################
coord_window::coord_window(window & parent, int w, int h, int x, int y, 
	                   int rxl, int rxr, int ryd, int ryu) :
    pixmap_window(parent,w,h,x,y), rxl(rxl), ryd(ryd) { 
    w_diff = rxr + rxl; h_diff = ryd + ryu; // the border minus
}

void coord_window::define_coord(float x1, float y1, float x2, float y2) {
  xl = x1; yd = y1; xr = x2; yu = y2;
  x0 = rxl; y0 = height - ryd; // Window-coordinates of origin
  w_eff = width - w_diff; h_eff = height - h_diff;
  xf = w_eff/(x2 - x1); yf = h_eff/(y2 - y1);
}

void coord_window::resize(int w, int h) {
  // compute parameters for new window size w*h
  x0 = rxl; y0 = h - ryd; // Window-coordinates of origin
  w_eff = w - w_diff; h_eff = h - h_diff;
  xf = w_eff/(xr - xl); yf = h_eff/(yu - yd);
  pixmap_window::resize(w,h);
}
 
// compute total window-coordinates from world-values
int coord_window::x_window(float x) { return x0 + (int)(xf * (x - xl) + .5); }
int coord_window::y_window(float y) { return y0 - (int)(yf * (y - yd) + .5); }

XPoint coord_window::p_window(float x, float y) { // returns same as XPoint
  XPoint temp = { x_window(x), y_window(y) };
  return temp;
}
   
// back transformation : window coords to world-values
float coord_window::x_org(int xw) { return (xw - x0)/xf + xl; }
float coord_window::y_org(int yw) { return (y0 - yw)/yf + yd; }

void coord_window::draw_coords() { wline(0,yd,0,yu); wline(xl,0,xr,0); }

void coord_window::x_ticks(float dx, int n) { 
  float xx; 
  if (xr < xl) dx = -dx; // failsafe
  int y = y_window(0.0);
  for (xx = xl; xx < xr; xx+= dx) { 
    int x = x_window(xx); line(x,y,x,y+2); 
    if (n-- == 0) break;
  } 
}
void coord_window::y_ticks(float dy, int n) { 
  int i; float yy = yd;
  if (yu < yd) dy = -dy;
  int x = x_window(0.0);
  for (i = 0; i < n; i++) { 
    int y = y_window(yy); line(x,y,x-2,y); 
    yy += dy; if (yy > yu) break;
  }
}
 
void coord_window::graph(int nx, double f[]) { 
  int i, x , y, xp = 0, yp = 0; 
  for (i=0; i<nx-1; i++) { 
    y = y_window(f[i]); x = x_window(i); 
    if (i > 0) line(xp, yp ,x, y);
    xp = x; yp = y; 
  }
}

//   ###########  system buttons && xwd_buttons ##########

void system_button::BPress_1_CB(XButtonEvent) { 
  printf("calling system('%s')\n",cmdline);
  system(cmdline);
}

void xwd_button::BPress_1_CB(XButtonEvent) { 
  char cmdline[200]; 
  sprintf(cmdline,"xwd -id 0x%lx %s",dumpw->Win, arg);
  printf("dump : calling system('%s')\n",cmdline);
  system(cmdline);
}

// ############################################################
//              SLIDERS &   SCROLLBARS

slider::slider(window &parent, int w, int h, int x, int y) : 
  plate (parent,w,h,x,y,up3d) { }

void slider::redraw() { // called from Expose_CB, not invoked from move !
  plate::redraw();
  line(width/2,0,width/2,height);
}

void pure_scrollbar::set_slider(int x) {
  bar = new slider(*this,sw,sh,x+2,sy); 
  xact = x;
}

void pure_scrollbar::init () { 
  sw = 19; sh = height-10; sy = 5; // start-values for slider
  xoff = sw/2+2; xmax = width-sw/2-2; xspan = xmax-xoff-1;
  set_backing_store(); // to avoid flickering of scrollbar when drawing slider
  selection_mask |= PointerMotionMask | ButtonPressMask; 
  nticks = 0; // default : no ticks
}

void pure_scrollbar::move(int x) { 
  if (x >= 0 && x <= xspan) { // the real value in intervall [0..xspan]
    XMoveWindow(display,bar->Win,x + xoff - sw/2,sy); 
  }  
}

void pure_scrollbar::move_cb(int x) {
 if (x >= 0 && x <= xspan) { // the real value in intervall [0..xspan]
    XMoveWindow(display,bar->Win,x + xoff - sw/2,sy); 
    callbck(x);
  }  
}

void pure_scrollbar::redraw() {   
  plate::redraw(); 
  line(xoff,height/2,xmax,height/2); // horizontal line
  // line(xoff,4,xoff,height-4); line(xmax,4,xmax,height-4)
  for (int i = 0; i < nticks; i++) { // only drawn if nticks > 0
    int x = xoff + (i+1)*(xmax-xoff)/(nticks+1);
    line(x,4,x,height-4);
  }
}

void pure_scrollbar::resize(int w, int h) {
  plate::resize(w,h);
  sw = bar->width; sy = (height - bar->height)/2;
  xoff = sw/2+2; xmax = width-sw/2-2; xspan = xmax-xoff-1;
} 

void display_window::set_text_mode(int mode) { 
  // mode 0 : clear, 1 : write
  text_gc = (mode) ? gc_copy : button_fg_gc;
} 
 
void scrollbar::init(window &parent, int w, int h, int x, int y, 
                     float minp, float maxp, float xstart) 
{  min = minp; max = (maxp) ? maxp : xspan; 
   factor = ((double) (max - min))/xspan; 
   set_slider( val_to_pix(xstart) );
   setval(xstart); // Anfangs-default
   disp_win = new display_window(parent,str,60,h,x+w-60,y,down3d);
 }

scrollbar::scrollbar(window &parent, void (*inf)(), int w, int h, 
		     int x, int y, float minp, float maxp, float xstart,
		     char *format) :
     pure_scrollbar (parent,pwidth(w),h,x,y), format(format) {
       init(parent,w,h,x,y,minp,maxp,xstart);
       to_inform = NULL;
       inffn.empty = inf;
  }
 
// 2. constructor :
scrollbar::scrollbar(window &parent, void (*inf)(void*), void * to_inf,
		     int w, int h, int x, int y, 
		     float minp, float maxp, float xstart, char *format) :
     pure_scrollbar (parent,pwidth(w),h,x,y), format(format) {
       init(parent,w,h,x,y,minp,maxp,xstart);
       to_inform = to_inf;
       inffn.vptr = inf;
  }
    
void scrollbar::callbck_val(float x) {   
  // quick clearing of the old display : overwrite with  background gc
  disp_win->set_text_mode(0); disp_win->draw_val();
  setval(x); // compute new display
  disp_win->set_text_mode(1); disp_win->draw_val();
  // reset default gc !!! , new value
  if (inffn.value) 
    { if (to_inform) (*inffn.vptr)(to_inform); else (*inffn.empty)(); }
}

void scrollbar::resize(int w, int h) {
  pure_scrollbar::resize(w,h);  
  factor = ((double) (max - min))/xspan; 
}

// **** class tick_scrollbar ***** : scrollbar with ticks and numbers
// has fixed height (20 + 15 = 35) with ticks and value displays 
void tick_scrollbar::tickstr() { // compute tick strings
  char *stptr = strvec;
  for (int i=0; i < nticks+2; i++) {
    sprintf(stptr,format,i*(max-min)/(nticks+1)+min); valstr[i]= stptr;
    stptr += strlen(stptr)+1;
  }
}

tick_scrollbar::tick_scrollbar(window &parent, void (*inf)(void*), 
			       void *to_inf,
			       int w, int x, int y, int n_ticks, 
			       float minp, float maxp, float xstart, 
			       char *format) :
  scrollbar(parent, inf, to_inf, w, 20, x, y, minp, maxp, xstart, format) {   
    nticks = n_ticks;
    if (nticks > MAXTCKS) 
      error("too many ticks for scrollbar %d (max %d)", nticks,MAXTCKS);
    strvec = new char[(nticks+2)*12]; // room should suffice
    tickstr();
    for (int i = 0; i < nticks+2; i++) { 
      // windows to display numbers below ticks
      // int nl = strlen(valstr[i]);
      int xp = xoff + i*xspan/(nticks+1) ;
      valtxt[i] = new text_win(parent, valstr[i], 30, 15, x + xp - 15, y+20);
    }
  }

tick_scrollbar::~tick_scrollbar() { delete strvec; }

void tick_scrollbar::adapt(int maxp, int xstart) {
  max = maxp; factor = ((double) max - min)/xspan; 
  change(xstart); // set slider to xstart and displays value
  tickstr();
  for (int i = 0; i < nticks+2; i++) { // displays numbers below ticks
    valtxt[i]->clear(); valtxt[i]->redraw();
  }
}

// **** class play_scrollbar ***
// a tick_scrollbar, that adds playback- and stepping buttons left and right  
play_scrollbar::play_scrollbar(window &parent, int w, int x, int y, 
			       int n_ticks, stepper *stp, int minp, int maxp, 
			       int xstart) 
: stp(stp),
  tick_scrollbar(parent, (VVP) &play_scrollbar::updated, this, w-2*(15+20), 
		 x+50, y, n_ticks, minp, maxp, xstart) {   
    new instance_button <play_scrollbar> // left
      (parent,"<", &play_scrollbar::step_bck, this, 15,20, x, y); 
    
    new instance_button <play_scrollbar> // left +15
      (parent,">", &play_scrollbar::step_fwd, this, 15, 20, x+15, y); 
    
    new switch_button(parent,">>","||",&play_mode, // left + 30
		      (VVP) &play_scrollbar::switch_play,this, 20, 20, x+30,y);
    new instance_button <play_scrollbar>  // right - 20
      (parent,"<<", &play_scrollbar::reset, this, 20, 20, x+w-20, y);
  }

// ***************************************
// class "edit_window" for editing of strings, max 80 chars 
edit_window::edit_window(window &parent, char *str, int w,int h, int x, int y)
: plate(parent,w,h,x,y,down3d) { 
  strncpy(value,str,200);
  XDefineCursor(display,Win,text_cursor);
  cp = strlen(value); // set behind the string
  selection_mask |= KeyPressMask;
}

void edit_window::mark_cursor() { 
  XFillRectangle(display, Win, gc_but_xor, xs + 6*cp, 2, 6, 15);
}
void edit_window::Enter_CB(XCrossingEvent) { // frame3d(flat3d); 
  mark_cursor(); 
}
void edit_window::Leave_CB(XCrossingEvent) { // default_frame(); 
  mark_cursor(); 
}

void edit_window::redraw() {
  plate::redraw(); 
  xs = width - 6*strlen(value) - 10; 
  DrawString(xs,16,value);
}
  
void edit_window::del_char() { // clear chars left from cursor
  for (unsigned i = cp; i <= strlen(value); i++) value[i-1] = value[i];
  cp--;
}

void edit_window::ins_char(char x) { // insertion left
  for (int i = strlen(value); i >= cp; i--) value[i] = value[i-1];
  value[cp] = x; cp++;
}

void edit_window::KeyPress_CB(XKeyEvent ev) {
  mark_cursor();
  KeySym keysym = XLookupKeysym(&ev,ev.state & 1);
  switch(keysym) {
  case XK_Left: while (cp <= 0) ins_char(0x20); // fill with space
    cp -= 1; break;
  case XK_Right: if (cp < (int) strlen(value)) cp += 1; break;
  case XK_BackSpace: if (cp > 0) del_char(); break;
  case XK_Delete: if (cp > 0) del_char(); break;
  case XK_Escape: escape(); break;
  case XK_Return: enter(); break;               // Return = Enter
  default: 
    if ((keysym >= 0x20) && (keysym<= 0x7e)) // all ASCII-chars
      ins_char(keysym);
  }
  redraw();
  mark_cursor(); 
}

// class clock_win : public pixmap_window 
// class to display a simple analogue clock bound to an external time variable

clock_win::clock_win(window &parent, int *tptr,int w, int h, int x, int y) :
  pixmap_window(parent,w,h,x,y,0), tptr(tptr) { init(); }

// display a centered arrow with radius r
void clock_win::arrow(double phi, float r) {
  double c, s; 
  s = sin(phi); c = cos(phi);
  line(xc,yc, irint(xc + r*s), irint(yc - r*c));
}

void clock_win::draw_interior()  {
  int r = d/2;
  XDrawArc(display,pix,gc_copy,xc-r,yc-r,d,d,0,360*64);
  arrow(*tptr/3600.*2.0*3.1416, rm); // minutes arrow
  arrow(*tptr/86400.*4.0*3.1416, rh); // hours arrow
}
  
void clock_win::init() {
  xc = width/2; yc = height/2; 
  d = MIN(width, height) - 2; // minimum of width and height
  rh = d/3 - 2; rm = d/2 - 4;
  // printf(" %d %d %d %d %d\n",width,height,xc,yc,d);
}

void clock_win::resize(int w, int h) {
  pixmap_window::resize(w,h);
  init();
  redraw();
}

//  ************* twodim_input ***********
// a class used to get a twodim value from a mouse pointer driven slider

twodim_input::twodim_input(window &parent, int w, int h, int x, int y, 
			   int swp, int shp, void *vptr)
: plate (parent,w,h,x,y,down3d), vptr(vptr), sw(swp), sh(shp) { 
  set_backing_store(); 
  selection_mask |= PointerMotionMask | ButtonPressMask; 
  set_vars();  
  bar = new plate(*this,sw,sh,sx,sy,up3d);
  cbhook = NULL;
}  

void twodim_input::set_vars() {
  sx = 2; sy = 2; 
  yact = sy; xact = sx; 
  if (sh == 0) sh = height - 2*sy; // horiz. shifter : yspan = 0
  if (sw == 0) sw = width  - 2*sx; // vert . shifter : xspan = 0
  yspan = height - 2*sy - sh; 
  xspan = width - 2*sx - sw;
}

void twodim_input::move(int x, int y) { // called from Events
  set_slider(x - sx - sw/2,y - sy - sh/2);
}
void twodim_input::BPress_CB(XButtonEvent ev)  {
  switch (ev.button) {
  case 1: case 2: // all the same
  case 3: move(ev.x,ev.y); break;  
  }
}

void twodim_input::Motion_CB(XMotionEvent ev)  { 
  if (ev.state & (Button1Mask | Button2Mask | Button3Mask)) { 
    move(ev.x,ev.y); }
}

void twodim_input::set_slider(int x, int y)  {  // set slider y = 0..zspan
  // printf(" %d %d\n",x,y);
  y = MAX(0, MIN(yspan,y)); 
  x = MAX(0, MIN(xspan,x));     
  if (x != xact || y != yact) {
    xact = x; yact = y;
    XMoveWindow(display,bar->Win,x + sx ,y + sy); 
    if (cbhook) cbhook(vptr,this);
  }
}

// called from resize && others 
void twodim_input::slider_adapt(int swp,int shp) { // adaptate to new slider size
  sw = swp; sh = shp; 
  set_vars(); 
  bar->resize(sw,sh); set_slider(0,0);
} 


void twodim_input::resize(int w, int h)  {
  //    if (width == w && height == h) return; // only once
  width = w; height = h; 
  XResizeWindow(display,Win,width,height); 
  slider_adapt(sw,sh);
}

void twodim_input::configure(int w, int h, int swp, int shp, int x, int y)  { 
  resize(w,h);
  slider_adapt(swp,shp);
  XMoveWindow(display,Win,x,y);
  RealizeChildren(); // in case it was unmaped before
}


// scrolled_window has a virtual window which can be shifted with shifters
// arguments are : real width/height, virtual width/height
// (the first window with correct resizing)
// the virtual window must be derived from class virtual_window !!!

scrolled_window::scrolled_window(window &parent, int w, int h, int vw, int vh, 
				 int x, int y) 
: window(parent,w,h,x,y,0) {
  xp = yp = 0;
  hvirt = vh; wvirt = vw;
  hs = vs = 0; clip = 0;
  resize(w,h);
  virt_win =  0; // must be set later !!!
} 

void scrolled_window::cbhook(twodim_input *ts) {
  if (virt_win == 0) error("no virtual window bound");
  int x = ts->xact, y = ts->yact, xs = ts->xspan, ys = ts->yspan;
  // printf("hook %d %d\n",ts->xact,ts->yact);
  // transform slider-pos to shift values
  if (xs != 0) xp = -x*(wvirt - wvis)/xs; 
  // else vert.shifter: use old xp
  if (ys != 0) yp = -y*(hvirt - hvis)/ys;
  XMoveWindow(display,virt_win->Win,xp,yp);
}

static void sw_cbhook_wrapper(void *instance, twodim_input *tdi) {
  ((scrolled_window *)instance)->cbhook(tdi);
}

void scrolled_window::resize(int w, int h) {
  // if (w == width && h == height) return;
  width = w; height = h;
  // printf("resize scrolled window %d %d %d %d\n",w,h,wvirt,hvirt);
  XResizeWindow(display,Win,w,h);
  int sz = 15; // shifter dim. (heigth or width)
  Bool horz_scr = (wvirt > w);
  if (horz_scr) h -= sz; // left room for horz_shifter (bottom) 
  Bool vert_scr = (hvirt > h); 
  if (vert_scr) w -= sz; // left room for shifter (right) 
  wvis = w; hvis = h;    // that is the region which is actually visible

  if (vert_scr) {
    int sh = (h-4) * h/hvirt; if (sh < 4) sh = 4; // else too small
    if (vs == 0) // create new 
      vs = new vertical_shifter(*this,sz,h,w,0,sh, &sw_cbhook_wrapper, this);
    else vs->configure(sz,h,0,sh,w,0); 
  } else if (vs) XUnmapWindow(display,vs->Win);
  if (horz_scr) {
    int sw = (w-4) * w/wvirt; if (sw < 4) sw = 4; // else too small
    if (hs == 0) // create new 
      hs = new horizontal_shifter(*this,w,sz,0,h,sw, &sw_cbhook_wrapper,this);
    else hs->configure(w,sz,sw,0,0,h); 
  } else if (hs) XUnmapWindow(display,hs->Win);
  
  if (clip == 0) clip = new window(*this,wvis, hvis, 0, 0); // clip window
  else XResizeWindow(display,clip->Win,wvis,hvis);
  // printf("%d %d %d %d %d %d\n",wvirt,hvirt,wvis,hvis,w,h);
}

horizontal_shifter::horizontal_shifter(window &parent, int w, int h, int x, 
				       int y, int sw, CBHOOK cbh, void *vptr) 
: twodim_input(parent,w,h,x,y,sw,0,vptr) { 
  cbhook = cbh; 
  XDefineCursor(display,Win,hs_cursor); 
}

vertical_shifter::vertical_shifter(window &parent, int w, int h, int x, int y, 
				   int sh, CBHOOK cbh, void *vptr) 
: twodim_input(parent,w,h,x,y,0,sh,vptr) { 
  cbhook = cbh; 
  XDefineCursor(display,Win,vs_cursor); 
}
