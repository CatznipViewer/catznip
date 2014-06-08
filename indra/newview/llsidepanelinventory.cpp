/**
 * @file LLSidepanelInventory.cpp
 * @brief Side Bar "Inventory" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010-2013, Kitty Barnett
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
#include "llsidepanelinventory.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llappviewer.h"
#include "llavataractions.h"
#include "llbutton.h"
#include "lldate.h"
#include "llfirstuse.h"
#include "llfloatersidepanelcontainer.h"
#include "llfoldertype.h"
#include "llfolderview.h"
#include "llhttpclient.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "lllayoutstack.h"
#include "lloutfitobserver.h"
#include "llpanelmaininventory.h"
#include "llpanelmarketplaceinbox.h"
#include "llselectmgr.h"
#include "llsidepaneliteminfo.h"
#include "llsidepaneltaskinfo.h"
#include "llstring.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "llviewermedia.h"
#include "llviewernetwork.h"
// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2012-07-18 (Catznip-3.3)
#include "llfloaterreg.h"
#include "llstartup.h"
#include "lltoggleablemenu.h"
#include "llviewermenu.h"
// [/SL:KB]
#include "llweb.h"

static LLPanelInjector<LLSidepanelInventory> t_inventory("sidepanel_inventory");

//
// Constants
//

// No longer want the inbox panel to auto-expand since it creates issues with the "new" tag time stamp
#define AUTO_EXPAND_INBOX	0

static const char * const INBOX_BUTTON_NAME = "inbox_btn";
static const char * const INBOX_LAYOUT_PANEL_NAME = "inbox_layout_panel";
static const char * const MAIN_INVENTORY_LAYOUT_PANEL_NAME = "main_inventory_layout_panel";

static const char * const INVENTORY_LAYOUT_STACK_NAME = "inventory_layout_stack";

static const char * const MARKETPLACE_INBOX_PANEL = "marketplace_inbox";

//
// Helpers
//
class LLInboxAddedObserver : public LLInventoryCategoryAddedObserver
{
public:
	LLInboxAddedObserver(LLSidepanelInventory * sidepanelInventory)
		: LLInventoryCategoryAddedObserver()
		, mSidepanelInventory(sidepanelInventory)
	{
	}
	
	void done()
	{
		for (cat_vec_t::iterator it = mAddedCategories.begin(); it != mAddedCategories.end(); ++it)
		{
			LLViewerInventoryCategory* added_category = *it;
			
			LLFolderType::EType added_category_type = added_category->getPreferredType();
			
			switch (added_category_type)
			{
				case LLFolderType::FT_INBOX:
//					mSidepanelInventory->enableInbox(true);
// [SL:KB] - Patch: Inventory-ReceivedItemsPanel | Checked: 2012-07-25 (Catznip-3.3)
					mSidepanelInventory->refreshInboxVisibility();
// [/SL:KB]
					mSidepanelInventory->observeInboxModifications(added_category->getUUID());
					break;
				default:
					break;
			}
		}
	}
	
private:
	LLSidepanelInventory * mSidepanelInventory;
};

// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2012-07-18 (Catznip-3.3)
class LLSidepanelActionHelper
{
public:
	LLSidepanelActionHelper(LLPanelMainInventory* pPanelMainInventory)
	{
		mPanelMainInventory = pPanelMainInventory->getHandle();

		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		registrar.add("Inventory.Action", boost::bind(&LLSidepanelActionHelper::onActionPerform, this, _2));

		LLToggleableMenu* pMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
			"menu_inventory_actions.xml", LLMenuGL::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());
		if (pMenu)
		{
			pMenu->setVisibilityChangeCallback(boost::bind(&LLSidepanelActionHelper::onMenuVisibilityChange, this, _2));
			mMenuHandle = pMenu->getHandle();
		}
	}

	virtual ~LLSidepanelActionHelper()
	{
		if (!mMenuHandle.isDead())
		{
			mMenuHandle.get()->die();
		}
	}

	LLToggleableMenu* getMenu() const
	{
		return mMenuHandle.get();
	}

protected:
	const LLFolderViewModelItemInventory* getSelectedFVItem() const
	{
		/*const*/ LLInventoryPanel* pInvPanel = (!mPanelMainInventory.isDead()) ? static_cast<LLPanelMainInventory*>(mPanelMainInventory.get())->getActivePanel() : NULL;
		if (pInvPanel)
		{
			const LLFolderViewItem* pFVItem = (pInvPanel->getRootFolder()) ? pInvPanel->getRootFolder()->getCurSelectedItem() : NULL;
			if (pFVItem)
			{
				return pFVItem->getViewModelItem<LLFolderViewModelItemInventory>();
			}
		}
		return NULL;
	}

	const LLViewerInventoryItem* getSelectedItem() const
	{
		const LLFolderViewModelItemInventory* pFVItem = getSelectedFVItem();
		return (pFVItem) ? pFVItem->getInventoryObject<LLViewerInventoryItem>() : NULL;
	}

	void onActionPerform(const LLSD& sdParam)
	{
		LLInventoryPanel* pInvPanel = (!mPanelMainInventory.isDead()) ? static_cast<LLPanelMainInventory*>(mPanelMainInventory.get())->getActivePanel() : NULL;
		if (!pInvPanel)
			return;

		const std::string strParam = sdParam.asString();
		if ( ("open" == strParam) || ("properties" == strParam) ||                          /* General*/  
		     ("goto" == strParam)|| ("find_links" == strParam) || 
		     ("playworld" == strParam) || ("playlocal" == strParam) ||                      /* Animations */ 
		     ("wear" == strParam) || ("attach" == strParam) || ("wear_add" == strParam) ||  /* Wearable items*/
		     ("take_off" == strParam) || ("detach" == strParam) || ("edit" == strParam) || 
		     ("activate" == strParam) || ("deactivate" == strParam) ||                      /* Gestures*/ 
		     ("about" == strParam) ||                                                       /* Landmarks */
		     ("save_as" == strParam) )                                                      /* Textures */
		{
			LLInventoryAction::doToSelected(pInvPanel->getModel(), pInvPanel->getRootFolder(), strParam);
		}
		else if ("teleport" == strParam)                                                    /* Landmarks */
		{
			LLInventoryAction::doToSelected(pInvPanel->getModel(), pInvPanel->getRootFolder(), "open");
		}
	}

	void onMenuVisibilityChange(const LLSD& sdParam)
	{
		if ( (!sdParam.has("visibility")) || (!sdParam["visibility"].asBoolean()) )
		{
			return;
		}

		const LLViewerInventoryItem* pItem = getSelectedItem();
		const LLFolderViewModelItemInventory* pFVItem = getSelectedFVItem();
		LLToggleableMenu* pMenu = mMenuHandle.get();
		if ( (!pItem) || (!pMenu) )
		{
			return;
		}

		bool fIsBodyPart = (LLAssetType::AT_BODYPART == pItem->getType());
		bool fIsClothing = (LLAssetType::AT_CLOTHING == pItem->getType());
		bool fIsGesture = (LLAssetType::AT_GESTURE == pItem->getType());
		bool fIsObject = (LLAssetType::AT_OBJECT == pItem->getType());
		bool fIsWearable = (fIsBodyPart) || (fIsClothing) || (fIsObject);
		bool fIsWorn = ((fIsWearable) || (fIsGesture)) ? get_is_item_worn(pItem->getUUID()) : false;

		// Generic options
		pMenu->findChildView("open")->setVisible(  (!fIsWearable) && (LLAssetType::AT_LANDMARK != pItem->getType()) );
		pMenu->findChildView("find_original")->setVisible(pItem->getIsLinkType());
		pMenu->findChildView("find_links")->setVisible( (!pItem->getIsLinkType()) && (LLAssetType::lookupCanLink(pItem->getType())) );
		pMenu->findChildView("properties")->setVisible(true);
		// Animations
		pMenu->findChildView("playworld")->setVisible( (LLAssetType::AT_ANIMATION == pItem->getType()) );
		pMenu->findChildView("playlocal")->setVisible( (LLAssetType::AT_ANIMATION == pItem->getType()));
		// Wearable and attachment options
		pMenu->findChildView("wear")->setVisible( (fIsBodyPart || fIsClothing) && (!fIsWorn) );
		pMenu->findChildView("attach")->setVisible( (fIsObject) && (!fIsWorn) );
		pMenu->findChildView("wear_add")->setVisible( (fIsClothing || fIsObject) && (!fIsWorn) );
		pMenu->findChildView("take_off")->setVisible( (fIsClothing) && (fIsWorn) );
		pMenu->findChildView("detach")->setVisible( (fIsObject) && (fIsWorn) );
		pMenu->findChildView("edit")->setVisible( (fIsWearable) && (fIsWorn) );
		// Gestures
		pMenu->findChildView("activate")->setVisible( (fIsGesture) && (!fIsWorn) );
		pMenu->findChildView("deactivate")->setVisible( (fIsGesture) && (fIsWorn) );
		// Landmarks
		pMenu->findChildView("teleport")->setVisible( (LLAssetType::AT_LANDMARK == pItem->getType()) );
		pMenu->findChildView("about")->setVisible( (LLAssetType::AT_LANDMARK == pItem->getType()) );
		// Textures and snapshots
		const LLTextureBridge* pTexBridge = dynamic_cast<const LLTextureBridge*>(pFVItem);
		pMenu->findChildView("save_as")->setVisible( (LLAssetType::AT_TEXTURE == pItem->getType()) );
		pMenu->findChildView("save_as")->setEnabled( (pTexBridge) && (pTexBridge->canSaveTexture()) );
	}

