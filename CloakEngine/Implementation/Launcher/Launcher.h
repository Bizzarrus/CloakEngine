#pragma once
#ifndef CE_IMPL_LAUNCHER_LAUNCHER_H
#define CE_IMPL_LAUNCHER_LAUNCHER_H

#include "CloakEngine/Launcher/Launcher.h"
#include "Implementation/Helper/SavePtr.h"

#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Launcher {
			inline namespace Launcher_v1 {
				class Launcher : public API::Launcher::Launcher_v1::ILauncher, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL Launcher();
						virtual CLOAK_CALL ~Launcher();

						void CLOAK_CALL_THIS Init();
						void CLOAK_CALL_THIS SendResultAndClose(In bool success, In_opt bool destroy = true);
						bool CLOAK_CALL_THIS IsOpen() const;
						HWND CLOAK_CALL_THIS GetWindow() const;

						void CLOAK_CALL_THIS SetWidth(In float w) override;
						void CLOAK_CALL_THIS SetHeight(In float h) override;
						void CLOAK_CALL_THIS SetName(In const std::u16string& name) override;
					private:
						std::atomic<bool> m_isOpen;
						std::atomic<bool> m_isInit;
						float m_width;
						float m_height;
						HWND m_wind;
						std::wstring m_name;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif