# Maze3D_Console (ray casting - same as Wolfenstein 3D)
3D rendering in console/command prompt/ASCII:

![Snapshot](/Snapshot.png)

Prepared to be opened with:
- MS Visual Studio
- CodeBlocks

How to control the viewer:
- A, S, D, W: to move forward/backward and strafe left/right
- Left arrow, right arrow: to rotate left/right
- E, C: jump, crunch

Some design aims:
- use texture on walls, as opposed to many other similar implementations
- try to avoid using FPU (many devices work faster with integers; some devices have no FPU - you need workarounds with integers/LUTs/etc)
- don't sacrifice performance in favor of simpler implementation
- try using standard libraries only (aiming for cross IDE, cross platform)
- no extended ASCII chars as sometimes they are not really standard
- no colors as not all consoles support colors
- stick to the 80x25 screen resolution as some consoles only support that one (but make it configurable for custom resolutions)
- as the resolution is really low, improve the visual appearance:
    - use texture with little details
    - make sure the walls border is always drawn
    - anti-aliasing with smaller chars at texture edges
	
(handling key pressing is implemented for Windows only so far)
