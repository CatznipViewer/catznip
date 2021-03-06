/**
 * @file llfloaterimsessiontab.cpp
 * @brief LLFloaterIMSessionTab class implements the common behavior of LNearbyChatBar
 * @brief and LLFloaterIMSession for hosting both in LLIMContainer
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "llfloaterimsessiontab.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llavataractions.h"
#include "llchatentry.h"
#include "llchathistory.h"
#include "llchiclet.h"
#include "llchicletbar.h"
#include "lldraghandle.h"
#include "llfloaterreg.h"
#include "llfloaterimsession.h"
#include "llfloaterimcontainer.h" // to replace separate IM Floaters with multifloater container
// [SL:KB] - Patch: Chat-Tabs | Checked: Catznip-6.7
#include "llfloaterimcontainertab.h"
// [/SL:KB]
#include "lllayoutstack.h"
#include "lltoolbarview.h"
#include "llfloaterimnearbychat.h"

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
#include "llavatarlist.h"
#include "llconversationview.h"
#include "llparticipantlist.h"
#include "lltrans.h"
#include "llviewercontrol.h"
// [/SL:KB]
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
#include "llmenubutton.h"
#include "lltoggleablemenu.h"
// [/SL:KB]
// [SL:KB] - Patch: Chat-IMPanel | Checked: 2014-02-20 (Catznip-3.7)
#include "llpanelimcontrolpanel.h"
// [/SL:KB]

const F32 REFRESH_INTERVAL = 1.0f;

void cb_group_do_nothing()
{
}

LLFloaterIMSessionTab::LLFloaterIMSessionTab(const LLSD& session_id)
:	LLTransientDockableFloater(NULL, false, session_id),
	mIsP2PChat(false),
	mExpandCollapseBtn(NULL),
//	mTearOffBtn(NULL),
//	mCloseBtn(NULL),
	mSessionID(session_id.asUUID()),
	mConversationsRoot(NULL),
	mScroller(NULL),
	mChatHistory(NULL),
	mInputEditor(NULL),
	mInputEditorPad(0),
	mRefreshTimer(new LLTimer()),
	mIsHostAttached(false),
	mHasVisibleBeenInitialized(false),
	mIsParticipantListExpanded(true),
	mChatLayoutPanel(NULL),
	mInputPanels(NULL),
	mChatLayoutPanelHeight(0),
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	mParticipantList(NULL),
// [/SL:KB]
// [SL:KB] - Patch: Chat-Refactor | Checked: 2013-08-28 (Catznip-3.6)
	mBodyStack(NULL),
	mParticipantListAndHistoryStack(NULL),
	mParticipantListPanel(NULL),
	mRightPartPanel(NULL),
	mContentPanel(NULL),
	mToolbarPanel(NULL),
	mInputButtonPanel(NULL),
	mExpandCollapseLineBtn(NULL),
	mGearBtn(NULL),
	mViewBtn(NULL),
	mAddBtn(NULL),
	mVoiceButton(NULL),
//	mTranslationCheckBox(NULL),
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
	mSearchBtn(NULL)
// [/SL:KB]
// [/SL:KB]
{
    setAutoFocus(FALSE);
	mSession = LLIMModel::getInstance()->findIMSession(mSessionID);

	mCommitCallbackRegistrar.add("IMSession.Menu.Action",
			boost::bind(&LLFloaterIMSessionTab::onIMSessionMenuItemClicked,  this, _2));
	mEnableCallbackRegistrar.add("IMSession.Menu.CompactExpandedModes.CheckItem",
			boost::bind(&LLFloaterIMSessionTab::onIMCompactExpandedMenuItemCheck, this, _2));
	mEnableCallbackRegistrar.add("IMSession.Menu.ShowModes.CheckItem",
			boost::bind(&LLFloaterIMSessionTab::onIMShowModesMenuItemCheck,   this, _2));
	mEnableCallbackRegistrar.add("IMSession.Menu.ShowModes.Enable",
			boost::bind(&LLFloaterIMSessionTab::onIMShowModesMenuItemEnable,  this, _2));

// [SL:KB] - Patch: Chat-GroupModerators | Checked: Catznip-3.6
	mCommitCallbackRegistrar.add("IMSession.Menu.ParticipantList", boost::bind(&LLFloaterIMSessionTab::onMenuParticipantListItemClicked,  this, _2));
	mEnableCallbackRegistrar.add("IMSession.Menu.CheckParticipantList", boost::bind(&LLFloaterIMSessionTab::onMenuParticipantListItemEnable,  this, _2));
// [/SL:KB]

	// Right click menu handling
    mEnableCallbackRegistrar.add("Avatar.CheckItem",  boost::bind(&LLFloaterIMSessionTab::checkContextMenuItem,	this, _2));
    mEnableCallbackRegistrar.add("Avatar.EnableItem", boost::bind(&LLFloaterIMSessionTab::enableContextMenuItem, this, _2));
    mCommitCallbackRegistrar.add("Avatar.DoToSelected", boost::bind(&LLFloaterIMSessionTab::doToSelected, this, _2));
    mCommitCallbackRegistrar.add("Group.DoToSelected", boost::bind(&cb_group_do_nothing));

// [SL:KB] - Patch: Chat-IMSessionMenu | Checked: 2013-08-18 (Catznip-3.6)
	mEnableCallbackRegistrar.add("IMSession.Menu.IsNearbyChat", boost::bind(&LLFloaterIMSessionTab::onIMCheckNearbyChat, this));
	mCommitCallbackRegistrar.add("IMSession.Menu.SetChatBarType", boost::bind(&LLFloaterIMSessionTab::onIMSetChatBarType, _2));
	mEnableCallbackRegistrar.add("IMSession.Menu.CheckChatBarType", boost::bind(&LLFloaterIMSessionTab::onIMCheckChatBarType, _2));
	mCommitCallbackRegistrar.add("IMSession.Menu.SetFontSize", boost::bind(&LLFloaterIMSessionTab::onIMSetFontSize, _2));
	mEnableCallbackRegistrar.add("IMSession.Menu.CheckFontSize", boost::bind(&LLFloaterIMSessionTab::onIMCheckFontSize, _2));
// [/SL:KB]
}

LLFloaterIMSessionTab::~LLFloaterIMSessionTab()
{
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
	if (mGearMenuHandle.get())
	{
		mGearMenuHandle.get()->die();
	}
// [/SL:KB]

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	if (LLFloaterIMContainerBase::CT_VIEW != LLFloaterIMContainerBase::getContainerType())
	{
		delete mParticipantList;
	}
// [/SL:KB]

	delete mRefreshTimer;
}

//static
LLFloaterIMSessionTab* LLFloaterIMSessionTab::findConversation(const LLUUID& uuid)
{
	LLFloaterIMSessionTab* conv;

	if (uuid.isNull())
	{
		conv = LLFloaterReg::findTypedInstance<LLFloaterIMSessionTab>("nearby_chat");
	}
	else
	{
		conv = LLFloaterReg::findTypedInstance<LLFloaterIMSessionTab>("impanel", LLSD(uuid));
	}

	return conv;
};

//static
LLFloaterIMSessionTab* LLFloaterIMSessionTab::getConversation(const LLUUID& uuid)
{
	LLFloaterIMSessionTab* conv;

	if (uuid.isNull())
	{
		conv = LLFloaterReg::getTypedInstance<LLFloaterIMSessionTab>("nearby_chat");
	}
	else
	{
		conv = LLFloaterReg::getTypedInstance<LLFloaterIMSessionTab>("impanel", LLSD(uuid));
		conv->setOpenPositioning(LLFloaterEnums::POSITIONING_RELATIVE);
	}

	return conv;
};

//void LLFloaterIMSessionTab::setVisible(BOOL visible)
//{
//	if(visible && !mHasVisibleBeenInitialized)
//	{
//		mHasVisibleBeenInitialized = true;
//		if(!gAgentCamera.cameraMouselook())
//		{
//// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-25 (Catznip-3.5)
//			// Don't show the container if there are no hosted conversations
//			LLFloaterIMContainerBase* pContainer = LLFloaterIMContainerBase::findInstance();
//			if (pContainer)
//			{
//				// NOTE: LLFloaterIMContainerView will always contain the "Redock this conversation" tab panel so we need to check for cnt > 1
//				bool fTabbedContainer = LLFloaterIMContainerBase::isTabbedContainer();
//				if ( ((!fTabbedContainer) && (pContainer->getFloaterCount() > 1)) || ((fTabbedContainer) && (pContainer->getFloaterCount() > 0)) )
//				{
//					pContainer->setVisible(true);
//				}
//			}
//// [/SL:KB]
////			LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container")->setVisible(true);
//		}
//		LLFloaterIMSessionTab::addToHost(mSessionID);
//		LLFloaterIMSessionTab* conversp = LLFloaterIMSessionTab::getConversation(mSessionID);
//
//		if (conversp && conversp->isNearbyChat() && gSavedPerAccountSettings.getBOOL("NearbyChatIsNotCollapsed"))
//		{
//			onCollapseToLine(this);
//		}
//		mInputButtonPanel->setVisible(isTornOff());
//	}
//
//	LLTransientDockableFloater::setVisible(visible);
//}

/*virtual*/
void LLFloaterIMSessionTab::setFocus(BOOL focus)
{
	LLTransientDockableFloater::setFocus(focus);

    //Redirect focus to input editor
    if (focus)
	{
    	updateMessages();

        if (mInputEditor)
        {
    	    mInputEditor->setFocus(TRUE);
        }
	}
}


