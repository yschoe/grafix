#include "window.h"
#include "palette.h"

main() {
  main_window mw("palette",220,60);
  palette_popup *pp = new palette_popup(100);
  palette_popup *pp2 = new palette_popup(100);

  popup_button pb(mw,pp,"popup 1",100,20,10,0); 
  popup_button pb2(mw,pp2,"popup 2",100,20,110,0);
  quit_button qb(mw,100,20,60,30);
  mw.main_loop();
}
