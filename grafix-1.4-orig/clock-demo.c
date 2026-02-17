/* clock.c : wolf 6/95
   demo for the use of clock_win 
   display two clocks for different timezones
*/

#include "window.h"
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

clock_win *clck1, *clck2;

int ticker1 = 0, ticker2;

class clock_main_window : public main_window {
public:
  clock_main_window(char *Name, int w, int h) : main_window(Name,w,h) { 
    polling_mode = True; 
  }
  virtual void polling_handler() {
    if (ticker1) sleep(1); // not at first, then sleep for 1 secs
    time_t secs =  time(0); // seconds from 1.1.1970
    struct tm *loc = localtime(&secs); // transform to local time
    ticker1 =  loc->tm_sec + 60*(loc->tm_min + 60*loc->tm_hour);
    // printf(" %d\n",ticker);
    ticker2 = ticker1 - 8*3600; // 8hrs back
    clck1->redraw(); clck2->redraw(); 
  }
};

int main(int /*argc*/, char *argv[]) {
  main_window *mw = new clock_main_window(argv[0],100,300);
  int y = 0;
  clck1 = new clock_win(*mw, &ticker1,100,100,0,0); y+= 110;
  new text_win(*mw,"GMT",80,20,10,y,1); y+= 30;
  clck2 = new clock_win(*mw, &ticker2,100,100,0,y); y+= 110;
  new text_win(*mw,"New York",80,20,10,y,1); y+= 30;
  new quit_button(*mw,80,20,10,y);
  mw->main_loop();		     
}
