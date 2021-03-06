/**
 * @file llpaneloutfitsinventory.h
 * @brief Outfits inventory panel
 * class definition
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLPANELOUTFITSINVENTORY_H
#define LL_LLPANELOUTFITSINVENTORY_H

#include "llpanel.h"

//class LLOutfitGallery;
class LLOutfitsList;
class LLOutfitListGearMenuBase;
class LLPanelAppearanceTab;
class LLPanelWearing;
class LLMenuGL;
class LLSidepanelAppearance;
class LLTabContainer;
class LLSaveOutfitComboBtn;
// [SL:KB] - Patch: UI-SidepanelOutfitsView | Checked: 2010-11-09 (Catznip-3.0)
class LLPanelOutfitsTab;
// [/SL:KB]

class LLPanelOutfitsInventory : public LLPanel
{
	LOG_CLASS(LLPanelOutfitsInventory);
public:
	LLPanelOutfitsInventory();
	virtual ~LLPanelOutfitsInventory();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	
	void onSearchEdit(const std::string& string);
	void onSave();
	
	bool onSaveCommit(const LLSD& notification, const LLSD& response);

	static LLSidepanelAppearance* getAppearanceSP();

// [RLVa:KB] - Checked: 2010-08-24 (RLVa-1.4.0a) | Added: RLVa-1.2.1a
	LLTabContainer* getAppearanceTabs()		{ return mAppearanceTabs; }
	LLPanelOutfitsTab*  getMyOutfitsPanel()	{ return mMyOutfitsPanel; }
	LLPanelWearing* getCurrentOutfitPanel()	{ return mCurrentOutfitPanel; }
// [/RLVa:KB]

	static LLPanelOutfitsInventory* findInstance();

protected:
	void updateVerbs();

private:
	LLTabContainer*			mAppearanceTabs;
	std::string 			mFilterSubString;
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	std::unique_ptr<LLSaveOutfitComboBtn> mOutfitsSaveComboBtn;
	std::unique_ptr<LLSaveOutfitComboBtn> mWearingSaveComboBtn;
// [/SL:KB]
//// [SL:KB] - Patch: Viewer-Build | Checked: Catznip-6.6
//	std::unique_ptr<LLSaveOutfitComboBtn> mSaveComboBtn;
//// [/SL:KB]
////	std::auto_ptr<LLSaveOutfitComboBtn> mSaveComboBtn;

	//////////////////////////////////////////////////////////////////////////////////
	// tab panels                                                                   //
protected:
	void 					initTabPanels();
	void 					onTabChange();
	bool 					isCOFPanelActive() const;
	bool 					isOutfitsListPanelActive() const;
//	bool 					isOutfitsGalleryPanelActive() const;

private:
	LLPanelAppearanceTab*	mActivePanel;
//	LLOutfitsList*			mMyOutfitsPanel;
// [SL:KB] - Patch: UI-SidepanelOutfitsView | Checked: 2010-11-09 (Catznip-2.4)
	LLPanelOutfitsTab*		mMyOutfitsPanel;
// [/SL:KB]
//    LLOutfitGallery*        mOutfitGalleryPanel;
	LLPanelWearing*			mCurrentOutfitPanel;

	// tab panels                                                                   //
	//////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////
	// List Commands                                                                //
protected:
	void initListCommandsHandlers();
	void updateListCommands();
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-08-10 (Catznip-3.3)
	void onWearItemsClick();
	void onWearOutfitClick();
// [/SL:KB]
//	void onWearButtonClick();
//	void showGearMenu();
	void onTrashButtonClick();
	bool isActionEnabled(const LLSD& userdata);
	void setWearablesLoading(bool val);
	void onWearablesLoaded();
	void onWearablesLoading();
private:
//	LLPanel*					mListCommands;
	LLMenuGL*					mMenuAdd;
	// List Commands                                                                //
	//////////////////////////////////////////////////////////////////////////////////

	bool mInitialized;
};

#endif //LL_LLPANELOUTFITSINVENTORY_H