void LLFloaterIMSessionTab::addToHost(const LLUUID& session_id)
{
//	if ((session_id.notNull() && !gIMMgr->hasSession(session_id))
//			|| !LLFloaterIMSessionTab::isChatMultiTab())
// [SL:KB] - Patch: Chat-Container | Checked: 2013-04-25 (Catznip-3.5)
	if (session_id.notNull() && !gIMMgr->hasSession(session_id))
// [/SL:KB]
	{
		return;
	}

	// Get the floater: this will create the instance if it didn't exist
	LLFloaterIMSessionTab* conversp = LLFloaterIMSessionTab::getConversation(session_id);
	if (conversp)
	{
//		LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
//
//		// Do not add again existing floaters
//		if (floater_container && !conversp->isHostAttached())
// [SL:KB] - Patch: Chat-Container | Checked: 2013-04-25 (Catznip-3.5)
		// Don't add the floater if we're showing separate floaters, or if the floater was already attached to the container
		LLFloaterIMContainerBase* floater_container = NULL;
		if ( (LLFloaterIMContainerBase::CT_SEPARATE != LLFloaterIMContainerBase::getContainerType()) &&
		     (floater_container = LLFloaterIMContainerBase::getInstance()) && (!conversp->isHostAttached()) )
// [/SL:KB]
		{
			conversp->setHostAttached(true);

			if (!conversp->isNearbyChat()
					|| gSavedPerAccountSettings.getBOOL("NearbyChatIsNotTornOff"))
			{
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-11-18 (Catznip-3.6)
				floater_container->addFloater(conversp, false, (session_id.notNull()) ? LLTabContainer::END : LLTabContainer::START);
// [/SL:KB]
//				floater_container->addFloater(conversp, false, LLTabContainer::RIGHT_OF_CURRENT);
			}
			else
			{
				// setting of the "potential" host for Nearby Chat: this sequence sets
				// LLFloater::mHostHandle = NULL (a current host), but
				// LLFloater::mLastHostHandle = floater_container (a "future" host)
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-12-15 (Catznip-3.6)
				// Code at the end of LLFloater::draw() will set "can tear off" to false if the nearby floater existed before this was called
				conversp->setCanTearOff(TRUE);
// [/SL:KB]
				conversp->setHost(floater_container);
				conversp->setHost(NULL);

				conversp->forceReshape();
			}
			// Added floaters share some state (like sort order) with their host
			conversp->setSortOrder(floater_container->getSortOrder());
		}
// [SL:KB] - Patch: Chat-Refactor | Checked: 2013-08-28 (Catznip-3.6)
		conversp->updateShowParticipantList();
		conversp->updateExpandCollapseBtn();
		conversp->hideOrShowTitle();
// [/SL:KB]
	}
}

//void LLFloaterIMSessionTab::assignResizeLimits()
//{
//	bool is_participants_pane_collapsed = mParticipantListPanel->isCollapsed();
//
//    // disable a layoutstack's functionality when participant list panel is collapsed
//	mRightPartPanel->setIgnoreReshape(is_participants_pane_collapsed);
//
//    S32 participants_pane_target_width = is_participants_pane_collapsed?
//    		0 : (mParticipantListPanel->getRect().getWidth() + mParticipantListAndHistoryStack->getPanelSpacing());
//
//    S32 new_min_width = participants_pane_target_width + mRightPartPanel->getExpandedMinDim() + mFloaterExtraWidth;
//
//	setResizeLimits(new_min_width, getMinHeight());
//
//	this->mParticipantListAndHistoryStack->updateLayout();
//}

