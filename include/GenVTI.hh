/* 
 * GenVTI.hh
 * 
 */

#pragma once

#include <vector>
#include <string>

class VTIWriter {
public:
    static void Write(const std::string& filename,
                      const std::vector<float>& data,
                      int NX, int NY, int NZ,
                      float xmin, float ymin, float zmin,
                      float dx, float dy, float dz);
};

