#include "window.h"
#include "files.h"

#define FITMAX 1000
#define BHEIGHT 18 // button height
#define ITDISP 20  // max of displayed items in selectors

// Umlaute are only recognized in global strings !!
static char *sampdef = "abdefgiujklmkqrstuvwxyäßÜABC";
char *sampstr = sampdef;

class font_selection_box : public main_window {
public: 
  char *fname;
  char **fitems;
  int fits;
private:
  edit_window *mask_ed;
  file_display *fdsp;
  selector *fsel;
  button *okb, *abortb, *chb, *imtgl;
  window *fntw; // window to display sample
  XFontStruct *fnt; // the loaded Xfont
  Bool immediate; // change display when the mouse is moved

  void bpress_cb(char *val) {   
    fname = val; fdsp->set_val(val);  
    fnt = XLoadQueryFont(display,fname);
    fntw->clear();
    if (fnt) fntw->PlaceText(sampstr,0,0,fnt); 
  }

  void move_cb(char *val) { // called from moving selections
    if (immediate) bpress_cb(val); //
  }
 
  void ok() { 
    if (fnt) exit_flag = True; else printf("error loading font %s\n",fname);
  }

  void chmask() { // callback for change mask
    make_select(mask_ed->value);
  }  
  // searches pwd for matching files and displays them in fsel
  void make_select(char *mask) {
    fitems = XListFonts(display,mask,FITMAX, &fits);
    // for (int i=0; i<fits; i++) printf("%s\n",fitems[i]);
    fsel->set_items(fitems, fits);
    int ffits = MIN(fits,ITDISP);   
    int wh = ffits * BHEIGHT + 6; 
    XResizeWindow(display,Win,width, wh+160);
    fsel->resize(width, ffits*BHEIGHT + 6); 
    // printf("window %d %d\n",Win,wh+110);
  }
  
public:
  int xl,ww;
  font_selection_box() : 
  main_window("font selection box",500,1) { // height is set in make_select    
    char empty = 0; fname = &empty; 
    xl = 10; ww = width-2*xl;
    fdsp = new file_display(*this,ww,20,xl,10);
    new text_win(*this,"selection mask",ww,20,xl,30);
    mask_ed = new mask_edit(*this, (VPCP) &make_select, this,ww,20,xl,50);

    // font selector
    fsel = new selector(*this, ITDISP, (VPCP) &move_cb, (VPCP) &bpress_cb,
			ww,200,xl,80,BHEIGHT);
    okb = new instance_button <font_selection_box> 
      (*this,"Ok",&font_selection_box::ok,this,50,20,0,0);
    chb = new instance_button <font_selection_box> 
      (*this,"Mask",&font_selection_box::chmask,this,50,20,0,0);
    abortb =
      new variable_button <int> (*this,"Abort",&exit_flag,True,50,20,0,0);
    imtgl = new toggle_button(*this,"immed",&immediate,50,20,0,0);
    fntw = new window(*this,ww,50,0,0);
    immediate = True;
  }   
 
  void resize(int w, int h) { 
    if (width == w && height == h) return; // only once
    // printf("resize %x ->  %d %d\n",Win,w,h); 
    width = w; height = h; ww = width - 2*xl;
    XMoveWindow(display,fntw->Win,xl,height-80);
    int by = height - 25, bw = ww - 4*50, dx = 50 + bw/3;
    // distribute 4 buttons of width 50 on ww
    XMoveWindow(display,okb->Win,xl,by);  
    XMoveWindow(display,chb->Win,xl+dx,by);
    XMoveWindow(display,abortb->Win,xl+2*dx,by); 
    XMoveWindow(display,imtgl->Win,xl+3*dx,by);
  } 

  ~font_selection_box() { }

  // calls main_loop, returns selected fontname and opened font
  XFontStruct *font_input_loop(char *defname) {
    strcpy(mask_ed->value, defname);
    // getcwd(pwd,100); 
    fdsp->val = defname; 
    fname = NULL;
    make_select(mask_ed->value);
    main_loop();
    printf("%s\n",fname);
    return fnt;
  }
};

XFontStruct *xfont_select(char *mask = "-*-*-*-*-*-*-*-*-*-*-*-*-*-*") {
  font_selection_box *fsb = new font_selection_box();
  // returns the selected XFontStruct 
  XFontStruct *fnt = fsb->font_input_loop(mask);
  delete fsb;
  return fnt;
}

