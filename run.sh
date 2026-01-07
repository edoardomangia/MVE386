#!/bin/bash

#SBATCH --job-name=TRA220
#SBATCH --partition=vera
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --time=00:45:00

#SBATCH --output=log.out
#SBATCH --open-mode=truncate

#SBATCH --account=c3se2026-1-16

set -euo pipefail

module purge
module load GCCcore/13.2.0 
module load CMake/3.27.6-GCCcore-13.2.0 
module load Geant4/11.3.0-GCC-13.2.0 
module load assimp/5.3.1-GCCcore-13.2.0 

cd "$SLURM_SUBMIT_DIR"

rm -f build/CMakeCache.txt
cmake -S . -B build 
cmake --build build -j "${SLURM_CPUS_PER_TASK:-32}"

export G4NUM_THREADS="${SLURM_CPUS_PER_TASK:-32}"

mkdir -p output

SETUPS=(
    #"setups/setup.json"
    
    #"setups/setup_grid_10.json"
    "setups/setup_grid_100.json"
    #"setups/setup_grid_1000.json"
    
    "setups/setup_exp_1.json"
    #"setups/setup_exp_5.json"
    #"setups/setup_exp_10.json"
    
    "setups/setup_acq_fly.json"
    #"setups/setup_acq_step1.json"
    #"setups/setup_acq_step5.json"
)

for cfg in "${SETUPS[@]}"; do
  base=$(basename "$cfg" .json)
  vti_out="output/dose_${base}.vti"

  python export_scene_vtk.py "$cfg" -o "output/${base}_scene.vtk"

  srun build/run --setup "$cfg"
  mv output/dose.vti "$vti_out"

  #module purge
  #module load gfbf/2025b
  #module load SciPy-bundle/2025.07-gfbf-2025b

  #heat_prefix="output/heat_${base}"
  #rad_prefix="output/rad_${base}"

  #srun ./heat_geant.py "$vti_out" "${GEANT_PHOTONS}" "$cfg" "$heat_prefix"
  #srun ./radiolysis_geant.py "$vti_out" "${GEANT_PHOTONS}" "$cfg" "$rad_prefix"

  #module purge
  #module load GCCcore/13.2.0
  #module load CMake/3.27.6-GCCcore-13.2.0
  #module load Geant4/11.3.0-GCC-13.2.0
  #module load assimp/5.3.1-GCCcore-13.2.0
done
