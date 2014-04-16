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
  * Job done command to signal completion of a job.
  *
  * Author: Omid Mashayekhi <omidm@stanford.edu>
  */

#ifndef NIMBUS_SHARED_JOB_DONE_COMMAND_H_
#define NIMBUS_SHARED_JOB_DONE_COMMAND_H_


#include <string>
#include "shared/scheduler_command.h"

namespace nimbus {
class JobDoneCommand : public SchedulerCommand {
  public:
    JobDoneCommand();
    JobDoneCommand(const ID<job_id_t>& job_id,
        const IDSet<job_id_t>& after_set,
        const Parameter& params);
    JobDoneCommand(const ID<job_id_t>& job_id,
        const IDSet<job_id_t>& after_set,
        const Parameter& params,
        double run_time,
        double wait_time);
    ~JobDoneCommand();

    virtual SchedulerCommand* Clone();
    virtual bool Parse(const std::string& param_segment);
    virtual std::string toString();
    virtual std::string toStringWTags();
    ID<job_id_t> job_id();
    IDSet<job_id_t> after_set();
    Parameter params();
    double run_time();
    double wait_time();

  private:
    ID<job_id_t> job_id_;
    IDSet<job_id_t> after_set_;
    Parameter params_;
    double run_time_;
    double wait_time_;
};

}  // namespace nimbus

#endif  // NIMBUS_SHARED_JOB_DONE_COMMAND_H_