BOOL LLFloaterIMSessionTab::postBuild()
{
	BOOL result;

	mBodyStack = getChild<LLLayoutStack>("main_stack");
    mParticipantListAndHistoryStack = getChild<LLLayoutStack>("im_panels");

//	mCloseBtn = getChild<LLButton>("close_btn");
//	mCloseBtn->setCommitCallback(boost::bind(&LLFloater::onClickClose, this));

	mExpandCollapseBtn = getChild<LLButton>("expand_collapse_btn");
	mExpandCollapseBtn->setClickedCallback(boost::bind(&LLFloaterIMSessionTab::onSlide, this));

	mExpandCollapseLineBtn = getChild<LLButton>("minz_btn");
	mExpandCollapseLineBtn->setClickedCallback(boost::bind(&LLFloaterIMSessionTab::onCollapseToLine, this));

//	mTearOffBtn = getChild<LLButton>("tear_off_btn");
//	mTearOffBtn->setCommitCallback(boost::bind(&LLFloaterIMSessionTab::onTearOffClicked, this));

//	mGearBtn = getChild<LLButton>("gear_btn");
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
	mGearBtn = getChild<LLMenuButton>("gear_btn");

	std::string strGearMenu = (mIsP2PChat) ? "menu_im_conversation.xml" : ((mSession) && (mSession->isGroupSessionType()) ? "menu_im_conversation_group.xml" : "");
	if (!strGearMenu.empty())
	{
		LLToggleableMenu* pMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(strGearMenu, LLMenuGL::sMenuContainer, LLMenuHolderGL::child_registry_t::instance());
		mGearBtn->setMenu(pMenu, mGearBtn->getMenuPosition(), false);
		mGearMenuHandle = pMenu->getHandle();
	}
//	mViewBtn = getChild<LLButton>("view_options_btn");
// [/SL:KB]
// [SL:KB] - Patch: Chat-GroupModerators | Checked: Catznip-3.6
	mViewBtn = getChild<LLMenuButton>("view_options_btn");
	if (mViewBtn->getMenu())
	{
		mViewBtn->getMenu()->setVisibilityChangeCallback(boost::bind(&LLFloaterIMSessionTab::onToggleViewMenu, this, _1, _2));
	}
// [/SL:KB]
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
	mSearchBtn = getChild<LLButton>("text_search_btn");
	mSearchBtn->setClickedCallback(boost::bind(&LLFloaterIMSessionTab::onHistorySearchClicked, this));
// [/SL:KB]
    mAddBtn = getChild<LLButton>("add_btn");
	mVoiceButton = getChild<LLButton>("voice_call_btn");
//    mTranslationCheckBox = getChild<LLUICtrl>("translate_chat_checkbox_lp");
    
	mParticipantListPanel = getChild<LLLayoutPanel>("speakers_list_panel");
	mRightPartPanel = getChild<LLLayoutPanel>("right_part_holder");

	mToolbarPanel = getChild<LLLayoutPanel>("toolbar_panel");
	mContentPanel = getChild<LLLayoutPanel>("body_panel");
	mInputButtonPanel = getChild<LLLayoutPanel>("input_button_layout_panel");
	mInputButtonPanel->setVisible(false);

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	if (LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType())
	{
// [/SL:KB]
		// Add a scroller for the folder (participant) view
		LLRect scroller_view_rect = mParticipantListPanel->getRect();
		scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
		LLScrollContainer::Params scroller_params(LLUICtrlFactory::getDefaultParams<LLFolderViewScrollContainer>());
		scroller_params.rect(scroller_view_rect);
		mScroller = LLUICtrlFactory::create<LLFolderViewScrollContainer>(scroller_params);
		mScroller->setFollowsAll();

		// Insert that scroller into the panel widgets hierarchy
		mParticipantListPanel->addChild(mScroller);	
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	}
	else
	{
		if (!mIsP2PChat)
		{
			LLAvatarList::Params p(LLUICtrlFactory::getDefaultParams<LLAvatarList>());

			p.name("speakers_list");
			LLRect rect = mParticipantListPanel->getRect();
			rect.translate(-rect.mLeft, -rect.mBottom);
			p.rect = rect;
			p.ignore_online_status(true);
			p.show_info_btn(true);
			p.show_profile_btn(false);
			p.show_speaking_indicator(false);

			LLAvatarList* pAvatarList = LLUICtrlFactory::create<LLAvatarList>(p);
			pAvatarList->setFollowsAll();
			mParticipantListPanel->addChild(pAvatarList);

			LLSpeakerMgr* pSpeakerMgr = (mIsNearbyChat) ? (LLSpeakerMgr*)LLLocalSpeakerMgr::getInstance() : LLIMModel::getInstance()->getSpeakerManager(mSessionID);
			mParticipantList = new LLParticipantAvatarList(pSpeakerMgr, pAvatarList);
		}
// [SL:KB] - Patch: Chat-IMPanel | Checked: 2014-02-20 (Catznip-3.7)
		else
		{
			LLPanel* pIMPanel = new LLPanelIMControlPanel(LLIMModel::getInstance()->getOtherParticipantID(mSessionID));

			LLRect rect = mParticipantListPanel->getRect();
			rect.translate(-rect.mLeft, -rect.mBottom);
			pIMPanel->setShape(rect);
			pIMPanel->setFollowsAll();

			mParticipantListPanel->addChild(pIMPanel);
		}
// [/SL:KB]
	}
// [/SL:KB]
	
	mChatHistory = getChild<LLChatHistory>("chat_history");
// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2012-01-10 (Catznip-3.2)
	mChatHistory->getEditor()->setContextMenu(LLUICtrlFactory::instance().createFromFile<LLContextMenu>("menu_chat_bar.xml", LLMenuGL::sMenuContainer, LLMenuHolderGL::child_registry_t::instance()), true);
// [/SL:KB]

	mInputEditor = getChild<LLChatEntry>("chat_editor");
// [SL:KB] - Patch: Chat-IMSessionMenu | Checked: 2012-01-10 (Catznip-3.2)
	mInputEditor->setContextMenu(LLUICtrlFactory::instance().createFromFile<LLContextMenu>("menu_chat_bar.xml", LLMenuGL::sMenuContainer, LLMenuHolderGL::child_registry_t::instance()), true);
	mInputEditor->enableSingleLineMode(!gSavedSettings.getBOOL("ChatMultiLine"));
// [/SL:KB]

	mChatLayoutPanel = getChild<LLLayoutPanel>("chat_layout_panel");
	mInputPanels = getChild<LLLayoutStack>("input_panels");
	
	mInputEditor->setTextExpandedCallback(boost::bind(&LLFloaterIMSessionTab::reshapeChatLayoutPanel, this));
	mInputEditor->setMouseUpCallback(boost::bind(&LLFloaterIMSessionTab::onInputEditorClicked, this));
	mInputEditor->setCommitOnFocusLost( FALSE );
	mInputEditor->setPassDelete(TRUE);
	mInputEditor->setFont(LLViewerChat::getChatFont());

	mChatLayoutPanelHeight = mChatLayoutPanel->getRect().getHeight();
	mInputEditorPad = mChatLayoutPanelHeight - mInputEditor->getRect().getHeight();

	setOpenPositioning(LLFloaterEnums::POSITIONING_RELATIVE);

	mSaveRect = isNearbyChat()
					&&  !gSavedPerAccountSettings.getBOOL("NearbyChatIsNotTornOff");
	initRectControl();

//	if (isChatMultiTab())
// [SL:KB] - Patch: Chat-Container | Checked: 2014-05-14 (Catznip-3.6)
	if (LLFloaterIMContainerBase::CT_SEPARATE != LLFloaterIMContainerBase::getContainerType())
// [/SL:KB]
	{
		result = LLFloater::postBuild();
	}
	else
	{
		result = LLDockableFloater::postBuild();
	}

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	if (LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType())
	{
// [/SL:KB]
		// Create the root using an ad-hoc base item
		LLConversationItem* base_item = new LLConversationItem(mSessionID, mConversationViewModel);
		LLFolderView::Params p(LLUICtrlFactory::getDefaultParams<LLFolderView>());
		p.rect = LLRect(0, 0, getRect().getWidth(), 0);
		p.parent_panel = mParticipantListPanel;
		p.listener = base_item;
		p.view_model = &mConversationViewModel;
		p.root = NULL;
		p.use_ellipses = true;
		p.options_menu = "menu_conversation.xml";
		p.name = "root";
		mConversationsRoot = LLUICtrlFactory::create<LLFolderView>(p);
		mConversationsRoot->setCallbackRegistrar(&mCommitCallbackRegistrar);
		// Attach that root to the scroller
		mScroller->addChild(mConversationsRoot);
		mConversationsRoot->setScrollContainer(mScroller);
		mConversationsRoot->setFollowsAll();
		mConversationsRoot->addChild(mConversationsRoot->mStatusTextBox);
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	}
// [/SL:KB]
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
	if ( (mChatHistory) && (mChatHistory->getEditor()) && (mChatHistory->getEditor()->getInplaceSearchPanel()) )
		mChatHistory->getEditor()->getInplaceSearchPanel()->setVisibleCallback(boost::bind(&LLFloaterIMSessionTab::onHistorySearchVisibilityChanged, this));
// [/SL:KB]

	setMessagePaneExpanded(true);

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	if (LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType())
	{
// [/SL:KB]
		buildConversationViewParticipant();
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	}
// [/SL:KB]
	refreshConversation();


	// Zero expiry time is set only once to allow initial update.
	mRefreshTimer->setTimerExpirySec(0);
	mRefreshTimer->start();
//	initBtns();

//	if (mIsParticipantListExpanded != (bool)gSavedSettings.getBOOL("IMShowControlPanel"))
// [SL:KB] - Patch: Chat-IMPanel | Checked: 2014-02-02 (Catznip-3.6)
	if (mIsParticipantListExpanded != (bool)gSavedSettings.getBOOL(getShowControlPanelControl()))
// [/SL:KB
	{
		LLFloaterIMSessionTab::onSlide(this);
	}
// [SL:KB] - Patch: Chat-Refactor | Checked: 2013-08-28 (Catznip-3.6)
	updateShowParticipantList();
	updateExpandCollapseBtn();
// [/SL:KB]
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-20 (Catznip-3.6)
	updateGearBtn();
// [/SL:KB]

//	// The resize limits for LLFloaterIMSessionTab should be updated, based on current values of width of conversation and message panels
//	mParticipantListPanel->getResizeBar()->setResizeListener(boost::bind(&LLFloaterIMSessionTab::assignResizeLimits, this));
//	mFloaterExtraWidth =
//			getRect().getWidth()
//			- mParticipantListAndHistoryStack->getRect().getWidth()
//			- (mParticipantListPanel->isCollapsed()? 0 : LLPANEL_BORDER_WIDTH);
//
//	assignResizeLimits();

	return result;
}

LLParticipantList* LLFloaterIMSessionTab::getParticipantList()
{
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	return (mParticipantList) ? mParticipantList : dynamic_cast<LLParticipantList*>(LLFloaterIMContainerBase::getInstance()->getSessionModel(mSessionID));
// [/SL:KB]
//	return dynamic_cast<LLParticipantList*>(LLFloaterIMContainer::getInstance()->getSessionModel(mSessionID));
}

// [SL:KB] - Patch: Chat-IMPanel | Checked: 2014-02-02 (Catznip-3.6)
const std::string LLFloaterIMSessionTab::getShowControlPanelControl() const
{
	return (isNearbyChat()) ? "IMShowControlPanelNearby" : ( (!mIsP2PChat) ? "IMShowControlPanelGroup" : "IMShowControlPanelP2P");
}
// [/SL:KB]

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
void LLFloaterIMSessionTab::setParticipantList(LLParticipantList* participant_list)
{
	mParticipantList = participant_list;
}
// [/SL:KB]

void LLFloaterIMSessionTab::draw()
{
	if (mRefreshTimer->hasExpired())
	{
		LLParticipantList* item = getParticipantList();
		if (item)
		{
			// Update all model items
			item->update();

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
			LLParticipantModelList* item= dynamic_cast<LLParticipantModelList*>(getParticipantList());
			if (item)
			{
// [/SL:KB]
				// If the model and view list diverge in count, rebuild
				// Note: this happens sometimes right around init (add participant events fire but get dropped) and is the cause
				// of missing participants, often, the user agent itself. As there will be no other event fired, there's
				// no other choice but get those inconsistencies regularly (and lightly) checked and scrubbed.
				if (item->getChildrenCount() != mConversationsWidgets.size())
				{
					buildConversationViewParticipant();
				}
				refreshConversation();
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
			}
// [/SL:KB]
		}
// [SL:KB] - Patch: Chat-Base | Checked: 2014-02-21 (Catznip-3.6)
		refresh();
// [/SL:KB]

		// Restart the refresh timer
		mRefreshTimer->setTimerExpirySec(REFRESH_INTERVAL);
	}

	LLTransientDockableFloater::draw();
}

// [SL:KB] - Patch: Chat-Misc | Checked: 2012-02-19 (Catznip-3.2)
BOOL LLFloaterIMSessionTab::handleUnicodeChar(llwchar uni_char, BOOL called_from_parent)
{
	if ( (!called_from_parent) && (iswgraph(uni_char)) && (hasFocus()) && (!mInputEditor->hasFocus()) )
	{
		// Give focus to the line editor and let it handle the character
		mInputEditor->setFocus(TRUE);
		return mInputEditor->handleUnicodeChar(uni_char, called_from_parent);
	}
	return LLTransientDockableFloater::handleUnicodeChar(uni_char, called_from_parent);
}
// [/SL:KB]
 
void LLFloaterIMSessionTab::enableDisableCallBtn()
{
    mVoiceButton->setEnabled(
    		mSessionID.notNull()
    		&& mSession
    		&& mSession->mSessionInitialized
    		&& LLVoiceClient::getInstance()->voiceEnabled()
    		&& LLVoiceClient::getInstance()->isVoiceWorking()
    		&& mSession->mCallBackEnabled);
}

