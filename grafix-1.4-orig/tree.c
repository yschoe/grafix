// tree.c : 
//          wolf 10/95
//  classes and functions used to build tree-grafix (class-browser, dir-tree)

#include "tree.h"
#include "tree_icon.h"

static void rect3d(Window Win, short x, short y, short w, short h, 
		   GC left_top, GC right_bot) {
  int i, thickness = 2;  
  for (i = 0; i < thickness; i++) {
    short xa = x+i, ya = y+i, xe = x+w-i, ye = y+h-i ; 
    XPoint xp[5] = { {xa,ye}, {xa,ya}, {xe,ya}, {xe,ye}, {xa,ye}}; // short x,y
    XDrawLines(display,Win, left_top, xp, 3, CoordModeOrigin);
    XDrawLines(display,Win, right_bot, xp+2, 3, CoordModeOrigin);
  }
}

// calculation of GCs for button colors, outside class definition, 
// since many buttons can use the same GC 
// give the brightest colors, the other 2 are set proportional
void color_GC(unsigned short r, unsigned short g, unsigned short b, GC3 *gc3) {
  int fg_pix = alloc_color(3*r/4,3*g/4,3*b/4); // inner part
  int br_pix = alloc_color(r,g,b); // bright rim
  int lw_pix = alloc_color(r/2,g/2,b/2); // dark rim
  XGCValues values;
  
  values.foreground = fg_pix;
  gc3->forgr = CreateGC(GCForeground, &values);
  values.foreground = br_pix;
  gc3->bright =  CreateGC(GCForeground, &values);
  values.foreground = lw_pix;
  gc3->dark =  CreateGC(GCForeground, &values);
}

// a button with special colors

color_button::color_button(window &parent, char *Name, int w,int h,int x,int y) 
: window(parent, w, h, x, y, 0), Name(Name) { 
  selection_mask |= EnterWindowMask | LeaveWindowMask | 
                    ButtonPressMask | ButtonReleaseMask; 
  // by default use button colors
  forgr = button_fg_gc; bright = button_br_gc; dark = button_lw_gc; 
}

void color_button::redraw() {
  XFillRectangle(display,Win,forgr,0,0,width,height); 
  PlaceText(Name);  
  rise();
}  

void color_button::set_GC(struct GC3 *gc3) {
  forgr = gc3->forgr; bright = gc3->bright; dark = gc3->dark;
}
 
void color_button::rise() { rect3d(Win,0,0,width-1,height-1, bright, dark); }
void color_button::drop() { rect3d(Win,0,0,width-1,height-1, dark, bright); }

static int bwmax = 100; // max button width
static int bhgt = 20; // button height
static int yoff = bhgt + 2; // vertical offset (between buttons)

// class Node
Node::Node(char *name) : name(name) {
  depth = 0; descendants = 0; 
  xp = yp = -1; 
  w  = bwmax <? 6*strlen(name)+6; 
  h  = bhgt;
}  

// Node_button definitions
Node_button::Node_button(window &parent, window *infw, Node *ci)
: color_button(parent,ci->name,ci->w,ci->h,ci->xp,ci->yp), infw(infw), ci(ci) {}
void Node_button::Enter_CB(XCrossingEvent) {  ci->Enter_cb(infw); }
void Node_button::bpress(XButtonEvent* ev) { ci->Press_cb(ev); }

// compare function for qsort : use total number of descendants
static int qcomp(const Tree **e1, const Tree **e2);
typedef int (*IVPVP)(const void *,const void *); // the correct type for qsort

// class Tree : public Node
Tlist *lin_tree = 0;

Tree::Tree(char *name, Tree *parent) : Node(name) {
  children = 0; nch = 0;
  if (parent)  
    parent->children = parent->children->push(this); // updtate parent list
  lin_tree = lin_tree->push(this);
}  

void Tree::rec_depth(int d) { // recursively set depth
  depth = d;
  for (Tlist *ch = children; ch; ch = ch->cdr) ch->car->rec_depth(d+1);
}

// recursively tree-parsing downwards, compute cumulative treebreadth
// at each depth on vector counter, depth is computed independently
// so it works from any level, depth = 0 -> always 1 (myself)
void Tree::rec_breadth(int depthp, int maxdepth, int counter[]) {
  if (depthp == maxdepth) return;
  counter[depthp++]++;
  if (depthp == maxdepth) return; // no need to parse further
  for (Tlist *ch = children; ch; ch = ch->cdr)
    ch->car->rec_breadth(depthp, maxdepth, counter);
}
  
