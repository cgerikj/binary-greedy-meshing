# Binary Greedy Meshing

Fast voxel meshing algorithm - creates 'greedy' meshes with support for voxel types, baked light & UVs.

## Installation (visual studio)
```
git clone git@github.com:cgerikj/binary-greedy-meshing.git --recursive
```
```
cmake -G "Visual Studio 16 2019" .
```

Open binaryMesher.sln

## Usage

- Noclip: WASD
- Toggle wireframe: X
- Regenerate: Spacebar

Meshing duration is printed to the console.

## Screenshots
![Mesh](screenshots/cap1.png)
![Wireframe](screenshots/cap2.png)

## Benchmarks
Basic benchmarks for the terrain in the screenshot (Ryzen 3800x):  
meshing took 712us (0.712ms)  
vertex count: 29964  
meshing took 651us (0.651ms)  
vertex count: 24288  
meshing took 802us (0.802ms)  
vertex count: 32808  
meshing took 676us (0.676ms)  
vertex count: 31188  
meshing took 660us (0.66ms)  
vertex count: 28770  
meshing took 708us (0.708ms)  
vertex count: 31806  
meshing took 691us (0.691ms)  
vertex count: 29670  
meshing took 653us (0.653ms)  
vertex count: 28776  
meshing took 750us (0.75ms)  
vertex count: 29652  

## Contributing
Pull requests are welcome. (AO and indexing is still missing)

## License
[MIT](https://choosealicense.com/licenses/mit/)