void LLFloaterIMSessionTab::onFocusReceived()
{
	setBackgroundOpaque(true);

	if (mSessionID.notNull() && isInVisibleChain())
	{
		LLIMModel::instance().sendNoUnreadMessages(mSessionID);
	}

	LLTransientDockableFloater::onFocusReceived();
}

void LLFloaterIMSessionTab::onFocusLost()
{
	setBackgroundOpaque(false);
	LLTransientDockableFloater::onFocusLost();
}

void LLFloaterIMSessionTab::onInputEditorClicked()
{
// [SL:KB] - Patch: Chat-BaseConversationsBtn | Checked: 2013-04-25 (Catznip-3.5)
	LLFloaterIMContainerBase* im_box = LLFloaterIMContainerBase::findInstance();
	if (im_box)
	{
		im_box->setConversationFlashing(mSessionID, false);
	}
	gToolBarView->flashCommand(LLCommandId((getSessionID().notNull()) ? "conversations" : "chat"), false);
// [/SL:KB]
//	LLFloaterIMContainer* im_box = LLFloaterIMContainer::findInstance();
//	if (im_box)
//	{
//		im_box->flashConversationItemWidget(mSessionID,false);
//	}
//	gToolBarView->flashCommand(LLCommandId("chat"), false);
}

std::string LLFloaterIMSessionTab::appendTime()
{
	time_t utc_time;
	utc_time = time_corrected();
	std::string timeStr ="["+ LLTrans::getString("TimeHour")+"]:["
		+LLTrans::getString("TimeMin")+"]";
// [SL:KB] - Patch: Chat-TimestampSeconds | Checked: 2011-12-07 (Catznip-3.2)
	if (gSavedSettings.getBOOL("ChatTimestampSeconds"))
		timeStr += ":[" + LLTrans::getString("TimeSec") + "]";
// [/SL:KB]

	LLSD substitution;

	substitution["datetime"] = (S32) utc_time;
	LLStringUtil::format (timeStr, substitution);

	return timeStr;
}

void LLFloaterIMSessionTab::appendMessage(const LLChat& chat, const LLSD &args)
{

	// Update the participant activity time
//	LLFloaterIMContainer* im_box = LLFloaterIMContainer::findInstance();
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-25 (Catznip-3.5)
	LLFloaterIMContainerBase* im_box = LLFloaterIMContainerBase::findInstance();
// [/SL:KB]
	if (im_box)
	{
		im_box->setTimeNow(mSessionID,chat.mFromID);
	}
	

	LLChat& tmp_chat = const_cast<LLChat&>(chat);

	if(tmp_chat.mTimeStr.empty())
		tmp_chat.mTimeStr = appendTime();

//	if (!chat.mMuted)
// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2012-01-11 (Catznip-3.2)
	if ( (!chat.mMuted) || ((isNearbyChat()) && (gSavedSettings.getBOOL("ShowBlockedChat"))) )
// [/SL:KB]
	{
		tmp_chat.mFromName = chat.mFromName;
		LLSD chat_args;
		if (args) chat_args = args;
		chat_args["use_plain_text_chat_history"] =
				gSavedSettings.getBOOL("PlainTextChatHistory");
		chat_args["show_time"] = gSavedSettings.getBOOL("IMShowTime");
		chat_args["show_names_for_p2p_conv"] =
				!mIsP2PChat || gSavedSettings.getBOOL("IMShowNamesForP2PConv");

		if (mChatHistory)
		{
			mChatHistory->appendMessage(chat, chat_args);
		}
	}
}

static LLTrace::BlockTimerStatHandle FTM_BUILD_CONVERSATION_VIEW_PARTICIPANT("Build Conversation View");
void LLFloaterIMSessionTab::buildConversationViewParticipant()
{
	LL_RECORD_BLOCK_TIME(FTM_BUILD_CONVERSATION_VIEW_PARTICIPANT);

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	llassert(LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType());
// [/SL:KB]

	// Clear the widget list since we are rebuilding afresh from the model
	conversations_widgets_map::iterator widget_it = mConversationsWidgets.begin();
	while (widget_it != mConversationsWidgets.end())
	{
		removeConversationViewParticipant(widget_it->first);
		// Iterators are invalidated by erase so we need to pick begin again
		widget_it = mConversationsWidgets.begin();
	}
	
	// Get the model list
//	LLParticipantList* item = getParticipantList();
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	LLParticipantModelList* item = dynamic_cast<LLParticipantModelList*>(getParticipantList());
// [/SL:KB]
	if (!item)
	{
		// Nothing to do if the model list is inexistent
		return;
	}
	
	// Create the participants widgets now
	LLFolderViewModelItemCommon::child_list_t::const_iterator current_participant_model = item->getChildrenBegin();
	LLFolderViewModelItemCommon::child_list_t::const_iterator end_participant_model = item->getChildrenEnd();
	while (current_participant_model != end_participant_model)
	{
		LLConversationItem* participant_model = dynamic_cast<LLConversationItem*>(*current_participant_model);
		addConversationViewParticipant(participant_model);
		current_participant_model++;
	}
}

void LLFloaterIMSessionTab::addConversationViewParticipant(LLConversationItem* participant_model, bool update_view)
{
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	llassert(LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType());
// [/SL:KB]

	if (!participant_model)
	{
		// Nothing to do if the model is inexistent
		return;
	}

	// Check if the model already has an associated view
	LLUUID uuid = participant_model->getUUID();
	LLFolderViewItem* widget = get_ptr_in_map(mConversationsWidgets,uuid);
	
	// If not already present, create the participant view and attach it to the root, otherwise, just refresh it
	if (widget && update_view)
	{
		updateConversationViewParticipant(uuid); // overkill?
	}
	else
	{
		LLConversationViewParticipant* participant_view = createConversationViewParticipant(participant_model);
		mConversationsWidgets[uuid] = participant_view;
		participant_view->addToFolder(mConversationsRoot);
		participant_view->addToSession(mSessionID);
		participant_view->setVisible(TRUE);
	}
}

void LLFloaterIMSessionTab::removeConversationViewParticipant(const LLUUID& participant_id)
{
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	llassert(LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType());
// [/SL:KB]

	LLFolderViewItem* widget = get_ptr_in_map(mConversationsWidgets,participant_id);
	if (widget)
	{
		mConversationsRoot->extractItem(widget);
		delete widget;
	}
	mConversationsWidgets.erase(participant_id);
}

void LLFloaterIMSessionTab::updateConversationViewParticipant(const LLUUID& participant_id)
{
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	llassert(LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType());
// [/SL:KB]

	LLFolderViewItem* widget = get_ptr_in_map(mConversationsWidgets,participant_id);
	if (widget)
	{
		widget->refresh();
	}
}

void LLFloaterIMSessionTab::refreshConversation()
{
//	// Note: We collect participants names to change the session name only in the case of ad-hoc conversations
//	bool is_ad_hoc = (mSession ? mSession->isAdHocSessionType() : false);
//	uuid_vec_t participants_uuids; // uuids vector for building the added participants name string
//	// For P2P chat, we still need to update the session name who may have changed (switch display name for instance)
//	if (mIsP2PChat && mSession)
//	{
//		participants_uuids.push_back(mSession->mOtherParticipantID);
//	}
//
//// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
//	if (LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType())
//	{
//// [/SL:KB]
//		conversations_widgets_map::iterator widget_it = mConversationsWidgets.begin();
//		while (widget_it != mConversationsWidgets.end())
//		{
//			// Add the participant to the list except if it's the agent itself (redundant)
//			if (is_ad_hoc && (widget_it->first != gAgentID))
//			{
//				participants_uuids.push_back(widget_it->first);
//			}
//			widget_it->second->refresh();
//			widget_it->second->setVisible(TRUE);
//			++widget_it;
//		}
//// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
//	}
//// [/SL:KB]
//
//	if (is_ad_hoc || mIsP2PChat)
//	{
//		// Build the session name and update it
//		std::string session_name;
//		if (participants_uuids.size() != 0)
//		{
//			LLAvatarActions::buildResidentsString(participants_uuids, session_name);
//		}
//		else
//		{
//			session_name = LLIMModel::instance().getName(mSessionID);
//		}
//		updateSessionName(session_name);
//	}
// [SL:KB] - Patch: Chat-Title | Checked: 2013-12-15 (Catznip-3.6)
	if ( (mIsP2PChat) || ((mSession) && (mSession->isAdHocSessionType())) )
	{
		updateSessionName();
	}
// [/SL:KB]

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	if (LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType())
	{
// [/SL:KB]
		if (mSessionID.notNull())
		{
//			LLParticipantList* participant_list = getParticipantList();
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
			LLParticipantModelList* participant_list = dynamic_cast<LLParticipantModelList*>(getParticipantList());
// [/SL:KB]
			if (participant_list)
			{
				LLFolderViewModelItemCommon::child_list_t::const_iterator current_participant_model = participant_list->getChildrenBegin();
				LLFolderViewModelItemCommon::child_list_t::const_iterator end_participant_model = participant_list->getChildrenEnd();
				LLIMSpeakerMgr *speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionID);
				while (current_participant_model != end_participant_model)
				{
					LLConversationItemParticipant* participant_model = dynamic_cast<LLConversationItemParticipant*>(*current_participant_model);
					if (speaker_mgr && participant_model)
					{
						LLSpeaker *participant_speaker = speaker_mgr->findSpeaker(participant_model->getUUID());
						LLSpeaker *agent_speaker = speaker_mgr->findSpeaker(gAgentID);
						if (participant_speaker && agent_speaker)
						{
							participant_model->setDisplayModeratorRole(agent_speaker->mIsModerator && participant_speaker->mIsModerator);
						}
					}
					current_participant_model++;
				}
			}
		}
	
		mConversationViewModel.requestSortAll();
		if(mConversationsRoot != NULL)
		{
			mConversationsRoot->arrangeAll();
			mConversationsRoot->update();
		}
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	}
	else
	{
		if (mSessionID.notNull())
		{
			LLParticipantAvatarList* participant_list = dynamic_cast<LLParticipantAvatarList*>(getParticipantList());
			if (participant_list)
			{
				participant_list->onAvatarListRefreshed();
			}
		}
	}
