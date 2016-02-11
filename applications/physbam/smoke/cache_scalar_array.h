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
 * Author: Chinmayee Shah <chshah@stanford.edu>
 */

#ifndef NIMBUS_APPLICATION_SMOKE_CACHE_SCALAR_ARRAY_H_
#define NIMBUS_APPLICATION_SMOKE_CACHE_SCALAR_ARRAY_H_

#include <string>

#include "application/smoke/physbam_include.h"
#include "data/cache/cache_var.h"
#include "data/physbam/translator_physbam.h"
#include "shared/geometric_region.h"
#include "worker/data.h"

namespace application {

template<class T, class TS = float>
class CacheScalarArray : public nimbus::CacheVar {
        typedef typename PhysBAM::VECTOR<TS, 3> TV;
        typedef typename PhysBAM::VECTOR<int, 3> TV_INT;
        typedef typename PhysBAM::RANGE<TV> Range;
        typedef typename PhysBAM::ARRAY<T, TV_INT> PhysBAMScalarArray;
        typedef typename PhysBAM::GRID<TV> Grid;
        typedef typename nimbus::TranslatorPhysBAM<TS> Translator;

    public:
        explicit CacheScalarArray(const nimbus::GeometricRegion &global_reg,
                                  const int ghost_width,
                                  bool make_proto = false);

        PhysBAMScalarArray *data() {
            return data_;
        }
        void set_data(PhysBAMScalarArray *d) {
            data_ = d;
        }

    protected:
        explicit CacheScalarArray(const nimbus::GeometricRegion &global_reg,
                                  const nimbus::GeometricRegion &ob_reg,
                                  const int ghost_width);

        virtual nimbus::CacheVar *CreateNew(const nimbus::GeometricRegion &ob_reg) const;

        virtual void ReadToCache(const nimbus::DataArray &read_set,
                                 const nimbus::GeometricRegion &read_reg);
        virtual void WriteFromCache(const nimbus::DataArray &write_set,
                                    const nimbus::GeometricRegion &write_reg) const;

    private:
        nimbus::GeometricRegion global_region_;
        nimbus::GeometricRegion local_region_;
        int ghost_width_;
        nimbus::Coord shift_;
        PhysBAMScalarArray *data_;
        Grid mac_grid_;
}; // class CacheScalarArray

} // namespace application

#endif // NIMBUS_APPLICATION_SMOKE_CACHE_SCALAR_ARRAY_H_