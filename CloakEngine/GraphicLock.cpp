#include "stdafx.h"
#include "Engine/Graphic/GraphicLock.h"
#include "Engine/Graphic/Core.h"
#include "Engine/WindowHandler.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Game.h"

#include "Implementation/Global/Game.h"
#include "Implementation/Global/Mouse.h"
#include "Implementation/Interface/BasicGUI.h"
#include "Implementation/Global/Debug.h"
#include "Implementation/Global/Graphic.h"

#include <atomic>

#define WAIT_TIME 128
#define MAX_FRAME_BATCH 3
#define NUM_PHASES 4

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {
			namespace Lock {
				struct ResizeUpdate {
					bool HasUpdate = false;
					UINT Width = 0;
					UINT Height = 0;
				};
				struct SettingsUpdate {
					bool HasUpdate = false;
					API::Global::Graphic::Settings Settings;
					SettingsFlag Flag = SettingsFlag::None;
				};
				std::atomic<bool> g_everUpdSettings = false;
				std::atomic<ResizeUpdate> g_ResizeUpdate;
				std::atomic<SettingsUpdate> g_SetUpdate;

				inline bool CLOAK_CALL WaitForSettings()
				{
					ResizeUpdate resize = g_ResizeUpdate.load();
					SettingsUpdate setting = g_SetUpdate.load();
					return resize.HasUpdate || setting.HasUpdate;
				}

				//Since updateing graphic settings may recreate some resources, we do this in a single thread in order to prevent
				//using illegal resource pointer
				bool CLOAK_CALL CheckSettingsUpdateMaster()
				{
					if (WaitForSettings() == true)
					{
						ResizeUpdate nr;
						nr.HasUpdate = false;
						ResizeUpdate resize = g_ResizeUpdate.exchange(nr);
						SettingsUpdate nu;
						nu.HasUpdate = false;
						SettingsUpdate setting = g_SetUpdate.exchange(nu);
						if (g_everUpdSettings.exchange(true) == false)
						{
							setting.Flag |= SettingsFlag::Shader;
						}

						Core::WaitForGPU();
						if (setting.HasUpdate == true)
						{
							API::Global::Graphic::Settings newSet;
							Impl::Global::Graphic::GetModifedSettings(&newSet);
							WindowHandler::updateSettings(newSet, setting.Settings);
							Core::UpdateSettings(newSet, setting.Settings, setting.Flag);
						}
						if (resize.HasUpdate == true) { Core::ResizeBuffers(resize.Width, resize.Height); }
						if (setting.HasUpdate == true || resize.HasUpdate == true) { Impl::Global::Mouse::CheckClipping(); }
						Core::WaitForGPU();
					}
					return g_everUpdSettings;
				}
				void CLOAK_CALL UpdateSettings(In const API::Global::Graphic::Settings& oldSet, In_opt SettingsFlag flag)
				{
					SettingsUpdate last = g_SetUpdate.load();
					SettingsUpdate next;
					do {
						next = last;
						if (last.HasUpdate == false)
						{
							next.HasUpdate = true;
							next.Settings = oldSet;
						}
						next.Flag |= flag;
					} while (!g_SetUpdate.compare_exchange_strong(last, next));
				}
				void CLOAK_CALL RegisterResizeBuffers(In UINT width, In UINT height)
				{
					ResizeUpdate last = g_ResizeUpdate.load();
					ResizeUpdate next;
					do {
						next = last;
						next.HasUpdate = true;
						next.Width = width;
						next.Height = height;
					} while (!g_ResizeUpdate.compare_exchange_strong(last, next));
				}
			}
		}
	}
}