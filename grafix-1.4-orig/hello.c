#include "window.h"

main(int /*argc*/, char *argv[]) {
  main_window *mw = new main_window(argv[0],200,200);
  new text_win(*mw,"Hello World !",180,20,10,50);
  new quit_button(*mw,100,20,50,150);
  mw->main_loop();
}
