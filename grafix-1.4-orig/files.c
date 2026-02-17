// files.c : interactively opening files
//           wolf 12/94
#include "window.h"
#include "files.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// popup window with two buttons, special loop handler,
// can be used as stack object    
class confirm_box : public main_window {
char **text;
int status;
public:  
  confirm_box(char *Name, int w, int h, char **text ) :
    main_window(Name,w,h,2), text(text) { // 2 means centre window !
    status = CONFIRM_UNDEF;
    int y = h - 25, x0 = 10, x1 = w/2 + 10, wb = w/2 - 20; 
  
    new variable_button <int> (*this,"Yes",&status, CONFIRM_YES,wb,20,x0,y);
    new variable_button <int> (*this,"No",&status, CONFIRM_NO, wb, 20,x1,y);
    
  }
  void Expose_CB(XExposeEvent /* ev */) {
    int ln = 0, y = 10;
    while (text[ln]) { y+= 15; PlaceText(text[ln++],0,y); }
  }  
  
  int local_loop() {
    RealizeChildren(); 
    do { 
      XEvent event;   
      XNextEvent(display, &event);
      // only handle events for myself and my children windows
      // events for others are flushed !
      unsigned wid = event.xany.window; 
      if ( wid == Win ) { handle_event(event); continue; }
      win_list *ch = children;
      while (ch) {
        if (wid == ch->child->Win) { handle_event(event); break; }
	ch = ch->next;
      }
    } while (status == CONFIRM_UNDEF);
    Unmap();
    return(status);
  }
};

int yes_no_box(char *title, char **text, int w, int h) {
  confirm_box cbox(title,w,h,text); 
  int stat = cbox.local_loop();
  return stat;
}

// for opening output files (type = "w") with overwrite test
// if it already exists opens confirmation box "yes" , "no" 
// in case "no" returns NULL, else FILE-pointer to opened file
FILE *fopen_test(char *fina) {
  FILE *ff = fopen(fina,"r");
  if (ff) { // file exists
    fclose(ff);
    char *text[] = {"File exists :",fina,"","Overwrite it ?",0};
    int stat = yes_no_box("Existing File",text,150,120);
    if (stat == CONFIRM_NO) return (NULL);
  }
  return fopen(fina,"w");
}

class file_window;

// class for entering filenames
class fina_edit : public edit_window {
  void (file_window::*meth)();
  file_window *inst;
public: 
  fina_edit(window &parent, char *defname, void (file_window::*meth)(), 
            file_window *inst, int w, int h, int x, int y) : 
    edit_window(parent,defname,w,h,x,y), meth(meth), inst(inst) {}
  virtual void enter() {
    (inst->*meth)(); // also calls ok-method like the button
   // else format_error();
  } 
};

// opens a popup window to enter a filename (with defname)
// ok-button tries to open it, depending on mode
// MODE_WRITE : if file already exists then popup a confirmation box 
// MODE_READ : no more used -> replaced by file_selection_box
class file_window : public main_window {
public: 
  FILE *fptr;
  char *fname;
  int mode;
private:
  fina_edit *name_ed;
  void ok() { 
    switch (mode) {
    case MODE_WRITE: fptr = fopen_test(fname); break;
    case MODE_READ: fptr = fopen(fname,"r"); break;
    case MODE_OVWRT: fptr = fopen(fname,"w"); break;
    }
    if (fptr) exit_flag = True; else printf("error opening file %s\n",fname);
  }

public:
  file_window() : 
    main_window("open file",200,100) {
    name_ed = new fina_edit(*this,"",&file_window::ok,this,180,20,10,10);
    fname = name_ed->value; 
    new instance_button <file_window> 
      (*this,"Ok",&file_window::ok,this,60,20,20,50);     
    // simply exits main_loop since sets exit_flag
    new variable_button <int> (*this,"Abort",&exit_flag,True,60,20,120,50);
  }  
 // calls main_loop, returns name and opened file
 FILE* file_input_loop(char *defname, int amode, char *name_ret) {
    fptr = NULL; // initial value -> for Abort
    mode = amode; strcpy(name_ed->value,defname);
    main_loop();
    strcpy(name_ret,fname);
    return fptr;
  }
};

// class file_display : public plate { // plate for string display
void file_display::redraw() { 
  plate::redraw(); 
  // if string too long cut it *lefthand* !! charwidth = 6 !!
  int offset =  (int(strlen(val)) - (width-6)/6 ) >? 0; 
  PlaceText(val+offset); 
}

