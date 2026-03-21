# DOSBox-VirtIsa

This fork focuses on building an emulation environment for a 
[custom ISA card](https://github.com/ifilot/slot-otter).

## Purpose of this fork

- Keep a DOSBox-based runtime for ISA development and experiments.
- Add a dedicated ISA emulation insertion point that is easy to tune (`src/isa/isa.cpp`).
- Provide an automated GitHub Actions compile check.

## Build instructions

### Linux (CMake)

1. Install build dependencies:
   - `cmake`
   - `build-essential`
   - `libsdl1.2-dev`
   - `libsdl-net1.2-dev`
   - `libpng-dev`
   - `libasound2-dev`
   - `libgl1-mesa-dev`
2. Configure and build:

   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ```

3. Run:

   ```bash
   ./build/dosbox
   ```
