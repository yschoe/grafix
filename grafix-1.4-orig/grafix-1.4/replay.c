// replay.c
//     wolf 7/95
// C++-main program with grafix-display to show stored lattice animations
// (video files)
// some arrays of one file may be visited simultaniously (spawn button)
// also different files may be visited synchron (synchron button) :
// (the program may start another image (with diff. video file) that
//  runs synchronously with the master via a pipe)
// programmer attn: the started image must not use the scrollbars !

#include "animator.h"
#include "files.h"
#include <unistd.h>

// can serve either as master or slave 
class replayer : public player, public stepper, public master, public slave {
  Bool top_proc;
  int istep_act;
  int itime; // global time in seconds , read from video file
  char *prg_name; // got from argv[0]
  play_scrollbar *pl_scrb;
public:
  replayer(animator *animp, char *argv0, Bool top = False) :
  player(animp,&itime,top), master(), slave(), prg_name(argv0) {   
    init_lat();
    top_proc = top;
    if (! top) step_mode = polling_mode = True; 
    // I am a process driven from the pipe
    seek_step(0);

    if (top)  // a scrollbar only on the top window 
      pl_scrb = new play_scrollbar(*this, width, 0, 0, 11, this, 0, 
				   animp->nsteps, 0);
  }
  
  // the 2 members for stepper : must be redefined !!
  virtual void seek_step(float istep) { 
    istep_act = irint(istep); // for start_synchron
    make_step(istep_act);
  } 
  
  VVP stepfn; void *stepptr; 
  // the stepfn is passed as parameter of play_mode and invoked from 
  // the polling_handler

  virtual Bool switch_play_mode(VVP step, void *vp) {
    step_mode = ! step_mode; pmset();
    stepfn = step; stepptr = vp;
    return(step_mode);
  } 
  
  virtual void step_handler() {  
    if (top_proc) { 
      if (animp->eof) { stop(); return; } else (*stepfn)(stepptr); }
    else pstep();
  }
  
  // virtual fn for slaves
  virtual int make_step(int istep) { // read current record and display it
    animp->seek(istep);
    if (animp->eof) return 1; 
    int it = animp->read_step(); 
    if (it > 0) itime = it; // else read error or eof
    if (animp->eof) return 1; 

    static Bool reinit = False; // first call is init
    if (reinit) 
      lat->redraw_spawned(); 
    else reinit = True;  // second call is reinit
    synchronize(istep); // master fn
    return 0;
  }
  
  // create a new Unix-process running the same program with selected file
  // ( with argument -pipe -> without the play buttons)
  // but synchronized with the current window display via a pipe & signals
  void start_command() { 
    char *defname = "*.video"; char fina[100];
    if (spipe) return; // only one possible
    FILE *fload = open_file_interactive(defname,fina,MODE_READ);
    if (fload == NULL) return; // aborted
    fclose(fload);

    char path[200]; // get the path of the current prog.
    // start the new process as pipe with command -pipe
    if (prg_name[0] == '/')  // absolute path beginning with /
      strcpy(path,prg_name);
    else {
      getcwd(path,200); // concat pwd + path
      sprintf(path+strlen(path),"/%s",prg_name);
    }
    printf("%s\n",path);

    char command[200]; sprintf(command,"%s %s -pipe",path,fina);
    start_synchron(command, istep_act); // master fn
  }
  
  void load_file() {   
    // it only works, if the old and new video file have the same 
    // number of arrays !!
    char *defname = "*.video"; char *fina = animp->wina;
    FILE *fload = open_file_interactive(defname,fina,MODE_READ);
    if (fload == NULL) return; 
    fclose(animp->fvideo); 
    animp->init_read(fload, False);

    pl_scrb->adapt(animp->nsteps, 0);
    seek_step(istep_act);
    XStoreName(display,mainw->Win,fina); // set WM name
  }
};

// nearly the quit_button class, only other string
class close_button : public button { 
public: 
  close_button(menu_bar & parent) : button(parent, "close") {}
private:
  void BPress_1_CB(XButtonEvent) { mainw->exit_flag = TRUE; }
};

// extern "C" char *getwd(char*); never use it under Linux !!!!
//  needs more than 1000 bytes !!!! -> horrible errors !!!

char *replay_help[] = {
  "'replay' is used to view the saved '*.video' files from an integration run",
  "of 'three-dim' or similiar programs.","",
  "'rotate' lets the view rotate around z-axis, to stop press it again",
  "To switch between arrays to display use the pulldown menues (v) lefthand.",
  "'spawn' creates a new window to enable simultaneous viewing of several arrays",
  "'file' loads a new video file (must have the same header size !) into the view",
  "'synchron' starts the program as a new process to visit another file",
  "synchronously with the current one.","",
  "The top scrollbar controls the time selection, setting the slider (with left",
  "mouse button) jumps immediately to the selected record.",
  "The buttons '>' and '>>' enable stepwise and continuous viewing",
  "(to stop press button '>>' <-> '||' again)",
  "'<' goes stepwise back, '<<' resets to zero",0 };

int main(int argc, char *argv[]) {

  // if argument is given : use it as filename !
  char *name = (argc > 1) ? argv[1] : "three-dim";
  char *full_name;
  if (strchr(name,'.')) full_name = name; 
  else { // if name contains no "." append ".video" 
    full_name = new char[strlen(name) + 10];
    sprintf(full_name,"%s.video",name);
  } 
  animator *animp = new animator(); 
  animp->init_read(full_name, True); 
  Bool top_proc = True;
  if (argc > 2 && strcmp(argv[2],"-pipe") == 0) top_proc = False; 
  // printf("new replayer %s %x %d\n", name, animp, itime);
  replayer *rep = new replayer(animp, argv[0], top_proc);
  menu_bar *mb = rep->mb; 
  new instance_button <play_main> (*mb," spawn ",rep->spawn,rep);
  if (top_proc) { // only the top process gets these buttons
    new instance_button <replayer>(*mb," file ",rep->load_file,rep);
    new instance_button <replayer> (*mb,"synchron",rep->start_command,rep);
    new help_button (*mb," help ",replay_help);
    new quit_button(*mb);
  } else {
    new close_button(*mb);
  }

  rep->main_loop();
  rep->cleanup();
}

