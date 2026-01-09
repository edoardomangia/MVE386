#!/bin/bash

#SBATCH --job-name=TRA220
#SBATCH --partition=vera
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --time=02:00:00

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

# Some scene descriptions 
SETUPS=(
    #"setups/setup.json"
    
    #"setups/setup_acq_fly.json"
    #"setups/setup_acq_step1.json"
    #"setups/setup_acq_step5.json"
    
    # Crash!
    #"setups/setup_beam_parallel.json"
    #"setups/setup_beam_point_baseline.json"
    #"setups/setup_beam_point_long.json"
    #"setups/setup_beam_point_short.json"
    
    "setups/setup_energy_100keV.json"
    "setups/setup_energy_25keV.json"
    "setups/setup_energy_50keV.json"
    
    "setups/setup_exp_1.json"
    "setups/setup_exp_10.json"
    "setups/setup_exp_5.json"
    
    "setups/setup_grid_10.json"
    "setups/setup_grid_100.json"
    #"setups/setup_grid_1000.json"
    
    "setups/setup_material_bone.json"
    "setups/setup_material_ethanol.json"
    "setups/setup_material_milk.json"
    "setups/setup_material_water.json"
)

failed_runs=()

for cfg in "${SETUPS[@]}"; do
  base=$(basename "$cfg" .json)
  vti_out="output/dose_${base}.vti"

  echo "[run] Starting ${cfg}"

  # Visualizer for the scene, just to understand how it looks like
  if ! srun ./export_scene_vtk.py "$cfg" -o "output/${base}_scene.vtk"; then
    echo "[run] FAILED export_scene_vtk.py for ${cfg}"
    failed_runs+=("${cfg}:export_scene_vtk")
    continue
  fi

  # Geant4 code
  if ! srun build/run --setup "$cfg"; then
    echo "[run] FAILED Geant4 run for ${cfg}"
    failed_runs+=("${cfg}:geant4")
    continue
  fi

  if ! mv output/dose.vti "$vti_out"; then
    echo "[run] FAILED move output/dose.vti for ${cfg}"
    failed_runs+=("${cfg}:move_dose_vti")
    continue
  fi

  module purge
  module load gfbf/2025b
  module load SciPy-bundle/2025.07-gfbf-2025b

  # Post-processing scripts
  dose_prefix="output/dose_${base}"
  heat_prefix="output/heat_${base}"
  rad_prefix="output/rad_${base}"

  if ! srun ./dosage_geant.py "$vti_out" "$cfg" "$dose_prefix"; then
    echo "[run] FAILED dosage_geant.py for ${cfg}"
    failed_runs+=("${cfg}:dosage_geant")
    module purge
    module load GCCcore/13.2.0
    module load CMake/3.27.6-GCCcore-13.2.0
    module load Geant4/11.3.0-GCC-13.2.0
    module load assimp/5.3.1-GCCcore-13.2.0
    continue
  fi

  if ! srun ./heat_geant.py "${dose_prefix}_dose_Gy.vti" "$cfg" "$heat_prefix"; then
    echo "[run] FAILED heat_geant.py for ${cfg}"
    failed_runs+=("${cfg}:heat_geant")
    module purge
    module load GCCcore/13.2.0
    module load CMake/3.27.6-GCCcore-13.2.0
    module load Geant4/11.3.0-GCC-13.2.0
    module load assimp/5.3.1-GCCcore-13.2.0
    continue
  fi

  if ! srun ./radiolysis_geant.py "$vti_out" "$cfg" "$rad_prefix"; then
    echo "[run] FAILED radiolysis_geant.py for ${cfg}"
    failed_runs+=("${cfg}:radiolysis_geant")
    module purge
    module load GCCcore/13.2.0
    module load CMake/3.27.6-GCCcore-13.2.0
    module load Geant4/11.3.0-GCC-13.2.0
    module load assimp/5.3.1-GCCcore-13.2.0
    continue
  fi

  module purge
  module load GCCcore/13.2.0
  module load CMake/3.27.6-GCCcore-13.2.0
  module load Geant4/11.3.0-GCC-13.2.0
  module load assimp/5.3.1-GCCcore-13.2.0
done

if [ "${#failed_runs[@]}" -gt 0 ]; then
  echo "[run] Failed runs summary:"
  for item in "${failed_runs[@]}"; do
    echo "  - ${item}"
  done
else
  echo "[run] All runs completed successfully."
fi
