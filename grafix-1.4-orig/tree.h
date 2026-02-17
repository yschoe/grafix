// tree.h : include file for tree.c
//          used to build tree-grafix (class-browser, dir-tree)

#include "window.h"

template <class T>
class list { 
public:
  T *car; list *cdr; 
  list(T *car) : car(car) { cdr = NULL; }   
  list *push(T *vx) { // push item T in front of list, return new
    list *gx = new list(vx); gx->cdr = this;
    return gx;
  }
  void append(list *hx) { // append to the end of list, slow !!
    list *h = this; while(h->cdr) h= h->cdr; 
    h->cdr = hx; 
  }  
  int length() {
    int n = 0;
    list *h = this; while(h) { h= h->cdr; n++; } 
    return n;
  }
  list *nth(int n) { // return the n-th element, slow !!
    int i; list *h = this;
    for (i=0; i<n;i++) h= h->cdr;
    return h;
  }
  T *last() { // return last car
    list *h = this; while (h->cdr) h = h->cdr;
    return h->car;
  }
  list *remove(T *xcar) {
    list *cl = this; 
    if (this->car == xcar) { cl = this->cdr; delete this; return cl; }
  
    while (cl) { 
      list *pred = cl; cl = pred->cdr;
      if (cl->car == xcar) { pred->cdr = cl->cdr;  delete cl; break; }
    } 
    return this;
  }
  void print() { // call print methods of all elements : -> incomplete type
    list *h = this; while(h) { /* h->car->print(); */  h= h->cdr; }
  } 
};

struct GC3 { GC forgr, bright, dark; }; // 3 GCs for buttons

extern void color_GC(unsigned short r, unsigned short g, unsigned short b, 
		     GC3 *gc3);

// a button with special colors
class color_button : public window { 
  GC forgr,bright,dark;
  char *Name;
public:
  color_button(window & parent, char *Name, int w, int h, int x, int y);
  virtual void redraw();
  void set_GC(struct GC3 *gc3);
 
  virtual void BRelease_CB(XButtonEvent) { rise(); }
  virtual void BPress_CB(XButtonEvent ev) { drop(); bpress(&ev); }
protected:  
  void rise();
  void drop();
  virtual void bpress(XButtonEvent*) { }
};

class Node;

class Node_button : public color_button  {
  window *infw; // to inform for Enter
  Node *ci;
public:
  Node_button(window &parent, window *infw, Node *ci);
  virtual void Enter_CB(XCrossingEvent);
  virtual void bpress(XButtonEvent* ev);
};

class Node {
public:
  int depth; // balanced depth in graph
  int xp,yp,w,h; // drawing position and size 
  char *name;
  Node_button *nb;
  int descendants; // total # of all desc
  Node(char *name);
  virtual void Enter_cb(window* /*infw*/) {}
  virtual void Press_cb(XButtonEvent* /*ev*/) {} // activated from button press
};

class Tree;
typedef list <Tree> Tlist;

extern Tlist *lin_tree; // linear list of all nodes, needed from Tree_main

class Tree : public Node {
public:
  Tlist *children; // a linked list of Tree-pointers
  int nch; // number of children
  Tree(char *name, Tree *parent = 0);
  void rec_depth(int d); // recursively set depth
  void rec_breadth(int depthp, int maxdepth, int counter[]); 
  int rec_descendants(); 
  int rec_geometry(int ytop[], Tlist* tcol[], int ypar, int dbmax);
  int rec_insert(Tlist *tcol[], int &vheight, int &vwidth);
};

// class for displaying the tree-graph in a scrolled_window
class Tree_window : public virtual_window {
  Tlist *cll; // linear list of all nodes (AllNodes)
public:
  Tree_window(scrolled_window *scr, window *infw, Tlist *cll);
  virtual void redraw();
};

// Tree_main contains all subwindows for displaying the Tree 
class Tree_main : public main_window {
protected:
  scrolled_window *scr;
  Tree_window *grw;
  menu_bar *mb;
  window *infow; // to show infos about class on button EnterCB
  int vwidth, vheight,yroot; // virtual size and y of root    
  int hinf,hmb; // height of info window, menubar
  Tlist *tll; // linear list of all nodes
  Tlist *tcol[100];// vector of linear lists for each depth column (depth < 100)
                   // for easy geometry parsing, generated in rec_geometry
public: 
  Tree_main(char * WMName, int w, int h, Tlist *tll);
  virtual void init();
  virtual void resize(int w, int h);
  virtual void make_graph();
  void make_tree(Tree *top);
};
