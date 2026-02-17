// class-browser.c
//          wolf 10/95
// complete graphic browser for class graphs : searches all class declarations
// in a set of source files, displays inheritance graph with buttons
// upon BPress the source pops up 

#include "tree.h" // includes window.h
#include "tree_icon.h"

#include "files.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include "regex.h"

#if 0
#define Dprintf(format, args...) printf(format, ## args)
#else
#define Dprintf(format, args...)
#endif

class class_popup : public main_window {
  Bool unmapped;
public:
  class_popup(char *name, char *decl, int ww, int wh, int x, int y) :
  main_window(name,ww,wh,1,x,y) {
    int nc = 0; // should be the length of decl !!
    new text_viewer(*this,ww,wh,0,0,decl,nc); 
    set_icon(tree_icon_bits, tree_icon_width, tree_icon_height);  
    unmapped = True;
  } 
  void toggle() { // toggle between mappd/unmapped state
    if (unmapped) RealizeChildren(); else Unmap();
    unmapped = ! unmapped;
  }
};

class ClassTree;

typedef list <ClassTree> Clist;

// ClassTree derived from Tree
// stores classname, subclasses, superclasses and textposition
class ClassTree : public Tree {
public:
  Clist *parents;

  char *declaration; // address of decl in some textbuffer
  char *path; // filename
  int tree; // index of separate tree (unused)
  class_popup *class_pop;

  ClassTree(char *name, char *decl = 0, char *path = 0) : Tree(name), 
  declaration(decl), path(path) { 
    parents = 0; 
    class_pop = 0; 
    tree = 0;
  }

  virtual void Press_cb(XButtonEvent *ev) { // activated from button press
    if (declaration == 0) return; // no declaration given
    if (class_pop == 0) {
      char title[200]; sprintf(title,"%s (%s)",name,path); 
      // raise popup just below the button, second press unmaps again
      class_pop = new class_popup(title,declaration,500,200,
				  ev->x_root, ev->y_root+20);
    }
    class_pop->toggle();
  }

  // aux function for line output concenated : x is set behind last char
  void info_cat(window *infw, int &x, int y, char *str) {
    infw->DrawString(x,y,str); 
    x += 6*(strlen(str)+1);  // charwidth = 6 (+ 1 space behind)
  }

  virtual void Enter_cb(window *infw) { // activated from button Enter
    if (infw) {      
      infw->clear();
      char text[200]; int x = 2, lhgt = 13, y = lhgt+2;
      if (path) sprintf(text,"class '%s' defined in file '%s'",name,path);
      else sprintf(text,"class '%s' is nowhere defined",name);
      infw->DrawString(x,y,text); y += lhgt;      
      Clist *ch;   
      info_cat(infw,x,y,"base :");
      for (ch = parents; ch; ch = ch->cdr) info_cat(infw,x,y,ch->car->name);
      x = 2; y+= lhgt;  
      info_cat(infw,x,y,"derived :"); 
      for (ch = (Clist*) children; ch; ch = ch->cdr) 
	info_cat(infw,x,y,ch->car->name);
     }
  }
};

ClassTree *SearchName(Clist *cl, char *sname) { 
  // returns element with sname or 0
  for( ; cl; cl = cl->cdr) 
    if (strcmp(cl->car->name,sname) == 0) return(cl->car);
  return 0;
}

class Regexp {
  struct re_pattern_buffer rpb;
public:
  char *exp;
  Regexp(char *exp) : exp(exp) { 
    rpb.buffer = malloc(100); 
    rpb.allocated = 100;
    rpb.translate = 0;
    rpb.fastmap = (char *) malloc(256);
    re_compile_pattern(exp,strlen(exp),&rpb);
  }
  Search(char *string, int size) { 
    int pos = re_search(&rpb,string,size,0,size,0);
    if (pos < 0) return -1;
    int match = re_match(&rpb,string,size,pos,0);
    return (pos+match); // returns pos *after* regexp
  }
};

#define nil (char*)(-1)

class TextBuffer {
  char *buf;
  int length;
public:
  TextBuffer(char *buf) : buf(buf) { length = strlen(buf); }

