#pragma once
#ifndef CE_EDIT_PROJECT_FILETREE_H
#define CE_EDIT_PROJECT_FILETREE_H

#include "CloakEngine/CloakEngine.h"
#include "AFXCore.h"

#include <vector>

namespace Editor {
	namespace Project {
		class FileTreeItem {
			friend class FileTree;
			public:
				CLOAK_CALL FileTreeItem(In UINT contextMenu);
				CLOAK_CALL ~FileTreeItem();
				std::u16string CLOAK_CALL_THIS GetName() const;
				size_t CLOAK_CALL_THIS GetChildCount() const;
				FileTreeItem* CLOAK_CALL_THIS AddItem(In UINT contextMenu, In HTREEITEM item, In const std::u16string& name, In int img, In int selImg);
				void CLOAK_CALL_THIS RemoveItem(In size_t pos);
				FileTreeItem* CLOAK_CALL_THIS GetChild(In size_t pos) const;
				void CLOAK_CALL_THIS SetHandle(In HTREEITEM item);
				void CLOAK_CALL_THIS SetName(In const std::u16string& name);
				void CLOAK_CALL_THIS SetImage(In int img);
				void CLOAK_CALL_THIS SetImage(In int img, int selImg);
				HTREEITEM CLOAK_CALL_THIS GetHandle() const;
				UINT CLOAK_CALL_THIS GetContextMenu() const;
				int CLOAK_CALL_THIS GetImage() const;
				int CLOAK_CALL_THIS GetSelectedImage() const;
			protected:
				std::vector<FileTreeItem*> m_childs;
				HTREEITEM m_handle;
				std::u16string m_name;
				int m_img;
				int m_selImg;
				const UINT m_contextMenu;
		};
		class FileTree {
			public:
				CLOAK_CALL FileTree(In const std::u16string& projName);
				FileTreeItem* CLOAK_CALL_THIS GetRoot();
				FileTreeItem* CLOAK_CALL_THIS GetImagesRoot() const;
			protected:
				FileTreeItem m_root;
				FileTreeItem* m_imgs;
		};
	}
}

#endif