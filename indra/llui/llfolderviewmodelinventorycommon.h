/** 
 * @file llfolderviewmodelinventorycommon.h
 * @brief view model implementation specific to inventory
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * Copyright (C) 2010-2015, Kitty Barnett
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

#ifndef LL_LLFOLDERVIEWMODELINVENTORYCOMMON_H
#define LL_LLFOLDERVIEWMODELINVENTORYCOMMON_H

#include "llfolderviewmodel.h"
#include "llinventory.h"

class LLFolderViewModelItemInventoryCommon
	:	public LLFolderViewModelItemCommon
{
public:
	LLFolderViewModelItemInventoryCommon(LLFolderViewModelInterface& root_view_model)
		:	LLFolderViewModelItemCommon(root_view_model)
	{
	}
	
	virtual const LLUUID& getUUID() const = 0;
// [SL:KB] - Patch: Inventory-Filter | Checked: Catznip-5.2
	virtual const LLUUID& getCreatorUUID() const = 0;
// [/SL:KB]
	virtual time_t getCreationDate() const = 0;	// UTC seconds
	virtual void setCreationDate(time_t creation_date_utc) = 0;
	virtual PermissionMask getPermissionMask() const = 0;
	virtual LLFolderType::EType getPreferredType() const = 0;
	virtual void showProperties(void) = 0;
	virtual BOOL isItemInTrash( void) const { return FALSE; } // TODO: make   into pure virtual.
	virtual BOOL isLink() const = 0;
	virtual BOOL isUpToDate() const = 0;
	virtual bool hasChildren() const = 0;
	virtual LLInventoryType::EType getInventoryType() const = 0;
	virtual void performAction(LLInventoryModel* model, std::string action)   = 0;
// [SL:KB] - Patch: Inventory-ContextMenu | Checked: 2013-05-20 (Catznip-3.5)
	virtual bool isItemWorn() const = 0;
	virtual LLAssetType::EType getAssetType() const = 0;
// [/SL:KB]
//	virtual LLWearableType::EType getWearableType() const = 0;
	virtual EInventorySortGroup getSortGroup() const = 0;
	virtual LLInventoryObject* getInventoryObject() const = 0;
// [SL:KB] - Patch: Inventory-Base | Checked: Catznip-3.5
	template <class T> T* getInventoryObject() const
	{
		return dynamic_cast<T*>(getInventoryObject());
	}
// [/SL:KB]
};

#endif // LL_LLFOLDERVIEWMODELINVENTORYCOMMON_H
