/** 
 * @file llfolderviewmodelinventorycommon.h
 * @brief view model implementation specific to inventory
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
	virtual time_t getCreationDate() const = 0;	// UTC seconds
	virtual void setCreationDate(time_t creation_date_utc) = 0;
	virtual PermissionMask getPermissionMask() const = 0;
	virtual LLFolderType::EType getPreferredType() const = 0;
	virtual void showProperties(void) = 0;
	virtual BOOL isItemInTrash( void) const { return FALSE; } // TODO: make   into pure virtual.
	virtual BOOL isUpToDate() const = 0;
	virtual bool hasChildren() const = 0;
	virtual LLInventoryType::EType getInventoryType() const = 0;
	virtual void performAction(LLInventoryModel* model, std::string action)   = 0;
//	virtual LLWearableType::EType getWearableType() const = 0;
	virtual EInventorySortGroup getSortGroup() const = 0;
	virtual LLInventoryObject* getInventoryObject() const = 0;
// [SL:KB] - Patch: Inventory-Base | Checked: 2013-05-21 (Catznip-3.5)
	template <class T> T* getInventoryObject() const
	{
		return dynamic_cast<T*>(getInventoryObject());
	}
// [/SL:KB]
};

#endif // LL_LLFOLDERVIEWMODELINVENTORYCOMMON_H