  int ForwardSearch(Regexp *rex, int beg) {
    Dprintf("searching for '%s' from %d ",rex->exp,beg);
    int pos = rex->Search(buf+beg,length-beg);
    Dprintf("returns %d\n",pos);
    if (pos < 0) return -1; else return (pos + beg);
  }
  char *Text(int pos = 0) { return buf+pos; }
  char *Line(int pos) { // search linestart *before* pos 
    char *cp = buf+pos;
    while (*cp != '\n' && cp-- > buf);
    return cp;
  }
  int Length() { return length; }
};

char* strnnew (const char* s, int len) {
  char* dup = new char[len + 1];
  strncpy(dup, s, len);
  dup[len] = '\0';
  return dup;
}

inline int IsValidChar (char c) {
  return isalpha(c) || c == '_' || c == '$';
}

inline int KeyWord (const char* string) {
  return 
    strcmp(string, "public") == 0 ||
      strcmp(string, "protected") == 0 ||
        strcmp(string, "private") == 0 ||
	  strcmp(string, "virtual") == 0;
}

void print_classes(Clist *cl);

class ClassBuffer : public TextBuffer {
public:
  ClassBuffer(char *buf) : TextBuffer(buf) {}

  char* FindClassDecl(int& beg) {
    static Regexp classkw("^class[ $]");
    static Regexp delimiter("[:{;]");    
    char *className = nil;
    for (;;) {
      beg = ForwardSearch(&classkw,beg);
      if (beg < 0) break;
      int tmp = beg;
      int delim = ForwardSearch(&delimiter,tmp);
      // check if only empty declaration
      if (delim >= 0 && *Text(delim-1) != ';') {
	className = Identifier(beg);
	break;
      }
    }
    return className;
  }
  
  char* Identifier (int& beg) {
    int i, j;
    const char* text = Text();
    char* string = nil;
  
    for (i = beg; i < Length(); ++i) 
      if (IsValidChar(text[i])) break;
    
    for (j = i+1; j < Length(); ++j) {
      char c = text[j];
      if (!IsValidChar(c) && !isdigit(c)) break;
    }
    if (j < Length()) {
      string = strnnew(&text[i], j-i);
      beg = j;
    }
    return string;
  }
  
  char* ParentName (int& beg) {
    static Regexp delimiter("{");
    int delim = ForwardSearch(&delimiter, beg);
    if (delim < 0) return nil;
  
    for (;;) {
      char* string = Identifier(beg);
    
      if (string == nil || beg >= delim) {
	delete string;
	beg = delim;
	return nil;
      
      } else if (KeyWord(string)) {
	delete string;
	string = nil;
	
      } else return string;
    }
  }
  // search for classdefs -> append to allcl
  int SearchTextBuffer(Clist* &allcl, char* path) {
    int nc = 0;
    int beg = 0;
    for (;;) { // search classname
      char *className = FindClassDecl(beg);
      if (className == nil) break;
      char * decl = Line(beg);
      Dprintf("\nnew class '%s' at %d\n",className, beg);
      // print_classes(allcl);
      ClassTree *cli = SearchName(allcl,className); 
      nc++;
      if (cli == 0) { // new class
	cli = new ClassTree(className,decl,path);
	allcl = allcl->push(cli);
      } else { // already there : update 
	cli->declaration = decl;
	cli->path = path;
      }
      for (;;) { // search all parents
	char* parentName = ParentName(beg);
	if (parentName == nil) break;
	
	ClassTree *cpi = SearchName(allcl,parentName);
	if (cpi == 0) { // new class (only the name)
	  cpi = new ClassTree(parentName);
	  allcl = allcl->push(cpi);
	} // else already there : nothing 
      
	cli->parents = cli->parents->push(cpi); // my parentlist
	cpi->children = cpi->children->push(cli); // I am child of parent
	Dprintf(" '%s'",parentName);
      }
    }
    return nc; // number of defined classes
  }
};

typedef int (*IVPVP)(const void *,const void *); // the correct type for qsort
// compare function for qsort : sorting of child nodes
static int qcomp(const Tree **e1, const Tree **e2) { 
  return ((*e2)->descendants - (*e1)->descendants);  
  // return (strcmp((*e1)->name,(*e2)->name)); 
}

