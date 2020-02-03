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
- Cycle mesh type: Tab

Meshing duration is printed to the console.

## Screenshots
![Mesh 1](screenshots/cap1.png)
![Wireframe 1](screenshots/cap2.png)
![Mesh 2](screenshots/cap3.png)
![Wireframe 2](screenshots/cap4.png)

## Benchmarks
Basic benchmarks for the terrain in the screenshot (Ryzen 3800x):
### Mesh 1:  
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

### Mesh 2 (Worst case scenario):  
meshing took 28523us (28.523ms)  
vertex count: 1826160  
meshing took 28231us (28.231ms)  
vertex count: 1825536  
meshing took 27119us (27.119ms)  
vertex count: 1822308  
meshing took 27138us (27.138ms)  
vertex count: 1826634  
meshing took 26825us (26.825ms)  
vertex count: 1825998  
meshing took 27076us (27.076ms)  
vertex count: 1824378  

## Contributing
Pull requests are welcome. (AO and indexing is still missing)

## License
[MIT](https://choosealicense.com/licenses/mit/)