void file_display::set_val(char *x) { val = x; redraw(); }

// class for item in selector :
// 2 callbacks : enter_cb on enter, and act_cb on bpress
class select_item : public plate {
  VPCP ecb, acb;
  void *wptr;

public: 
  char * value;
  select_item(window &parent, VPCP enter_cb, VPCP act_cb, void *wp,
	      int w, int h, int x, int y) :
    plate(parent,w,h,x,y,flat3d) { 
    static char empty = 0; value = &empty;
    ecb = enter_cb; acb = act_cb, wptr = wp;
    selection_mask |= ButtonPressMask | ButtonReleaseMask;
  }

  virtual void redraw() { 
    if (!hidden) { 
      plate::redraw(); 
      if (value) PlaceText(value); else error("select_item == 0"); 
    } 
  }
  virtual void Enter_CB(XCrossingEvent) {  frame3d(up3d); ecb(wptr,value); }
  
  virtual void Leave_CB(XCrossingEvent) { default_frame(); }
  virtual void BPress_CB(XButtonEvent) { frame3d(down3d); acb(wptr,value); }
 
  virtual void BRelease_CB(XButtonEvent) { default_frame(); }
  void enable(char *x) { value = x; hidden = False; redraw(); }
  void disable() { hidden = True; }
};
//  ************* vert_scrollbar ***********
//  vertical scrollbar with re-definable behaviour :
//  defines virtual functions for move (button 2) and jmp (button 1 &3)
//  

vert_scrollbar::vert_scrollbar(window &parent, int w, int h, int x, int y, 
			       int sh) :
plate (parent,w,h,x,y,down3d), sh(sh) { 
  set_backing_store(); 
  selection_mask |= PointerMotionMask | ButtonPressMask; 
  set_vars();
  bar = new plate(*this,sw,sh,sx,sy,up3d);
  XDefineCursor(display,Win,vs_cursor); 
}  

void vert_scrollbar::set_vars() {
  sx = 3; sw = width - 2*sx; sy = 2; yact = sy;  
  yspan = height - 2*sy - sh; 
}

// jump on mouse click
void vert_scrollbar::BPress_CB(XButtonEvent ev) {
  switch (ev.button) {
  case 1: jmp_callback(False); break;
  case 2: move_callback(ev.y); break;  
  case 3: jmp_callback(True); break;
  }
}
// draw if button 2 pressed
void vert_scrollbar::Motion_CB(XMotionEvent ev) { 
  if (ev.state & Button2Mask) { move_callback(ev.y); }
}

void vert_scrollbar::set_slider(int y) {  // set slider y = 0..yspan
  y = MAX(0, MIN(yspan,y)); 
  yact = y;
  XMoveWindow(display,bar->Win,sx,y + sy); 
}

// called from resize && others 
void vert_scrollbar::slider_adapt(int shp) { // adaptate to new slider height
  sh = shp; 
  set_vars(); 
  bar->resize(sw,sh); set_slider(0);
}

void vert_scrollbar::resize(int w, int h) {
  //    if (width == w && height == h) return; // only once
  width = w; height = h; 
  XResizeWindow(display,Win,width,height); 
  slider_adapt(sh);
}

void vert_scrollbar::adapt(int h, int shp) { 
  // adapt scrollbar to new height & slider height
  height = h; XResizeWindow(display,Win,width,height); 
  slider_adapt(shp);
}

class selector;

// select_scrollbar : specialized vert_scrollbar for selector windows
class select_scrollbar : public vert_scrollbar {
  selector *sel;
public:
  select_scrollbar(window &parent, int w, int h, int x, int y, int sh,
		   selector *sel) :
    vert_scrollbar(parent, w, h, x, y, sh), sel(sel) { }  
  virtual void move_callback(int y); // called from draw (Button 2)
  virtual void jmp_callback(Bool up); // called from pg (Button 1 & 3)
};

// makes a plate with (max) nits select_items -> selection box
// not all are neseccarily drawn
// this should be completed with a scrollbar
// class selector : public plate {
selector::selector(window &parent, int nits, VPCP show_cb, VPCP act_cb,
		   int w, int h, int x, int y, int hi) 
: plate (parent, w - 15,h,x,y,down3d) {
  selection_mask |= KeyPressMask; // catch key events
  itdsp = nits; 
  interior = new window(*this,width-6,height-6,3,3,0); // 3 pixels boundary
  vs = new select_scrollbar(parent,15,h,x+w-15,y,100,this);
  int yp = 0; int i;
  pits = new select_item* [nits];
  for (i=0; i < nits; i++) {
    pits[i] = new select_item(*interior,show_cb,act_cb,&parent,width-6,hi,
			      0,yp);
    yp += hi;
  }
}
  
