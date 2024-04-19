# Binary Greedy Meshing

Fast voxel meshing algorithm that creates 'greedy' meshes using extremely fast bitwise operations. Supports voxel types and bakes ambient occlusion.

This repository serves as a simple example project for the algorithm.

Demo of a larger world: https://www.youtube.com/watch?v=LxfDmF0HxSg

Video by youtuber Tantan that showcases his Rust port and explains part of the algorithm: https://www.youtube.com/watch?v=qnGoGq7DWMc

**UPDATE: 2024-04-19:**  
**The ambient occlusion implementation was fixed - it should look better now.**

## Setup example (Visual Studio)
```
> git clone https://github.com/cgerikj/binary-greedy-meshing --recursive
> cd binary-greedy-meshing
> mkdir build && cd build
> cmake .. -G "Visual Studio 16 2019"
> start binaryMesher.sln
```

## Demo usage

- Noclip: WASD
- Toggle wireframe: X
- Regenerate: Spacebar
- Cycle mesh type: Tab

Meshing duration is printed to the console.

## Algorithm usage
The main implementation is in src/mesher.h

Input data:  
std::vector<uint8_t> voxels (values 0-31 usable)  

* The input data includes duplicate edge data from neighboring chunks which is used for visibility culling and AO. For optimal performance, your world data should already be structured this way so that you can feed the data straight into this algorithm.

* Input data is ordered in YXZ and is 64^3 which results in a 62^3 mesh. 

Output data:  
std::vector<uint32_t> of vertices in chunk-space.

## Mesh details

Vertex data is packed into one unsigned integer:
- x, y, z: 6 bit each (0-63)
- Type: 8 bit (0-255)
- Normal: 3 bit (0-5)
- AO: 2 bit

Meshes can be offset to world space using a per-draw uniform or by packing xyz in gl_BaseInstance if rendering with glMultiDrawArraysIndirect.

## Screenshots
| Mesh                       | Wireframe                  |
| -------------------------- |:--------------------------:|
| ![](screenshots/cap1.png)  | ![](screenshots/cap2.png)  |
| ![](screenshots/cap7.png)  | ![](screenshots/cap8.png)  |
| ![](screenshots/cap3.png)  | ![](screenshots/cap4.png)  |
| ![](screenshots/cap5.png)  | ![](screenshots/cap6.png)  |

## Benchmarks
Average execution time running on Ryzen 3800x.

| Scene                 | Milliseconds   | Vertices   |
| ---------------------:|:--------------:|:----------:|
| 3d hills (AO)         | 0.774          | 41753      |
| 3d hills (No AO)      | 0.450          | 28378      |
| Red sphere (AO)       | 0.785          | 71532      |
| White noise (AO)      | 13.24          | 1596720    |
| 3d checkerboard (AO)  | 22.27          | 4289904    |
| Empty (AO)            | 0.174          | 0          |
