// animator.C
//     wolf   5/95
// class to store time sequences of grid values (nx*ny) in a file 
// and to enable animated playback of them (program "replay.C")
// the arrays are arranged in separate columns, which are connected
// with menu-buttons

#include "animator.h"
#include "files.h"
#include <sys/types.h>
#include <sys/stat.h>

animator::animator() { 
  fvideo = NULL; ncols = 0;
}

animator::animator(char *name) { 
  strcpy(wina, name);
  fvideo = NULL; ncols = 0;
}

// add a column (bound to a menu button) for write ops
void animator::add_column(char *name, int nitems, d2disp *items) {
  column* colp = columns+ncols; // the actual column pointer 
  ncols++; 
  colp->name = name; colp->nitems = nitems;
  colp->items = items;
}

// create a new file-stream for fina
// write initial values into fvideo, possibly called more than once
// str is a description string, put in the first line of file
void animator::init_write(char *fina, char *str, int nxp, int nyp) {  
  nx = nxp; ny = nyp;
  if (fvideo == NULL) { 
    fvideo = fopen_test(fina); //  fopen(fina,"w"); 
    if (fvideo == NULL) exit(0);  
  }
  else fvideo = fopen(fina,"w");  // -> re-init 
  fprintf(fvideo,"%s\n%d %d %d\n",str,nx,ny,ncols);
  int ic,ii,ntot = 0;
  for (ic=0; ic < ncols; ic++) {
    column* colp = columns + ic;
    fprintf(fvideo,"%s\n%d\n",colp->name,colp->nitems);
    d2disp* itp = colp->items; 
    ntot += colp->nitems;
    for (ii=0; ii < colp->nitems; ii++) {
      fprintf(fvideo,"%s\n%g %g\n",itp[ii].name,itp[ii].gamma,itp[ii].z0);
    }
  }  
  rec_length = sizeof(int) + ntot*nx*ny*sizeof(float) ; 
  fp_old = 0;
}

void animator::write_step(int itime) { 
  // write one time step to file : the arrays are transformed to float
  // for FORTRAN they are rearranged into C ordering
  long fp = ftell(fvideo);
  if (fp_old > 0) { 
    if (fp != fp_old + rec_length) 
      printf("error write video file %d %ld\n",itime,fp-fp_old);
  } 
  fp_old = fp;
  if (fwrite(&itime,sizeof(int),1,fvideo) != 1) error("video file it"); 

  for (int ic = 0; ic < ncols; ic++) {
    column* colp = columns+ic;
    for (int ii = 0; ii < colp->nitems; ii++) {
      d2disp *cii = &(colp->items[ii]); // shortcut
      double (*fptr)[nx] = (double (*) [nx]) cii->array.darr; 
      double (*cptr)[ny] = (double (*) [ny]) cii->array.darr; 
      int array_mode = cii->array_mode;
      for (int ix=0; ix<nx; ix++) for (int iy=0; iy<ny; iy++) {
	float ff;
	if (array_mode & ARRAY_NY_NX) // swap x <-> y
	  ff = fptr[iy][ix]; else ff = cptr[ix][iy];
	if (fwrite(&ff, sizeof(float), 1, fvideo) != 1)
	  error("write video file ff"); 
      }
      // fprintf(fvideo," %g ",colp->items[ii].darr[0]); -> test
    }
  }
  fflush(fvideo);
}

// read a column description from file into colp element
// and allocate the needed arrays (items, strings) with new
// return number of items (fields)
int animator::get_column(column *colp, Bool alloc) { 
  if (alloc) colp->name = new char[51]; 
  char *cname = colp->name; 
  fgets(cname,51,fvideo); // = the menu name 
  cname[strlen(cname)-1] = 0; // suppress the newline in the string
  int nits;
  if (fscanf(fvideo,"%d%*c",&nits) != 1) error("format 'nitems'");
  // "%*c" to skip the newline after %d

  if (alloc) colp->nitems = nits; 
  else if (nits != colp->nitems) error("incompatible records");

  // printf("cname = %s\n",cname); printf("nitems = %d\n",nits);
  if (alloc) colp->items = new d2disp[nits];
  for (int ii = 0; ii < nits; ii++) {
    d2disp *cii = &(colp->items[ii]); // shortcut
    if (alloc) cii->name = new char[81]; 
    char *itn = cii->name; fgets(itn,81,fvideo);
    itn[strlen(itn)-1] = 0; // suppress the newline in the string
    if (fscanf(fvideo,"%g%g%*c",&cii->gamma,&cii->z0) 
	!= 2) error("format 'gamma z0'");
    // the place to read arrays into
    if (alloc) cii->array.farr = new float[nx*ny];
    cii->array_mode = ARRAY_FLOAT; 
    // printf("%s %s %g %g\n",cname,itn,cii->gamma,cii->z0);
  }
  return nits;
}