protected:
	LLHandle<LLToggleableMenu> mMenuHandle;
	LLHandle<LLPanel>          mPanelMainInventory;
};
// [/SL:KB]

//
// Implementation
//

LLSidepanelInventory::LLSidepanelInventory()
	: LLPanel()
	, mItemPanel(NULL)
	, mPanelMainInventory(NULL)
	, mInboxEnabled(false)
	, mCategoriesObserver(NULL)
	, mInboxAddedObserver(NULL)
// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2012-07-18 (Catznip-3.3)
	, mToolbarActionPanel(NULL)
	, mToolbarActionBtn(NULL)
	, mToolbarActionsPanel(NULL)
	, mToolbarActionsBtn(NULL)
	, mToolbarActionsFlyoutBtn(NULL)
	, mToolbarActionsHelper(NULL)
// [/SL:KB]
{
	//buildFromFile( "panel_inventory.xml"); // Called from LLRegisterPanelClass::defaultPanelClassBuilder()
}

LLSidepanelInventory::~LLSidepanelInventory()
{
	LLLayoutPanel* inbox_layout_panel = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME);

	// Save the InventoryMainPanelHeight in settings per account
	gSavedPerAccountSettings.setS32("InventoryInboxHeight", inbox_layout_panel->getTargetDim());

	if (mCategoriesObserver && gInventory.containsObserver(mCategoriesObserver))
	{
		gInventory.removeObserver(mCategoriesObserver);
	}
	delete mCategoriesObserver;
	
	if (mInboxAddedObserver && gInventory.containsObserver(mInboxAddedObserver))
	{
		gInventory.removeObserver(mInboxAddedObserver);
	}
	delete mInboxAddedObserver;

// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2012-07-18 (Catznip-3.3)
	delete mToolbarActionsHelper;
// [/Sl:KB]
}

void handleInventoryDisplayInboxChanged()
{
// [SL:KB] - Patch: Inventory-ReceivedItemsPanel | Checked: 2012-07-25 (Catznip-3.3)
	LLFloaterReg::const_instance_list_t& invFloaters = LLFloaterReg::getFloaterList("inventory");
	for (LLFloaterReg::const_instance_list_t::const_iterator itFloater = invFloaters.begin(); itFloater != invFloaters.end(); ++itFloater)
	{
		LLSidepanelInventory* sidepanel_inventory = (*itFloater)->findChild<LLSidepanelInventory>("main_panel");
		if (sidepanel_inventory)
		{
			sidepanel_inventory->refreshInboxVisibility();
		}
	}
// [/SL:KB]
//	LLSidepanelInventory* sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
//	if (sidepanel_inventory)
//	{
//		sidepanel_inventory->enableInbox(gSavedSettings.getBOOL("InventoryDisplayInbox"));
//	}
}

BOOL LLSidepanelInventory::postBuild()
{
	// UI elements from inventory panel
	{
		mInventoryPanel = getChild<LLPanel>("sidepanel_inventory_panel");

//		mInfoBtn = mInventoryPanel->getChild<LLButton>("info_btn");
//		mInfoBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onInfoButtonClicked, this));
//		
//		mShareBtn = mInventoryPanel->getChild<LLButton>("share_btn");
//		mShareBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onShareButtonClicked, this));
//		
//		mShopBtn = mInventoryPanel->getChild<LLButton>("shop_btn");
//		mShopBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onShopButtonClicked, this));
//
//		mWearBtn = mInventoryPanel->getChild<LLButton>("wear_btn");
//		mWearBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onWearButtonClicked, this));
//		
//		mPlayBtn = mInventoryPanel->getChild<LLButton>("play_btn");
//		mPlayBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onPlayButtonClicked, this));
//		
//		mTeleportBtn = mInventoryPanel->getChild<LLButton>("teleport_btn");
//		mTeleportBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onTeleportButtonClicked, this));
//		
//		mOverflowBtn = mInventoryPanel->getChild<LLButton>("overflow_btn");
//		mOverflowBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onOverflowButtonClicked, this));
		
		mPanelMainInventory = mInventoryPanel->getChild<LLPanelMainInventory>("panel_main_inventory");
		mPanelMainInventory->setSelectCallback(boost::bind(&LLSidepanelInventory::onSelectionChange, this, _1, _2));
// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2012-07-18 (Catznip-3.3)
		mToolbarActionPanel = mPanelMainInventory->findChild<LLPanel>("default_action_btn_panel", TRUE);
		mToolbarActionBtn = mToolbarActionPanel->findChild<LLButton>("default_action_btn");
		mToolbarActionBtn->setCommitCallback(boost::bind(&LLSidepanelInventory::onToolbarActionClicked, this));

		mToolbarActionsPanel = mPanelMainInventory->findChild<LLPanel>("actions_btn_panel", TRUE);
		mToolbarActionsBtn = mToolbarActionsPanel->findChild<LLButton>("actions_btn");
		mToolbarActionsBtn->setCommitCallback(boost::bind(&LLSidepanelInventory::onToolbarActionClicked, this));
		mToolbarActionsFlyoutBtn = mToolbarActionsPanel->findChild<LLButton>("actions_flyout_btn");
		mToolbarActionsFlyoutBtn->setCommitCallback(boost::bind(&LLSidepanelInventory::onToolbarFlyoutClicked, this));
		mToolbarActionsHelper =	new LLSidepanelActionHelper(mPanelMainInventory);
// [/SL:KB]
		LLTabContainer* tabs = mPanelMainInventory->getChild<LLTabContainer>("inventory filter tabs");
		tabs->setCommitCallback(boost::bind(&LLSidepanelInventory::updateVerbs, this));

		/* 
		   EXT-4846 : "Can we suppress the "Landmarks" and "My Favorites" folder since they have their own Task Panel?"
		   Deferring this until 2.1.
		LLInventoryPanel *my_inventory_panel = mPanelMainInventory->getChild<LLInventoryPanel>("All Items");
		my_inventory_panel->addHideFolderType(LLFolderType::FT_LANDMARK);
		my_inventory_panel->addHideFolderType(LLFolderType::FT_FAVORITE);
		*/

		LLOutfitObserver::instance().addCOFChangedCallback(boost::bind(&LLSidepanelInventory::updateVerbs, this));
	}

	// UI elements from item panel
	{
		mItemPanel = getChild<LLSidepanelItemInfo>("sidepanel__item_panel");
		
		LLButton* back_btn = mItemPanel->getChild<LLButton>("back_btn");
		back_btn->setClickedCallback(boost::bind(&LLSidepanelInventory::onBackButtonClicked, this));
	}

	// UI elements from task panel
	{
		mTaskPanel = findChild<LLSidepanelTaskInfo>("sidepanel__task_panel");
		if (mTaskPanel)
		{
			LLButton* back_btn = mTaskPanel->getChild<LLButton>("back_btn");
			back_btn->setClickedCallback(boost::bind(&LLSidepanelInventory::onBackButtonClicked, this));
		}
	}
	
	// Received items inbox setup
	{
		LLLayoutStack* inv_stack = getChild<LLLayoutStack>(INVENTORY_LAYOUT_STACK_NAME);

		// Set up button states and callbacks
		LLButton * inbox_button = getChild<LLButton>(INBOX_BUTTON_NAME);

		inbox_button->setCommitCallback(boost::bind(&LLSidepanelInventory::onToggleInboxBtn, this));

		// Get the previous inbox state from "InventoryInboxToggleState" setting.
		bool is_inbox_collapsed = !inbox_button->getToggleState();

		// Restore the collapsed inbox panel state
		LLLayoutPanel* inbox_panel = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME);
		inv_stack->collapsePanel(inbox_panel, is_inbox_collapsed);
		if (!is_inbox_collapsed)
		{
			inbox_panel->setTargetDim(gSavedPerAccountSettings.getS32("InventoryInboxHeight"));
		}

// [SL:KB] - Patch: Inventory-ReceivedItemsPanel | Checked: 2012-07-25 (Catznip-3.3)
		// Set the inbox visible based on debug settings (final setting comes from http request below)
		refreshInboxVisibility();

		// Trigger callback for after login so we can setup to track inbox changes after initial inventory load
		if (LLStartUp::getStartupState() < STATE_STARTED)
			LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLSidepanelInventory::updateInbox, this));
		else
			updateInbox();
