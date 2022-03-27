/** 
 * @file lllistcontextmenu.cpp
 * @brief Base class of misc lists' context menus
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */


#include "llviewerprecompiledheaders.h"

#include "lllistcontextmenu.h"

// libs
#include "llmenugl.h" // for LLContextMenu

// newview
#include "llviewermenu.h" // for LLViewerMenuHolderGL
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-08-10 (Catznip-3.3)
#include "llinventorymodel.h"
#include "llviewerinventory.h"
// [/SL:KB]

LLListContextMenu::LLListContextMenu()
{
}

LLListContextMenu::~LLListContextMenu()
{
	// do not forget delete LLContextMenu* mMenu.
	// It can have registered Enable callbacks which are called from the LLMenuHolderGL::draw()
	// via selected item (menu_item_call) by calling LLMenuItemCallGL::buildDrawLabel.
	// we can have a crash via using callbacks of deleted instance of ContextMenu. EXT-4725

	// menu holder deletes its menus on viewer exit, so we have no way to determine if instance
	// of mMenu has already been deleted except of using LLHandle. EXT-4762.
	if (!mMenuHandle.isDead())
	{
		mMenuHandle.get()->die();
	}
}

// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2014-01-19 (Catznip-3.6)
void LLListContextMenu::show(LLView* spawning_view, const uuid_vec_t& uuids, S32 x, S32 y)
{
	if (uuids.empty())
	{
		return;
	}

	LLContextMenu* pMenu = mMenuHandle.get();
	if (pMenu)
	{
		pMenu->die();
		mMenuHandle.markDead();
		mUUIDs.clear();
	}

	mUUIDs.resize(uuids.size());
	std::copy(uuids.begin(), uuids.end(), mUUIDs.begin());

	pMenu = createMenu();
	if (!pMenu)
	{
		LL_WARNS() << "Context menu creation failed" << LL_ENDL;
		return;
	}
	mMenuHandle = pMenu->getHandle();

	if ( (-1 == x) && (-1 == y) )
	{
		pMenu->buildDrawLabels();
		pMenu->arrangeAndClear();

		LLRect rct = spawning_view->getRect();
		x = rct.mLeft;
		y = rct.mTop + pMenu->getRect().getHeight();

		spawning_view = spawning_view->getParent();
	}

	pMenu->show(x, y);
	LLMenuGL::showPopup(spawning_view, pMenu, x, y);
}
// [/SL:KB]
//void LLListContextMenu::show(LLView* spawning_view, const uuid_vec_t& uuids, S32 x, S32 y)
//{
//	LLContextMenu* menup = mMenuHandle.get();
//	if (menup)
//	{
//		//preventing parent (menu holder) from deleting already "dead" context menus on exit
//		LLView* parent = menup->getParent();
//		if (parent)
//		{
//			parent->removeChild(menup);
//		}
//		delete menup;
//		mUUIDs.clear();
//	}
//
//	if ( uuids.empty() )
//	{
//		return;
//	}
//
//	mUUIDs.resize(uuids.size());
//	std::copy(uuids.begin(), uuids.end(), mUUIDs.begin());
//
//	menup = createMenu();
//	if (!menup)
//	{
//		LL_WARNS() << "Context menu creation failed" << LL_ENDL;
//		return;
//	}
//
//	mMenuHandle = menup->getHandle();
//	menup->show(x, y);
//	LLMenuGL::showPopup(spawning_view, menup, x, y);
//}

void LLListContextMenu::hide()
{
	if(mMenuHandle.get())
	{
		mMenuHandle.get()->hide();
	}
}

// static
void LLListContextMenu::handleMultiple(functor_t functor, const uuid_vec_t& ids)
{
	uuid_vec_t::const_iterator it;
	for (it = ids.begin(); it != ids.end(); ++it)
	{
		functor(*it);
	}
}

// static
LLContextMenu* LLListContextMenu::createFromFile(const std::string& filename)
{
	llassert(LLMenuGL::sMenuContainer != NULL);
	return LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
		filename, LLContextMenu::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());
}

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-08-10 (Catznip-3.3)
// static
void LLListContextMenu::handlePerFolder(LLListContextMenu::functor_t functor, const uuid_vec_t& item_ids)
{
	uuid_vec_t folder_ids;
	for (uuid_vec_t::const_iterator itItem = item_ids.begin(); itItem != item_ids.end(); ++itItem)
	{
		LLViewerInventoryItem* pItem = gInventory.getLinkedItem(*itItem);
		if (pItem)
		{
			if (folder_ids.end() == std::find(folder_ids.begin(), folder_ids.end(), pItem->getParentUUID()))
				folder_ids.push_back(pItem->getParentUUID());
		}
	}

	for (uuid_vec_t::const_iterator itFolder = folder_ids.begin(); itFolder != folder_ids.end(); ++itFolder)
	{
		functor(*itFolder);
	}
}

// static
bool LLListContextMenu::enableIfOne(enable_functor_t functor, const uuid_vec_t& ids)
{
	uuid_vec_t::const_iterator it;
	for (const LLUUID& id : ids)
	{
		if (functor(id))
			return true;
	}
	return false;
}
// [/SL:KB]

// EOF
