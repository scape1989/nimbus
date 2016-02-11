/*
 * Copyright 2013 Stanford University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither the name of the copyright holders nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file contains a loop iteration job that spawns the sub step jobs to
 * calculate the current frame. It keeps spawning the iteration in a loop as
 * long as frame computation in not complete. When the frame is done it will
 * spawn the loop frame job for the next frame. The granularity of the sub step
 * jobs could be controlled by changing the changing the GRANULARITY_STATE
 * macro in this file as follows:
 * ONE_JOB:              calculate the frame iteration in one job (like water coarse).
 * SUPER_JOBS:           break the frame iteration in to three super jobs.
 * BREAK_SUPER_JOB_1:    further break super job 1 in to its components.
 * BREAK_SUPER_JOB_2:    further break super job 2 in to its components.
 * BREAK_SUPER_JOB_3:    further break super job 3 in to its components.
 * BREAK_ALL_SUPER_JOBS: break all three super jobs in to their components.
 *
 * Author: Omid Mashayekhi <omidm@stanford.edu>
 */

#define GRANULARITY_STATE BREAK_ALL_SUPER_JOBS

#include "application/water_alternate_fine/app_utils.h"
#include "application/water_alternate_fine/data_names.h"
#include "application/water_alternate_fine/job_loop_iteration.h"
#include "application/water_alternate_fine/job_names.h"
#include "application/water_alternate_fine/physbam_utils.h"
#include "application/water_alternate_fine/water_driver.h"
#include "application/water_alternate_fine/water_example.h"
#include "application/water_alternate_fine/water_sources.h"
#include "application/water_alternate_fine/reg_def.h"
#include "shared/dbg.h"
#include "shared/nimbus.h"
#include <sstream>
#include <string>

namespace application {

  JobLoopIteration::JobLoopIteration(nimbus::Application *app) {
    set_application(app);
  };

  nimbus::Job* JobLoopIteration::Clone() {
    return new JobLoopIteration(application());
  }

  void JobLoopIteration::Execute(nimbus::Parameter params, const nimbus::DataArray& da) {
    dbg(APP_LOG, "Executing loop iteration job\n");

    // Get parameters: frame, time
    InitConfig init_config;
    std::string params_str(params.ser_data().data_ptr_raw(),
        params.ser_data().size());
    LoadParameter(params_str, &init_config.frame, &init_config.time);

    const int& frame = init_config.frame;
    const T& time = init_config.time;

    dbg(APP_LOG, "Frame %i and time %f in iteration job\n",
        frame, time);

    // Initialize the state of example and driver.
    PhysBAM::WATER_EXAMPLE<TV>* example;
    PhysBAM::WATER_DRIVER<TV>* driver;
    init_config.set_boundary_condition = true;
    DataConfig data_config;
    data_config.SetAll();
    InitializeExampleAndDriver(init_config, data_config,
                               this, da, example, driver);

    // check whether the frame is done or not
    bool done = false;
    T target_time = example->Time_At_Frame(driver->current_frame+1);
    T dt = example->cfl * example->incompressible.CFL(example->face_velocities);
    T temp_dt =
        example->particle_levelset_evolution.cfl_number
        * example->particle_levelset_evolution.particle_levelset.levelset.CFL(
            example->face_velocities);
    if (temp_dt < dt) {
      dt = temp_dt;
    }
    if (time + dt >= target_time) {
      dt = target_time - time;
      done = true;
    } else {
      if (time + 2*dt >= target_time)
        dt = .5 * (target_time-time);
    }

    dbg(APP_LOG, "Frame=%d, Time=%f, dt=%f\n", frame, time, dt);

    // spawn the jobs to compute the frame, depending on the
    // level of granularity we will have different sub jobs.
    switch (GRANULARITY_STATE) {
      case ONE_JOB:
        SpawnWithOneJobGranularity(done, frame, time, dt, da);
        break;
      case SUPER_JOBS:
        SpawnWithSuperJobsGranularity(done, frame, time, dt, da);
        break;
      case BREAK_SUPER_JOB_1:
        SpawnWithBreakSuperJob1Granularity(done, frame, time, dt, da);
        break;
      case BREAK_ALL_SUPER_JOBS:
        SpawnWithBreakAllGranularity(done, frame, time, dt, da);
        break;
      default:
        dbg(APP_LOG, "ERROR: The granularity state is not defined.");
        exit(-1);
        break;
    }

    // Free resources.
    DestroyExampleAndDriver(example, driver);
  }