// [/SL:KB]
//	updateHeaderAndToolbar();
//	refresh();
}

// Copied from LLFloaterIMContainer::createConversationViewParticipant(). Refactor opportunity!
LLConversationViewParticipant* LLFloaterIMSessionTab::createConversationViewParticipant(LLConversationItem* item)
{
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	llassert(LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType());
// [/SL:KB]

    LLRect panel_rect = mParticipantListPanel->getRect();
	
	LLConversationViewParticipant::Params params;
	params.name = item->getDisplayName();
	params.root = mConversationsRoot;
	params.listener = item;
	params.rect = LLRect (0, 24, panel_rect.getWidth(), 0); // *TODO: use conversation_view_participant.xml itemHeight value in lieu of 24
	params.tool_tip = params.name;
	params.participant_id = item->getUUID();
	
	return LLUICtrlFactory::create<LLConversationViewParticipant>(params);
}

void LLFloaterIMSessionTab::setSortOrder(const LLConversationSort& order)
{
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	if (LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType())
	{
// [/SL:KB]
		mConversationViewModel.setSorter(order);
		mConversationsRoot->arrangeAll();
		refreshConversation();
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	}
// [/SL:KB]
}

// [SL:KB] - Patch: Chat-IMSessionMenu | Checked: 2012-01-10 (Catznip-3.2)
bool LLFloaterIMSessionTab::onIMCheckNearbyChat()
{
	return isNearbyChat();
}

void LLFloaterIMSessionTab::onIMSetChatBarType(const LLSD& sdParam)
{
	const std::string strParam = sdParam.asString();
	gSavedSettings.setBOOL("ChatMultiLine", "expandable" == strParam);
}

bool LLFloaterIMSessionTab::onIMCheckChatBarType(const LLSD& sdParam)
{
	const std::string strParam = sdParam.asString();
	bool fMultiLine = gSavedSettings.getBOOL("ChatMultiLine");
	return ("expandable" == strParam) ? fMultiLine : !fMultiLine;
}

void LLFloaterIMSessionTab::onIMSetFontSize(const LLSD& sdParam)
{
	const std::string strParam = sdParam.asString();

	S32 nFontSize = 1;
	if ("small" == strParam)
		nFontSize = 0;
	else if ("medium" == strParam)
		nFontSize = 1;
	else if ("large" == strParam)
		nFontSize = 2;
	else
		return;
	gSavedSettings.setS32("ChatFontSize", nFontSize);
}

bool LLFloaterIMSessionTab::onIMCheckFontSize(const LLSD& sdParam)
{
	const std::string strParam = sdParam.asString();

	S32 nFontSize = gSavedSettings.getS32("ChatFontSize");
	if ("small" == strParam)
		return 0 == nFontSize;
	else if ("medium" == strParam)
		return 1 == nFontSize;
	else if ("large" == strParam)
		return 2 == nFontSize;
	return false;
}
// [/SL:KB]

void LLFloaterIMSessionTab::onIMSessionMenuItemClicked(const LLSD& userdata)
{
	std::string item = userdata.asString();

	if (item == "compact_view" || item == "expanded_view")
	{
		gSavedSettings.setBOOL("PlainTextChatHistory", item == "compact_view");
	}
	else
	{
		bool prev_value = gSavedSettings.getBOOL(item);
		gSavedSettings.setBOOL(item, !prev_value);
	}

	LLFloaterIMSessionTab::processChatHistoryStyleUpdate();
}

bool LLFloaterIMSessionTab::onIMCompactExpandedMenuItemCheck(const LLSD& userdata)
{
	std::string item = userdata.asString();
	bool is_plain_text_mode = gSavedSettings.getBOOL("PlainTextChatHistory");

	return is_plain_text_mode? item == "compact_view" : item == "expanded_view";
}


bool LLFloaterIMSessionTab::onIMShowModesMenuItemCheck(const LLSD& userdata)
{
	return gSavedSettings.getBOOL(userdata.asString());
}

// enable/disable states for the "show time" and "show names" items of the show-modes menu
bool LLFloaterIMSessionTab::onIMShowModesMenuItemEnable(const LLSD& userdata)
{
	std::string item = userdata.asString();
	bool plain_text = gSavedSettings.getBOOL("PlainTextChatHistory");
	bool is_not_names = (item != "IMShowNamesForP2PConv");
	return (plain_text && (is_not_names || mIsP2PChat));
}

// [SL:KB] - Patch: Chat-GroupModerators | Checked: Catznip-3.6
void LLFloaterIMSessionTab::onToggleViewMenu(LLUICtrl* pCtrl, const LLSD& sdParam)
{
	if ( (pCtrl) && (sdParam["visibility"].asBoolean()) )
	{
		pCtrl->getChildView("Participant List Sort")->setVisible(!mIsP2PChat);
	}
}

void LLFloaterIMSessionTab::onMenuParticipantListItemClicked(const LLSD& sdParam)
{
	const std::string strParam = sdParam.asString();
	if (LLFloaterIMContainerBase::CT_VIEW != LLFloaterIMContainerBase::getContainerType())
	{
		if ("sort_name" == strParam)
			LLParticipantAvatarList::setSortOrder(LLParticipantAvatarList::E_SORT_BY_NAME);
		else if ("sort_by_recent_speakers" == strParam)
			LLParticipantAvatarList::setSortOrder(LLParticipantAvatarList::E_SORT_BY_RECENT_SPEAKERS);
	}
	else
	{
		if (LLFloaterIMContainerView* pIMContainer = dynamic_cast<LLFloaterIMContainerView*>(LLFloaterIMContainerBase::getInstance()))
		{
			if ("sort_name" == strParam)
				pIMContainer->setSortOrderParticipants(LLConversationFilter::SO_NAME);
			else if ("sort_by_recent_speakers" == strParam)
				pIMContainer->setSortOrderParticipants(LLConversationFilter::SO_DATE);
		}
	}
}

bool LLFloaterIMSessionTab::onMenuParticipantListItemEnable(const LLSD& sdParam)
{
	const std::string strParam = sdParam.asString();
	if (LLFloaterIMContainerBase::CT_VIEW != LLFloaterIMContainerBase::getContainerType())
	{
		if ("sort_name" == strParam)
			return LLParticipantAvatarList::E_SORT_BY_NAME == LLParticipantAvatarList::getSortOrder();
		else if ("sort_by_recent_speakers" == strParam)
			return LLParticipantAvatarList::E_SORT_BY_RECENT_SPEAKERS == LLParticipantAvatarList::getSortOrder();
	}
	else
	{
		if (LLFloaterIMContainerView* pIMContainer = dynamic_cast<LLFloaterIMContainerView*>(LLFloaterIMContainerBase::getInstance()))
		{
			if ("sort_name" == strParam)
				return (pIMContainer->getSortOrder().getSortOrderParticipants() == LLConversationFilter::SO_NAME);
			else if ("sort_by_recent_speakers" == strParam)
				return (pIMContainer->getSortOrder().getSortOrderParticipants() == LLConversationFilter::SO_DATE);
		}
	}
	return false;
}
// [/SL:KB]

void LLFloaterIMSessionTab::hideOrShowTitle()
{
	const LLFloater::Params& default_params = LLFloater::getDefaultParams();
	S32 floater_header_size = default_params.header_height;
	LLView* floater_contents = getChild<LLView>("contents_view");

	LLRect floater_rect = getLocalRect();
	S32 top_border_of_contents = floater_rect.mTop - (isTornOff()? floater_header_size : 0);
	LLRect handle_rect (0, floater_rect.mTop, floater_rect.mRight, top_border_of_contents);
	LLRect contents_rect (0, top_border_of_contents, floater_rect.mRight, floater_rect.mBottom);
	mDragHandle->setShape(handle_rect);
	mDragHandle->setVisible(isTornOff());
	floater_contents->setShape(contents_rect);
}

