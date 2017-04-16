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
class LLOutfitsViewGearMenu;
class LLSaveFolderState;

// ============================================================================
// LLOutfitsView - Inventory panel based outfit view
//

class LLOutfitsView : public LLPanelOutfitsTab
{
public:
	LLOutfitsView();
	virtual ~LLOutfitsView();

	bool				canWearSelected();
	void				closeAllFolders();
	LLInventoryPanel*	getInventoryPanel()	{ return mInvPanel; }

	bool onIdle();
	void onOpen(const LLSD& info) override;
	BOOL postBuild() override;

	// LLPanelAppearanceTab overrides
	void setFilterSubString(const std::string& string) override;
	bool isActionEnabled(const LLSD& userdata) override;
	void getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const override;

	// LLPanelOutfitsTab overrides
	bool hasItemSelected() override;
	void performAction(std::string action) override;
	void removeSelected() override;
	void setSelectedOutfitByUUID(const LLUUID& outfit_uuid) override;
	void wearSelectedItems() override;

	boost::signals2::connection setSelectionChangeCallback(selection_change_callback_t cb) override;

protected:
	void highlightBaseOutfit();
	void onOutfitsRemovalConfirmation(const LLSD& notification, const LLSD& response);
	void onSelectionChange(const std::deque<LLFolderViewItem*> &selItems, BOOL fUserAction);

protected:
	LLInventoryPanel*			mInvPanel;
	LLOutfitsViewGearMenu*		mGearMenu;
	LLSaveFolderState*			mSavedFolderState;

	std::string					mFilterSubString;
	LLUUID						mHighlightedFolder;			// UUID of the last highlighted (outfit) folder

	bool						mInitialized;				// TRUE if we started fetching everything under "My Outfits"
	bool						mItemSelection;				// TRUE if the selection consists solely of inventory items
	bool						mOutfitSelection;			// TRUE if mSelectedCategory is of type FT_OUTFIT
	LLUUID						mSelectedCategory;			// Parent UUID of the currently selected items
	selection_change_signal_t	mSelectionChangeSignal;
};

#endif //LL_LLOUTFITSVIEW_H
