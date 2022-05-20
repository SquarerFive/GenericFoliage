# GenericFoliage
This is a generic approach to spawning foliage on planetary environments, it is intended to be fast and adjustable by the developer. 

Currently render targets (colour, normal, depth) are used to generate the foliage points, but other paths of spawning will also be added.

**Work in progress**

# Current Features
- Foliage points are sampled from render targets for colour, normal and depth so line traces aren't used.
- Works on planets and flat surfaces.
- HISMs are partitioned into tiles, so they can gradually be updated without impacting performance too signficantly.
- Collision is only enabled on the active tile (closest to the camera). This is done to speed up the time taken to add instances to the HISM.

![Sampling using GPU scene depth and world normals](Resources/Screenshot_235.png)

## Example in Cesium for Unreal

See https://github.com/SquarerFive/Procedural for the example project

![Ground Screenshot](Resources/HighresScreenshot00001.png)
![Air screenshot](Resources/HighresScreenshot00000.png)