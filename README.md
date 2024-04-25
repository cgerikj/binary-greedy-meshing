# Binary Greedy Meshing

Fast voxel meshing algorithm that creates 'greedy' meshes using extremely fast bitwise operations. Supports voxel types and bakes ambient occlusion.

This repository serves as a simple example project for the algorithm.

Demo of a larger world: https://www.youtube.com/watch?v=LxfDmF0HxSg

**UPDATE: 2024-04-19:**  
**The ambient occlusion implementation was fixed - it should look better now.**

**UPDATE: 2024-04-21:**  
**Optimized the code to be up to 25% faster.**

**UPDATE: 2024-04-25:**  
**Optimized the code to be up to 15% faster and allocate less memory on every run.**

## Setup example (Visual Studio)
```
> git clone https://github.com/cgerikj/binary-greedy-meshing --recursive
> cd binary-greedy-meshing
> mkdir build && cd build
> cmake .. -G "Visual Studio 17 2022"
> start binaryMesher.sln
> (Switch to Release Mode / RelWithDebInfo)
```

## Demo usage

- Noclip: WASD
- Toggle wireframe: X
- Regenerate: Spacebar
- Cycle mesh type: Tab

Meshing duration is printed to the console.

## How to use the mesher
The main implementation is in src/mesher.h

### Input data:  
**- std::vector<uint8_t>& voxels**  
&nbsp;&nbsp;&nbsp;&nbsp; - The input data includes duplicate edge data from neighboring chunks which is used for visibility culling and AO. For optimal performance, your world data should already be structured this way so that you can feed the data straight into this algorithm.  
&nbsp;&nbsp;&nbsp;&nbsp; - Input data is ordered in YXZ and is 64^3 which results in a 62^3 mesh. 

**- MeshData& meshData**  
&nbsp;&nbsp;&nbsp;&nbsp; - Data that needs to be allocated once (per thread if meshing with multiple threads)  

**- bool bake_ao**  
&nbsp;&nbsp;&nbsp;&nbsp; - true if you want baked ambient occlusion.  

### Output data:  
&nbsp;&nbsp;&nbsp;&nbsp; - The allocated vertices in MeshData with a length of meshData.vertexCount.

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
Average execution time (1000+ runs)

| Scene                     | Ryzen 7 3800x (Windows)      | Ryzen 7 5800h (Linux) | Ryzen 3950x (Windows) |
|:--------------------------|:-----------------------------|-----------------------|-----------------------|
| 3d hills (AO)             | 552us / 43021 verts          | 502us                 | 615ms
| 3d hills (No AO)          | 325us / 24751 verts          | 
| Red sphere (AO)           | 656us / 71533 verts          | 592us
| Red sphere (No AO)        | 317us / 43201 verts          |
| Empty (AO)                | 148us / 0 verts              | 221us
| Empty (No AO)             | 125us / 0 verts              |
| White noise (AO)          | 10.211ms / 1594069 verts     | 7.509ms
| White noise (No AO)       | 3.545ms / 1415419 verts      | 
| 3d checkerboard (AO)      | 11.696ms / 4289904 verts     | 8.154ms
| 3d checkerboard (No AO)   | 6.271ms / 4289904 verts      |

## Other resources
### Meshing in a minecraft game:
https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/  

### Ambient occlusion for Minecraft-like worlds
https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/