// read header from fp and arrange columns accordingly
// if alloc = TRUE : allocate the space (only first time)
void animator::init_read(FILE *fp, Bool alloc) {
  fvideo = fp;
 
  char dummy[200];
  fgets(dummy,200,fvideo); // not used yet
  if (fscanf(fvideo,"%d%d%d",&nx,&ny,&ncols) != 3) error("not a video file");
  printf("header = %snx = %d ny = %d ncols = %d\n",dummy,nx,ny,ncols); 
  fgets(dummy,10,fvideo); // skip the newline
  int i, ntot = 0;  
  // count all items :
  for (i=0; i < ncols; i++) ntot += get_column(columns+i, alloc);
  // columns+i = actual column pointer
  data = ftell(fvideo); // now data arrays begin
  //           the itime   +   record of ntot grids
  
  rec_length = sizeof(int) + ntot*nx*ny*sizeof(float) ; 

  fsize(); // compute nsteps
}

void animator::init_read(char *fina, Bool alloc) {
  strcpy(wina, fina);
  fvideo = fopen(wina,"r");
  if (fvideo == NULL) { // 
    char *defname = "*.video"; 
    fvideo = open_file_interactive(defname,wina,MODE_READ);
  } 
  if (fvideo == 0) exit(0);
  init_read(fvideo, alloc); 
}  

// determine step number of the video file (independently of init_read)
// for reading new files
int animator::fsize() {
  struct stat buf;
  fstat(fileno(fvideo), &buf);
  nsteps = (buf.st_size - data)/rec_length;
  printf("rec length = %ld nsteps = %d \n", rec_length,nsteps);
  return nsteps;
}


int animator::read_step() { // read sequentially one time step from file 
  int itime; 
  if (fread(&itime,sizeof(int),1,fvideo) < 1) { 
    eof = TRUE; return -1; // EOF 
  }
  int ic,ii;
  for (ic=0; ic < ncols; ic++) {
    column* colp = columns+ic;
    for (ii=0; ii < colp->nitems; ii++) 
      if (fread(colp->items[ii].array.farr, sizeof(float), nx*ny, fvideo) 
	  < (unsigned) nx*ny) return -1; // EOF
    // fprintf(fvideo," %g ",colp->items[ii].array[0]); -> test
  }
  return itime;
}

void animator::seek(int step) { 
  // set file pointer for read ops to, set eof=TRUE if it failed
  // beginning of record number "step"
  eof = (fseek(fvideo,data + step*rec_length,0) != 0); // SEEK_SET = 0
}


// make a radion_menu from a d2disp-column 
void animator::menu_from_column(window *parent, char *Name, window *action_win,
				int nitems, d2disp *items) {
  int i;
  char **sel_list = new char* [nitems+1]; 
  for (i=0; i<nitems; i++) sel_list[i] = items[i].name;
  sel_list[nitems] = 0;
  make_radio_menu(*parent, Name, sel_list, action_win);		   
  
}

// combine menu creation with saving to file (add_column)
void animator::make_saved_menu(window *parent, char *Name, window *action_win,
			       int nitems, d2disp *items) {
  menu_from_column(parent, Name, action_win, nitems, items);
  add_column(Name, nitems, items);
}

// create all menus stored in the columns (as read from the file) 
void animator::make_all_menus(window *parent, window *action_win) {
  int i;
  for (i=0; i < ncols; i++) { 
    column *col = columns+i; // printf("%s \n",col->name); 
    menu_from_column(parent, col->name, action_win, col->nitems, col->items);
  }
}

// translate menu-enries into corresponding d2disp-pointer, 
// by comparing the string addresses !
// mainly used for action methods in menus
d2disp* animator::find(char *menu, char *name) {
  int i,ic;
  for (ic=0; ic < ncols; ic++) {
    column *colp = columns+ic; 
    if (menu == colp->name) {
      d2disp *ddp = colp->items;
      for (i=0; i < colp->nitems; i++) if (name == ddp[i].name) return ddp+i;
    }
  }
  return 0; // not found
}

//************************************************************************//
// class time_lat : a lattice manager for display time sequences of arrays
// with selector menues, clock and spawning feature

time_lat::time_lat(window &parent, int w, int h, int x, int y, 
		   int nx, int ny, d2disp *dstart, int *tptr) : 
  lattice_manager(parent, w, h-20,x, y+20, nx, ny, NULL), itime(*tptr) {
    headline = new info_window(parent,w,20,x,y);
    qptr = new float[nx*ny]; 
    setsel(dstart);
    body = False;  
    spl = NULL; spawned_from = NULL;
    clck = new clock_win(*this, tptr,50,50,w-55,h-120);
  }

