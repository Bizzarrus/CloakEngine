#pragma once
#ifndef CE_E_GAMECORELOOP_H
#define CE_E_GAMECORELOOP_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Graphic.h"

//Implementation of the core game loop, following this frame-task-dependency graphs:
//--------------------------------------------------------------------------------------------//
//----------------------------Loop with Window - IDPL 0---------------------------------------//
//--------------------------------------------------------------------------------------------//
//      ║                                 ║             ║          ║        ║         ║       //
//  Read Input                         Read AI          ║          ║        ║         ║       //
//      ║ ╔═════════════════════════════╝ ║      Update Visibility ║        ║         ║       //
// Process Input                          ║             ║          ║        ║         ║       //
//      ║                                 ║      Update Animations ║        ║         ║       //
//  Update Game                           ║             ╚════════╗ ║        ║         ║       //
//      ║ ╚══════╗                    Process AI          Mirror Game State ║         ║       //
//      ║        ║ ╔══════════════════════║══════════════════════╝ ╚══════╗ ║         ║       //
//      ║   Update Components             ║                            Copy Mirror    ║       //
//      ║        ║ ╚══════════════════════║═══╗                    ╔══════╝ ║ ╔═══════╝       //
//      ║        ║ ╔══════════════════════╣   ║                    ║  Render Submit           //
//      ║   Update Positions         Writeback AI                  ║        ║ ╚═══════╗       //
//      ║         ║                       ║                        ║        ║  Render Present //
//      ║         ║                Read Animation State            ║        ║         ║       //
//      ║         ╚═════════════════════╗ ║                        ║        ║         ║       //
//      ║                           Update Root Motion             ║        ║         ║       //
//      ║                                 ║                        ║        ║         ║       //
//      ║                             Update Physics               ║        ║         ║       //
//      ║                                 ║ ╚═══════════╗          ║        ║         ║       //
//      ║                        Audio Source Update    ║          ║        ║         ║       //
//      ║                                 ║             ║          ║        ║         ║       //
//--------------------------------------------------------------------------------------------//
//
//
//
//--------------------------------------------------------------------------------------------//
//----------------------------Loop with Window - IDPL 1---------------------------------------//
//--------------------------------------------------------------------------------------------//
//                                        ║             ║          ║        ║         ║       //
//                                     Read AI          ║          ║        ║         ║       //
//               ╔══════════════════════╝ ║             ║          ║        ║         ║       //
//           Read Input                   ║      Update Visibility ║        ║         ║       //
//               ║                        ║             ║          ║        ║         ║       //
//         Process Input                  ║      Update Animations ║        ║         ║       //
//               ║                        ║             ╚════════╗ ║        ║         ║       //
//          Update Game               Process AI          Mirror Game State ║         ║       //
//               ║ ╔══════════════════════║══════════════════════╝ ╚══════╗ ║         ║       //
//          Update Components             ║                            Copy Mirror    ║       //
//               ║ ╚══════════════════════║═══╗                    ╔══════╝ ║ ╔═══════╝       //
//               ║ ╔══════════════════════╣   ║                    ║  Render Submit           //
//          Update Positions         Writeback AI                  ║        ║ ╚═══════╗       //
//                ║                       ║                        ║        ║  Render Present //
//                ║                Read Animation State            ║        ║         ║       //
//                ╚═════════════════════╗ ║                        ║        ║         ║       //
//                                  Update Root Motion             ║        ║         ║       //
//                                        ║                        ║        ║         ║       //
//                                    Update Physics               ║        ║         ║       //
//                                        ║ ╚═══════════╗          ║        ║         ║       //
//                               Audio Source Update    ║          ║        ║         ║       //
//                                        ║             ║          ║        ║         ║       //
//--------------------------------------------------------------------------------------------//
//
//
//
//--------------------------------------------------------------------------------------------//
//----------------------------Loop with Window - IDPL 2---------------------------------------//
//--------------------------------------------------------------------------------------------//
//                                        ║             ║          ║                  ║       //
//                                     Read AI          ║          ║                  ║       //
//               ╔══════════════════════╝ ║             ║          ║                  ║       //
//           Read Input                   ║      Update Visibility ║                  ║       //
//               ║                        ║             ║          ║                  ║       //
//         Process Input                  ║      Update Animations ║                  ║       //
//               ║                        ║             ╚════════╗ ║                  ║       //
//          Update Game               Process AI          Mirror Game State           ║       //
//               ║ ╔══════════════════════║══════════════════════╝ ╚═══════╗          ║       //
//          Update Components             ║                            Copy Mirror    ║       //
//               ║ ╚══════════════════════║═══╗                            ║  ╔═══════╝       //
//               ║ ╔══════════════════════╣   ║                       Render Submit           //
//          Update Positions         Writeback AI                  ╔═══════╝  ╚═══════╗       //
//                ║                       ║                        ║           Render Present //
//                ║                Read Animation State            ║                  ║       //
//                ╚═════════════════════╗ ║                        ║                  ║       //
//                                  Update Root Motion             ║                  ║       //
//                                        ║                        ║                  ║       //
//                                    Update Physics               ║                  ║       //
//                                        ║ ╚═══════════╗          ║                  ║       //
//                               Audio Source Update    ║          ║                  ║       //
//                                        ║             ║          ║                  ║       //
//--------------------------------------------------------------------------------------------//
//
//
//
//--------------------------------------------------------------------------------------------//
//----------------------------Loop with Window - IDPL 3---------------------------------------//
//--------------------------------------------------------------------------------------------//
//                                        ║             ║                ║                    //
//                                     Read AI          ║                ║                    //
//               ╔══════════════════════╝ ║             ║                ║                    //
//           Read Input                   ║      Update Visibility       ║                    //
//               ║                        ║             ║                ║                    //
//         Process Input                  ║      Update Animations       ║                    //
//               ║                        ║             ╚════════╗ ╔═════╝                    //
//          Update Game               Process AI          Mirror Game State                   //
//               ║ ╔══════════════════════║══════════════════════╝ ╚═════╗                    //
//          Update Components             ║                         Copy Mirror               //
//               ║ ╚══════════════════════║═══╗                          ║                    //
//               ║ ╔══════════════════════╣   ║                    Render Submit              //
//          Update Positions         Writeback AI                        ║                    //
//                ║                       ║                        Render Present             //
//                ║                Read Animation State                  ║                    //
//                ╚═════════════════════╗ ║                              ║                    //
//                                  Update Root Motion                   ║                    //
//                                        ║                              ║                    //
//                                    Update Physics                     ║                    //
//                                        ║ ╚═══════════╗                ║                    //
//                               Audio Source Update    ║                ║                    //
//                                        ║             ║                ║                    //
//--------------------------------------------------------------------------------------------//
//
//
//
//--------------------------------------------------------------------------------------------//
//-------------------------------Loop without Window------------------------------------------//
//--------------------------------------------------------------------------------------------//
//      ║                                 ║             ║                                     //
//  Read Input                         Read AI          ║                                     //
//      ║ ╔═════════════════════════════╝ ║      Update Visibility                            //
// Process Input                          ║             ║                                     //
//      ║                                 ║      Update Animations                            //
//  Update Game    ╔══════════════════════║═════════════╝                                     //
//      ║ ╚═════╗  ║                  Process AI                                              //
//      ║   Update Components             ║                                                   //
//      ║       ║  ╚══════════════════════║═══╗                                               //
//      ║       ║  ╔══════════════════════╣   ║                                               //
//      ║   Update Positions         Writeback AI                                             //
//      ║         ║                       ║                                                   //
//      ║         ║                Read Animation State                                       //
//      ║         ╚═════════════════════╗ ║                                                   //
//      ║                           Update Root Motion                                        //
//      ║                                 ║                                                   //
//      ║                             Update Physics                                          //
//      ║                                 ║ ╚═══════════╗                                     //
//      ║                        Audio Source Update    ║                                     //
//      ║                                 ║             ║                                     //
//--------------------------------------------------------------------------------------------//
//Notice: This graph resembles one frame. It is explicitly crafted to ensure no race
//conditions can occour during one frame, so no synchronisation with SyncSection and
//locks is needed for entites and components. 
//Explanation to each task:
//	- Read Input: Reads the input of all HIDs and saves them for later processing
//	- Read AI: Copies all AI-relevant informations, like position, current state, etc, 
//		into a temporary buffer for later processing
//	- Process Input: Processes the previously saved input data (executes key bindings, 
//		check if something was clicked on, etc)
//	- Update Game: User game update. Anything could happen here
//	- Process AI: Using the previously saved input data, calculates the new AI state,
//		movement, etc for each entity. This new data is written back to the temporary 
//		buffer, NOT to the components
//	- Update Components: Calling ComponentManager::FixAll to "activate" new components
//		and remove deleted ones
//	- Update Positions: Calculates new positions of all entites, based on AI and whatever
//		changes were made with the position component (by input processing or game update)
//	- Writeback AI: Copies AI state data back into the component
//	- Read Animation State: Copies and updates relevant animation state data and current 
//		game time for later animation calculations
//	- Update Root Motion: Updates the positions of all entites with the root motion of
//		their animation
//	- Update Physics: Runs the physics calculation and updates the positions of all entites
//		accordingly
//	- Update Visibility: Tests for all active cameras, which entities are visible or may cast
//		a shadow into the view frustrum of that camera, and saves that information in the 
//		coresponding component
//	- Update Animations: For all actually visible entites, updates their current animation
//		data according to the previously saved animation state
//	- Audio Source Update: Copies the position of all entities that may generate sound into 
//		the audio system
//	- Mirror Game State: Copies all information that is relevant for rendering (like position,
//		animation state, etc) of each visible entity into a buffer
//	- Copy Mirror: Swaps the buffer with the mirrored game state
//	- Render Submit: Fills and commits a new set of rendering commands for the current frame.
//		Notice: Only the very end needs to wait for the last Render Present to prevent over-
//		submitting, there are not other dependencies in that direction
//	- Render Present: Waits for the last frame and presents the current one
//
//
//
// Expected Input Delay in Frames (Not including display delay):
//	IDPL 0: 3 - 6
//  IDPL 1: 2 - 5
//  IDPL 2: 2 - 4
//  IDPL 3: 2 - 3


namespace CloakEngine {
	namespace Engine {
		namespace Game {
			void CLOAK_CALL Initialize(In bool hasWindow, In float targetFPS, In API::Global::IGameEvent* game);
			void CLOAK_CALL SignalStop();
			void CLOAK_CALL SetIDPL(In API::Global::Graphic::InputDelayPrevention idp);
			void CLOAK_CALL SetFPS(In float fps);
			float CLOAK_CALL GetFPS();
		}
	}
}

#endif