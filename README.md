# 🛰️ Orbital Debris Collision Avoidance System

[![C++](https://img.shields.io/badge/C++-14%2B-blue.svg)](https://isocpp.org/)
[![Python](https://img.shields.io/badge/Python-3.8%2B-yellow.svg)](https://www.python.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)

A 3D simulation tool that computes orbital trajectories, maps satellite fleets, and detects potential collisions with space debris. 

This project was built to combine software engineering with orbital mechanics. It reads Two-Line Element (TLE) data, calculates Earth-Centered Inertial (ECI) coordinates, and uses an Octree algorithm to make 3D collision detection fast and efficient for large datasets.

## 🚀 What it does

* **Orbital Math:** Uses the Newton-Raphson method to solve Kepler's Equation, converting standard orbital elements` into 3D Cartesian coordinates.
* **Octree Spatial Partitioning:** To avoid checking every single satellite against every piece of debris (`O(n^2)`), it was implemented a custom 3D Octree in C++. This drops the search time complexity to `O(n \log n)`, allowing for quick scans across large fleets without dropping frames.
* **Collision Detection:** Uses an Axis-Aligned Bounding Box (AABB) approach to scan a specific radius (e.g., 50 km) around a target satellite to flag dangerous neighbors.
* **3D Visualization:** A Python script that reads the C++ output and renders an interactive 3D view of Earth, active satellites, debris (like Iridium-33 and Fengyun-1C fragments), and collision threat vectors.

## 🛠️ How it works

The system is split into a C++ engine for heavy calculations and a Python script for visualization, communicating only through `.csv` files.

**1. C++ Engine (`detc.cpp`)**
* Parses TLE text files to extract satellite data.
* Calculates 2D orbital planes.
* Applies 3D rotation matrices to map coordinates into the ECI frame.
* Builds the Octree, runs the collision query, and exports the data.

**2. Python Visualizer (`render.py`)**
* Loads the CSV data using Pandas.
* Plots the Earth, the satellite fleet, and draws lines pointing to nearby collision threats using Matplotlib 3D.

## 🧮 The Math Behind It

The code relies on a few core orbital mechanics concepts:
* **Kepler's Equation:** `M = E - e * sin(E)` (solved iteratively).
* **Kepler's Third Law:** Used to find the semi-major axis from the Mean Motion.
* **Coordinate Transformation:** Uses RAAN (Ω), Inclination (i), and Argument of Perigee (ω) in a 3x3 matrix to rotate the 2D plane into 3D space.

## 📂 Structure

```text
├── src/
│   ├── detc.cpp               # Core C++ engine and Octree implementation
│   └── render.py              # Python 3D rendering script
├── data/
│   ├── active.txt             # TLE data for active satellites
│   ├── iridium-33-debris.txt  # TLE data for Iridium-33 collision fragments
│   └── fengyun-1c-debris.txt  # TLE data for Fengyun-1C ASAT debris
├── output/
│   ├── orbital_data.csv       # Generated fleet coordinates
│   └── collision_alert.csv    # Generated threat coordinates
└── README.md
```

## ⚙️ Quick Start

### **Pre-requisites:**
  
* **C++ Compiler:** GCC or Clang

* **Python 3:** Ensure you have a working Python environment.

### **Build and Run (terminal):**

* **1. Get the Code:**

Download the repository as a ZIP file (click the green **Code** button > **Download ZIP**) and extract it. Then, open your terminal and navigate to the project folder:

`cd path/to/Orbital-Debris-Collision-Avoidance-System`
  
* **2. Install Python Dependencies:**

`pip install pandas numpy matplotlib`

* **3. Compile the C++ Engine:**

`g++ -O3 src/detc.cpp -o detc`

* **4. Run the calculation:**

`./detc`

* **5. Open the 3D Visualizer:**

`python src/render.py`

## ⌨️ Interactive Controls (3D Visualizer in Python)

* **Z:** Toggle zoom on the target satellite.
  
* **1:** Show/hide active satellites (Green).
  
* **2:** Show/hide space debris (Red).
  