// [/SL:KB]
//		// Set the inbox visible based on debug settings (final setting comes from http request below)
//		enableInbox(gSavedSettings.getBOOL("InventoryDisplayInbox"));
//
//		// Trigger callback for after login so we can setup to track inbox changes after initial inventory load
//		LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLSidepanelInventory::updateInbox, this));
	}

	gSavedSettings.getControl("InventoryDisplayInbox")->getCommitSignal()->connect(boost::bind(&handleInventoryDisplayInboxChanged));
// [SL:KB] - Patch: Inventory-ReceivedItemsPanel | Checked: 2012-07-25 (Catznip-3.3)
	gSavedSettings.getControl("ShowReceivedItemsPanel")->getCommitSignal()->connect(boost::bind(&handleInventoryDisplayInboxChanged));
// [/SL:KB]

	// Update the verbs buttons state.
	updateVerbs();

	return TRUE;
}

void LLSidepanelInventory::updateInbox()
{
	//
	// Track inbox folder changes
	//

	const bool do_not_create_folder = false;

	const LLUUID inbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, do_not_create_folder);
	
	// Set up observer to listen for creation of inbox if at least one of them doesn't exist
	if (inbox_id.isNull())
	{
		observeInboxCreation();
	}
	// Set up observer for inbox changes, if we have an inbox already
	else 
	{
// [SL:KB] - Patch: Inventory-ReceivedItemsPanel | Checked: 2012-07-25 (Catznip-3.3)
		refreshInboxVisibility();
// [/SL:KB]
//		// Enable the display of the inbox if it exists
//		enableInbox(true);

		observeInboxModifications(inbox_id);
	}
}

void LLSidepanelInventory::observeInboxCreation()
{
	//
	// Set up observer to track inbox folder creation
	//
	
	if (mInboxAddedObserver == NULL)
	{
		mInboxAddedObserver = new LLInboxAddedObserver(this);
		
		gInventory.addObserver(mInboxAddedObserver);
	}
}

