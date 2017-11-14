/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2011-2016 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#ifndef __PLUMED_core_PlumedMain_h
#define __PLUMED_core_PlumedMain_h

#include "WithCmd.h"
#include <cstdio>
#include <string>
#include <vector>
#include <set>
#include <map>


// !!!!!!!!!!!!!!!!!!!!!!    DANGER   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11
// THE FOLLOWING ARE DEFINITIONS WHICH ARE NECESSARY FOR DYNAMIC LOADING OF THE PLUMED KERNEL:
// This section should be consistent with the Plumed.h file.
// Since the Plumed.h file may be included in host MD codes, **NEVER** MODIFY THE CODE DOWN HERE

/* Generic function pointer */
typedef void (*plumed_function_pointer)(void);

/* Holder for function pointer */
typedef struct {
  plumed_function_pointer p;
} plumed_function_holder;

// END OF DANGER
////////////////////////////////////////////////////////////

namespace PLMD {



class ActionAtomistic;
class ActionPilot;
class Log;
class Atoms;
class ActionSet;
class DLLoader;
class Communicator;
class Stopwatch;
class Citations;
class ExchangePatterns;
class FileBase;

/**
Main plumed object.
In MD engines this object is not manipulated directly but it is wrapped in
plumed or PLMD::Plumed objects. Its main method is cmd(),
which defines completely the external plumed interface.
It does not contain any static data.
*/
class PlumedMain:
  public WithCmd
{
public:
/// Communicator for plumed.
/// Includes all the processors used by plumed.
  Communicator&comm;
  Communicator&multi_sim_comm;

private:
  DLLoader& dlloader;

  WithCmd* cltool;
  Stopwatch& stopwatch;
  WithCmd* grex;
/// Flag to avoid double initialization
  bool  initialized;
/// Name of MD engine
  std::string MDEngine;
/// Log stream
  Log& log;
/// tools/Citations.holder
  Citations& citations;

/// Present step number.
  long int step;

/// Condition for plumed to be active.
/// At every step, PlumedMain is checking if there are Action's requiring some work.
/// If at least one Action requires some work, this variable is set to true.
  bool active;

/// Name of the input file
  std::string plumedDat;

/// Object containing information about atoms (such as positions,...).
  Atoms&    atoms;           // atomic coordinates

/// Set of actions found in plumed.dat file
  ActionSet& actionSet;

/// Set of Pilot actions.
/// These are the action the, if they are Pilot::onStep(), can trigger execution
  std::vector<ActionPilot*> pilots;

/// Suffix string for file opening, useful for multiple simulations in the same directory
  std::string suffix;

/// The total bias (=total energy of the restraints)
  double bias;

/// The total work.
/// This computed by accumulating the change in external potentials.
  double work;

/// ***CHANGED***
/// The bias centre.
/// This returns the bias centre
  double at;

/// Class of possible exchange patterns, used for BIASEXCHANGE but also for future parallel tempering
  ExchangePatterns& exchangePatterns;

/// Set to true if on an exchange step
  bool exchangeStep;

/// Flag for restart
  bool restart;

  std::set<FileBase*> files;
  typedef std::set<FileBase*>::iterator files_iterator;

/// Stuff to make plumed stop the MD code cleanly
  int* stopFlag;
  bool stopNow;

public:
/// Flag to switch off virial calculation (for debug and MD codes with no barostat)
  bool novirial;

/// Flag to switch on detailed timers
  bool detailedTimers;

/// Add a citation, returning a string containing the reference number, something like "[10]"
  std::string cite(const std::string&);

/// word list command
  std::map<std::string, int> word_map;

/// Get number of threads that can be used by openmp
  unsigned getNumThreads()const;

/// Get a reasonable number of threads so as to access to an array of size s located at x
  template<typename T>
  unsigned getGoodNumThreads(const T*x,unsigned s)const;

/// Get a reasonable number of threads so as to access to vector v;
  template<typename T>
  unsigned getGoodNumThreads(const std::vector<T> & v)const;

public:
  PlumedMain();
// this is to access to WithCmd versions of cmd (allowing overloading of a virtual method)
  using WithCmd::cmd;
/**
 cmd method, accessible with standard Plumed.h interface.
 \param key The name of the command to be executed.
 \param val The argument of the command to be executed.
 It is called as plumed_cmd() or as PLMD::Plumed::cmd()
 It is the interpreter for plumed commands. It basically contains the definition of the plumed interface.
 If you want to add a new functionality to the interface between plumed
 and an MD engine, this is the right place
 Notice that this interface should always keep retro-compatibility
*/
  void cmd(const std::string&key,void*val=NULL);
  ~PlumedMain();
/**
  Read an input file.
  \param str name of the file
*/
  void readInputFile(std::string str);
/**
  Read an input string.
  \param str name of the string
*/
  void readInputWords(const std::vector<std::string> &  str);

/**
  Initialize the object.
  Should be called once.
*/
  void init();
/**
  Prepare the calculation.
  Here it is checked which are the active Actions and communication of the relevant atoms is initiated.
  Shortcut for prepareDependencies() + shareData()
*/
  void prepareCalc();
/**
  Prepare the list of active Actions and needed atoms.
  Scan the Actions to see which are active and which are not, so as to prepare a list of
  the atoms needed at this step.
*/
  void prepareDependencies();
/**
  Share the needed atoms.
  In asynchronous implementations, this method sends the required atoms to all the plumed processes,
  without waiting for the communication to complete.
*/
  void shareData();
/**
  Perform the calculation.
  Shortcut for waitData() + justCalculate() + justApply()
*/
  void performCalc();
/**
  Complete PLUMED calculation.
  Shortcut for prepareCalc() + performCalc()
*/
  void calc();
/**
  Scatters the needed atoms.
  In asynchronous implementations, this method waits for the communications started in shareData()
  to be completed. Otherwise, just send around needed atoms.
*/
  void waitData();
/**
  Perform the forward loop on active actions.
*/
  void justCalculate();

/// ***CHANGED***
  void setAt(void*);
  double getAt();
  double getCurrentAng();

/**
  Perform the backward loop on active actions.
  Needed to apply the forces back.
*/
  void justApply();
/**
  If there are calculations that need to be done at the very end of the calculations this
  makes sures they are done
*/
  void runJobsAtEndOfCalculation();
/// Reference to atoms object
  Atoms& getAtoms();
/// Reference to the list of Action's
  const ActionSet & getActionSet()const;
/// Referenge to the log stream
  Log & getLog();
/// Return the number of the step
  long int getStep()const{return step;}
/// Stop the run
  void exit(int c=0);
/// Load a shared library
  void load(const std::string&);
/// Get the suffix string
  const std::string & getSuffix()const;
/// Set the suffix string
  void setSuffix(const std::string&);
/// get the value of the bias
  double getBias()const;
/// get the value of the work
  double getWork()const;
/// Opens a file.
/// Similar to plain fopen, but, if it finds an error in opening the file, it also tries with
/// path+suffix.  This trick is useful for multiple replica simulations.
  FILE* fopen(const char *path, const char *mode);
/// Closes a file opened with PlumedMain::fopen()
  int fclose(FILE*fp);
/// Insert a file
  void insertFile(FileBase&);
/// Erase a file
  void eraseFile(FileBase&);
/// Flush all files
  void fflush();
/// Check if restarting
  bool getRestart()const;
/// Set restart flag
  void setRestart(bool f){restart=f;}
/// Set exchangeStep flag
  void setExchangeStep(bool f);
/// Get exchangeStep flag
  bool getExchangeStep()const;
/// Stop the calculation cleanly (both the MD code and plumed)
  void stop();
/// Enforce active flag.
/// This is a (bit dirty) hack to solve a bug. When there is no active ActionPilot,
/// several shortcuts are used. However, these shortcuts can block GREX module.
/// This function allows to enforce active plumed when doing exchanges,
/// thus fixing the bug.
  void resetActive(bool active);

/// Access to exchange patterns
  ExchangePatterns& getExchangePatterns(){return exchangePatterns;}
};

/////
// FAST INLINE METHODS:

inline
const ActionSet & PlumedMain::getActionSet()const{
  return actionSet;
}

inline
Atoms& PlumedMain::getAtoms(){
  return atoms;
}

inline
const std::string & PlumedMain::getSuffix()const{
  return suffix;
}

inline
void PlumedMain::setSuffix(const std::string&s){
  suffix=s;
}

inline
bool PlumedMain::getRestart()const{
  return restart;
}

inline
void PlumedMain::setExchangeStep(bool s){
  exchangeStep=s;
}

inline
bool PlumedMain::getExchangeStep()const{
  return exchangeStep;
}

inline
void PlumedMain::resetActive(bool active){
  this->active=active;
}

}

#endif
