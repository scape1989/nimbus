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
 * Author: Chinmayee Shah <chinmayee.shah@stanford.edu>
 */

#ifndef NIMBUS_APPLICATION_WATER_TEST_MULTIPLE_V1_PROTO_FILES_SERIALIZE_DATA_2D_H_
#define NIMBUS_APPLICATION_WATER_TEST_MULTIPLE_V1_PROTO_FILES_SERIALIZE_DATA_2D_H_

// IMPORTANT: first include the protocol buffer generated files
// in order to avoid pollution due to using namespace physbam
// in water_data_types, and functions overloaded by physbam
#include "pb_include_2d.h"

// Now include physbam related files
#include "physbam_data_include.h"

namespace physbam_pb {

    void make_pb_object(VI2 *phys_vec,
            ::communication::PhysbamVectorInt2d *pb_vec);

    void make_pb_object(VF2 *phys_vec,
            ::communication::PhysbamVectorFloat2d *pb_vec);

    void make_pb_object(RangeI2 *phys_range,
            ::communication::PhysbamRangeInt2d *pb_range);

    void make_pb_object(RangeF2 *phys_range,
            ::communication::PhysbamRangeFloat2d *pb_range);

    void make_pb_object(Grid2 *phys_grid,
            ::communication::PhysbamGrid2d *pb_grid);

} // namespace physbam_pb

#endif // NIMBUS_APPLICATION_WATER_TEST_MULTIPLE_V1_PROTO_FILES_SERIALIZE_DATA_2D_H_