void LLSidepanelInventory::observeInboxModifications(const LLUUID& inboxID)
{
	//
	// Silently do nothing if we already have an inbox inventory panel set up
	// (this can happen multiple times on the initial session that creates the inbox)
	//

	if (mInventoryPanelInbox.get() != NULL)
	{
		return;
	}

	//
	// Track inbox folder changes
	//

	if (inboxID.isNull())
	{
		LL_WARNS() << "Attempting to track modifications to non-existent inbox" << LL_ENDL;
		return;
	}

	if (mCategoriesObserver == NULL)
	{
		mCategoriesObserver = new LLInventoryCategoriesObserver();
		gInventory.addObserver(mCategoriesObserver);
	}

	mCategoriesObserver->addCategory(inboxID, boost::bind(&LLSidepanelInventory::onInboxChanged, this, inboxID));

	//
	// Trigger a load for the entire contents of the Inbox
	//

	LLInventoryModelBackgroundFetch::instance().start(inboxID);

	//
	// Set up the inbox inventory view
	//

	LLPanelMarketplaceInbox * inbox = getChild<LLPanelMarketplaceInbox>(MARKETPLACE_INBOX_PANEL);
    LLInventoryPanel* inventory_panel = inbox->setupInventoryPanel();
	mInventoryPanelInbox = inventory_panel->getInventoryPanelHandle();
}

// [SL:KB] - Patch: Inventory-ReceivedItemsPanel | Checked: 2012-07-25 (Catznip-3.3)
void LLSidepanelInventory::refreshInboxVisibility()
{
	// Rules for showing the "Received Items" panel:
	//   - "Received Items" folder exists (and contains items)
	//   - "ShowReceivedItemsPanel" is set to TRUE
	// -> "InventoryDisplayInbox" overrides and always shows the panel
	bool fEnableInbox = false;
	if (gInventory.isInventoryUsable())
	{
		const LLUUID idInbox = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX);
		fEnableInbox = idInbox.notNull();
//		if (fEnableInbox)
//		{
//			LLInventoryModel::cat_array_t* cats; LLInventoryModel::item_array_t* items;
//			gInventory.getDirectDescendentsOf(idInbox, cats, items);
//			fEnableInbox = ((cats) && (!cats->empty())) || ((items) && (!items->empty()));
//		}
	}

	if (!gSavedSettings.getBOOL("InventoryDisplayInbox"))
		fEnableInbox &= (bool)gSavedSettings.getBOOL("ShowReceivedItemsPanel");
	else
		fEnableInbox = true;

	mInboxEnabled = fEnableInbox;
	getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME)->setVisible(fEnableInbox);
}
// [/SL:KB]
//void LLSidepanelInventory::enableInbox(bool enabled)
//{
//	mInboxEnabled = enabled;
//	
//	LLLayoutPanel * inbox_layout_panel = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME);
//	inbox_layout_panel->setVisible(enabled);
//}

void LLSidepanelInventory::openInbox()
{
	if (mInboxEnabled)
	{
		getChild<LLButton>(INBOX_BUTTON_NAME)->setToggleState(true);
		onToggleInboxBtn();
	}
}

void LLSidepanelInventory::onInboxChanged(const LLUUID& inbox_id)
{
	// Trigger a load of the entire inbox so we always know the contents and their creation dates for sorting
	LLInventoryModelBackgroundFetch::instance().start(inbox_id);

#if AUTO_EXPAND_INBOX
	// Expand the inbox since we have fresh items
	if (mInboxEnabled)
	{
		getChild<LLButton>(INBOX_BUTTON_NAME)->setToggleState(true);
		onToggleInboxBtn();
	}
#endif
}

void LLSidepanelInventory::onToggleInboxBtn()
{
	LLButton* inboxButton = getChild<LLButton>(INBOX_BUTTON_NAME);
	LLLayoutPanel* inboxPanel = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME);
	LLLayoutStack* inv_stack = getChild<LLLayoutStack>(INVENTORY_LAYOUT_STACK_NAME);
	
	const bool inbox_expanded = inboxButton->getToggleState();
	
// [SL:KB] - Patch: Inventory-ReceivedItemsPanel | Checked: 2012-08-11 (Catznip-3.3)
	// Only track the inbox button state on the primary inventory floater
	if (this == LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory"))
	{
		gSavedPerAccountSettings.setBOOL("InventoryInboxToggleState", inbox_expanded);
	}
// [/SL:KB]

	// Expand/collapse the indicated panel
	inv_stack->collapsePanel(inboxPanel, !inbox_expanded);

	if (inbox_expanded)
	{
		inboxPanel->setTargetDim(gSavedPerAccountSettings.getS32("InventoryInboxHeight"));
		if (inboxPanel->isInVisibleChain())
	{
		gSavedPerAccountSettings.setU32("LastInventoryInboxActivity", time_corrected());
	}
}
	else
	{
		gSavedPerAccountSettings.setS32("InventoryInboxHeight", inboxPanel->getTargetDim());
	}

}

