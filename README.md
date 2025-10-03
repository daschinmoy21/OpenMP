# OpenMP Package Installation Simulation

This project simulates package installation in serial and parallel modes using OpenMP for performance comparison.

## Prerequisites

- Python 3
- g++ compiler with OpenMP support (e.g., `g++ -fopenmp`)
- C++17 standard library

## Setup

1. Generate test data:
   ```
   python generate_test_data.py
   ```
   This creates a `pkgs` directory with 100 packages, each containing a `manifest.json` and 20 binary files, and updates `packages.txt`.

## Compilation

Compile the serial version:
```
g++ -O2 -std=c++17 bun_sim_serial.cpp -o bun_serial
```

Compile the parallel version:
```
g++ -O2 -fopenmp -std=c++17 parallel.cpp -o bun_parallel
```

## Usage

Run the serial simulation:
```
./bun_serial packages.txt out_serial
```

Run the parallel simulation:
```
./bun_parallel packages.txt parallel_out
```

Both commands process the packages listed in `packages.txt` and output to the specified directory, including timing information.

## Output

- Processed packages are copied to the output directory with metadata files.
- An `install_db.txt` file tracks installed packages.
- Console output shows processing time and thread count (for parallel).