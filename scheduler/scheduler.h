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
  * Nimbus scheduler.
  *
  * Author: Omid Mashayekhi <omidm@stanford.edu>
  */

#ifndef NIMBUS_SCHEDULER_SCHEDULER_H_
#define NIMBUS_SCHEDULER_SCHEDULER_H_

#include <boost/thread.hpp>
#include <iostream> // NOLINT
#include <fstream> // NOLINT
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include "shared/nimbus_types.h"
#include "shared/log.h"
#include "shared/cluster.h"
#include "shared/parser.h"
#include "shared/scheduler_server.h"
#include "scheduler/data_manager.h"
#include "scheduler/job_manager.h"
#include "scheduler/version_manager.h"
#include "shared/id_maker.h"

namespace nimbus {

class Scheduler {
  public:
    explicit Scheduler(port_t listening_port);
    virtual ~Scheduler();

    virtual void Run();
    virtual void SchedulerCoreProcessor();
    virtual void ProcessQueuedSchedulerCommands(size_t max_num);
    virtual void ProcessSchedulerCommand(SchedulerCommand* cm);
    virtual void ProcessSpawnComputeJobCommand(SpawnComputeJobCommand* cm);
    virtual void ProcessSpawnCopyJobCommand(SpawnCopyJobCommand* cm);
    virtual void ProcessDefineDataCommand(DefineDataCommand* cm);
    virtual void ProcessHandshakeCommand(HandshakeCommand* cm);
    virtual void ProcessJobDoneCommand(JobDoneCommand* cm);
    virtual void ProcessDefinePartitionCommand(DefinePartitionCommand* cm);
    virtual void ProcessTerminateCommand(TerminateCommand* cm);

    virtual void RegisterInitialWorkers(size_t min_to_join);
    virtual size_t RegisterPendingWorkers();
    virtual void AddMainJob();
    virtual void TerminationProcedure();

    virtual size_t AssignReadyJobs();
    virtual bool AssignJob(JobEntry* job);
    virtual bool GetWorkerToAssignJob(JobEntry* job, SchedulerWorker*& worker);

    virtual bool PrepareDataForJobAtWorker(JobEntry* job,
                                SchedulerWorker* worker, logical_data_id_t l_id);
    virtual bool AllocateLdoInstanceToJob(JobEntry* job,
        LogicalDataObject* ldo, PhysicalData pd);
    virtual size_t GetObsoleteLdoInstanceAtWorker(SchedulerWorker* worker,
        LogicalDataObject* ldo, PhysicalDataVector* dest);

    virtual bool SendComputeJobToWorker(SchedulerWorker* worker, JobEntry* job);
    virtual bool SendCreateJobToWorker(SchedulerWorker* worker,
        const std::string& data_name, const logical_data_id_t& logical_data_id,
        const IDSet<job_id_t>& before, const IDSet<job_id_t>& after,
        job_id_t* job_id, physical_data_id_t* physical_data_id);
    virtual bool SendLocalCopyJobToWorker(SchedulerWorker* worker,
        const ID<physical_data_id_t>& from_physical_data_id,
        const ID<physical_data_id_t>& to_physical_data_id,
        const IDSet<job_id_t>& before, const IDSet<job_id_t>& after,
        job_id_t* job_id);
    virtual bool SendCopyReceiveJobToWorker(SchedulerWorker* worker,
        const physical_data_id_t& physical_data_id,
        const IDSet<job_id_t>& before, const IDSet<job_id_t>& after,
        job_id_t* job_id);
    virtual bool SendCopySendJobToWorker(SchedulerWorker* worker,
        const job_id_t& receive_job_id, const physical_data_id_t& physical_data_id,
        const IDSet<job_id_t>& before, const IDSet<job_id_t>& after,
        job_id_t* job_id);

    void set_min_worker_to_join(size_t num);

    virtual void LoadClusterMap(std::string) {}
    virtual void DeleteWorker(SchedulerWorker * worker) {}
    virtual void AddWorker(SchedulerWorker * worker) {}
    virtual SchedulerWorker* GetWorker(int workerId) {
      return NULL;
    }

  protected:
    SchedulerServer* server_;
    DataManager* data_manager_;
    JobManager* job_manager_;
    IDMaker id_maker_;
    CmSet user_command_set_;
    SchedulerCommand::PrototypeTable worker_command_table_;
    size_t min_worker_to_join_;
    bool terminate_application_flag_;
    exit_status_t terminate_application_status_;
    Log log_;
    Log log_assign_;
    Log log_table_;

  private:
    virtual void SetupUserInterface();
    virtual void SetupWorkerInterface();
    virtual void SetupDataManager();
    virtual void SetupJobManager();
    virtual void GetUserCommand();
    virtual void LoadUserCommands();
    virtual void LoadWorkerCommands();

    boost::thread* user_interface_thread_;
    boost::thread* worker_interface_thread_;
    Computer host_;
    port_t listening_port_;
    app_id_t appId_;
    ClusterMap cluster_map_;
    size_t registered_worker_num_;
};

}  // namespace nimbus
#endif  // NIMBUS_SCHEDULER_SCHEDULER_H_
