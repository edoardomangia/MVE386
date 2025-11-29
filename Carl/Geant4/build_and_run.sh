#!/bin/bash
set -e

source /mnt/d/Software/Geant4/geant4-v11.3.2-install/share/Geant4/geant4make/geant4make.sh

# If script is run from root, go into build
if [ -d "build" ] && [ ! -f "CMakeCache.txt" ]; then
    cd build
fi

geant4make || true
cmake ..
make -j4
./sim