void selector::set_items(char **items, int nits) { 
  int i; itptr = items; itot = nits; ishift = 0;
  for (i=0; i < itdsp; i++) { 
    if (i < nits) pits[i]->enable(items[i]); else pits[i]->disable();
  }
}
  
//  ~selector() { delete[] pits; }

void selector::redraw() { 
  if (itot > 0) plate::redraw(); 
  else { // makes no sense to see empty windows
    XUnmapWindow(display,Win); XUnmapWindow(display,vs->Win); 
  }
}
void selector::resize(int w, int h) { 
  if (width == w && height == h) return; // only once
  height = h; // only height can be changed
  XResizeWindow(display,Win,width,h);
  int sh;  
  if (itot > 0) { // at least 1 item 
    XResizeWindow(display,interior->Win,width-6,h-6); 
    RealizeChildren();  
    // printf(" %d %d %d\n",itdsp,itot,h);
    sh = h-4; // max slider height
    if (itdsp < itot) sh = (sh*itdsp)/itot; // more than displayable
    vs->adapt(h,sh); // adapt scrollbar
    vs->RealizeChildren(); 
  }
}

void selector::shift(int ip) { // set ishift to ip, set slider, redraw
  ip = ip >? 0; ip = ip <? ispan(); // limit to range [0..ispan]
  if (ip != ishift) {
    ishift = ip;
    vs->set_slider_rel(ishift/float(itot-itdsp)); 
    int i;
    for (i=0;i < itdsp; i++) pits[i]->enable(itptr[i+ishift]);
  } 
}

void selector::shift_rel(int di) {   // shift rel to current pos
  int ish = ishift + di; 
  ish = MAX(0, MIN(ish, itot-itdsp));
  shift(ish);
}

// intervall for ishift
//  int ispan() { return (itot - itdsp); } 

void selector::KeyPress_CB(XKeyEvent ev) {
  KeySym ks = XLookupKeysym(&ev,ev.state);
  switch (ks) { 
  case XK_Up: 	shift_rel(-1); break;	   
  case XK_Down: 	shift_rel(1); break;	 
  case XK_Prior:	shift_rel(-itdsp); break;	 
  case XK_Next: 	shift_rel(itdsp); break;
  }
}

// methods for class select_scrollbar 
void select_scrollbar::move_callback(int y) { // called from draw (Button 2)
  int ispan = sel->ispan();
  int ip = (yspan > 0) ? (irint((y * ispan) / float(yspan))) : 0;
  sel->shift(ip);
}

void select_scrollbar::jmp_callback(Bool up) { // called from pg (Button 1 & 3)
  sel->shift_rel( ((up) ? -1 : 1) * sel->itdsp);
} 

// recursively parses filename against mask, recons only *, but yet no ?
// see also "test-compare.c"
int filter(const char *mask, const char *fina) {
  // printf("%s %s\n",mask,fina);
  if (*mask == 0) return (*fina == 0); else
    if (*mask == '*') { 
      int i; 
      // compare all tail-strings of fina with mask part following "*"
      for (i = strlen(fina); i >= 0; i--) // end 0 is included !
        if (filter(mask+1,fina+i)) return (1);
      return (0); // no match found
    } else 
      if (*mask == *fina) return (filter(mask+1,fina+1)); else return (0);
}

#define ITMAX 200  // max # of items in each selector
#define BHEIGHT 18 // button height
#define ITDISP 20  // max of displayed items in selectors

// compare function for qsort
int qcomp(char **e1, char **e2) { 
  return (strcmp(*e1,*e2));  //  compare strings pointed to, not the pointers
}

typedef int (*IVPVP)(const void *,const void *); // the correct type for qsort

// interactively open files for reading (scanning directory)
// after completion fptr is either the opened FILE* (Ok) or NULL (Abort)
// functions: 
//   selection mask = edit_window (*.c), enter-key (Mask)
//   selectors - BPress (Ok), Enter (display)
//   buttons : Ok, Mask, Abort  
class file_selection_box : public main_window {
public: 
  FILE *fptr;
  char *fname;
  char strings[10000]; // all names in one field
  char *fitems[ITMAX],*ditems[ITMAX]; // max 200 entries into this field
  char pwd[100]; // current directory path
  int fits, dits;
private:
  edit_window *mask_ed;
  file_display *fdsp;
  selector *fsel,*dsel;
  button *okb, *abortb, *chb;