time_lat::~time_lat() { 
  delete qptr; 
  if (spawned_from == NULL) return;
  // inhibit redrawing initiated from the spawner of myself 
  splist *spx = spawned_from->spl; 
  // printf("delete %x\n",this);
  // instead of compilcated re-arranging the list simply set spawned = 0
  while (spx) { 
    if (spx->spawned == this) { spx->spawned = NULL; }
    spx = spx->next;
  }
}

// hook a new spawned gl into chain
void time_lat::install_spawned(time_lat *gsp) { 
  splist *sn = new splist; 
  sn->next = spl; sn->spawned = gsp;
  spl = sn; 
  gsp->spawned_from = this;  // remember my spawner for destructor !
  // printf("installing %x from %x\n",gsp,this);
  gsp->dsel = dsel; // use selected display also
  gsp->gamma = gamma; gsp->z0 = z0; // but the actual values here

}

void time_lat::redraw() { lattice_manager::redraw(); clck->redraw(); }

void time_lat::draw_interior() { 
  // make a copy from member "array" to array "qptr", depending on type 
  float (*lptr)[ny] = (float (*) [ny]) qptr; // ptr to the array to draw "qptr"

  realp array = dsel->array;
  double (*dnx)[nx] = (double (*) [nx]) array.darr; // 4 accessors to "array"
  double (*dny)[ny] = (double (*) [ny]) array.darr; // for different precis.
  float (*fnx)[nx] = (float (*) [nx]) array.farr;   // and orderings
  float (*fny)[ny] = (float (*) [ny]) array.farr; 
  int array_mode = dsel->array_mode;
  int x,y; 
  for (x=0; x<nx; x++)  for (y=0; y<ny; y++) {
    // !! indexing of array[ny][nx] !! 
    if (array_mode & ARRAY_NY_NX) 
      // indexing of array = [ny][nx] -> reverse
      lptr[x][y] = (array_mode & ARRAY_DOUBLE) ? dnx[y][x] : fnx[y][x];
    else //  indexing of array = [nx][ny] 
      lptr[x][y] = (array_mode & ARRAY_DOUBLE) ? dny[x][y] : fny[x][y];
    
  } 
  // old: for (i=0; i<nx*ny; i++) qptr[i] = array[i];
  // printf("draw_int %s %g %g %g\n",dsel->name,gamma,z0,lptr[0][0]);

  sprintf(headline->info,"%s t= %ds %d:%02dh",dsel->name, itime,
	  itime/3600, (itime % 3600)/60);
  headline->redraw();
  lattice_manager::draw_interior();
}

// redraws all spawned windows; parsing the spl list
void time_lat::redraw_spawned() {
  redraw(); // first redraw myself
  splist *spx = spl;
  while (spx) { 
    time_lat *gsp = spx->spawned; if (gsp) gsp->redraw(); 
    spx = spx->next; 
  }
}

void time_lat::setsel(d2disp *ddp) {
  gamma = ddp->gamma; z0 = ddp->z0;
  dsel = ddp;
  // printf("setsel %g %g %x\n",gamma,z0,ddp->array.darr);
}

/**************************************************************************/
// play_lat : a time_lat with animator and action 
// used for replay and integrators

play_lat::play_lat(window &parent, int w, int h, int x, int y, 
		   animator *animp, int *tptr) : animp(animp),
  time_lat(parent,w,h,x,y,animp->nx,animp->ny,animp->columns[0].items, tptr)
{ } // start with first array of column 0

void play_lat::action(char * menu, char * value) { 
  d2disp *ddp = animp->find(menu,value);
  setsel(ddp);
  redraw();
}

/******************** play_main *****************************/
// a main window with menubar and play_lat-window 
// it is bound to an animator, which columns are displayable
// a spawn-method can be bound to a button which spawns a new play_main

// initialize lattice and menues from animator, must be called after constr.
void play_main::init_lat() {
  lat = new play_lat(*this, width, wh, 0, ly, animp, tptr); 
  animp->make_all_menus(mb, lat); 
}

// if top = True : reserve additional place for scrollbar on top
// and place the window in centre of screen
play_main::play_main(animator *animp, int *tptr, Bool centre, Bool top) 
: main_window(animp->wina, 540, 520 + ((top) ? 40 : 0), (centre) ? 2 : 0), 
  tptr(tptr), animp(animp) {
    // printf("play %s\n",animp->wina);
    bh = 20; sh = 40; by = 0;
    wh = height-bh; ly = bh; 
    if (top) { wh -= sh; ly+= sh; by += sh; }
    mb = new menu_bar(*this,width,bh,0,by,40,100,0);
  }

void play_main::spawn() {
  play_main *rsm = new play_main(animp,tptr,False);
  rsm->init_lat();
  lat->install_spawned(rsm->lat); // hook into redraw chain
  new delete_button(*(rsm->mb),rsm);
  rsm->RealizeChildren();   
  rsm->lat->redraw();
}

