#!/bin/bash

#SBATCH -p gpu-long           # partition name
#SBATCH -A kdss                # account name
#SBATCH --cpus-per-task=64     # number of CPUs
#SBATCH --mem=128GB            # memory
#SBATCH --time=168:00:00       # time limit (HH:MM:SS)
#SBATCH --gres=gpu:H100        # GPU resource
#SBATCH -o __slurm__/job-%j.out        # output file (%j expands to job ID)

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Print some info for debugging
echo "Running on node: $(hostname)"
echo "Current directory: $(pwd)"
echo "Scripts directory: ${SCRIPT_DIR}"

ID=$(date +%Y%m%d-%H%M%S)

# Activate virtual environment if needed (uncomment and modify if you use one)
# source /path/to/your/venv/bin/activate

# Run the Python script from scripts folder regardless of where this sbatch is called from
# cd "${SCRIPT_DIR}"
python cuda_test.py > results/cuda_test_${ID}.csv

echo "Job completed"