  // searches pwd for matching files and displays them in fsel
  void make_select(char *mask) {
    DIR *dirp = opendir(".");
    struct dirent *dp; 
    fits = dits = 0; int nstrp = 0; 
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
      struct stat st; 
      stat(dp->d_name,&st);
      Bool regfi = (st.st_mode & S_IFREG), dirfi = (st.st_mode & S_IFDIR);
      if ((dirfi && (dits < ITMAX)) || 
          (regfi && filter(mask,dp->d_name) && (fits < ITMAX))) {	 
	int nl = strlen(dp->d_name) + 1; 
	if ((nl + nstrp) >= 4000) error("too long name strings");
        char *strent = strings + nstrp; // entry point in strings
	strcpy(strent,dp->d_name); 
      
        if (regfi) fitems[fits++] = strent; else ditems[dits++] = strent;
        nstrp += nl;
      }
    }   
    qsort(fitems,fits,4,(IVPVP) qcomp); // sorting the pointers 
    qsort(ditems,dits,4,(IVPVP) qcomp); // not the strings !
    fsel->set_items(fitems, fits);
    dsel->set_items(ditems, dits);
    int ffits = MIN(fits,ITDISP), ddits =  MIN(dits,ITDISP);   
    int wh = MAX(ffits, ddits) * BHEIGHT + 6; 
    XResizeWindow(display,Win,220,wh+110);
    dsel->resize(80,ddits*BHEIGHT + 6);
    fsel->resize(110,ffits*BHEIGHT + 6); 

    // printf("window %d %d\n",Win,wh+110);
  }
   
  void ok() { 
    fptr = fopen(fname,"r");
    if (fptr) exit_flag = True; else printf("error opening file %s\n",fname);
  }

  void fsel_cb(char *val) { fname = val; fdsp->set_val(val);  }
  // callback for dsel enter 
  void dsel_cb(char *val) { 
    //  getcwd(pwd,100); 
    int lpwd = strlen(pwd);
    sprintf(pwd+lpwd,"/%s",val); // append val to pwd
    fdsp->set_val(pwd);  
    pwd[lpwd] = 0; // reset pwd
  }

  void chmask() { // callback for change mask
    make_select(mask_ed->value);
  }
  void cd_cb(char *val) { 
    chdir(val); 
    getcwd(pwd,100); fdsp->set_val(pwd);  
    make_select(mask_ed->value); 
 }
public:
  file_selection_box() : 
    main_window("file selection box",220,250) {
    
    char empty = 0; fname = &empty; 
    fdsp = new file_display(*this,200,20,10,10);
    new text_win(*this,"selection mask",200,20,10,30);
    mask_ed = new mask_edit(*this, (VPCP) &make_select, this,200,20,10,50);
    // directory selector 
    dsel = new selector(*this, ITDISP, (VPCP) &dsel_cb, (VPCP) &cd_cb,
		       80,0,10,80,BHEIGHT);
    // file selector
    fsel = new selector(*this, ITDISP, (VPCP) &fsel_cb, (VPCP) &ok,
		       110,100,100,80,BHEIGHT);
    okb = new instance_button <file_selection_box> 
                 (*this,"Ok",&file_selection_box::ok,this,50,20,0,0);
    chb = new instance_button <file_selection_box> 
                 (*this,"Mask",&file_selection_box::chmask,this,50,20,0,0);
    abortb =
       new variable_button <int> (*this,"Abort",&exit_flag,True,50,20,0,0);

  }   
 
  void resize(int w, int h) { 
    if (width == w && height == h) return; // only once
    // printf("resize %x ->  %d %d\n",Win,w,h); 
    width = w; height = h;
    XMoveWindow(display,okb->Win,10,height-25);  
    XMoveWindow(display,chb->Win,75,height-25);
    XMoveWindow(display,abortb->Win,140,height-25);
  } 
  ~file_selection_box() { }

  // calls main_loop, returns selected filename and opened file
  FILE* file_input_loop(char *defname, char *name_ret) {
    fptr = NULL; // initial value -> for Abort
    strcpy(mask_ed->value, defname);
    getcwd(pwd,100); fdsp->val = pwd; 
    fname = NULL;
    make_select(mask_ed->value);
    main_loop();
    if (fptr) strcpy(name_ret,fname);
    return fptr;
  }
};