static char* help_text[] = { 
  "Call the program with filenames or directory names as commandline args",
  "eg.  \"class_browser win* ../grab/\"  or  \"class_browser . *.h *.c\" ",
  "Directories given as argument are completely parsed",
  "Only files that end in '.c' '.C' '.h' are recognized as C-files",
  "All given files are parsed for class definitions and linked in one graph",
  "The different colors represent the file where the class is defined","",
  "By pressing a button with class name a window with the header pops up,",
  "pressing again unmaps it","",
  "The bottom window shows info about the class under the cursor","",
  "'hardcopy' - makes a 'xwd | xpr | lpr' pipe to the printer",
  "             but only the visual part is printed","",
  "The program will fail for some exotic graphs at the moment", 0 };
 
class Class_main : public Tree_main {
public: 
  Class_main(char * WMName, int w, int h, Clist *cll) : 
  Tree_main(WMName,w,h, (Tlist *) cll) {
    new help_button(*mb,"help",help_text);
  }

  virtual void init() { // modify : change button colors
    Tree_main::init(); // first usual init
    hinf = 45; // larger info window

    unsigned int pmax = 42, pid; // max number of colors
    struct GC3 gc3[pmax]; // vector of button GCs
    for (pid = 0; pid < pmax; pid++) {
      unsigned short b = (pid % 7 + 7)*5040, r = (pid % 3 + 3) * 13100,
      g = (pid / 21 + 9) * 6500; // the colors for the file indexed pid
      color_GC(r,g,b,&gc3[pid]); // create new GCs
    }
    // compute colors directly from filename : everytime get the same color !
    Clist *cll = (Clist *) tll; // linear list of all nodes
    for (Clist *cl = cll; cl; cl = cl->cdr) { 
      ClassTree *ci = cl->car; 
      char* path = ci->path; 
      if (path) { // if == 0 : no file with definition, used default color
	char *slash = strrchr(path,'/');
	if (slash) path = slash+1; // get rid of all before last slash 
	for (pid=0; *path;) pid += (unsigned char) *(path++);
	pid %= pmax; // simply rest of character sum 
	ci->nb->set_GC(gc3+pid); // set the colors from gc3[pid]
      }
    }
  }
  // to be called when all classes read in : 
  // build geometrical graph for display
  // vwidth and vheight return size of needed virtual window
  virtual void make_graph() {
    Clist *cll = (Clist *) tll; // linear list of all nodes
    int nclasses = tll->length(); //  total number of classes

    // use iteration algorithm to assign each node a depth value starting with 0
    // the outer loop (cl) goes with the linear list 
    // the inner loop (chl) checks all children, if there a mismatch is found
    // the outer is started again
    // can be called dynamically
    // suppositions : initial depth values = 0

    int nloop = 0, maxnloop = 1000000 >? (nclasses*nclasses*nclasses);
    Clist *cl = cll; 
    do { 
      ClassTree *ci = cl->car;
      int dd = ci->depth; Clist *chl;
      for (chl = (Clist*) ci->children; chl; chl = chl->cdr) {
	Node *chn = chl->car; 
	if (chn->depth < dd+1) { chn->depth = dd+1; break; } else
	if (chn->depth > dd+1) { ci->depth = chn->depth - 1; break; }
      } 
      if (chl == 0) cl = cl->cdr; // the inner loop completed
      else cl = cll; // there was a mismatch, start again
      if (nloop++ > maxnloop) error("the class graph contains cycles");
    } while(cl);

    // printf("nloop = %d\n",nloop);
    // second loop : look for the node with most desc to use as top
    int ndescmax = 0; 
    ClassTree *ctop = 0;
    for (cl = cll; cl; cl = cl->cdr) { 
      ClassTree *ci = cl->car;
      int nd = ci->rec_descendants(); //
      if (nd > ndescmax) { ndescmax = nd; ctop = ci; }
    }

    make_tree(ctop); // build main tree

    // now all buttons not in the main tree must be inserted
    // 1. : find all nodes with no parents
    // 2. : sort them for number of descendants, so largest come first
    // 3. : insert into existing tree

    ClassTree *ctemp[nclasses];// for sorting children we must use a temp vector
    // the number needed is less than nclass
    int nc = 0;
    for (cl = cll; cl; cl = cl->cdr) { 
      ClassTree *ci = cl->car;
      if (ci->parents == 0 && ci != ctop)  // has no superclasses -> subtree
	ctemp[nc++] = ci;
    }
    qsort(ctemp, nc , sizeof(void*), (IVPVP) qcomp); // sort with qcomp

    for (int i = 0; i < nc; i++) 
      ctemp[i]->rec_insert(tcol, vheight, vwidth);

   //for (Tlist *tl= tcol[1]; tl; tl = tl->cdr) printf("%s\n",tl->car->name);
  }
};

// test if fina is a c-file eg. ends with  ".c/.C/.h"
Bool cfile(char *fina) { 
  int length = strlen(fina); 
  char clast = fina[length-1];
  return fina[length-2] == '.' && (clast == 'c' || clast == 'C' || clast == 'h' );
}
  
void parse_file(Clist* &allcl, char *fina) {
  if (! cfile(fina) ) return; 
  struct stat st; 
  if (stat(fina, &st) != 0) return; 
  if (st.st_mode & S_IFDIR) return;
  int bufsize = st.st_size;
  printf("file %s (%d byte)",fina,bufsize);
  FILE *f = fopen(fina,"r"); if (f == 0) return; 

  char *buf = new char[bufsize]; // the buffer for ClassBuffer
  if (buf == 0) error("no space for textbuffer");
  int c, n = 0;
  while ((c = fgetc(f)) != EOF) buf[n++] = c;
  buf[n] = 0;
  if (n != bufsize) { printf("error reading %s %d\n",fina,n); exit(0); }
  ClassBuffer *tb = new ClassBuffer(buf);
  int nc = tb->SearchTextBuffer(allcl,fina);
  printf(" has %d class definitions\n",nc);
  if (nc == 0) { delete tb; delete buf; } // not needed
  fclose(f);
}

void parse_directory(Clist* &allcl, char *path) {
  printf("reading directory %s\n",path);
  DIR *dirp = opendir(path);
  struct dirent *dp; 
  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) { 
    int length = strlen(dp->d_name);
    if (dp->d_name[length-1] != '.') { // not for . or .. file
      int plen = strlen(path); 
      if (path[plen-1] == '/') path[plen-1] = 0; // eliminate tail-slashes
      char *fina = new char[length+strlen(path)+2];
      sprintf(fina,"%s/%s",path,dp->d_name);
      parse_file(allcl,fina);
    }
  }
}

// output of all found classes with children and parents on stdout
void print_classes(Clist *cl) {
  while (cl) {
    ClassTree *ci = cl->car;
    printf("\n%s %d %d",ci->name,ci->depth,ci->tree);
    // char *decl = ci->declaration;
    // for (int i=0; i< 10; i++) printf("%c",*(decl+i));
    Clist *ch = (Clist*) ci->children;
    if (ch) { 
      printf(" [");
      while (ch) { printf(" %s",ch->car->name); ch = ch->cdr; }
      printf(" ]");
    }
    Clist *cp = ci->parents;
    if (cp) { 
      printf(" (");
      while (cp) { printf(" %s",cp->car->name); cp = cp->cdr; }
      printf(" )");
    }
    cl = cl->cdr;
  }
  printf("\n");
}

// call with filename arguments : 
// eg "*" "win*" "*.h" "f1.c f2.h" ". ../fgrab/"
// filenames other then .c/.h/.C are ignored
// directories as args are completely parsed
 
int main(int argc, char* argv[]) {
  Clist *AllClasses = 0; // linear list of all classes
  // old version : explicit files
  char *finas[] = {"window.h",  "lattice.h", "lat_win.c", "class_browser.c" };
  int nfi = sizeof(finas)/sizeof(char*);
  if (0) for (int i=0; i< nfi; i++) parse_file(AllClasses, finas[i]);
  
  if (argc < 2) parse_directory(AllClasses,"."); // if no argument given

  for (int arg = 1; arg < argc; arg++) {
    char *fina = argv[arg];
    struct stat st; 
    if (stat(fina, &st) == 0) { // else no file 
      if (st.st_mode & S_IFDIR) // a directory
	parse_directory(AllClasses,fina);
      else
	parse_file(AllClasses,fina);
    }
  }

  int nctot = AllClasses->length();
  printf("total %d classes\n",nctot);
  // print_classes(AllClasses);
  if (nctot == 0) return 0;
  // **** the drawing ********
  Class_main *mw = new Class_main(argv[0],400,450,AllClasses);
  mw->init();
  mw->main_loop();
}
