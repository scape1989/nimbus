#ifndef __JOB_IMPL__ 
#define __JOB_IMPL__

#include "shared/nimbus.h"
using nimbus::Application;
using nimbus::Data;
using nimbus::Job;

class Main : public Job {
  public:
    Main(Application* app);
    virtual void Execute(Parameter params, const DataArray& da);
    virtual Job* Clone();
};

class Initialization : public Job {
 public:
  Initialization(Application* app); 
  virtual void Execute(Parameter params, const DataArray& da);
  virtual Job* Clone();
};

class SpawnOneIterationIfNeeded : public Job {
 public:
  SpawnOneIterationIfNeeded(Application* app);
  virtual void Execute(Parameter params, const DataArray& da);
  virtual Job* Clone();
};

class OneIteration : public Job {
 public:
  OneIteration(Application* app); 
  virtual void Execute(Parameter params, const DataArray& da);
  virtual Job* Clone();
};

class Finish : public Job {
 public:
  Finish(Application* app); 
  virtual void Execute(Parameter params, const DataArray& da);
  virtual Job* Clone();
};

#endif
