#include <stdio.h>
#include <sys/stat.h>
#include "window.h"
#include "files.h"

// read opened FILE f into char array text, with line pointers lptr
// returns : number of lines read; fina only for print
char* read_file (FILE *f, char *fina, int &n) {

  struct stat st; 
  stat(fina, &st); 
  int bufsize = st.st_size;
  printf("file %s (%d byte)\n",fina,bufsize);

  char *buf = new char[bufsize]; // it is needed later
  int c;
  n = 0;
  while ((c = fgetc(f)) != EOF) buf[n++] = c;
  buf[n] = 0;
  if (n != bufsize) { printf("error reading %s %d\n",fina,n); exit(0); }
  return buf;
}

// reads a new file and displays it in ts, 
// in fina the cosed file name is returned
void open_new_file(text_viewer *ts, char *fina, char *buf) {
  char *defname = "*.c"; 
  FILE *fload;
  fload = open_file_interactive(defname,fina,MODE_READ);
  if (fload == NULL) return;   
  int nc; 
  delete buf;
  buf = read_file(fload,fina,nc);
  ts->reset(buf,nc);
  fclose(fload);
  XStoreName(display,ts->mainw->Win,fina); // set WM name
}

int top = 0;

void spawn(char *last_fina) { // creates a new window with the last file
  int ww = 300, wh = 300;
  char *fina = new char [200]; strcpy(fina,last_fina);

  FILE *f = fopen(fina,"r"); 
  if (f == NULL) { printf("cannot open file '%s'\n",fina); return; }
  int nc;
  char *buf = read_file(f,fina,nc);
  main_window *sw = new main_window(fina,ww,wh+20);
  text_viewer *tv = new text_viewer(*sw,ww,wh,0,20,buf,nc); 

  menu_bar *mb = new menu_bar(*sw,ww,20,0,0);
  new function_button(*mb,"new file",(CB) open_new_file,tv,fina, buf);
  new function_button(*mb,"spawn",(CB) spawn,fina);

  // only the top window has a quit button the others have delete_buttons
  // that delete the sw-window tree
  // upon delete also the new arrays (lptr, text, fina) should be deleted
  if (top == 0) new quit_button(*mb); else new delete_button(*mb,sw);

  // to have only one main_loop :
  if (top++ == 0) sw->main_loop(); else sw->RealizeChildren();
 
}

int main(int argc, char **argv) {
  // if argument given : display it
  char *fina = (argc > 1) ? argv[1] : "files.c"; 
  spawn(fina) ;
}