// simple interface function
// creates popup window and calls its main_loop 
// the opened FILE* is returned and its filename (with strcpy)
FILE *open_file_interactive(char *defname, char *name_ret, int mode) {
  // extern int debug_create; debug_create = True;
  if (mode == MODE_READ) {
    file_selection_box fsb;
    return (fsb.file_input_loop(defname, name_ret));
  } else {
    file_window ofw;   
    return (ofw.file_input_loop(defname, mode, name_ret));
  }
}

static int line_hgt = 13; // line height in pixels

// window to display a clipped portion of text
class text_window : public window {
  int zd,ishift; 
  char *bufstart;
  char *lptr[10000]; // the vector of linestarts max 10000 lines
public:
  int lhgt; // height of 1 line in pixels
  int zz; // zz number of total lines
  // nc = number of chars : not yet used !!
  text_window(window &parent, int w, int h, int x, int y, char *buf, int ):
    window (parent,w,h,x,y) { 
      lhgt = line_hgt;
      zd = h/lhgt; ishift = 0; init(buf); }

  void DrawTextLine(int y, char *lp) {
    char temp[210], *tp = temp, ch; // copy to temp to expand tabs !! 
    while ((ch = *lp++) != '\n') { 
      if (ch == 0) return; // buffer end
      if (ch == '\t') for (int i=0; i < 8; i++) *tp++ = ' '; // tab => 8 sp.
      else *tp++ = ch;
      if (tp-temp > 200) break;
    }
    XDrawString(display,Win,text_gc,2,y, temp,tp-temp);
  }
  void redraw() {
    int i, y = lhgt, ip; 
    for (i = 0, ip = ishift; i < zd && ip < zz; i++,ip++) {  
      DrawTextLine(y,lptr[ip]); y+= lhgt; 
    }
  }
  void init(char *buf) { // compute linepointer vector
    bufstart = buf; zz = 0;
    do {
      lptr[zz++] = ++buf;
      buf = strchr(buf,'\n');
    } while (buf);
  }
  void reset(char *buf, int /* nc */) { 
    init(buf);
    clear(); redraw();
  }

  void shift(int i) { // shift starting line and redraw
    i = i >? 0; i = i <? zz-zd; // limit ishift to range [0..zz-zd]
    if (i == ishift) return;
    ishift = i; clear(); redraw();
  }
  float move(float z) { // z in [0..1]
    shift(int(z * (zz - zd))); 
    return (ishift/(zz - zd + 0.0001)); // return actual position !
  }
  float jmp(Bool up) { // called from scrollbar on pgup/down
   if (up) shift( MAX(0,ishift - zd) ); else shift( MIN(zz,ishift + zd) );
   return (ishift/(zz - zd + 0.0001)); // return actual position !
  }
};

// scrollbar with specialized callback for text scrolling
class text_scrollbar : public vert_scrollbar {
  text_window *tw;
public:
  text_scrollbar(window &parent, int w, int h, int x, int y, int sh, 
                text_window *tw) :
     vert_scrollbar(parent, w, h, x, y, sh), tw(tw) {}
  // replaces callback form vert_scrollbar
  // 1. moves the text window, then sets the slider relative
  virtual void move_callback(int y) { 
    set_slider_rel( tw->move(y/float(yspan)) ); 
  } 
  virtual void jmp_callback(Bool up) { 
    set_slider_rel( tw->jmp(up) );
  }
};

// class text_viewer :
// text_window with an attached scrollbar

int text_viewer::slider_height(int zz) {   
  int heff = height-4; // eff height in scrollbar
  int sh = height*heff/(line_hgt * MAX(zz,1));
  return MIN(heff,sh); // slider height
}

void text_viewer::reset(char *buf, int nc) {
  tw->reset(buf,nc); 
  vs->slider_adapt(slider_height(tw->zz));
}

text_viewer::text_viewer(window &parent, int w, int h, int x, int y, 
			 char *buf, int nc, int bw):
  window(parent,w,h,x,y,bw) {
    tw = new text_window(*this, width - 15, height, 0, 0, buf, nc);
    int sh = slider_height(tw->zz);
    vs = new text_scrollbar(*this, 15, height, width-15, 0, sh, tw);
    selection_mask |= KeyPressMask; // catch key events
}

void text_viewer::KeyPress_CB(XKeyEvent ev) {
  KeySym ks = XLookupKeysym(&ev,ev.state);
  switch (ks) { 
      case XK_Up: 	break;	   
      case XK_Down: 	break;	 
      case XK_Prior:	vs->jmp_callback(True); break;	 
      case XK_Next: 	vs->jmp_callback(False); break;
    }
}

