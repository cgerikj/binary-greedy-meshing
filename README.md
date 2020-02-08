# Binary Greedy Meshing

Fast voxel meshing algorithm - creates 'greedy' meshes with support for voxel types, baked light & Ambient Occlusion.
UVs can easily be added but the vertex structure would have to be changed from a single integer.

## Installation example (visual studio)
```
git clone git@github.com:cgerikj/binary-greedy-meshing.git --recursive
cd binary-greedy-meshing
cmake -G "Visual Studio 16 2019" .
```

Open binaryMesher.sln

## Mesh details

Vertex data is packed into one unsigned integer:
- x, y, z: 6 bit each (0-63)
- Type: 5 bit (0-31)
- Light: 4 bit (0-15)
- Normal: 3 bit (0-5)
- AO: 2 bit

Meshes can be offset to world space using a per-draw uniform or by packing xyz in gl_BaseInstance if rendering with glMultiDrawArraysIndirect.

## Usage

- Noclip: WASD
- Toggle wireframe: X
- Regenerate: Spacebar
- Cycle mesh type: Tab

Meshing duration is printed to the console.

## Screenshots
![Mesh 1](screenshots/cap1.png)
![Wireframe 1](screenshots/cap2.png)
![Mesh 2](screenshots/cap3.png)
![Wireframe 2](screenshots/cap4.png)

## Benchmarks
Average execution time running on Ryzen 3800x.

| Scene           | Milliseconds   | Vertices   |
| --------------- |:--------------:|:----------:|
| 1 - 3d hills    | 1.139          | 46798      |
| 2 - White noise | 33.315         | 2142608    |