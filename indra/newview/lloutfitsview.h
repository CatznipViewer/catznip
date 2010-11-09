/** 
 *
 * Copyright (c) 2010, Kitty Barnett
 * 
 * The source code in this file is provided to you under the terms of the 
 * GNU Lesser General Public License, version 2.1, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. Terms of the LGPL can be found in doc/LGPL-licence.txt 
 * in this distribution, or online at http://www.gnu.org/licenses/lgpl-2.1.txt
 * 
 * By copying, modifying or distributing this software, you acknowledge that
 * you have read and understood your obligations described above, and agree to 
 * abide by those obligations.
 * 
 */

#ifndef LL_LLOUTFITSVIEW_H
#define LL_LLOUTFITSVIEW_H

#include "llpanel.h"
#include "llpaneloutfitstab.h"

class LLFolderViewItem;
class LLInventoryPanel;

class LLOutfitsView : public LLPanelOutfitsTab
{
public:
	LLOutfitsView();
	virtual ~LLOutfitsView();

	bool canWearSelected();

	/*virtual*/ void onOpen(const LLSD& info);
	/*virtual*/ BOOL postBuild();

	// LLPanelAppearanceTab overrides
	/*virtual*/ void setFilterSubString(const std::string& string);
	/*virtual*/ bool isActionEnabled(const LLSD& userdata);
	/*virtual*/ void getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const;

	// LLPanelOutfitsTab overrides
	/*virtual*/ void performAction(std::string action);
	/*virtual*/ void removeSelected();
	/*virtual*/ void setSelectedOutfitByUUID(const LLUUID& outfit_uuid);
	/*virtual*/ void wearSelectedItems();
	/*virtual*/ bool hasItemSelected();

	void									onSelectionChange(const std::deque<LLFolderViewItem*> &selItems, BOOL fUserAction);
	/*virtual*/ boost::signals2::connection setSelectionChangeCallback(selection_change_callback_t cb);

protected:
	LLInventoryPanel*				mInvPanel;
	LLSaveFolderState*				mSavedFolderState;

	std::string						mFilterSubString;

	bool							mItemSelection;				// TRUE if the selection consists solely of inventory items
	LLUUID							mSelectedCategory;			// Parent UUID of the currently selected items
	selection_change_signal_t		mSelectionChangeSignal;
};

#endif //LL_LLOUTFITSVIEW_H
