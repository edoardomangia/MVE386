/*
 * gen_vti.cc
 */

#include "GenVTI.hh"
#include <fstream>

void VTIWriter::Write(const std::string& filename,
                      const std::vector<float>& data,
                      int NX, int NY, int NZ,
                      float xmin, float ymin, float zmin,
                      float dx, float dy, float dz)
{
    std::ofstream f(filename);

    int x0 = 0, y0 = 0, z0 = 0;
    int x1 = NX - 1;
    int y1 = NY - 1;
    int z1 = NZ - 1;

    f << "<?xml version=\"1.0\"?>\n";
    f << "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
    f << "  <ImageData WholeExtent=\""
      << x0 << " " << x1 << " "
      << y0 << " " << y1 << " "
      << z0 << " " << z1 << "\" "
      << "Origin=\"" << xmin << " " << ymin << " " << zmin << "\" "
      << "Spacing=\"" << dx << " " << dy << " " << dz << "\">\n";

    f << "    <Piece Extent=\""
      << x0 << " " << x1 << " "
      << y0 << " " << y1 << " "
      << z0 << " " << z1 << "\">\n";

    f << "      <PointData Scalars=\"edep_keV\">\n";
    f << "        <DataArray type=\"Float32\" Name=\"edep_keV\" format=\"ascii\">\n";

    for (size_t i = 0; i < data.size(); ++i)
        f << data[i] << " ";

    f << "\n        </DataArray>\n";
    f << "      </PointData>\n";
    f << "      <CellData/>\n";
    f << "    </Piece>\n";

    f << "  </ImageData>\n";
    f << "</VTKFile>\n";
}

