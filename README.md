# Maze3D_Console (ray casting)
3D rendering in console:
![Snapshot](/Snapshot.png)

Prepared to be opened with:
- MS Visual Studio
- CodeBlocks

How to control the viewer:
- A, S, D, W: to move forward/backward and strafe left/right
- Left arrow, right arrow: to rotate left/right

Some design aims:
- use texture on walls, as opposed to many other similar implementations
- try to avoid using FPU (many devices work faster with integers; some devices have no FPU - you need workarounds with integers/LUTs/etc)
- try using standard libraries (aiming for cross IDE, cross platform)
- no extended ASCII chars as they are sometimes not really standard
- no colors as not all consoles support colors
- as the resolution is really low, improve the visual appearance:
    - use texture with little details
    - make sure the walls bounder is always drawn
    - anti-aliasing with smaller chars at texture edges
	
