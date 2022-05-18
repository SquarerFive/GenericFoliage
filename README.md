# GenericFoliage
Generic foliage plugin for UE5

**Work in progress**

# Current Features
- Scene capture depth & world normals are used to spawn each instance rather than using line traces. The captured data is copied onto the CPU using a compute shader.
- In Editor Generic Foliage asset type which can be saved in the content browser
- Chunked spawner. By default 25k instances are spawned per tile, per tick.
- Near-player collision, collision is only enabled for the instances in the nearest tile to the player. This reduces the amount of time it takes to spawn the foliage.
- Async foliage builder. Foliage transforms are calculated on a background thread to prevent it from blocking the game thread. It also has the ability to run in parallel (using ParallelFor) at the cost of memory if needed.

# Known issues
- Stutter & delay when rebuilding a tile

![Sampling using GPU scene depth and world normals](Resources/Screenshot_235.png)

## Example in Cesium for Unreal

See https://github.com/SquarerFive/Procedural for the example project

![Ground Screenshot](Resources/HighresScreenshot00001.png)
![Air screenshot](Resources/HighresScreenshot00000.png)