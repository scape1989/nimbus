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
 * spawn the loop frame job for the next frame.
 *
 * Author: Omid Mashayekhi <omidm@stanford.edu>
 */

#include "application/water_alternate_fine/app_utils.h"
#include "application/water_alternate_fine/job_loop_iteration.h"
#include "application/water_alternate_fine/job_loop_frame.h"
#include "application/water_alternate_fine/job_super_1.h"
#include "application/water_alternate_fine/job_super_2.h"
#include "application/water_alternate_fine/job_super_3.h"
#include "application/water_alternate_fine/job_calculate_frame.h"
#include "application/water_alternate_fine/job_write_frame.h"
#include "application/water_alternate_fine/physbam_utils.h"
#include "application/water_alternate_fine/water_driver.h"
#include "application/water_alternate_fine/water_example.h"
#include "application/water_alternate_fine/water_sources.h"
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
        int frame;
        T time;
        std::string params_str(params.ser_data().data_ptr_raw(),
                               params.ser_data().size());
        LoadParameter(params_str, &frame, &time);

        dbg(APP_LOG, "Frame %i and time %i in iteration job\n",
                     frame, time);

        // check whether the frame is done or not
        bool done = false;
        PhysBAM::WATER_EXAMPLE<TV>* example;
        PhysBAM::WATER_DRIVER<TV>* driver;
        InitializeExampleAndDriver(da, frame, time,
                                   this, example, driver);
        // assert(init_success);

        T target_time = example->Time_At_Frame(driver->current_frame+1);
        T dt = example->cfl *
            example->incompressible.CFL(example->face_velocities);
        T temp_dt = example->particle_levelset_evolution.CFL(false,false);
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

        if (!done) {
          // spawn the jobs to compute the frame, depending on the
          // level of granularity we will have different sub jobs.

          //Spawn the super jobs to start computing the frame.
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

          nimbus::DataArray::const_iterator it = da.begin();
          for (; it != da.end(); ++it) {
            read.insert((*it)->logical_id());
            write.insert((*it)->logical_id());
          }

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
          // spawn the write frame job, or maybe compute frame job
          // for last time before write frame, and then spawn loop frame job
          // for next fram computation."

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

          nimbus::DataArray::const_iterator it = da.begin();
          for (; it != da.end(); ++it) {
            read.insert((*it)->logical_id());
            write.insert((*it)->logical_id());
          }

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
        // Free resources.
        DestroyExampleAndDriver(example, driver);
}

} // namespace application