  void JobLoopIteration::SpawnWithSuperJobsGranularity(
      bool done, int frame, T time, T dt, const nimbus::DataArray& da) {
    if (!done) {
      //Spawn one iteration of frame computation and then one loop
      //iteration  job, for next iteration.
      dbg(APP_LOG, "Loop frame is spawning super job 1, 2, 3 for frame %i.\n", frame);

      int job_num = 4;
      std::vector<nimbus::job_id_t> job_ids;
      GetNewJobID(&job_ids, job_num);
      nimbus::IDSet<nimbus::logical_data_id_t> read, write;
      nimbus::IDSet<nimbus::job_id_t> before, after;
      nimbus::Parameter s1_params;
      nimbus::Parameter s2_params;
      nimbus::Parameter s3_params;
      nimbus::Parameter iter_params;

      LoadReadWriteSets(this, &read, &write);

      std::string s1_str;
      SerializeParameter(frame, time, dt, &s1_str);
      s1_params.set_ser_data(SerializedData(s1_str));
      after.clear();
      after.insert(job_ids[1]);
      before.clear();
      SpawnComputeJob(SUPER_1,
          job_ids[0],
          read, write,
          before, after,
          s1_params);

      std::string s2_str;
      SerializeParameter(frame, time, dt, &s2_str);
      s2_params.set_ser_data(SerializedData(s2_str));
      after.clear();
      after.insert(job_ids[2]);
      before.clear();
      before.insert(job_ids[0]);
      SpawnComputeJob(SUPER_2,
          job_ids[1],
          read, write,
          before, after,
          s2_params);

      std::string s3_str;
      SerializeParameter(frame, time, dt, &s3_str);
      s3_params.set_ser_data(SerializedData(s3_str));
      after.clear();
      after.insert(job_ids[3]);
      before.clear();
      before.insert(job_ids[1]);
      SpawnComputeJob(SUPER_3,
          job_ids[2],
          read, write,
          before, after,
          s3_params);

      std::string iter_str;
      SerializeParameter(frame, time + dt, &iter_str);
      iter_params.set_ser_data(SerializedData(iter_str));
      after.clear();
      before.clear();
      before.insert(job_ids[2]);
      SpawnComputeJob(LOOP_ITERATION,
          job_ids[3],
          read, write,
          before, after,
          iter_params);
    } else {
      // compute one last iteration, then spawn write frame job. Finally,
      // spawn loop frame job for next frame computation.

      int job_num = 5;
      std::vector<nimbus::job_id_t> job_ids;
      GetNewJobID(&job_ids, job_num);
      nimbus::IDSet<nimbus::logical_data_id_t> read, write;
      nimbus::IDSet<nimbus::job_id_t> before, after;
      nimbus::Parameter s1_params;
      nimbus::Parameter s2_params;
      nimbus::Parameter s3_params;
      nimbus::Parameter write_params;
      nimbus::Parameter frame_params;

      LoadReadWriteSets(this, &read, &write);

      std::string s1_str;
      SerializeParameter(frame, time, dt, &s1_str);
      s1_params.set_ser_data(SerializedData(s1_str));
      after.clear();
      after.insert(job_ids[1]);
      before.clear();
      SpawnComputeJob(SUPER_1,
          job_ids[0],
          read, write,
          before, after,
          s1_params);

      std::string s2_str;
      SerializeParameter(frame, time, dt, &s2_str);
      s2_params.set_ser_data(SerializedData(s2_str));
      after.clear();
      after.insert(job_ids[2]);
      before.clear();
      before.insert(job_ids[0]);
      SpawnComputeJob(SUPER_2,
          job_ids[1],
          read, write,
          before, after,
          s2_params);

      std::string s3_str;
      SerializeParameter(frame, time, dt, &s3_str);
      s3_params.set_ser_data(SerializedData(s3_str));
      after.clear();
      after.insert(job_ids[3]);
      before.clear();
      before.insert(job_ids[1]);
      SpawnComputeJob(SUPER_3,
          job_ids[2],
          read, write,
          before, after,
          s3_params);

      std::string write_str;
      SerializeParameter(frame, time + dt, 0, &write_str);
      write_params.set_ser_data(SerializedData(write_str));
      after.clear();
      after.insert(job_ids[4]);
      before.clear();
      before.insert(job_ids[2]);
      SpawnComputeJob(WRITE_FRAME,
          job_ids[3],
          read, write,
          before, after,
          write_params);

      std::string frame_str;
      SerializeParameter(frame + 1, &frame_str);
      frame_params.set_ser_data(SerializedData(frame_str));
      after.clear();
      before.clear();
      before.insert(job_ids[3]);
      SpawnComputeJob(LOOP_FRAME,
          job_ids[4],
          read, write,
          before, after,
          frame_params);
    }
  }