// [SL:KB] - Patch: Chat-Title | Checked: 2013-12-15 (Catznip-3.6)
void LLFloaterIMSessionTab::updateSessionName()
{
	mInputEditor->setLabel(LLTrans::getString("IM_to_label") + " " + getShortTitle());
}
// [/SL:KB]
//void LLFloaterIMSessionTab::updateSessionName(const std::string& name)
//{
//	mInputEditor->setLabel(LLTrans::getString("IM_to_label") + " " + name);
//}

//void LLFloaterIMSessionTab::hideAllStandardButtons()
//{
//	for (S32 i = 0; i < BUTTON_COUNT; i++)
//	{
//		if (mButtons[i])
//		{
//			// Hide the standard header buttons in a docked IM floater.
//			mButtons[i]->setVisible(false);
//		}
//	}
//}

// [SL:KB] - Patch: Chat-Refactor | Checked: 2013-08-28 (Catznip-3.6)
void LLFloaterIMSessionTab::updateExpandCollapseBtn()
{
	// Display collapse image (<<) if the floater is hosted
	// or if it is torn off but has an open control panel.
	bool is_view = (LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType());
	bool is_not_torn_off = !isTornOff();
	bool is_expanded = ((is_view) && (is_not_torn_off)) || mParticipantListPanel->getVisible();
   
	mExpandCollapseBtn->setImageOverlay(getString(is_expanded ? "collapse_icon" : "expand_icon"));
	mExpandCollapseBtn->setToolTip(
			is_not_torn_off ? getString("expcol_button_not_tearoff_tooltip")
			                : (is_expanded ? getString("expcol_button_tearoff_and_expanded_tooltip") :
			                                 getString("expcol_button_tearoff_and_collapsed_tooltip")));

// [SL:KB] - Patch: Chat-IMPanel | Checked: 2014-02-21 (Catznip-3.7)
	mExpandCollapseBtn->setEnabled( ((!is_view) || (!is_not_torn_off)) );
// [/SL:KB
//	// The button (>>) should be disabled for torn off P2P conversations.
//	mExpandCollapseBtn->setEnabled( (!mIsP2PChat) || ((is_view) && (is_not_torn_off)) );
}

void LLFloaterIMSessionTab::updateShowParticipantList()
{
	// Participant list should be visible only in torn off floaters.
//	bool is_participant_list_visible = ((LLFloaterIMContainerBase::CT_VIEW != LLFloaterIMContainerBase::getContainerType()) || (isTornOff())) && (mIsParticipantListExpanded) && (!mIsP2PChat);
// [SL:KB] - Patch: Chat-IMPanel | Checked: 2014-02-20 (Catznip-3.7)
	bool is_participant_list_visible = ((LLFloaterIMContainerBase::CT_VIEW != LLFloaterIMContainerBase::getContainerType()) || (isTornOff())) && (mIsParticipantListExpanded);
// [/SL:KB
	mParticipantListAndHistoryStack->collapsePanel(mParticipantListPanel, !is_participant_list_visible);
	mParticipantListPanel->setVisible(is_participant_list_visible);
}
// [/SL:KB]

//void LLFloaterIMSessionTab::updateHeaderAndToolbar()
//{
//	// prevent start conversation before its container
//    LLFloaterIMContainer::getInstance();
//
//	bool is_not_torn_off = !checkIfTornOff();
//	if (is_not_torn_off)
//	{
//		hideAllStandardButtons();
//	}
//
//	hideOrShowTitle();
//
//	// Participant list should be visible only in torn off floaters.
//	bool is_participant_list_visible =
//			!is_not_torn_off
//			&& mIsParticipantListExpanded
//			&& !mIsP2PChat;
//
//	mParticipantListAndHistoryStack->collapsePanel(mParticipantListPanel, !is_participant_list_visible);
//    mParticipantListPanel->setVisible(is_participant_list_visible);
//
//	// Display collapse image (<<) if the floater is hosted
//	// or if it is torn off but has an open control panel.
//	bool is_expanded = is_not_torn_off || is_participant_list_visible;
//    
//	mExpandCollapseBtn->setImageOverlay(getString(is_expanded ? "collapse_icon" : "expand_icon"));
//	mExpandCollapseBtn->setToolTip(
//			is_not_torn_off?
//				getString("expcol_button_not_tearoff_tooltip") :
//				(is_expanded?
//					getString("expcol_button_tearoff_and_expanded_tooltip") :
//					getString("expcol_button_tearoff_and_collapsed_tooltip")));
//
//	// toggle floater's drag handle and title visibility
//	if (mDragHandle)
//	{
//		mDragHandle->setTitleVisible(!is_not_torn_off);
//	}
//
//	// The button (>>) should be disabled for torn off P2P conversations.
//	mExpandCollapseBtn->setEnabled(is_not_torn_off || !mIsP2PChat);
//
//	mTearOffBtn->setImageOverlay(getString(is_not_torn_off? "tear_off_icon" : "return_icon"));
//	mTearOffBtn->setToolTip(getString(is_not_torn_off? "tooltip_to_separate_window" : "tooltip_to_main_window"));
//
//
//	mCloseBtn->setVisible(is_not_torn_off && !mIsNearbyChat);
//
//	enableDisableCallBtn();
//
//	showTranslationCheckbox();
//}
 
void LLFloaterIMSessionTab::forceReshape()
{
    LLRect floater_rect = getRect();
    reshape(llmax(floater_rect.getWidth(), this->getMinWidth()),
    		llmax(floater_rect.getHeight(), this->getMinHeight()),
    		true);
}


void LLFloaterIMSessionTab::reshapeChatLayoutPanel()
{
	mChatLayoutPanel->reshape(mChatLayoutPanel->getRect().getWidth(), mInputEditor->getRect().getHeight() + mInputEditorPad, FALSE);
}

//void LLFloaterIMSessionTab::showTranslationCheckbox(BOOL show)
//{
//	mTranslationCheckBox->setVisible(mIsNearbyChat && show);
//}

// static
void LLFloaterIMSessionTab::processChatHistoryStyleUpdate(bool clean_messages/* = false*/)
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("impanel");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin();
			iter != inst_list.end(); ++iter)
	{
		LLFloaterIMSession* floater = dynamic_cast<LLFloaterIMSession*>(*iter);
		if (floater)
		{
			floater->reloadMessages(clean_messages);
		}
	}

	LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
	if (nearby_chat)
	{
             nearby_chat->reloadMessages(clean_messages);
	}
}

// static
void LLFloaterIMSessionTab::reloadEmptyFloaters()
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("impanel");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin();
		iter != inst_list.end(); ++iter)
	{
		LLFloaterIMSession* floater = dynamic_cast<LLFloaterIMSession*>(*iter);
		if (floater && floater->getLastChatMessageIndex() == -1)
		{
			floater->reloadMessages(true);
		}
	}

	LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
	if (nearby_chat && nearby_chat->getMessageArchiveLength() == 0)
	{
		nearby_chat->reloadMessages(true);
	}
}

void LLFloaterIMSessionTab::updateCallBtnState(bool callIsActive)
{
	mVoiceButton->setImageOverlay(callIsActive? getString("call_btn_stop") : getString("call_btn_start"));
	mVoiceButton->setToolTip(callIsActive? getString("end_call_button_tooltip") : getString("start_call_button_tooltip"));

	enableDisableCallBtn();
}

void LLFloaterIMSessionTab::onSlide(LLFloaterIMSessionTab* self)
{
	bool should_be_expanded = false;
//	LLFloaterIMContainer* host_floater = dynamic_cast<LLFloaterIMContainer*>(self->getHost());
//	if (host_floater)
//	{
//		// Hide the messages pane if a floater is hosted in the Conversations
//		host_floater->collapseMessagesPane(true);
//	}
// [SL:KB] - Patch: Chat-Container | Checked: 2013-04-25 (Catznip-3.5)
	if ( (LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType()) && (!self->isTornOff()) )
	{
		// Hide the messages pane if a floater is hosted in the Conversations
		if (LLFloaterIMContainerView* host_floater = dynamic_cast<LLFloaterIMContainerView*>(self->getHost()))
			host_floater->collapseMessagesPane(true);
	}
// [/SL:KB]
	else ///< floater is torn off
	{
//		if (!self->mIsP2PChat)
		{
            // The state must toggle the collapsed state of the panel
           should_be_expanded = self->mParticipantListPanel->isCollapsed();

			// Update the expand/collapse flag of the participant list panel and save it
// [SL:KB] - Patch: Chat-IMPanel | Checked: 2014-02-02 (Catznip-3.6)
			gSavedSettings.setBOOL(self->getShowControlPanelControl(), should_be_expanded);
// [/SL:KB
//            gSavedSettings.setBOOL("IMShowControlPanel", should_be_expanded);
            self->mIsParticipantListExpanded = should_be_expanded;
            
            // Refresh for immediate feedback
            self->refreshConversation();
		}
	}

//	self->assignResizeLimits();
// [SL:KB] - Patch: Chat-Refactor | Checked: 2013-08-28 (Catznip-3.6)
	self->updateShowParticipantList();
	self->updateExpandCollapseBtn();
// [/SL:KB]
	if (should_be_expanded)
	{
		self->forceReshape();
	}
}