// count all descendants (all depths), include myself, but only once !!
int Tree::rec_descendants() {
  if (descendants == 0) { // else :already computed
    nch = children->length(); 
    for (Tlist *ch = children; ch; ch = ch->cdr)
      descendants += ch->car->rec_descendants();
    descendants++; // include myself
  }
  return (descendants); 
}
  
// recursively parse geometry, ytop is the current ymax for all depths
// ypar the parent height (if known), dbmax the depth with max breadth
// returns the own yp for parent node
int Tree::rec_geometry(int ytop[], Tlist* tcol[], int ypar, int dbmax) {
  tcol[depth] = tcol[depth]->push(this); 
  // tcol-lists for later dynamically inserting; sorted from bottom -> top
  int yc = 0; 
  if (nch > 0) {
    int ic; Tlist *cl = children; 
    Tree *ctemp[nch]; // for sorting children we must use a temp vector
    for (ic = 0; cl; cl = cl->cdr, ic++) ctemp[ic] = cl->car;
    qsort(ctemp, nch, sizeof(void*), (IVPVP) qcomp); // sort with qcomp
    int y = ypar >? ytop[depth]; yc = 0;
    for (ic = 0; ic < nch; ic++) { 
      int cyprop = y + (ic - (nch-1)/2)*yoff; // proposed child y
      yc += ctemp[ic]->rec_geometry(ytop, tcol, cyprop, dbmax);
    }
    yc /= nch; // mean of all children
  } 
  // for depths lower dbmax use child mean, else parent value
  if (depth < dbmax) yp = yc; else yp = ypar; 
  yp = yp >? ytop[depth]; // make sure no overlapping takes place
  xp = 5 + (bwmax+5)*depth;
  ytop[depth] = yp + yoff; 
  return yp;
}

// dynamically inserting subtrees into existing graph (recursively)
// by use the column-list of depth, arranged from bottom upwards
int Tree::rec_insert(Tlist *tcol[], int &vheight, int &vwidth) { 
  if (yp > 0) return yp; // already inserted !!
  // printf("%s %d %d\n",name,depth,vheight);
  int yc = 0; // used if no children
  if (nch > 0) { // first insert children and compute their mean y
    for (Tlist *ch = children; ch; ch = ch->cdr)
      yc += ch->car->rec_insert(tcol, vheight,vwidth);
    yc /= nch;
  } 
  int ylast = vheight+2*yoff, // to have at least space for one button on bottom
  dmin = ylast, yopt = 0; 
  Tlist *cp1 = 0,*cp2 = 0, *clast = NULL;
  // the loop goes one step behind list end to catch the lower bound = 0
  for (Tlist *cact = tcol[depth]; ; cact = cact->cdr) {
    int yact = (cact) ? cact->car->yp : 0;
    if (yc - yact > yoff && ylast - yc > yoff) { // we have optimal place
      cp1 = cact; cp2 = clast;
      dmin = 0; yopt = yc;
	break; // no need to search further 
    }
    if (ylast - yact > 2*yoff) { // there is space for another button
      int yx = yact+yoff, diff = abs(yc-yx);
      if (diff < dmin) { // find the space with minimal distance to children yc
	dmin = diff; yopt = yx; 
	cp1 = cact; cp2 = clast;
      }
    }
    if (cact == 0) break; // loop ends
    clast = cact; ylast = yact;
    }
  // insert new value into tcol-list between cp2 -> cnew -> cp1
  Tlist *cnew = cp1->push(this); 
  if (cp2) cp2->cdr = cnew; else tcol[depth] = cnew;
  
  yp = yopt;  xp = 5 + (bwmax+5)*depth;
  vheight = vheight >? yp+yoff;
  vwidth = vwidth >? xp+bwmax;
  return yp;
}

// compare function for qsort : sorting of child nodes
static int qcomp(const Tree **e1, const Tree **e2) { 
  return ((*e2)->descendants - (*e1)->descendants);  
  // return (strcmp((*e1)->name,(*e2)->name)); 
}

