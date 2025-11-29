/*
 * GenVTI.cc
 */

#include "GenVTI.hh"
#include <fstream>
#include <filesystem>
#include <iostream>

void VTIWriter::Write(const std::string& filename,
                      const std::vector<float>& data,
                      int NX, int NY, int NZ,
                      float xmin, float ymin, float zmin,
                      float dx, float dy, float dz,
                      const std::vector<std::pair<std::string, std::string>>& metadata)
{
    std::filesystem::path file_path(filename);
    if (file_path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(file_path.parent_path(), ec);
        if (ec) {
            std::cerr << "Failed to create directory for VTI output (" << file_path.parent_path()
                      << "): " << ec.message() << std::endl;
        }
    }

    std::ofstream f(filename);
    if (!f.is_open()) {
        std::cerr << "Unable to open VTI output file: " << filename << std::endl;
        return;
    }

    // Use cell data: extent uses point indices, so add one to cover all cells
    int x0 = 0, y0 = 0, z0 = 0;
    int x1 = NX;
    int y1 = NY;
    int z1 = NZ;

    f << "<?xml version=\"1.0\"?>\n";
    f << "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
    f << "  <ImageData WholeExtent=\""
      << x0 << " " << x1 << " "
      << y0 << " " << y1 << " "
      << z0 << " " << z1 << "\" "
      << "Origin=\"" << xmin << " " << ymin << " " << zmin << "\" "
      << "Spacing=\"" << dx << " " << dy << " " << dz << "\">\n";

    if (!metadata.empty()) {
        f << "    <FieldData>\n";
        for (const auto& kv : metadata) {
            f << "      <DataArray type=\"String\" Name=\""
              << kv.first << "\" format=\"ascii\" NumberOfComponents=\"1\" NumberOfTuples=\"1\">\n";
            f << "        " << kv.second << "\n";
            f << "      </DataArray>\n";
        }
        f << "    </FieldData>\n";
    }

    f << "    <Piece Extent=\""
      << x0 << " " << x1 << " "
      << y0 << " " << y1 << " "
      << z0 << " " << z1 << "\">\n";

    f << "      <PointData/>\n";
    f << "      <CellData Scalars=\"edep_keV\">\n";
    f << "        <DataArray type=\"Float32\" Name=\"edep_keV\" format=\"ascii\">\n";

    for (size_t i = 0; i < data.size(); ++i)
        f << data[i] << " ";

    f << "\n        </DataArray>\n";
    f << "      </CellData>\n";
    f << "    </Piece>\n";

    f << "  </ImageData>\n";
    f << "</VTKFile>\n";
}
