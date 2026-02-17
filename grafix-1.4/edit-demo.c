// edit-demo.c : demonstration of edit_windows 
#include "window.h"

// derived from edit_window no init string, enter fn overwritten
// expects 2 ints as input
class myedit : public edit_window {
public: myedit(window &parent, int w,int h, int x, int y) : 
  edit_window(parent,"",w,h,x,y) {}
  virtual void enter() { 
    int reg, arg;
    printf("%s\n",value); 
    if (sscanf(value,"%d%d",&reg,&arg) == 2) printf("ok %d %d\n",reg,arg);
    else format_error();
  } 
};

main(int /*argc*/, char *argv[]) { 
  main_window *mw = new main_window(argv[0],200,100);
  new edit_window(*mw,"1234",180,20,10,10);  
  new myedit(*mw,180,20,10,40);
  new quit_button(*mw,100,20,10,70);
  mw->main_loop();
}