// class for displaying the tree-graph in a scrolled_window
// class Tree_window : public virtual_window 
Tree_window::Tree_window(scrolled_window *scr, window *infw, Tlist *cll) : 
  virtual_window(scr), cll(cll) {
  for (Tlist *cl = cll; cl; cl = cl->cdr) {
    Node *ci = cl->car; 
    if (ci->w > 0) // callbacks for ci are activated on BPress, Enter
      ci->nb = new Node_button(*this,infw,ci);
  }
}

void Tree_window::redraw() {
  // draw lines between all buttons and their children (not recursive !)
  for (Tlist *cl = cll; cl; cl = cl->cdr) {
    Tree *ci = cl->car;
    int x = ci->xp + ci->w, y = ci->yp+10; // point behind the button 
    for (Tlist *ch = ci->children; ch; ch = ch->cdr) { // child-list
      line(x,y,ch->car->xp,ch->car->yp+10);
    }
  }
}

// Tree_main contains all subwindows for displaying the Tree 
// class Tree_main : public main_window 

Tree_main::Tree_main(char * WMName, int w, int h, Tlist *tll) : 
  main_window(WMName,w,h), tll(tll) {
    hinf = 20; hmb = 20;
    set_icon(tree_icon_bits, tree_icon_width, tree_icon_height);
    mb = new menu_bar(*this,w,hmb,0,0,0);
    new quit_button(*mb); 
    infow = new window(*this,w,hinf,0,h-hinf);
  }

void Tree_main::init() { // to be called after constructor !! (uses virtual fns)
  make_graph(); // compute graph-nodes (xp,yp) and virtual size 
  scr = new scrolled_window(*this,width,height-hmb-hinf,vwidth,vheight,0,hmb);
  grw = new Tree_window(scr, infow, tll); // the virtual window
  // hardcopy only the clipped region of scrolled window (more not visible :-)
  new xwd_button(*mb,"hardcopy"," | xpr | lpr",scr->clip);
  width += 1; // hack to force resize on configure
}

void Tree_main::resize(int w, int h) {
  // printf("resize %d %d %d %d\n",w,h,width,height);
  if (width == w && height == h) return; 
  width = w; height = h;  
  mb->resize(width, hmb);
  scr->resize(width, height - hmb - hinf);
  infow->resize(width, hinf);
  XMoveWindow(display,infow->Win, 0, height - hinf);
  
  twodim_input *vs = scr->vs; // centre vertical slider
  if (vs) vs->set_slider(0,vs->yspan*yroot/vheight); 
}

// build geometrical graph for display
// vwidth and vheight return size of needed virtual window
// simple default : use *last* element of tll as top ???
void Tree_main::make_graph() { 
  Tree *top = tll->last();
  top->rec_depth(0); int nd = top->rec_descendants();
  if (False) printf("%s %d %d\n",top->name,nd,tll->length());
  //for (Tlist *ch=top->children;ch;ch=ch->cdr) printf("%s\n",ch->car->name);
  make_tree(top); 
}

void Tree_main::make_tree(Tree *top) {
  // compute depth with max breadth
  int d, dbmax = 0, bmax = 0, dtot, bcount[100]; // max depth = 100
  for (d = 0; d < 100; d++) bcount[d] = 0;  
  top->rec_breadth(0,100,bcount);
  for (d = 0; d < 100; d++) { 
    if (bcount[d] > bmax) { bmax = bcount[d]; dbmax = d; }
    if (bcount[d] == 0) break;
  } 
  dtot = d; // total depth 
  // printf("top = %s dbmax %d bmax %d depth %d\n",
  //         top->name, dbmax, bmax, dtot);
  int ytop[dtot]; 
  for (d = 0; d < dtot; d++) {
    ytop[d] = 2; // leave space 2 pixel
    tcol[d] = NULL; // init the column list
  } 
  vheight = 0; 
  yroot = top->rec_geometry(ytop,tcol,0,dbmax);
  vwidth = bwmax*d + 10; vheight = 0;  
  for (d = 0; d < dtot; d++) vheight = vheight >? ytop[d]; 
 
  vheight += 2; 
  // printf("%d %d\n",vwidth,vheight);
  // for (Tlist *tl= tcol[2]; tl; tl= tl->cdr) printf("%s\n",tl->car->name);
}