  void JobLoopIteration::SpawnWithBreakSuperJob1Granularity(
      bool done, int frame, T time, T dt, const nimbus::DataArray& da) {
    if (!done) {
      //Spawn one iteration of frame computation and then one loop
      //iteration  job, for next iteration.
      dbg(APP_LOG, "Loop frame is spawning su jobs in super job 1, and super jobs 2, 3 for frame %i.\n", frame); // NOLINT

      int job_num = 9;
      std::vector<nimbus::job_id_t> job_ids;
      GetNewJobID(&job_ids, job_num);
      nimbus::IDSet<nimbus::logical_data_id_t> read, write;
      nimbus::IDSet<nimbus::job_id_t> before, after;
      nimbus::Parameter s11_params;
      nimbus::Parameter s12_params;
      nimbus::Parameter s13_params;
      nimbus::Parameter s14_params;
      nimbus::Parameter s15_params;
      nimbus::Parameter s16_params;
      nimbus::Parameter s2_params;
      nimbus::Parameter s3_params;
      nimbus::Parameter iter_params;

      LoadReadWriteSets(this, &read, &write);

      std::string s11_str;
      SerializeParameter(frame, time, dt, &s11_str);
      s11_params.set_ser_data(SerializedData(s11_str));
      before.clear();
      after.clear();
      after.insert(job_ids[1]);
      SpawnComputeJob(ADJUST_PHI_WITH_OBJECTS,
          job_ids[0],
          read, write,
          before, after,
          s11_params);

      std::string s12_str;
      SerializeParameter(frame, time, dt, &s12_str);
      s12_params.set_ser_data(SerializedData(s12_str));
      before.clear();
      before.insert(job_ids[0]);
      after.clear();
      after.insert(job_ids[2]);
      SpawnComputeJob(ADVECT_PHI,
          job_ids[1],
          read, write,
          before, after,
          s12_params);

      std::string s13_str;
      SerializeParameter(frame, time, dt, &s13_str);
      s13_params.set_ser_data(SerializedData(s13_str));
      before.clear();
      before.insert(job_ids[1]);
      after.clear();
      after.insert(job_ids[3]);
      SpawnComputeJob(STEP_PARTICLES,
          job_ids[2],
          read, write,
          before, after,
          s13_params);

      std::string s14_str;
      SerializeParameter(frame, time, dt, &s14_str);
      s14_params.set_ser_data(SerializedData(s14_str));
      before.clear();
      before.insert(job_ids[2]);
      after.clear();
      after.insert(job_ids[4]);
      SpawnComputeJob(ADVECT_REMOVED_PARTICLES,
          job_ids[3],
          read, write,
          before, after,
          s14_params);

      std::string s15_str;
      SerializeParameter(frame, time, dt, &s15_str);
      s15_params.set_ser_data(SerializedData(s15_str));
      before.clear();
      before.insert(job_ids[3]);
      after.clear();
      after.insert(job_ids[5]);
      SpawnComputeJob(ADVECT_V,
          job_ids[4],
          read, write,
          before, after,
          s15_params);

      std::string s16_str;
      SerializeParameter(frame, time, dt, &s16_str);
      s16_params.set_ser_data(SerializedData(s16_str));
      before.clear();
      before.insert(job_ids[4]);
      after.clear();
      after.insert(job_ids[6]);
      SpawnComputeJob(APPLY_FORCES,
          job_ids[5],
          read, write,
          before, after,
          s16_params);

      std::string s2_str;
      SerializeParameter(frame, time, dt, &s2_str);
      s2_params.set_ser_data(SerializedData(s2_str));
      before.clear();
      before.insert(job_ids[5]);
      after.clear();
      after.insert(job_ids[7]);
      SpawnComputeJob(SUPER_2,
          job_ids[6],
          read, write,
          before, after,
          s2_params);

      std::string s3_str;
      SerializeParameter(frame, time, dt, &s3_str);
      s3_params.set_ser_data(SerializedData(s3_str));
      before.clear();
      before.insert(job_ids[6]);
      after.clear();
      after.insert(job_ids[8]);
      SpawnComputeJob(SUPER_3,
          job_ids[7],
          read, write,
          before, after,
          s3_params);

      std::string iter_str;
      SerializeParameter(frame, time + dt, &iter_str);
      iter_params.set_ser_data(SerializedData(iter_str));
      before.clear();
      before.insert(job_ids[7]);
      after.clear();
      SpawnComputeJob(LOOP_ITERATION,
          job_ids[8],
          read, write,
          before, after,
          iter_params);
    } else {
      // compute one last iteration, then spawn write frame job. Finally,
      // spawn loop frame job for next frame computation.

      int job_num = 10;
      std::vector<nimbus::job_id_t> job_ids;
      GetNewJobID(&job_ids, job_num);
      nimbus::IDSet<nimbus::logical_data_id_t> read, write;
      nimbus::IDSet<nimbus::job_id_t> before, after;
      nimbus::Parameter s11_params;
      nimbus::Parameter s12_params;
      nimbus::Parameter s13_params;
      nimbus::Parameter s14_params;
      nimbus::Parameter s15_params;
      nimbus::Parameter s16_params;
      nimbus::Parameter s2_params;
      nimbus::Parameter s3_params;
      nimbus::Parameter write_params;
      nimbus::Parameter frame_params;

      LoadReadWriteSets(this, &read, &write);

      std::string s11_str;
      SerializeParameter(frame, time, dt, &s11_str);
      s11_params.set_ser_data(SerializedData(s11_str));
      before.clear();
      after.clear();
      after.insert(job_ids[1]);
      SpawnComputeJob(ADJUST_PHI_WITH_OBJECTS,
          job_ids[0],
          read, write,
          before, after,
          s11_params);

      std::string s12_str;
      SerializeParameter(frame, time, dt, &s12_str);
      s12_params.set_ser_data(SerializedData(s12_str));
      before.clear();
      before.insert(job_ids[0]);
      after.clear();
      after.insert(job_ids[2]);
      SpawnComputeJob(ADVECT_PHI,
          job_ids[1],
          read, write,
          before, after,
          s12_params);

      std::string s13_str;
      SerializeParameter(frame, time, dt, &s13_str);
      s13_params.set_ser_data(SerializedData(s13_str));
      before.clear();
      before.insert(job_ids[1]);
      after.clear();
      after.insert(job_ids[3]);
      SpawnComputeJob(STEP_PARTICLES,
          job_ids[2],
          read, write,
          before, after,
          s13_params);

      std::string s14_str;
      SerializeParameter(frame, time, dt, &s14_str);
      s14_params.set_ser_data(SerializedData(s14_str));
      before.clear();
      before.insert(job_ids[2]);
      after.clear();
      after.insert(job_ids[4]);
      SpawnComputeJob(ADVECT_REMOVED_PARTICLES,
          job_ids[3],
          read, write,
          before, after,
          s14_params);

      std::string s15_str;
      SerializeParameter(frame, time, dt, &s15_str);
      s15_params.set_ser_data(SerializedData(s15_str));
      before.clear();
      before.insert(job_ids[3]);
      after.clear();
      after.insert(job_ids[5]);
      SpawnComputeJob(ADVECT_V,
          job_ids[4],
          read, write,
          before, after,
          s15_params);

      std::string s16_str;
      SerializeParameter(frame, time, dt, &s16_str);
      s16_params.set_ser_data(SerializedData(s16_str));
      before.clear();
      before.insert(job_ids[4]);
      after.clear();
      after.insert(job_ids[6]);
      SpawnComputeJob(APPLY_FORCES,
          job_ids[5],
          read, write,
          before, after,
          s16_params);


      std::string s2_str;
      SerializeParameter(frame, time, dt, &s2_str);
      s2_params.set_ser_data(SerializedData(s2_str));
      before.clear();
      before.insert(job_ids[5]);
      after.clear();
      after.insert(job_ids[7]);
      SpawnComputeJob(SUPER_2,
          job_ids[6],
          read, write,
          before, after,
          s2_params);

      std::string s3_str;
      SerializeParameter(frame, time, dt, &s3_str);
      s3_params.set_ser_data(SerializedData(s3_str));
      before.clear();
      before.insert(job_ids[6]);
      after.clear();
      after.insert(job_ids[8]);
      SpawnComputeJob(SUPER_3,
          job_ids[7],
          read, write,
          before, after,
          s3_params);

      std::string write_str;
      SerializeParameter(frame, time + dt, 0, &write_str);
      write_params.set_ser_data(SerializedData(write_str));
      before.clear();
      before.insert(job_ids[7]);
      after.clear();
      after.insert(job_ids[9]);
      SpawnComputeJob(WRITE_FRAME,
          job_ids[8],
          read, write,
          before, after,
          write_params);

      std::string frame_str;
      SerializeParameter(frame + 1, &frame_str);
      frame_params.set_ser_data(SerializedData(frame_str));
      before.clear();
      before.insert(job_ids[8]);
      after.clear();
      SpawnComputeJob(LOOP_FRAME,
          job_ids[9],
          read, write,
          before, after,
          frame_params);
    }
  }