/************************************************************************/
// class player has two handlers to switch
// between play and stop; to be used as primer window
player::player(animator *animp, int *tptr, Bool top) :
  play_main(animp,tptr,True,top) { // to be centred
    new instance_button <player> (*mb,"rotate",&player::rotate, this);
    rot_mode = False; step_mode = False;
}

void player::play() { step_mode = True; pmset(); }
void player::stop() { step_mode = False; pmset(); }

void player::rotate() { rot_mode = ! rot_mode; pmset(); }

void player::polling_handler() {
  if (step_mode) step_handler(); // the virtual handler of derived classes
  if (rot_mode) lat->rotate_alpha(0.05); // rotate the lattice for some 3 grad
}    
/************************************************************************/
// class integrator : general purpose interface to invoke any program
// that solves time step integrations
// the members init_solver, step_solver, exit_solver must be defined by app
integrator::integrator(char *name, char *video_file, int nx, int ny, int animt)
  : video_file(video_file), animt(animt), nx(nx), ny(ny), anim(name), 
  player(&anim, &itime) { 
    anim.nx = nx; anim.ny = ny;
    XStoreName(display,Win,name); // must be set again 
    // because of initialization order : player is always first initialized !
    header = new char[200]; header[0] = 0; 
}

void integrator::reinit() { 
  init_solver(); 
  static Bool reinit = False; // first call is init
  if (reinit) 
    lat->redraw_spawned(); 
  else reinit = True;  // second call is reinit
  anim.init_write(video_file,header,nx,ny); 
}

void integrator::start() {
  reinit();
  init_lat(); // must be done here !
  // add buttons unique to primer window
  new instance_button <integrator> (*mb,"reinit",this->reinit, this);
  new instance_button <integrator> (*mb,">",&integrator::step_handler,this);
  new switch_button(*mb,">>","||",&step_mode, (VVP) &player::pmset, this);
  new instance_button <play_main> (*mb,"spawn",this->spawn,this);
  new quit_button(*mb);
    
  main_loop();
  exit_solver();
}
void integrator::step_handler() {  
  int itold = itime;
  step_solver();   
  lat->redraw_spawned();
  // save results in video file every "animt" seconds
  if (itime / animt != itold / animt) anim.write_step(itime);
 
}

void integrator::add_column(char *menu, int nitems, d2disp *items) {
  anim.add_column(menu, nitems, items); 
}

/************************************************************************/
// classes to make up a synchronous driven pipe 
// the "master" creates a pipe which should be given a command with "...-pipe"
// the "slave" process then polls from stdin integer indexes for which it 
// invokes "make_step" and sends an USR1 signal upon completion to wake up 
// the master
// istep = -1 is used to terminate the slave process
 
#ifdef SPARC
extern "C" {
  int sigpause(int);
  int bzero(...); // used from FD_ZERO
  int select(...);
}
#endif

#include <signal.h>
#include <sys/time.h>

static void sig_usr1(int) {  // used to wait on signal USR1 
  signal(SIGUSR1, sig_usr1); //  printf("child interrupt\n");
}

void master::start_synchron(char *command, int istep) {
  if (spipe) return; // only one possible 
  spipe = popen(command,"w");
  signal(SIGPIPE,sig_pipe);  
  signal(SIGUSR1,sig_usr1); // used for wait with sigpause
  synchronize(istep); // startup index
}
void master::synchronize(int istep) {
  if (spipe) { // send actual timestep through pipe to child process
    // printf(" send step %d\n",istep);
    fprintf(spipe,"%d \n",istep); fflush(spipe);
    if (spipe) sigpause(0); // wait for signal from child
  }
}
  
void master::cleanup() {
  if (spipe) { // for termination of spawned processes
    fprintf(spipe," -1 "); pclose(spipe); 
  }
}

FILE* master::spipe; // definition needed

slave::slave() {
  signal(SIGUSR1,sig_usr1); // I may receive it myself
}

// read from stdin (pipe) the step index and display it
void slave::pstep() { // used as poll handler for synchronized processes
  fd_set readfd;
  struct timeval timeout;
  timeout.tv_sec = 0; timeout.tv_usec = 100000; // 100 msec 
  // in readfd sind die Bits von stdin gesetzt
  FD_ZERO(&readfd); FD_SET(0,&readfd);  
  // select returns either if stdin has input, or timeout happens

#ifdef HP
  int sel = select(8, (int*) &readfd, NULL, NULL , &timeout);
#else
  int sel = select(8, &readfd, NULL, NULL , &timeout);
#endif 
  if (sel == 0) return; // timeout

  int istep; 
  scanf("%d",&istep);
  // printf(" receive step = %i\n",istep);
  if (istep == -1) exit(0); // termination from spawner
  make_step(istep);
  kill(0,SIGUSR1); // awake parent process from sigpause !
}