void LLSidepanelInventory::onOpen(const LLSD& key)
{
	LLFirstUse::newInventory(false);
	mPanelMainInventory->setFocusFilterEditor();
#if AUTO_EXPAND_INBOX
	// Expand the inbox if we have fresh items
	LLPanelMarketplaceInbox * inbox = findChild<LLPanelMarketplaceInbox>(MARKETPLACE_INBOX_PANEL);
	if (inbox && (inbox->getFreshItemCount() > 0))
	{
		getChild<LLButton>(INBOX_BUTTON_NAME)->setToggleState(true);
		onToggleInboxBtn();
	}
#else
	if (mInboxEnabled && getChild<LLButton>(INBOX_BUTTON_NAME)->getToggleState())
	{
		gSavedPerAccountSettings.setU32("LastInventoryInboxActivity", time_corrected());
	}
#endif

	if(key.size() == 0)
		return;

	mItemPanel->reset();

	if (key.has("id"))
	{
		mItemPanel->setItemID(key["id"].asUUID());
		if (key.has("object"))
		{
			mItemPanel->setObjectID(key["object"].asUUID());
		}
		showItemInfoPanel();
	}
	if (key.has("task"))
	{
		if (mTaskPanel)
			mTaskPanel->setObjectSelection(LLSelectMgr::getInstance()->getSelection());
		showTaskInfoPanel();
	}
}

//void LLSidepanelInventory::onInfoButtonClicked()
//{
//	LLInventoryItem *item = getSelectedItem();
//	if (item)
//	{
//		mItemPanel->reset();
//		mItemPanel->setItemID(item->getUUID());
//		showItemInfoPanel();
//	}
//}

//void LLSidepanelInventory::onShareButtonClicked()
//{
//	LLAvatarActions::shareWithAvatars(this);
//}

//void LLSidepanelInventory::onShopButtonClicked()
//{
//	LLWeb::loadURLExternal(gSavedSettings.getString("MarketplaceURL"));
//}

// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2012-07-18 (Catznip-3.3)
void LLSidepanelInventory::onToolbarActionClicked()
{
	performActionOnSelection(getSelectionAction());
}

void LLSidepanelInventory::onToolbarFlyoutClicked()
{
	LLMenuGL* pMenu = mToolbarActionsHelper->getMenu();
	if (pMenu)
	{
		pMenu->buildDrawLabels();

		LLRect rctActionBtn;
		mToolbarActionBtn->localRectToOtherView(mToolbarActionBtn->getRect(), &rctActionBtn, this);
		LLMenuGL::showPopup(this, pMenu, rctActionBtn.mLeft, rctActionBtn.mBottom);
	}
}
// [/SL:KB]

void LLSidepanelInventory::performActionOnSelection(const std::string &action)
{
// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2010-04-15 (Catznip-2.0)
	LLInventoryPanel* pPanel = getActivePanel();
	if ( (!pPanel) || (!pPanel->getRootFolder()) )
		return;

	if (!action.empty())
	{
		LLInventoryAction::doToSelected(pPanel->getModel(), pPanel->getRootFolder(), action);
	}
// [/SL:KB]
//	LLFolderViewItem* current_item = mPanelMainInventory->getActivePanel()->getRootFolder()->getCurSelectedItem();
//	if (!current_item)
//	{
//		if (mInventoryPanelInbox.get() && mInventoryPanelInbox.get()->getRootFolder())
//		{
//			current_item = mInventoryPanelInbox.get()->getRootFolder()->getCurSelectedItem();
//		}
//
//		if (!current_item)
//		{
//			return;
//		}
//	}
//
//	static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->performAction(mPanelMainInventory->getActivePanel()->getModel(), action);
}

//void LLSidepanelInventory::onWearButtonClicked()
//{
//	// Get selected items set.
//	const std::set<LLUUID> selected_uuids_set = LLAvatarActions::getInventorySelectedUUIDs();
//	if (selected_uuids_set.empty()) return; // nothing selected
//
//	// Convert the set to a vector.
//	uuid_vec_t selected_uuids_vec;
//	for (std::set<LLUUID>::const_iterator it = selected_uuids_set.begin(); it != selected_uuids_set.end(); ++it)
//	{
//		selected_uuids_vec.push_back(*it);
//	}
//
//	// Wear all selected items.
//	wear_multiple(selected_uuids_vec, true);
//}

//void LLSidepanelInventory::onPlayButtonClicked()
//{
//	const LLInventoryItem *item = getSelectedItem();
//	if (!item)
//	{
//		return;
//	}
//
//	switch(item->getInventoryType())
//	{
//	case LLInventoryType::IT_GESTURE:
//		performActionOnSelection("play");
//		break;
//	default:
//		performActionOnSelection("open");
//		break;
//	}
//}

//void LLSidepanelInventory::onTeleportButtonClicked()
//{
//	performActionOnSelection("teleport");
//}

//void LLSidepanelInventory::onOverflowButtonClicked()
//{
//}

void LLSidepanelInventory::onBackButtonClicked()
{
	showInventoryPanel();
}

void LLSidepanelInventory::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	updateVerbs();
}

void LLSidepanelInventory::showItemInfoPanel()
{
	mItemPanel->setVisible(TRUE);
	if (mTaskPanel)
		mTaskPanel->setVisible(FALSE);
	mInventoryPanel->setVisible(FALSE);

	mItemPanel->dirty();
	mItemPanel->setIsEditing(FALSE);
}

void LLSidepanelInventory::showTaskInfoPanel()
{
	mItemPanel->setVisible(FALSE);
	mInventoryPanel->setVisible(FALSE);

	if (mTaskPanel)
	{
		mTaskPanel->setVisible(TRUE);
		mTaskPanel->dirty();
		mTaskPanel->setIsEditing(FALSE);
	}
}

