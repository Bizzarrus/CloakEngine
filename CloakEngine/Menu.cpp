#include "stdafx.h"
#include "CloakEngine/Interface/Menu.h"

#include "Engine/LinkedList.h"

#include <unordered_set>

namespace CloakEngine {
	namespace API {
		namespace Interface {
			inline namespace Menu_v1 {
				CLOAKENGINE_API bool CLOAK_CALL MENU_DESC::IsValid() const
				{
					if (MenuSize == 0) { return true; }
					if (Menu == nullptr) { return false; }
					API::Stack<const MENU_ITEM*> stack;
					std::unordered_set<ButtonID> ids;
					for (size_t a = 0; a < MenuSize; a++)
					{
						const MENU_ITEM* i = &Menu[a];
						if (i->SubMenuSize > 0 && i->SubMenu == nullptr) { return false; }
						if (ids.find(i->ID) != ids.end()) { return false; }
						ids.insert(i->ID);
						if (i->SubMenuSize > 0) { stack.push(i); }
					}
					while (stack.size() > 0)
					{
						const MENU_ITEM* s = stack.top();
						for (size_t a = 0; a < s->SubMenuSize; a++)
						{
							const MENU_ITEM* i = &s->SubMenu[a];
							if (i->SubMenuSize > 0 && i->SubMenu == nullptr) { return false; }
							if (ids.find(i->ID) != ids.end()) { return false; }
							ids.insert(i->ID);
							if (i->SubMenuSize > 0) { stack.push(i); }
						}
						stack.pop();
					}
					return true;
				}
			}
		}
	}
}