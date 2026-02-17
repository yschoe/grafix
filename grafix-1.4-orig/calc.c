// calc.c : a very simple calculator, part of the grafix package
//    wolf 12/94, 
//          hex/dec/bin conversion 
//          works only with integers, no overflow check is performed
//          arithmetic and logical operations

#include "window.h"
#include <ctype.h>

static int mode = 0, // actual display mode : 0 = decimal, 1 = hex, 2 = bin
       base = 10;    // 10       or  16
char *modestr[3]= {"dec","hex","bin"};

typedef void (*OP) (int&, int); // operator function
void identity(int &x,int y) { x = y; };

class acc_disp : public window { // display and manipulate the accumulator
  int value, acc;
  OP op_st; // operation stack
  Bool init; // start entering a new number (is set after ops )
public:
  acc_disp(window & parent, int w, int h, int x, int y) :
    window (parent,w,h,x,y,2) { 
      value = 0; acc = 0; op_st = identity; init = True;
   } 
  virtual void redraw()  {
    char st[33]; 
    switch (mode) {
      case 0: 
      case 1: sprintf(st, (base == 10) ? "%20d" : "0x%18X",value); break;
      case 2: unsigned n = 32, x = value; // unsigned !!
	memset(st,' ',n); st[n] = 0; 
        do { st[--n] = (x & 1) ? '1' : '0'; x >>= 1; } while (x); 	
    }
    clear(); PlaceText(st); 
  }
  virtual void enter(char digit) { 
    if (init) { init = False; value = 0; }
    value = base*value + digit;
    redraw(); 
  }
  void ce() { value = 0; redraw(); }
  void ca() { value = 0; acc = 0; op_st = identity; redraw(); }
  void operate (OP opf) { // for operations
    (*op_st)(acc,value); value = acc; redraw();
    op_st = opf; init = True; // enter
  } 
};

acc_disp *disp;

class mode_button : public button {
public:
  mode_button(window & parent, int w, int h, int x, int y) :
  button(parent,modestr[mode],w,h,x,y) { }
  
  virtual void BPress_1_CB(XButtonEvent) { 
    mode++; mode %= 3; base = (mode == 0) ? 10 : 16;
    Name = modestr[mode]; 
    redraw(); disp->redraw();
  }
};

class digit_butt : public button {
  char digit;
  char nstr[2];
 public:
  digit_butt(window & parent, char dig, int w, int h, int x, int y) :
    button(parent,"",w,h,x,y) { 
      digit = dig; sprintf(nstr,"%X",digit);
      Name = nstr; 
    }
  virtual void BPress_1_CB(XButtonEvent) { 
    if (mode > 0 || digit < 10 ) disp->enter(digit); 
    else XBell(display,-80);
  }
};

static void plus(int &x, int y) { x += y; }
static void minus(int &x, int y) { x -= y; }
static void mult(int &x, int y) { x *= y; }
static void divi(int &x, int y) { if (y == 0) y = 1; x /= y; }
static void divq(int &x, int y) { if (y == 0) y = 1; x %= y; }
static void eq(int &x, int y) { x = y; }

static void bit_and(int &x, int y) { x &= y; }
static void bit_or(int &x, int y) { x |= y; }
static void bit_xor(int &x, int y) { x ^= y; }
static void shl(int &x, int y) { x <<= y; }
static void shr(int &x, int y) { x >>= y; }

struct op_struct {char *Name; OP opf; };

op_struct arith[] = { {"+",plus}, {"-",minus}, {"*",mult}, 
		       {"/",divi}, {"%",divq}, {"=",eq} };

op_struct bitops[] = { {"&",bit_and }, {"|",bit_or   }, { "^", bit_xor},
		       {"<<",shl}, {">>",shr} };

class operator_button : public button {
  op_struct *opp;
public:
  operator_button(window & parent, op_struct *op, int w, int h, int x, int y) :
  button(parent,op->Name,w,h,x,y) { opp = op; }
  
  virtual void BPress_1_CB(XButtonEvent) {  
    disp->operate(opp->opf);
  }
};

#include "calc.icon"

// derive to handle keypress events !
class calc_main : public main_window { 
public:
  calc_main(char *name, int w, int h) : main_window(name,w,h) { 
    set_icon(icon_bits,icon_width,icon_height);
    selection_mask |= KeyPressMask;
  }
  virtual void KeyPress_CB(XKeyEvent ev) {
    KeySym keysym = XLookupKeysym(&ev,ev.state & 1);
    if (isxdigit(keysym & 0xff)) { // 0..F
      char c2[] = { keysym, 0}; // keysym convert to string
      disp->enter(strtol(c2,0,16)); // and then to int
    } else 
      if (keysym == 0xff0d) disp->operate(eq); // return -> =
      else
	for (int op=0; op < 6; op++) { // operators +-*/=%
	  op_struct *opp = arith+op;
	  if (keysym == (unsigned) opp->Name[0]) disp->operate(opp->opf);
	}   
  }
};

main(int , char *argv[]) 
{ 
  main_window *mainw = new calc_main(argv[0], 220, 300); 
  disp = new acc_disp(*mainw,200,20,10,10);

  // first column of operators
  new mode_button(*mainw,30,20,10,40); 
  new instance_button <acc_disp> 
                    (*mainw,"CA",&acc_disp::ca,disp,30,20,10,220);
  new instance_button <acc_disp> 
                    (*mainw,"CE",&acc_disp::ce,disp,30,20,10,250);
  digit_butt *db[16];
  int x = 50, y = 250, dig; 

  for (dig = 0; dig < 0x10; dig++) {
    db[dig] = new digit_butt(*mainw,dig,20,20,x,y); x+= 30;
    if (dig % 3 == 0) { x = 50; y -= 30; }
  }

  // colum with arithmetics
  new quit_button(*mainw,30,20, 145, 40);
  y = 70; int op;
  for (op=0; op < 6; op++) {
    new operator_button(*mainw,&arith[op],30,20,145,y); y+= 30;
  }  
  y = 70; 
  for (op=0; op < 5; op++) {
    new operator_button(*mainw,&bitops[op],30,20,180,y); y+= 30;
  }

  mainw->main_loop();
}