void LLSidepanelInventory::showInventoryPanel()
{
	mItemPanel->setVisible(FALSE);
	if (mTaskPanel)
		mTaskPanel->setVisible(FALSE);
	mInventoryPanel->setVisible(TRUE);
	updateVerbs();
}

void LLSidepanelInventory::updateVerbs()
{
// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2012-07-18 (Catznip-3.3)
	S32 cntItems = 0;
	const std::string strAction = getSelectionAction(&cntItems);
	if (!strAction.empty())
	{
		LLButton* pActionBtn = (cntItems > 1) ? mToolbarActionBtn : mToolbarActionsBtn;
		pActionBtn->setLabel(LLTrans::getString("InvAction " + strAction));
	}
	mToolbarActionPanel->setVisible( (cntItems > 1) && (!strAction.empty()) );
	mToolbarActionsPanel->setVisible( (cntItems == 1) && (!strAction.empty()) );;
// [/SL:KB]
//	mInfoBtn->setEnabled(FALSE);
//	mShareBtn->setEnabled(FALSE);
//
//	mWearBtn->setVisible(FALSE);
//	mWearBtn->setEnabled(FALSE);
//	mPlayBtn->setVisible(FALSE);
//	mPlayBtn->setEnabled(FALSE);
// 	mTeleportBtn->setVisible(FALSE);
// 	mTeleportBtn->setEnabled(FALSE);
// 	mShopBtn->setVisible(TRUE);
//
//	mShareBtn->setEnabled(canShare());
//
//	const LLInventoryItem *item = getSelectedItem();
//	if (!item)
//		return;
//
//	bool is_single_selection = getSelectedCount() == 1;
//
//	mInfoBtn->setEnabled(is_single_selection);
//
//	switch(item->getInventoryType())
//	{
//		case LLInventoryType::IT_WEARABLE:
//		case LLInventoryType::IT_OBJECT:
//		case LLInventoryType::IT_ATTACHMENT:
//			mWearBtn->setVisible(TRUE);
//			mWearBtn->setEnabled(canWearSelected());
//		 	mShopBtn->setVisible(FALSE);
//			break;
//		case LLInventoryType::IT_SOUND:
//		case LLInventoryType::IT_GESTURE:
//		case LLInventoryType::IT_ANIMATION:
//			mPlayBtn->setVisible(TRUE);
//			mPlayBtn->setEnabled(TRUE);
//		 	mShopBtn->setVisible(FALSE);
//			break;
//		case LLInventoryType::IT_LANDMARK:
//			mTeleportBtn->setVisible(TRUE);
//			mTeleportBtn->setEnabled(TRUE);
//		 	mShopBtn->setVisible(FALSE);
//			break;
//		default:
//			break;
//	}
}

bool LLSidepanelInventory::canShare()
{
	LLInventoryPanel* inbox = mInventoryPanelInbox.get();

	// Avoid flicker in the Recent tab while inventory is being loaded.
	if ( (!inbox || !inbox->getRootFolder() || inbox->getRootFolder()->getSelectionList().empty())
		&& (mPanelMainInventory && !mPanelMainInventory->getActivePanel()->getRootFolder()->hasVisibleChildren()) )
	{
		return false;
	}

	return ( (mPanelMainInventory ? LLAvatarActions::canShareSelectedItems(mPanelMainInventory->getActivePanel()) : false)
			|| (inbox ? LLAvatarActions::canShareSelectedItems(inbox) : false) );
}


bool LLSidepanelInventory::canWearSelected()
{

	std::set<LLUUID> selected_uuids = LLAvatarActions::getInventorySelectedUUIDs();

	if (selected_uuids.empty())
		return false;

	for (std::set<LLUUID>::const_iterator it = selected_uuids.begin();
		it != selected_uuids.end();
		++it)
	{
		if (!get_can_item_be_worn(*it)) return false;
	}

	return true;
}

//LLInventoryItem *LLSidepanelInventory::getSelectedItem()
//{
//	LLFolderViewItem* current_item = mPanelMainInventory->getActivePanel()->getRootFolder()->getCurSelectedItem();
//	
//	if (!current_item)
//	{
//		if (mInventoryPanelInbox.get() && mInventoryPanelInbox.get()->getRootFolder())
//		{
//			current_item = mInventoryPanelInbox.get()->getRootFolder()->getCurSelectedItem();
//		}
//
//		if (!current_item)
//		{
//			return NULL;
//		}
//	}
//	const LLUUID &item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
//	LLInventoryItem *item = gInventory.getItem(item_id);
//	return item;
//}

// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2012-01-17 (Catznip-3.2)
bool LLSidepanelInventory::getSelectedItems(LLInventoryModel::item_array_t& items) const
{
	items.clear();

	LLInventoryPanel* pActivePanel = mPanelMainInventory->getActivePanel();

	std::set<LLFolderViewItem*> selFVItems = pActivePanel->getRootFolder()->getSelectionList();
	for (std::set<LLFolderViewItem*>::const_iterator itFVItem = selFVItems.begin(); itFVItem != selFVItems.end(); itFVItem++)
	{
		const LLFolderViewModelItemInventory* pFVMItem = (*itFVItem) ? (*itFVItem)->getViewModelItem<LLFolderViewModelItemInventory>() : NULL;
		if (!pFVMItem)
			continue;

		LLInventoryObject* pInvObj = pFVMItem->getInventoryObject();
		if (pInvObj)
		{
			if (LLAssetType::AT_CATEGORY == pInvObj->getType())
			{
				// If there are categories selected then we don't want to show any actions so we return an empty selection
				items.clear();
				return false;
			}
			items.push_back(dynamic_cast<LLViewerInventoryItem*>(pInvObj));
		}
	}
	return !items.empty();
}
// [/SL:KB]

//U32 LLSidepanelInventory::getSelectedCount()
//{
//	int count = 0;
//
//	std::set<LLFolderViewItem*> selection_list =    mPanelMainInventory->getActivePanel()->getRootFolder()->getSelectionList();
//	count += selection_list.size();
//
//	if ((count == 0) && mInboxEnabled && (mInventoryPanelInbox != NULL))
//	{
//		selection_list = mInventoryPanelInbox->getRootFolder()->getSelectionList();
//
//		count += selection_list.size();
//	}
//
//	return count;
//}

LLInventoryPanel *LLSidepanelInventory::getActivePanel()
{
	if (!getVisible())
	{
		return NULL;
	}
	if (mInventoryPanel->getVisible())
	{
		return mPanelMainInventory->getActivePanel();
	}
	return NULL;
}

BOOL LLSidepanelInventory::isMainInventoryPanelActive() const
{
	return mInventoryPanel->getVisible();
}

void LLSidepanelInventory::clearSelections(bool clearMain, bool clearInbox)
{
	if (clearMain)
	{
		LLInventoryPanel * inv_panel = getActivePanel();
		
		if (inv_panel)
		{
			inv_panel->clearSelection();
		}
	}
	
	if (clearInbox && mInboxEnabled && mInventoryPanelInbox.get())
	{
		mInventoryPanelInbox.get()->clearSelection();
	}
	
	updateVerbs();
}

std::set<LLFolderViewItem*> LLSidepanelInventory::getInboxSelectionList()
{
	std::set<LLFolderViewItem*> inventory_selected_uuids;
	
	if (mInboxEnabled && mInventoryPanelInbox.get() && mInventoryPanelInbox.get()->getRootFolder())
	{
		inventory_selected_uuids = mInventoryPanelInbox.get()->getRootFolder()->getSelectionList();
	}
	
	return inventory_selected_uuids;
}

// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2010-04-15 (Catznip-2.0)

// Returns IT_XXX if every item has the same inventory type or IT_NONE otherwise
static LLInventoryType::EType get_items_invtype(const LLInventoryModel::item_array_t& items)
{
	LLInventoryType::EType invType = LLInventoryType::IT_NONE;
	for (LLInventoryModel::item_array_t::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
	{
		const LLViewerInventoryItem* pItem = itItem->get();
		if (!pItem)
			continue;

		LLInventoryType::EType invTypeItem = pItem->getInventoryType();
		if (LLInventoryType::IT_NONE == invType)
			invType = invTypeItem;
		else if (invType != invTypeItem)
			return LLInventoryType::IT_NONE;
	}
	return invType;
}

std::string LLSidepanelInventory::getSelectionAction(S32* pSelCount) const
{
	LLInventoryModel::item_array_t items;
	if (!getSelectedItems(items))
	{
		if (pSelCount)
			*pSelCount = 0;
		return LLStringUtil::null;
	}

	const LLUUID idTrash = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	const LLUUID idParent = get_items_parent(items);
	if ( (idParent.notNull()) && (gInventory.isObjectDescendentOf(idParent, idTrash)) )
	{
		return LLStringUtil::null;
	}

	if (pSelCount)
	{
		*pSelCount = items.size();
	}

	LLInventoryType::EType invType = get_items_invtype(items);
	switch (invType)
	{
		case LLInventoryType::IT_WEARABLE:
			if ( (1 == items.size()) && (LLAssetType::AT_BODYPART == items.front()->getType() ) )
				return (!get_items_worn(items)) ? "wear" : LLStringUtil::null;
			else
				return (!get_items_worn(items)) ? "wear" : "take_off";
		case LLInventoryType::IT_OBJECT:
		case LLInventoryType::IT_ATTACHMENT:
			return (!get_items_worn(items)) ? "attach" : "detach";
		case LLInventoryType::IT_GESTURE:
			return (!get_items_worn(items)) ? "activate" : "deactivate";
		case LLInventoryType::IT_LANDMARK:
			// It doesn't make much sense to teleport to more than one landmark
			return (1 == items.size()) ? "teleport" : LLStringUtil::null;
		case LLInventoryType::IT_NONE:
			// Mixed selection type
			return (get_items_wearable(items)) ? ( (!get_items_worn(items)) ? "wear" : "take_off" ) : "open";
		default:
			return "open";
	}
}

// [/SL:KB]
