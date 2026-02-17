// font-browser.c : demo of font-selector 

#include "font-selector.h"

main(int argc, char **argv) {
  if (argc > 1) sampstr = argv[1];
  char *mask = (argc > 2) ? argv[2] : "-*-*-*-*-*-*-*-*-*-*-*-*-*-*";
  xfont_select(mask);
}
