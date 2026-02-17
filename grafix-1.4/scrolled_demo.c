// scrolled_demo.c
//    wolf 10/95
// demo of scrolled_window class and twodim_input


#include "window.h"

class my_virtual_window  : public virtual_window {
public:
  my_virtual_window(scrolled_window *scr) : 
  virtual_window(scr) {
    for (int i= 0; i< 100; i++) 
      new button(*this,"test",50,20, rand() % width, rand() % height);
  }
  virtual void redraw() {
    line(0,0,width,height);
  }
};

main() {
  main_window *mw = new main_window("test",600,320);
  new quit_button(*mw,80,20,0,0);
  scrolled_window *scr = new scrolled_window(*mw,300,300,2000,300,0,20);
  new my_virtual_window(scr);

  new twodim_input(*mw,300,300,300,20,20,20);
  // the callback cbhook is not set -> no reaction 
  mw->main_loop();
}

