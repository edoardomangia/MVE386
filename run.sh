#!/bin/bash

#SBATCH --job-name=TRA220
#SBATCH --partition=vera
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=24
#SBATCH --time=00:45:00

#SBATCH --output=log.out
#SBATCH --open-mode=truncate

#SBATCH --account=c3se2025-1-15

set -euo pipefail

module purge
module load GCCcore/13.2.0 
module load CMake/3.27.6-GCCcore-13.2.0 
module load Geant4/11.3.0-GCC-13.2.0 
module load assimp/5.3.1-GCCcore-13.2.0 

cd "$SLURM_SUBMIT_DIR"

rm -f build/CMakeCache.txt
cmake -S . -B build 
cmake --build build -j "${SLURM_CPUS_PER_TASK:-24}"

export G4NUM_THREADS="${SLURM_CPUS_PER_TASK:-24}"

mkdir -p output

# GEANT_PHOTONS=1000000
# 
# srun build/run "${GEANT_PHOTONS}" setups/setup_fly.json
# srun build/run "${GEANT_PHOTONS}" setups/setup_step1.json
# srun build/run "${GEANT_PHOTONS}" setups/setup_step5.json
# 
# module purge
# module load gfbf/2025b
# module load SciPy-bundle/2025.07-gfbf-2025b
# 
# srun ./heat_geant.py output/dose.vti "${GEANT_PHOTONS}" setups/setup_fly.json
# srun ./heat_geant.py output/dose.vti "${GEANT_PHOTONS}" setups/setup_step1.json
# srun ./heat_geant.py output/dose.vti "${GEANT_PHOTONS}" setups/setup_step5.json
# 
# srun ./radiolysis_geant.py output/dose.vti "${GEANT_PHOTONS}" setups/setup_fly.json
# srun ./radiolysis_geant.py output/dose.vti "${GEANT_PHOTONS}" setups/setup_step1.json
# srun ./radiolysis_geant.py output/dose.vti "${GEANT_PHOTONS}" setups/setup_step5.json

SETUPS=(
    #"setups/setup.json"
    
    "setups/setup_grid10.json"
    "setups/setup_grid100.json"
    "setups/setup_grid1000.json"
    
    "setups/setup_exp1.json"
    "setups/setup_exp5.json"
    "setups/setup_exp10.json"
    
    "setups/setup_acqfly.json"
    "setups/setup_acqstep1.json"
    "setups/setup_acqstep5.json"
)

for cfg in "${SETUPS[@]}"; do
  base=$(basename "$cfg" .json)
  vti_out="output/dose_${base}.vti"

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