  void JobLoopIteration::SpawnWithOneJobGranularity(
      bool done, int frame, T time, T dt, const nimbus::DataArray& da) {
    if (!done) {
      //Spawn one iteration of frame computation and then one loop
      //iteration job, for next iteration.
      dbg(APP_LOG, "Loop frame is spawning calculate frame job for frame %i.\n", frame);

      int job_num = 2;
      std::vector<nimbus::job_id_t> job_ids;
      GetNewJobID(&job_ids, job_num);
      nimbus::IDSet<nimbus::logical_data_id_t> read, write;
      nimbus::IDSet<nimbus::job_id_t> before, after;
      nimbus::Parameter cal_params;
      nimbus::Parameter iter_params;

      LoadReadWriteSets(this, &read, &write);

      std::string cal_str;
      SerializeParameter(frame, time, dt, &cal_str);
      cal_params.set_ser_data(SerializedData(cal_str));
      after.clear();
      after.insert(job_ids[1]);
      before.clear();
      SpawnComputeJob(CALCULATE_FRAME,
          job_ids[0],
          read, write,
          before, after,
          cal_params);

      std::string iter_str;
      SerializeParameter(frame, time + dt, &iter_str);
      iter_params.set_ser_data(SerializedData(iter_str));
      after.clear();
      before.clear();
      before.insert(job_ids[0]);
      SpawnComputeJob(LOOP_ITERATION,
          job_ids[1],
          read, write,
          before, after,
          iter_params);
    } else {
      // compute one last iteration, then spawn write frame job. Finally,
      // spawn loop frame job for next frame computation.

      int job_num = 3;
      std::vector<nimbus::job_id_t> job_ids;
      GetNewJobID(&job_ids, job_num);
      nimbus::IDSet<nimbus::logical_data_id_t> read, write;
      nimbus::IDSet<nimbus::job_id_t> before, after;
      nimbus::Parameter cal_params;
      nimbus::Parameter write_params;
      nimbus::Parameter frame_params;

      LoadReadWriteSets(this, &read, &write);

      std::string cal_str;
      SerializeParameter(frame, time, dt, &cal_str);
      cal_params.set_ser_data(SerializedData(cal_str));
      after.clear();
      after.insert(job_ids[1]);
      before.clear();
      SpawnComputeJob(CALCULATE_FRAME,
          job_ids[0],
          read, write,
          before, after,
          cal_params);

      std::string write_str;
      SerializeParameter(frame, time + dt, 0, &write_str);
      write_params.set_ser_data(SerializedData(write_str));
      after.clear();
      after.insert(job_ids[2]);
      before.clear();
      before.insert(job_ids[0]);
      SpawnComputeJob(WRITE_FRAME,
          job_ids[1],
          read, write,
          before, after,
          write_params);

      std::string frame_str;
      SerializeParameter(frame + 1, &frame_str);
      frame_params.set_ser_data(SerializedData(frame_str));
      after.clear();
      before.clear();
      before.insert(job_ids[1]);
      SpawnComputeJob(LOOP_FRAME,
          job_ids[2],
          read, write,
          before, after,
          frame_params);
    }
  }

