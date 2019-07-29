#include "stdafx.h"
#include "Project/FileTree.h"

#include "resource.h"

namespace Editor {
	namespace Project {
		CLOAK_CALL FileTreeItem::FileTreeItem(In UINT contextMenu) : m_contextMenu(contextMenu)
		{
			m_handle = nullptr;
			m_img = 0;
			m_name = u"<Unnamed>";
			m_selImg = 0;
		}
		CLOAK_CALL FileTreeItem::~FileTreeItem()
		{
			for (size_t a = 0; a < m_childs.size(); a++) { delete m_childs[a]; }
		}
		std::u16string CLOAK_CALL_THIS FileTreeItem::GetName() const { return m_name; }
		size_t CLOAK_CALL_THIS FileTreeItem::GetChildCount() const { return m_childs.size(); }
		FileTreeItem* CLOAK_CALL_THIS FileTreeItem::AddItem(In UINT contextMenu, In HTREEITEM item, In const std::u16string& name, In int img, In int selImg)
		{
			FileTreeItem* i = new FileTreeItem(contextMenu);
			i->SetHandle(item);
			i->SetImage(img, selImg);
			i->SetName(name);
			m_childs.push_back(i);
			return i;
		}
		void CLOAK_CALL_THIS FileTreeItem::RemoveItem(In size_t pos)
		{
			if (pos < m_childs.size())
			{
				delete m_childs[pos];
				if (pos + 1 < m_childs.size()) { m_childs[pos] = m_childs[m_childs.size() - 1]; }
				m_childs.pop_back();
			}
		}
		FileTreeItem* CLOAK_CALL_THIS FileTreeItem::GetChild(In size_t pos) const
		{
			if (pos < m_childs.size()) { return m_childs[pos]; }
			return nullptr;
		}
		void CLOAK_CALL_THIS FileTreeItem::SetName(In const std::u16string& name) { m_name = name; }
		void CLOAK_CALL_THIS FileTreeItem::SetImage(In int img) { SetImage(img, img); }
		void CLOAK_CALL_THIS FileTreeItem::SetImage(In int img, int selImg) { m_img = img; m_selImg = selImg; }
		void CLOAK_CALL_THIS FileTreeItem::SetHandle(In HTREEITEM item) { m_handle = item; }
		HTREEITEM CLOAK_CALL_THIS FileTreeItem::GetHandle() const { return m_handle; }
		UINT CLOAK_CALL_THIS FileTreeItem::GetContextMenu() const { return m_contextMenu; }
		int CLOAK_CALL_THIS FileTreeItem::GetImage() const { return m_img; }
		int CLOAK_CALL_THIS FileTreeItem::GetSelectedImage() const { return m_selImg; }

		CLOAK_CALL FileTree::FileTree(In const std::u16string& projName) : m_root(IDR_CM_PROJECT)
		{
			m_root.SetName(projName);
			m_imgs = m_root.AddItem(IDR_CM_IMGS_MAIN, nullptr, u"Images", 0/*TODO*/, 0/*TODO*/);
		}
		FileTreeItem* CLOAK_CALL_THIS FileTree::GetRoot()
		{
			return &m_root;
		}
		FileTreeItem* CLOAK_CALL_THIS FileTree::GetImagesRoot() const
		{
			return m_imgs;
		}
	}
}