void LLFloaterIMSessionTab::onCollapseToLine(LLFloaterIMSessionTab* self)
{
//	LLFloaterIMContainer* host_floater = dynamic_cast<LLFloaterIMContainer*>(self->getHost());
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-25 (Catznip-3.5)
	LLFloaterIMContainerBase* host_floater = dynamic_cast<LLFloaterIMContainerBase*>(self->getHost());
// [/SL:KB]
	if (!host_floater)
	{
		bool expand = self->isMessagePaneExpanded();
		self->mExpandCollapseLineBtn->setImageOverlay(self->getString(expand ? "collapseline_icon" : "expandline_icon"));
		self->mContentPanel->setVisible(!expand);
		self->mToolbarPanel->setVisible(!expand);
		self->mInputEditor->enableSingleLineMode(expand);
		self->reshapeFloater(expand);
		self->setMessagePaneExpanded(!expand);
// [SL:KB] - Patch: Chat-NearbyChat | Checked: Catznip-5.3
		if (expand)
		{
			self->mMinFloaterHeight = self->getMinHeight();
			self->setResizeLimits(self->getMinWidth(), self->getRect().getHeight());
		}
		else
		{
			self->setResizeLimits(self->getMinWidth(), self->mMinFloaterHeight);
			self->mMinFloaterHeight = 0;
		}
// [/SL:KB]
	}
}

// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
void LLFloaterIMSessionTab::onHistorySearchClicked()
{
	mChatHistory->getEditor()->toggleInplaceSearchPanel();
}

void LLFloaterIMSessionTab::onHistorySearchVisibilityChanged()
{
	mSearchBtn->setToggleState(mChatHistory->getEditor()->getInplaceSearchPanel()->getVisible());
}
// [/SL:KB]

void LLFloaterIMSessionTab::reshapeFloater(bool collapse)
{
	LLRect floater_rect = getRect();

	if(collapse)
	{
		mFloaterHeight = floater_rect.getHeight();
		S32 height = mContentPanel->getRect().getHeight() + mToolbarPanel->getRect().getHeight()
			+ mChatLayoutPanel->getRect().getHeight() - mChatLayoutPanelHeight + 2;
		floater_rect.mTop -= height;
	}
	else
	{
		floater_rect.mTop = floater_rect.mBottom + mFloaterHeight;
	}

	enableResizeCtrls(true, true, !collapse);

	saveCollapsedState();
	setShape(floater_rect, true);
	mBodyStack->updateLayout();
}

void LLFloaterIMSessionTab::restoreFloater()
{
	if(!isMessagePaneExpanded())
	{
		if(isMinimized())
		{
			setMinimized(false);
		}
		mContentPanel->setVisible(true);
		mToolbarPanel->setVisible(true);
		LLRect floater_rect = getRect();
		floater_rect.mTop = floater_rect.mBottom + mFloaterHeight;
		setShape(floater_rect, true);
		mBodyStack->updateLayout();
		mExpandCollapseLineBtn->setImageOverlay(getString("expandline_icon"));
		setMessagePaneExpanded(true);
// [SL:KB] - Patch: Chat-NearbyChat | Checked: Catznip-5.3
		if (mMinFloaterHeight > 0)
		{
			setResizeLimits(getMinWidth(), mMinFloaterHeight);
		}
// [/SL:KB]
		saveCollapsedState();
		mInputEditor->enableSingleLineMode(false);
		enableResizeCtrls(true, true, true);
	}
}

/*virtual*/
void LLFloaterIMSessionTab::onOpen(const LLSD& key)
{
//	if (!checkIfTornOff())
//	{
//		LLFloaterIMContainer* host_floater = dynamic_cast<LLFloaterIMContainer*>(getHost());
//		// Show the messages pane when opening a floater hosted in the Conversations
//		host_floater->collapseMessagesPane(false);
//	}
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-25 (Catznip-3.5)
	if ( (LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType()) && (!isTornOff()) )
	{
		LLFloaterIMContainerView* host_floater = dynamic_cast<LLFloaterIMContainerView*>(getHost());
		// Show the messages pane when opening a floater hosted in the Conversations
		host_floater->collapseMessagesPane(false);
	}
// [/SL:KB]

	mInputButtonPanel->setVisible(isTornOff());
}


void LLFloaterIMSessionTab::onTearOffClicked()
{
	restoreFloater();
	setFollows(isTornOff()? FOLLOWS_ALL : FOLLOWS_NONE);
    mSaveRect = isTornOff();
    initRectControl();
// [SL:KB] - Patch: Control-FloaterTearOff |  Checked: 2013-05-03 (Catznip-3.5)
	LLFloater::onTearOffClicked();
// [/SL:KB]
//	LLFloater::onClickTearOff(this);
//	LLFloaterIMContainer* container = LLFloaterReg::findTypedInstance<LLFloaterIMContainer>("im_container");

// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-05-04 (Catznip-3.5)
	if (LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType())
	{
		LLFloaterIMContainerView* container = dynamic_cast<LLFloaterIMContainerView*>(LLFloaterIMContainerBase::getInstance());
// [/SL:KB]
		if (isTornOff())
		{
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-25 (Catznip-3.5)
			// If the torn off session was the last hosted conversation in the conversation floater, just close it
			if (1 == container->getFloaterCount())
			{
				// NOTE: * don't use LLFloaterIMContainerView::closeFloater() because that closes the IM session and not the conversations floater
				//       * we want LLFloaterIMContainerView::onClickCloseBtn() instead but it's private so we need to use the static LLFloater::onClickClose() to get at it
				LLFloater::onClickClose(container);
			}
			else
			{
				container->selectAdjacentConversation(false);
			}
// [/SL:KB]
//			container->selectAdjacentConversation(false);
			forceReshape();
		}
		//Upon re-docking the torn off floater, select the corresponding conversation line item
		else
		{
			container->selectConversation(mSessionID);

		}
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-05-04 (Catznip-3.5)
	}
	else
	{
		if (isTornOff())
		{
			forceReshape();
		}
	}
// [/SL:KB]
	mInputButtonPanel->setVisible(isTornOff());

	refreshConversation();
//	updateGearBtn();
// [SL:KB] - Patch: Chat-Refactor | Checked: 2013-08-28 (Catznip-3.6)
	updateShowParticipantList();
	updateExpandCollapseBtn();
	hideOrShowTitle();
// [/SL:KB]
}

void LLFloaterIMSessionTab::updateGearBtn()
{

	BOOL prevVisibility = mGearBtn->getVisible();
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-08-17 (Catznip-3.6)
	mGearBtn->setVisible( (mIsP2PChat) || ((mSession) && (mSession->isGroupSessionType())) );
// [/SL:KB]
//	mGearBtn->setVisible(checkIfTornOff() && mIsP2PChat);


	// Move buttons if Gear button changed visibility
	if(prevVisibility != mGearBtn->getVisible())
	{
		LLRect gear_btn_rect =  mGearBtn->getRect();
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
		LLRect view_btn_rect = mViewBtn->getRect();
// [/SL:KB]
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
		LLRect search_btn_rect = mSearchBtn->getRect();
// [/SL:KB]
		LLRect add_btn_rect = mAddBtn->getRect();
		LLRect call_btn_rect = mVoiceButton->getRect();
// [SL:KB] - Patch: Chat-Misc | Checked: Catznip-5.2
		LLRect extended_toolbar_rect = (mExtendedButtonPanel) ? mExtendedButtonPanel->getRect() : LLRect();
// [/SL:KB]
		S32 gap_width = call_btn_rect.mLeft - add_btn_rect.mRight;
		S32 right_shift = gear_btn_rect.getWidth() + gap_width;
		if(mGearBtn->getVisible())
		{
			// Move buttons to the right to give space for Gear button
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
			view_btn_rect.translate(right_shift,0);
// [/SL:KB]
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
			search_btn_rect.translate(right_shift,0);
// [/SL:KB]
			add_btn_rect.translate(right_shift,0);
			call_btn_rect.translate(right_shift,0);
// [SL:KB] - Patch: Chat-Misc | Checked: Catznip-5.2
			extended_toolbar_rect.translate(right_shift,0);
// [/SL:KB]
		}
		else
		{
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
			view_btn_rect.translate(-right_shift,0);
// [/SL:KB]
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
			search_btn_rect.translate(-right_shift,0);
// [/SL:KB]
			add_btn_rect.translate(-right_shift,0);
			call_btn_rect.translate(-right_shift,0);
// [SL:KB] - Patch: Chat-Misc | Checked: Catznip-5.2
			extended_toolbar_rect.translate(-right_shift,0);
// [/SL:KB]
		}
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
		mViewBtn->setRect(view_btn_rect);
// [/SL:KB]
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
		mSearchBtn->setRect(search_btn_rect);
// [/SL:KB]
		mAddBtn->setRect(add_btn_rect);
		mVoiceButton->setRect(call_btn_rect);
// [SL:KB] - Patch: Chat-Misc | Checked: Catznip-5.2
		if (mExtendedButtonPanel)
			mExtendedButtonPanel->setRect(extended_toolbar_rect);
// [/SL:KB]
	}
}

