# CloakEngine
CloakEngine is a self-written 3D Open World Game Engine, written in C++. The project is currently work-in-progress and far from finished. Some code-parts are based on 3rd-party example implementations or corresponding papers, more information is stated at the specific places of the source files in the comments.
# Dependencies
The engines uses the SteamAPI and tiny_obj_loader (corresponding files are contained in the reprository). Other dependencies are the following projects within VCPKG:
- concurrentqueue
- LZ4
- zLib
In addition to that, the engine requires the current Windows 10 API (Version 10.0.18362.0), including DirectX 12.
# Projects and current state
## CloakCompiler
This project handles offline-computation and compression of file types (images, meshes, shader, translation files, fonts). 
## CloakEditor
The CloakEditor is planed to be the GUI for developing with the engine. Due to the current state of the engine itself, this project is currently not in development and my not work at all.
## CloakEngine
The main engine. Currently supported features include:
- DirectX 12 based rendering engine
- Task-based multithreading using one thread per core
- Specialized memory allocators for fast and cache-friendly memory allocation
- Component based entity system
- Dynamic world streaming, supporting partially loaded files for faster load times
- Basic 2D interface

Furthermore, the following features and changes are currently planned or being developted (not in this repository):
- Swapping the shadow mapping technique from regulare MSM to Non-Linearly Quantized MSM
- Implementing clustered shading for dynamic light sources and improved culling methods
- More variations for task scheduling (priority and schedule hints)
- Asynchronous memory reordering within component management for better cache utilization
- Swapping from per-thread to data-driven world simulation
- Improving 2D interface implementations
- Implementing DirectX 11, Vulkan and OpenGL rendering engines and DXR support
- Implementing Physics, Animation and Sound sub-systems
- Implementing support for Windows 8.1 and other operating systems
## CloakEngineTest
A executable program to test most currently avaiblabe features of the engine, using simple and free available geometry, materials and images. Camera can be controlled with WASD, the mouse cursor can be made visible by holding the ALT key. Available commands for the additional command-line window can be seen by entering ``?`` or ``cloak help``.
# Notice
The engine is completly developed by myself and currenlty only written for private usage.
