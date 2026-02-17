// win-demo.c : 
//     demonstration of some basic classes and functionality of grafix

#include "window.h"

// derived class of window
class my_window : public window {
public: my_window(window & parent, int w, int h, int x, int y) :
  window(parent,w,h,x,y) {}
  virtual void redraw() { // virtual fn replaces window::redraw
    int y; 
    for (y=0; y < height ; y+= 3) line(0, y, width, y); 
  }
};


// window shows a variable string which is selected from radio_menu 
// deft is the starting value
class displ : public window {
public:
    char * val;
    displ(window & parent, char * deft, int w, int h, int x, int y) :
    window (parent,w,h,x,y) { val = deft; } 

    virtual void redraw()  { clear(); PlaceText(val); }
    virtual void action(char* /* menu */, char * value) { 
      val = value;
      redraw(); 
    }
};

// test of  keyboard-Events : output of state and key
class key_test : public button { 
char info[200]; 
public:
   key_test(window & parent, int w, int h, int x, int y) :
     button (parent,"keyboard test",w,h,x,y) {
     selection_mask |= KeyPressMask;
   }
   virtual void KeyPress_CB(XKeyEvent ev) { 
     // only Shift state (0x1) is recongnized in  XLookupKeysym
     KeySym keysym = XLookupKeysym(&ev,ev.state & 1);
     sprintf(info,"state = 0x%x code = 0x%x keysym = 0x%lx -> '%s'",
	     ev.state,ev.keycode,keysym,XKeysymToString(keysym));
  //   for complicated chars (Alt Ctrl Alt Gr...) 
  //   char bufret[20]; int bytes_buffer; KeySym keys_ret; XComposeStatus sio;
  //   XLookupString(&ev,bufret,bytes_buffer,&keys_ret,&sio);
  //   printf(" %s %x ",bufret,keys_ret);
     printf("%s\n",info); 
     Name = info; clear(); // draw info for Name
     redraw();
   }
 };

// quit as a callback function
void cb_fn() { printf(" cb   \n"); }
void quit() { exit(0); }
void funct(int n, char * s1, char * s2) {
  printf("funct called with int %d, char * %s, char * %s\n",n,s1,s2); }

scrollbar * scbr;
main_window *mainw;

void sbinf() { printf(" %g",scbr->value); fflush(stdout); };

// lists all children recursively
void child_tree(window *pp) {
  static int nest = 0;
  win_list * cc = pp->children;
  nest++;
  while (cc) { 
    window * cw = cc->child;
    int i; for (i=0; i < nest; i++) printf("  "); // recursion depth
    printf("Win %lx (%dx%d) at %d,%d\n",cw->Win,cw->width,cw->height,
	   cc->x,cc->y);
    child_tree(cw);
    cc = cc->next; }
  nest--;
}

void child_print(window *pp) {
  printf("window tree of Win %lx (%dx%d)\n",pp->Win,pp->width,pp->height);
  child_tree(pp);
} 

main(int /* argc */, char *argv[]) 
{ 
  main_window *mainw = new main_window(argv[0], 300, 300);  // Main Window

  // 1. child : a quit button
  new quit_button(*mainw,100,20, 0, 0);
  // 2. child : simple window  
  window *child = new window(*mainw,  80, 100, 0, 100); 

  // button instance as child of child (wo functionality) but help
  char *text1[] = {"this is context sensitive","Help-Text",""
		   "which is bound to button ",0};
  new button(*child, "button", text1, 50, 20, 10, 10);
  // a button with callback-function 
  button *sb = new callback_button(*child, "cb-info", cb_fn, 50,20,10, 40); 
  sb->CB_info = TRUE; // debug inform about all events

  new callback_button(*child, "quit", quit, 50,20,10, 70);
  
  // a popup window connected with a popup-button  
  main_window *popup = new main_window("popup", 200, 100); 
  popup->CB_info = True;
  new button(*popup, "hello", 100, 20, 0, 0);
  new unmap_button(*popup, "delete",100, 20, 100, 0);
  new popup_button(*mainw, popup, "popup", 100, 20, 0, 30);

  // help button 
  char * text[] = {"this is help text for help button", "here the 2. line", 0};
  new help_button(*mainw, 20, 60, text);
  new my_window(*mainw, 60, 60, 110, 0); // user-defined window 

  // a simple pulldown window connected with pulldown_button 
  // and two childs
  pulldown_window * pulldown;
  int w = 80, h = 20, x = 210, y = 0; 
  pulldown = new pulldown_window (w, 200);

  new button (*pulldown,"pp",w, 20, 0, 30);
  new pulldown_button(*mainw, pulldown, "pulldown-1", w, h, x, y);
  y+= 30;

  struct but_cb pd_list[] = { {"button1", cb_fn}, {"quit", quit}};
  make_pulldown_menu(*mainw, "pulldown-2", 2, pd_list, w, h, x, y); y+= 30;

  // xwd_button : calls "xwd -id wid | xwud &" (produces copy on  screen)
  xwd_button xwd(*mainw,"dump"," | xwud &", mainw, w, h, x, y); 
  y+= 30;
  
  new template_button <window*>(*mainw,"children",child_print,
				      mainw, w, h, x, y);
  y+= 30;

  // radio button menu with display of toggled value
  window *radio = new window(*mainw,100,70,100,100); 
  displ *dsp = new displ(*radio,"display",80,20,10,10);
  char *blist[] = { "val-1", "***b2***", "val-2" , "xxxxxx", 0 };
  char *help[] = {"this Help Text ","appears", "upon press", 
		  "of the right mouse button","as popup",0};
  make_radio_menu (*radio, "radio", blist, help, dsp, 80, 20, 10, 30); 

  new key_test(*mainw,300,20,0,200);
  
  scbr = new scrollbar (*mainw,&sbinf,200,30,0,240,0,1000,500,"x = %.0f");

  // a menu bar with autoplace buttons
  menu_bar *mb = new menu_bar(*mainw,300,25,0,270);
  int value = 1; // a toggle value for toggle_button below
  new button(*mb,"buttons"); new button(*mb, "with");
  new button(*mb,"autosizing");
  // Demo for function_button -> calls 'funct'
  new function_button(*mb,"funct",(CB) funct,1,"2.arg","3.");
  new toggle_button(*mb, "toggle", &value);
  new quit_button(*mb);

  mainw->main_loop();
  // delete mainw; -> is done automatically !
}