void LLFloaterIMSessionTab::initBtns()
{
	LLRect gear_btn_rect =  mGearBtn->getRect();
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
	LLRect view_btn_rect = mViewBtn->getRect();
// [/SL:KB]
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
	LLRect search_btn_rect = mSearchBtn->getRect();
// [/SL:KB]
	LLRect add_btn_rect = mAddBtn->getRect();
	LLRect call_btn_rect = mVoiceButton->getRect();
	S32 gap_width = call_btn_rect.mLeft - add_btn_rect.mRight;
	S32 right_shift = gear_btn_rect.getWidth() + gap_width;

// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
	view_btn_rect.translate(-right_shift,0);
// [/SL:KB]
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
	search_btn_rect.translate(-right_shift,0);
// [/SL:KB]
	add_btn_rect.translate(-right_shift,0);
	call_btn_rect.translate(-right_shift,0);

// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
	mViewBtn->setRect(view_btn_rect);
// [/SL:KB]
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
	mSearchBtn->setRect(search_btn_rect);
// [/SL:KB]
	mAddBtn->setRect(add_btn_rect);
	mVoiceButton->setRect(call_btn_rect);
}

//// static
//bool LLFloaterIMSessionTab::isChatMultiTab()
//{
//	// Restart is required in order to change chat window type.
//	return true;
//}

//bool LLFloaterIMSessionTab::checkIfTornOff()
//{
//	bool isTorn = !getHost();
//	
//	if (isTorn != isTornOff())
//	{
//		setTornOff(isTorn);
//		refreshConversation();
//	}
//
//	return isTorn;
//}

// [SL:KB] - Patch: Chat-NearbyChat | Checked: 2013-08-22 (Catznip-3.6)
bool LLFloaterIMSessionTab::hasInputText() const
{
	return (mInputEditor->getLength() > 0);
}
// [/SL:KB]

void LLFloaterIMSessionTab::doToSelected(const LLSD& userdata)
{
	// Get the list of selected items in the tab
    std::string command = userdata.asString();
    uuid_vec_t selected_uuids;
	getSelectedUUIDs(selected_uuids);
		
	// Perform the command (IM, profile, etc...) on the list using the general conversation container method
//	LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-25 (Catznip-3.5)
	LLFloaterIMContainerBase* floater_container = LLFloaterIMContainerBase::getInstance();
// [/SL:KB]
	// Note: By construction, those can only be participants so we can call doToParticipants() directly
	floater_container->doToParticipants(command, selected_uuids);
}

bool LLFloaterIMSessionTab::enableContextMenuItem(const LLSD& userdata)
{
	// Get the list of selected items in the tab
    std::string command = userdata.asString();
    uuid_vec_t selected_uuids;
	getSelectedUUIDs(selected_uuids);
	
	// Perform the item enable test on the list using the general conversation container method
//	LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-25 (Catznip-3.5)
	LLFloaterIMContainerBase* floater_container = LLFloaterIMContainerBase::getInstance();
// [/SL:KB]
	return floater_container->enableContextMenuItem(command, selected_uuids);
}

bool LLFloaterIMSessionTab::checkContextMenuItem(const LLSD& userdata)
{
	// Get the list of selected items in the tab
    std::string command = userdata.asString();
    uuid_vec_t selected_uuids;
	getSelectedUUIDs(selected_uuids);
	
	// Perform the item check on the list using the general conversation container method
//	LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-25 (Catznip-3.5)
	LLFloaterIMContainerBase* floater_container = LLFloaterIMContainerBase::getInstance();
// [/SL:KB]
	return floater_container->checkContextMenuItem(command, selected_uuids);
}

void LLFloaterIMSessionTab::getSelectedUUIDs(uuid_vec_t& selected_uuids)
{
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	if (LLFloaterIMContainerBase::CT_VIEW != LLFloaterIMContainerBase::getContainerType())
	{
		LLParticipantAvatarList* pParticipantList = dynamic_cast<LLParticipantAvatarList*>(mParticipantList);
		if (pParticipantList)
		{
			pParticipantList ->getSelectedUUIDs(selected_uuids);
		}
		return;
	}
// [/SL:KB]

    const std::set<LLFolderViewItem*> selected_items = mConversationsRoot->getSelectionList();
	
    std::set<LLFolderViewItem*>::const_iterator it = selected_items.begin();
    const std::set<LLFolderViewItem*>::const_iterator it_end = selected_items.end();
	
    for (; it != it_end; ++it)
    {
        LLConversationItem* conversation_item = static_cast<LLConversationItem *>((*it)->getViewModelItem());
        selected_uuids.push_back(conversation_item->getUUID());
    }
}

LLConversationItem* LLFloaterIMSessionTab::getCurSelectedViewModelItem()
{
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	llassert(LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType());
// [/SL:KB]

	LLConversationItem *conversationItem = NULL;

	if(mConversationsRoot && 
        mConversationsRoot->getCurSelectedItem() && 
        mConversationsRoot->getCurSelectedItem()->getViewModelItem())
	{
		conversationItem = static_cast<LLConversationItem *>(mConversationsRoot->getCurSelectedItem()->getViewModelItem()) ;
	}

	return conversationItem;
}

void LLFloaterIMSessionTab::saveCollapsedState()
{
	LLFloaterIMSessionTab* conversp = LLFloaterIMSessionTab::getConversation(mSessionID);
	if(conversp->isNearbyChat())
	{
		gSavedPerAccountSettings.setBOOL("NearbyChatIsNotCollapsed", isMessagePaneExpanded());
	}
}

LLView* LLFloaterIMSessionTab::getChatHistory()
{
	return mChatHistory;
}

BOOL LLFloaterIMSessionTab::handleKeyHere(KEY key, MASK mask )
{
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-05-04 (Catznip-3.5)
	if (LLFloaterIMContainerBase::CT_VIEW == LLFloaterIMContainerBase::getContainerType())
	{
		// LLFloaterIMContainerView needs custom handling of the navigaion keys
		BOOL handled = FALSE;
		if (mask == MASK_ALT)
		{
			LLFloaterIMContainerView* floater_container = dynamic_cast<LLFloaterIMContainerView*>(LLFloaterIMContainerBase::getInstance());
//			if ( (KEY_RETURN == key) && (isTornOff()) )
//			{
//				floater_container->expandConversation();
//				handled = TRUE;
//			}
			if ( ((KEY_UP == key) || (KEY_LEFT == key)) && (!isTornOff()) )
			{
				floater_container->selectNextorPreviousConversation(false);
				handled = TRUE;
			}
			if ( ((KEY_DOWN == key ) || (KEY_RIGHT == key)) && (!isTornOff()) )
			{
				floater_container->selectNextorPreviousConversation(true);
				handled = TRUE;
			}
		}
		return handled;
	}
	else if (LLFloaterIMContainerBase::CT_TABBED == LLFloaterIMContainerBase::getContainerType())
	{
		LLFloaterIMContainerTab* floater_container = dynamic_cast<LLFloaterIMContainerTab*>(LLFloaterIMContainerBase::getInstance());

		bool hasVericalTabs = floater_container->hasVerticalTabs();
		if ( ((!hasVericalTabs && key == KEY_LEFT) || (hasVericalTabs && key == KEY_UP)) && mask == (MASK_ALT | MASK_SHIFT) )
		{
			floater_container->selectPreviousUnreadConversation();
			return TRUE;
		}
		else if ( ((!hasVericalTabs && key == KEY_RIGHT) || (hasVericalTabs && key == KEY_DOWN)) && mask == (MASK_ALT | MASK_SHIFT) )
		{
			floater_container->selectNextUnreadConversation();
			return TRUE;
		}
	}

	// The LLTabContainer parent will handle the navigation keys
	return LLTransientDockableFloater::handleKeyHere(key, mask);
// [/SL:KB]
//	BOOL handled = FALSE;
//
//	if(mask == MASK_ALT)
//	{
//		LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
//		if (KEY_RETURN == key && !isTornOff())
//		{
//			floater_container->expandConversation();
//			handled = TRUE;
//		}
//		if ((KEY_UP == key) || (KEY_LEFT == key))
//		{
//			floater_container->selectNextorPreviousConversation(false);
//			handled = TRUE;
//		}
//		if ((KEY_DOWN == key ) || (KEY_RIGHT == key))
//		{
//			floater_container->selectNextorPreviousConversation(true);
//			handled = TRUE;
//		}
//	}
//	return handled;
}
