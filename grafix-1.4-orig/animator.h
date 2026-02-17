// animator.h
//     wolf   5/95
// class to store time sequences of grid values (nx*ny) in a file 
// and to enable animated playback of them (program "replay.c")
// the arrays are arranged in separate columns, which are connected
// with menu-buttons for the lattice class

#include "lattice.h"

// a displayable array with description fields, 
// darr and farr are used exclusively : "darr" for compuation, 
// "farr" for write/read and display of the saved arrays
// gamma and z0 are only for display fitting

union realp { double *darr; float *farr; };
struct d2disp { char *name; union realp array; float gamma,z0;
		int array_mode;
	      };

// a column connected with a vector of items, if write = True save in video
struct column { char *name; int nitems; Bool write; d2disp *items;  };

// modes for array indexing (array_mode) = 0..3 ( a | b)
#define ARRAY_NY_NX  1 // esp. for FORTRAN programs
#define ARRAY_NX_NY  0
#define ARRAY_DOUBLE 2 // the used precision for arrays in the hooked program
#define ARRAY_FLOAT  0

// at creation time use the call sequence: 
//    1. constructor, 2. add_column... , 3. init_write, 4. write_step ...
// at read time :
//    1. constructor, 2. init_read, 3. read_step ...
class animator {
  long data;   // the start position of data in the file 
  long rec_length; // record length (in bytes), computed from read_step !!
  long fp_old; // last file pointer (for write error checking only)
public:
  unsigned nsteps; // number of steps saved in the file (read)
  FILE *fvideo;   // the file stream
  char wina[200]; // its filename (for window WM name)
  Bool eof; // for reading only (beyond end of file)
  int ncols, nx, ny;
  column columns[10]; // max 10 columns
  animator();
  animator(char *name);

  // add a column (bound to a menu button) for write ops
  void add_column(char *name, int nitems, d2disp *items);

  // create a new file-stream for fina
  // write initial values into fvideo, possibly called more than once
  void init_write(char *fina, char *str, int nxp, int nyp);
      
  // write one time step to file : the arrays are transformed to float
  // for FORTRAN they are rearranged into C ordering, ie NX_NY
  void write_step(int itime);
 
  // read a column description from file
  int get_column(column *colp, Bool alloc);

  void init_read(char *fina, Bool alloc);
  void init_read(FILE *fp, Bool alloc);

  int fsize(); // determine step number of the video file

  // read sequentially one time step from file 
  int read_step();
  
  // set file pointer for read ops to beginning of record number "step"
  // set eof if an error occurs
  void seek(int step);
  
  // make a radion_menu from a d2disp-column 
  void menu_from_column(window *parent, char *Name, window *action_win,
			int nitems, d2disp *items);

  // combine menu creation with saving to file (add_column)
  void make_saved_menu(window *parent, char *Name, window *action_win,
		       int nitems, d2disp *items);
  
  // create all menus stored in the columns (as read from the file) 
  void make_all_menus(window *parent, window *action_win);

  // translate menu-enries into corresponding d2disp-pointer, 
  // by comparing the string addresses !
  // mainly used for action methods in menus
  d2disp *find(char *menu, char *name);

};

// class time_lat : a lattice manager for display time sequences of arrays
// with selector menus, clock and spawning feature
// the "action" method must be defined in derived classes (see replay_lat) !
class time_lat : public lattice_manager {
  info_window *headline;
  clock_win *clck;
  int& itime;
  d2disp *dsel; // actually selected display descriptor (array,name,gamma,...)
public:

  time_lat *spawned_from;
  struct splist { time_lat *spawned; splist *next;} *spl; 

  time_lat(window &parent, int w, int h, int x, int y, int nx, int ny, 
	   d2disp *dstart, int *tptr);
  ~time_lat();
  void install_spawned(time_lat *gsp); // hook a new spawned gl into chain

  virtual void redraw();
  virtual void draw_interior(); // recompute array do draw from member "array"
  void redraw_spawned(); // redraws all spawned windows; parsing the spl list
  void setsel(d2disp *ddp);
  virtual void action(char * menu, char * value) = 0;
};

// play_lat : a time_lat with animator and action 
// used for replay and integrators
class play_lat : public time_lat {
  animator *animp;
public:
  play_lat(window &parent, int w, int h, int x, int y, 
	     animator *animp, int *tptr);
  virtual void action(char * menu, char * value);
};

// a main window with menubar and play_lat-window 
// it is bound to an animator, which columns are displayable
// a spawn-method can be bound to a button which spawns a new play_main
class play_main : public main_window {
  int *tptr; 
  int wh, ly, bh, sh, by; // geometry parameters of windows
protected:
  animator *animp;
public:
  menu_bar *mb;
  play_lat *lat;    
  void spawn();
  // if top = True : reserve additional place for scrollbar on top
  play_main(animator *animp, int *tptr, Bool centre = False, Bool top = False);
  void init_lat(); // must be called after animator generation !!
};

// class player has two handlers to switch
// between play and stop; to be used as primer window
class player : public play_main {
protected:
  Bool rot_mode;  // both modes work independently, they switch on a
  Bool step_mode; // special polling handler 
  void pmset() { polling_mode = rot_mode || step_mode; }
public:
  player(animator *animp, int *tptr, Bool top = False);
  void play();
  void stop(); // to toggle step_mode;

private:
  virtual void step_handler() {} // the handler invoked for step_mode
  virtual void polling_handler();
  void rotate(); // button callback to toggle rot_mode
};

// class integrator : general purpose interface to invoke any program
// that solves time step integrations, creates an animator (and video file)
// the members init_solver, step_solver, exit_solver must be defined by app
class integrator : public player {
  char *video_file;
  int animt; // number of seconds for video file time step 
protected:
  char *header; // to be written in the video file
  int nx,ny;
public: 
  animator anim;
  int itime; 

  integrator(char *name, char *video_file, int nx, int ny, int animt);

  virtual void init_solver() = 0;
  virtual void step_solver() = 0;
  virtual void exit_solver() = 0;

  void reinit();
  void start();
  virtual void step_handler();
  
  void add_column(char *menu, int nitems, d2disp *items);
};

// classes to make up a synchronous driven pipe 
// the "master" creates a pipe which should be given a command with "...-pipe"
// the "slave" process then polls from stdin integer indexes for which it 
// invokes "make_step" and sends an USR1 signal upon completion to wake up 
// the master
// istep = -1 is used to terminate the slave process

class master {
protected:
  static FILE *spipe; // only one per process !!
  static void sig_pipe(int) { spipe = 0; } // delete on signal SIGPIPE
public:
  master() { spipe = 0; }
  ~master() { spipe = 0; }
  void start_synchron(char *command, int istep);
  void synchronize(int istep);
  void cleanup();
};

class slave {
public:
  slave();
  virtual int make_step(int i) = 0; // actually perform time step 
  void pstep(); // used as poll handler for synchronized processes
};