  void JobLoopIteration::SpawnWithBreakAllGranularity(
      bool done, int frame, T time, T dt, const nimbus::DataArray& da) {
    dbg(APP_LOG, "Loop frame is spawning super job 1, 2, 3 for frame %i.\n", frame);

    int job_num = 13;
    std::vector<nimbus::job_id_t> job_ids;
    GetNewJobID(&job_ids, job_num);
    nimbus::IDSet<nimbus::logical_data_id_t> read, write;
    nimbus::IDSet<nimbus::job_id_t> before, after;

    read.clear();
    LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL, APP_FACE_VEL_GHOST, APP_PHI, NULL);
    write.clear();
    LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL, APP_FACE_VEL_GHOST, APP_PHI, NULL);

    nimbus::Parameter s11_params;
    std::string s11_str;
    SerializeParameter(frame, time, dt, &s11_str);
    s11_params.set_ser_data(SerializedData(s11_str));
    before.clear();
    after.clear();
    after.insert(job_ids[1]);
    SpawnComputeJob(ADJUST_PHI_WITH_OBJECTS,
        job_ids[0],
        read, write,
        before, after,
        s11_params);


    read.clear();
    LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);
    write.clear();
    LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);

    nimbus::Parameter s12_params;
    std::string s12_str;
    SerializeParameter(frame, time, dt, &s12_str);
    s12_params.set_ser_data(SerializedData(s12_str));
    before.clear();
    before.insert(job_ids[0]);
    after.clear();
    after.insert(job_ids[2]);
    SpawnComputeJob(ADVECT_PHI,
        job_ids[1],
        read, write,
        before, after,
        s12_params);


    read.clear();
    LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL_GHOST, NULL);
    LoadLogicalIdsInSet(this, &read, kDomainParticles, APP_POS_PARTICLES,
        APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
        APP_LAST_UNIQUE_PARTICLE_ID , NULL);
    write.clear();
    LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL_GHOST, NULL);
    LoadLogicalIdsInSet(this, &write, kDomainParticles, APP_POS_PARTICLES,
        APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
        APP_LAST_UNIQUE_PARTICLE_ID , NULL);

    nimbus::Parameter s13_params;
    std::string s13_str;
    SerializeParameter(frame, time, dt, &s13_str);
    s13_params.set_ser_data(SerializedData(s13_str));
    before.clear();
    before.insert(job_ids[1]);
    after.clear();
    after.insert(job_ids[3]);
    SpawnComputeJob(STEP_PARTICLES,
        job_ids[2],
        read, write,
        before, after,
        s13_params);


    read.clear();
    LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);
    LoadLogicalIdsInSet(this, &read, kDomainParticles, APP_POS_REM_PARTICLES,
        APP_NEG_REM_PARTICLES, APP_LAST_UNIQUE_PARTICLE_ID , NULL);
    write.clear();
    LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);
    LoadLogicalIdsInSet(this, &write, kDomainParticles, APP_POS_REM_PARTICLES,
        APP_NEG_REM_PARTICLES, APP_LAST_UNIQUE_PARTICLE_ID , NULL);

    nimbus::Parameter s14_params;
    std::string s14_str;
    SerializeParameter(frame, time, dt, &s14_str);
    s14_params.set_ser_data(SerializedData(s14_str));
    before.clear();
    before.insert(job_ids[2]);
    after.clear();
    after.insert(job_ids[4]);
    SpawnComputeJob(ADVECT_REMOVED_PARTICLES,
        job_ids[3],
        read, write,
        before, after,
        s14_params);


    read.clear();
    LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL, APP_FACE_VEL_GHOST, APP_PHI, NULL);
    write.clear();
    LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL, APP_FACE_VEL_GHOST, APP_PHI, NULL);

    nimbus::Parameter s15_params;
    std::string s15_str;
    SerializeParameter(frame, time, dt, &s15_str);
    s15_params.set_ser_data(SerializedData(s15_str));
    before.clear();
    before.insert(job_ids[3]);
    after.clear();
    after.insert(job_ids[5]);
    SpawnComputeJob(ADVECT_V,
        job_ids[4],
        read, write,
        before, after,
        s15_params);


    read.clear();
    LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);
    write.clear();
    LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);

    nimbus::Parameter s16_params;
    std::string s16_str;
    SerializeParameter(frame, time, dt, &s16_str);
    s16_params.set_ser_data(SerializedData(s16_str));
    before.clear();
    before.insert(job_ids[4]);
    after.clear();
    after.insert(job_ids[6]);
    SpawnComputeJob(APPLY_FORCES,
        job_ids[5],
        read, write,
        before, after,
        s16_params);


    read.clear();
    LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL, APP_FACE_VEL_GHOST, APP_PHI, NULL);
    LoadLogicalIdsInSet(this, &read, kDomainParticles, APP_POS_PARTICLES,
        APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
        APP_LAST_UNIQUE_PARTICLE_ID , NULL);
    write.clear();
    LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL, APP_FACE_VEL_GHOST, APP_PHI, NULL);
    LoadLogicalIdsInSet(this, &write, kDomainParticles, APP_POS_PARTICLES,
        APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
        APP_LAST_UNIQUE_PARTICLE_ID , NULL);

    nimbus::Parameter modify_levelset_params;
    std::string modify_levelset_str;
    SerializeParameter(frame, time, dt, &modify_levelset_str);
    modify_levelset_params.set_ser_data(SerializedData(modify_levelset_str));
    after.clear();
    after.insert(job_ids[7]);
    before.clear();
    before.insert(job_ids[5]);
    SpawnComputeJob(MODIFY_LEVELSET,
        job_ids[6],
        read, write,
        before, after,
        modify_levelset_params);


    read.clear();
    LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_PHI, NULL);
    write.clear();
    LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_PHI, NULL);

    nimbus::Parameter adjust_phi_params;
    std::string adjust_phi_str;
    SerializeParameter(frame, time, dt, &adjust_phi_str);
    adjust_phi_params.set_ser_data(SerializedData(adjust_phi_str));
    after.clear();
    after.insert(job_ids[8]);
    before.clear();
    before.insert(job_ids[6]);
    SpawnComputeJob(ADJUST_PHI,
        job_ids[7],
        read, write,
        before, after,
        adjust_phi_params);

    read.clear();
    LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL_GHOST, APP_PHI, NULL);
    LoadLogicalIdsInSet(this, &read, kDomainParticles, APP_POS_PARTICLES,
        APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
        APP_LAST_UNIQUE_PARTICLE_ID , NULL);

    write.clear();
    LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL_GHOST, APP_PHI, NULL);
    LoadLogicalIdsInSet(this, &write, kDomainParticles, APP_POS_PARTICLES,
        APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
        APP_LAST_UNIQUE_PARTICLE_ID , NULL);

    nimbus::Parameter delete_particles_params;
    std::string delete_particles_str;
    SerializeParameter(frame, time, dt, &delete_particles_str);
    delete_particles_params.set_ser_data(SerializedData(delete_particles_str));
    after.clear();
    after.insert(job_ids[9]);
    before.clear();
    before.insert(job_ids[7]);
    SpawnComputeJob(DELETE_PARTICLES,
        job_ids[8],
        read, write,
        before, after,
        delete_particles_params);


    read.clear();
    LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);
    LoadLogicalIdsInSet(this, &read, kDomainParticles, APP_POS_PARTICLES,
        APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
        APP_LAST_UNIQUE_PARTICLE_ID , NULL);

    write.clear();
    LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);
    LoadLogicalIdsInSet(this, &write, kDomainParticles, APP_POS_PARTICLES,
        APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
        APP_LAST_UNIQUE_PARTICLE_ID , NULL);

    nimbus::Parameter reincorporate_particles_params;
    std::string reincorporate_particles_str;
    SerializeParameter(frame, time, dt, &reincorporate_particles_str);
    reincorporate_particles_params.set_ser_data(SerializedData(reincorporate_particles_str));
    after.clear();
    after.insert(job_ids[10]);
    before.clear();
    before.insert(job_ids[8]);
    SpawnComputeJob(REINCORPORATE_PARTICLES,
        job_ids[9],
        read, write,
        before, after,
        reincorporate_particles_params);

    {
      read.clear();
      LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);
      write.clear();
      LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);

      int index = 10;
      nimbus::Parameter projection_params;
      std::string projection_str;
      SerializeParameter(frame, time, dt, &projection_str);
      projection_params.set_ser_data(SerializedData(projection_str));
      after.clear();
      after.insert(job_ids[index+1]);
      before.clear();
      before.insert(job_ids[index-1]);
      SpawnComputeJob(PROJECTION,
                      job_ids[index],
                      read, write,
                      before, after,
                      projection_params);
    }

    {
      read.clear();
      LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);
      write.clear();
      LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL, APP_PHI, NULL);

      int index = 11;
      nimbus::Parameter extrapolation_params;
      std::string extrapolation_str;
      SerializeParameter(frame, time, dt, &extrapolation_str);
      extrapolation_params.set_ser_data(SerializedData(extrapolation_str));
      after.clear();
      after.insert(job_ids[index+1]);
      before.clear();
      before.insert(job_ids[index-1]);
      SpawnComputeJob(EXTRAPOLATION,
                      job_ids[index],
                      read, write,
                      before, after,
                      reincorporate_particles_params);
    }

    if (!done) {
      //Spawn one iteration of frame computation and then one loop
      //iteration  job, for next iteration.

      {
        read.clear();
        LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL, APP_FACE_VEL_GHOST, APP_PHI, NULL);
        LoadLogicalIdsInSet(this, &read, kDomainParticles, APP_POS_PARTICLES,
            APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
            APP_LAST_UNIQUE_PARTICLE_ID , NULL);
        write.clear();
        LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL, APP_FACE_VEL_GHOST, APP_PHI, NULL);
        LoadLogicalIdsInSet(this, &write, kDomainParticles, APP_POS_PARTICLES,
            APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
            APP_LAST_UNIQUE_PARTICLE_ID , NULL);

        int index = 12;
        nimbus::Parameter iter_params;
        std::string iter_str;
        SerializeParameter(frame, time + dt, &iter_str);
        iter_params.set_ser_data(SerializedData(iter_str));
        after.clear();
        before.clear();
        before.insert(job_ids[index-1]);
        SpawnComputeJob(LOOP_ITERATION,
                        job_ids[index],
                        read, write,
                        before, after,
                        iter_params);
      }
    } else {
      // compute one last iteration, then spawn write frame job. Finally,
      // spawn loop frame job for next frame computation.

      std::vector<nimbus::job_id_t> loop_job_id;
      GetNewJobID(&loop_job_id, 1);

      {
        read.clear();
        LoadLogicalIdsInSet(this, &read, kRegGhostw3Outer[0], APP_FACE_VEL, APP_FACE_VEL_GHOST, APP_PHI, NULL);
        LoadLogicalIdsInSet(this, &read, kDomainParticles, APP_POS_PARTICLES,
            APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
            APP_LAST_UNIQUE_PARTICLE_ID , NULL);
        write.clear();
        LoadLogicalIdsInSet(this, &write, kRegGhostw3Outer[0], APP_FACE_VEL, APP_FACE_VEL_GHOST, APP_PHI, NULL);
        LoadLogicalIdsInSet(this, &write, kDomainParticles, APP_POS_PARTICLES,
            APP_NEG_PARTICLES, APP_POS_REM_PARTICLES, APP_NEG_REM_PARTICLES,
            APP_LAST_UNIQUE_PARTICLE_ID , NULL);

        int index = 12;
        nimbus::Parameter write_params;
        std::string write_str;
        SerializeParameter(frame, time + dt, 0, &write_str);
        write_params.set_ser_data(SerializedData(write_str));
        after.clear();
        after.insert(loop_job_id[0]);
        before.clear();
        before.insert(job_ids[index-1]);
        SpawnComputeJob(WRITE_FRAME,
                        job_ids[index],
                        read, write,
                        before, after,
                        write_params);
      }

      {
        read.clear();
        write.clear();

        int index = 13;
        nimbus::Parameter frame_params;
        std::string frame_str;
        SerializeParameter(frame + 1, &frame_str);
        frame_params.set_ser_data(SerializedData(frame_str));
        after.clear();
        before.clear();
        before.insert(job_ids[index-1]);
        SpawnComputeJob(LOOP_FRAME,
                        loop_job_id[0],
                        read, write,
                        before, after,
                        frame_params);
      }
    }
  }


} // namespace application