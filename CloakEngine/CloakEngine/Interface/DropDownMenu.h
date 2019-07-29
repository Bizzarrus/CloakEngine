#pragma once
#ifndef CE_API_INTERFACE_DROPDOWNMENU_H
#define CE_API_INTERFACE_DROPDOWNMENU_H

#include "CloakEngine/Interface/Menu.h"
#include "CloakEngine/Interface/Button.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace DropDownMenu_v1 {
				CLOAK_INTERFACE_ID("{6632D679-0237-4379-809F-1FD8D08AF905}") IDropDownMenu : public virtual Menu_v1::IMenu, public virtual Button_v1::IButton {
					public:
						virtual void CLOAK_CALL_THIS Open() = 0;
						virtual void CLOAK_CALL_THIS Close() = 0;
						virtual bool CLOAK_CALL_THIS IsOpen() const = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif