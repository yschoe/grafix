// dir-tree.c 
//            wolf 10/95
// very simple demo for the Tree classes in grafix

#include "tree.h"
#include "files.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

class DirTree;

typedef list <DirTree> Dlist;

// a popup window which appears when directory button is pressed
class dir_popup : public main_window {
  Bool unmapped;
  char entries[10000]; // the whole thing as one string
  int ent;
public:
  dir_popup(char *name, char *path, int ww, int wh, int x, int y) :
  main_window(name,ww,wh,1,x,y) {
    int nc = 0; // should be the length of text !!
    DIR *dirp = opendir(path);
    entries[0] = 0;
    if (dirp == 0) {
      snprintf(entries, sizeof(entries), "cannot open directory: %s\n", path);
      new text_viewer(*this,ww,wh,0,0,entries,nc);
      unmapped = True;
      return;
    }
    struct dirent *dp; 
    ent = 0; char *ep = entries;
    size_t remain = sizeof(entries);
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
      int length = strlen(dp->d_name);
      if (length == 0) continue;
      if (dp->d_name[length-1] == '.') continue; 
      // if last char == . not display it ( . or .. )
      char *fina = new char[length+strlen(path)+2];
      sprintf(fina,"%s/%s",path,dp->d_name); // complete path
      struct stat st;
      if (!lstat(fina, &st)) {
	int wrote = snprintf(ep, remain, "%8ld %-20s\n", st.st_size, dp->d_name);
	if (wrote < 0 || (size_t)wrote >= remain) {
	  // keep trailing zero and stop appending on overflow
	  entries[sizeof(entries)-1] = 0;
	  delete [] fina;
	  break;
	}
	ep += wrote;
	remain -= wrote;
	ent++;
      }
      delete [] fina;
    }
    closedir(dirp);
    new text_viewer(*this,ww,wh,0,0,entries,nc); 
    unmapped = True;
  } 
  void toggle() { // toggle between mappd/unmapped state
    int y = MIN(13*ent + 6, height); // 13 = font height
    if (y < height) resize(width, y); // -> make window as small as needed
    if (unmapped) RealizeChildren(); else Unmap();
    unmapped = ! unmapped;
  }
};

class DirTree : public Tree {
  char *path;
  dir_popup *dir_pop;
public:
  DirTree(char *name, char *path, DirTree *parent = 0) : 
  Tree(name,parent), path(path) { dir_pop = 0; }
  // popup a window with directory content
  virtual void Press_cb(XButtonEvent *ev) {
    if (dir_pop == 0) {
      char title[200]; sprintf(title,"%s",path); 
      // raise popup just below the button, second press unmaps again
      dir_pop = new dir_popup(title,path, 400, 200, ev->x_root, ev->y_root+20);
    }
    dir_pop->toggle();
  }
  virtual void Enter_cb(window *infw) { infw->clear(); infw->PlaceText(path); }

};

char *help_text[] = { 
  "call with directory name (default = $HOME)","",
  "button press pops up the contents of directory",0 };

class Dir_main : public Tree_main {
public: 
  Dir_main(char * WMName, int w, int h) : 
  Tree_main(WMName,w,h, lin_tree) {
    new help_button(*mb,"help",help_text);
  }
  // virtual void init() { Tree_main::init(); }
};

// recursively read directory tree, parent the parent Node 
void rec_dir(char *path, DirTree* parent) {
  printf("reading directory %s\n",path);
  DIR *dirp = opendir(path);
  if (dirp == 0) {
    printf("can't open %s\n",path);
    return;
  }
  struct dirent *dp; 
  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
    int length = strlen(dp->d_name);
    if (length == 0) continue;
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) continue;
    int plen = strlen(path);
    int has_trailing_slash = (plen > 0 && path[plen-1] == '/');
    char *fina = new char[length + plen + 2];
    if (has_trailing_slash) sprintf(fina, "%s%s", path, dp->d_name);
    else sprintf(fina, "%s/%s", path, dp->d_name);
    struct stat st;
    if (!lstat(fina, &st) && S_ISDIR(st.st_mode)) {
      // *** problem with symbolic links : how to exclude ???? ***
      // **********     use 'lstat' instead of 'stat'     ************
      // ********** !!!! because 'stat' follows the links !!! ********
      // printf("reading directory %s %o\n",fina,st.st_mode);
      char *tail = strrchr(fina,'/'); // eliminate trailing path
      if (tail == 0) tail = fina; else tail++;
      DirTree *dt = new DirTree(tail,fina,parent);
      rec_dir(fina,dt);
    }
    delete [] fina;
  }
  closedir(dirp);
}

// if commandline arg given use it as search path, else use ".."
int main(int argc, char* argv[]) {
  char *path = (argc > 1) ? argv[1] : getenv("HOME");
  DirTree *top = new DirTree(path,path);
  rec_dir(path,top);
  Dir_main *mw = new Dir_main(argv[0],400,450);
  mw->init();
  mw->main_loop();

}
