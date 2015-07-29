#!/usr/bin/env python

# Author: Omid Mashayekhi <omidm@stanford.edu>


# US West (Northern California) Region
# EC2_LOCATION = 'us-west-1'
# NIMBUS_AMI = 'ami-50201815'
# UBUNTU_AMI = 'ami-660c3023'
# KEY_NAME = 'omidm-sing-key-pair-us-west-1'
# SECURITY_GROUP = 'nimbus_sg_uswest1'
# # INSTANCE_TYPE = 't1.micro'
# INSTANCE_TYPE = 'c3.xlarge'
# PLACEMENT_GROUP = None
# INSTANCE_NUM = 3

# EC2 configurations
# US West (Oregon) Region
EC2_LOCATION                    = 'us-west-2'
UBUNTU_AMI                      = 'ami-fa9cf1ca'
# NIMBUS_AMI                      = 'ami-9996aea9'
NIMBUS_AMI                      = 'ami-1f868b2f'
KEY_NAME                        = 'omidm-sing-key-pair-us-west-2'
SECURITY_GROUP                  = 'nimbus_sg_uswest2'
CONTROLLER_INSTANCE_TYPE        = 'c3.4xlarge'
WORKER_INSTANCE_TYPE            = 'c3.2xlarge'
PLACEMENT                       = 'us-west-2c' # None
PLACEMENT_GROUP                 = 'nimbus-cluster' # None
PRIVATE_KEY                     = '/home/omidm/.ssh/' + KEY_NAME + '.pem'
CONTROLLER_NUM                  = 0
WORKER_NUM                      = 0
INSTANCE_NUM                    = WORKER_NUM + CONTROLLER_NUM

# controller configurations
ASSIGNER_THREAD_NUM             = 8
BATCH_ASSIGN_NUM                = 200
COMMAND_BATCH_SIZE              = 10000
DEACTIVATE_CONTROLLER_TEMPLATE  = False
DEACTIVATE_COMPLEX_MEMOIZATION  = False
DEACTIVATE_BINDING_MEMOIZATION  = False
DEACTIVATE_WORKER_TEMPLATE      = False
DEACTIVATE_MEGA_RCR_JOB         = False
DEACTIVATE_DM_QUERY_CACHE       = False
ACTIVATE_LB                     = False
ACTIVATE_FT                     = False
LB_PERIOD                       = 60
FT_PERIOD                       = 600
FIRST_PORT                      = 5800


# worker/simulation configurations
SIMULATION_SCALE                = 1024
PART_X                          = 8
PART_Y                          = 8
PART_Z                          = 8
PROJ_PART_X                     = 8
PROJ_PART_Y                     = 8
PROJ_PART_Z                     = 8
FRAME_NUMBER                    = 1
OTHREAD_NUM                     = 8
ITERATION_BATCH                 = 1
MAX_ITERATION                   = 100
NO_PROJ_BOTTLENECK              = False
WRITE_PER_PART                  = True
RUN_WITH_TASKSET                = False
WORKER_TASKSET                  = '0-3,8-11'


# logging configurations
STD_OUT_LOG                     = 'ec2_log.txt'
LOAD_BALANCER_LOG               = 'load_balancer_log'
SCHED_PER_ITER_STAT_LOG         = 'controller_stats.txt'
WORKER_LB_LOG                   = 'lb_log.txt'
WORKER_FINAL_STAT_LOG           = 'time_per_thread.txt' 
WORKER_PER_ITER_STAT_LOG        = 'main_timers.txt' 
OUTPUT_PATH                     = 'output/'


# Build and Path configuration
SOURCE_NIMBUS_ROOT   = '~/cloud/src/nimbus/'
EC2_NIMBUS_ROOT      = '~/cloud/src/nimbus/'
# EC2_NIMBUS_ROOT      = '~/physbam/'
REL_APPLICATION_PATH = 'application/water_multiple/Build/Release/'
REL_SCHEDULER_PATH   = 'test/scheduler_v3/'
REL_WORKER_PATH      = 'test/water_multiple/'
# REL_WORKER_PATH      = 'Projects/Water/'

NIMBUS_LIB           = 'libnimbus.so'
APPLICATION_LIB      = 'libwater_app.so'
SCHEDULER_BINARY     = 'scheduler'
WORKER_BINARY        = 'worker'



