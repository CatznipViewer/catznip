/**
 * @file llfloaterimsessiontab.h
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

#ifndef LL_FLOATERIMSESSIONTAB_H
#define LL_FLOATERIMSESSIONTAB_H

//#include "lllayoutstack.h"
//#include "llparticipantlist.h"
#include "lltransientdockablefloater.h"
//#include "llviewercontrol.h"
//#include "lleventtimer.h"
#include "llimview.h"
#include "llconversationmodel.h"
//#include "llconversationview.h"
//#include "lltexteditor.h"
// [RLVa:KB] - @shownames
#include "rlvhelper.h"
// [/RLVa:KB]

//class LLPanelChatControlPanel;
class LLChatEntry;
class LLChatHistory;
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
class LLConversationItem;
class LLConversationViewParticipant;
class LLParticipantList;
class LLLayoutPanel;
class LLLayoutStack;
class LLTimer;
// [/SL:KB]
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
class LLMenuButton;
class LLToggleableMenu;
// [/SL:KB]

class LLFloaterIMSessionTab
	: public LLTransientDockableFloater
{
// [RLVa:KB] - @shownames
	friend struct RlvCommandHandler<RLV_TYPE_ADDREM, RLV_BHVR_SHOWNAMES>;
	friend struct RlvCommandHandler<RLV_TYPE_ADDREM, RLV_BHVR_SHOWNEARBY>;
// [/RLVa:KB]

public:
	LOG_CLASS(LLFloaterIMSessionTab);

	LLFloaterIMSessionTab(const LLSD& session_id);
	~LLFloaterIMSessionTab();

	// reload all message with new settings of visual modes
	static void processChatHistoryStyleUpdate(bool clean_messages = false);
	static void reloadEmptyFloaters();

//	/**
//	 * Returns true if chat is displayed in multi tabbed floater
//	 *         false if chat is displayed in multiple windows
//	 */
//	static bool isChatMultiTab();

	// add conversation to container
	static void addToHost(const LLUUID& session_id);

	bool isHostAttached() {return mIsHostAttached;}
	void setHostAttached(bool is_attached) {mIsHostAttached = is_attached;}

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	void setParticipantList(LLParticipantList* participant_list);
// [/SL:KB]

    static LLFloaterIMSessionTab* findConversation(const LLUUID& uuid);
    static LLFloaterIMSessionTab* getConversation(const LLUUID& uuid);

//	// show/hide the translation check box
//	void showTranslationCheckbox(const BOOL visible = FALSE);

// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-27 (Catznip-3.5)
	const LLUUID& getSessionID() const { return mSessionID; }
// [/SL:KB]
// [SL:KB] - Patch: Chat-NearbyChat | Checked: 2013-08-22 (Catznip-3.6)
	bool hasInputText() const;
// [/SL:KB]
//	bool isNearbyChat() {return mIsNearbyChat;}
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-02-02 (Catznip-3.6)
	bool isNearbyChat() const { return mIsNearbyChat; }
// [/SL:KB]
// [SL:KB] - Patch: Chat-Misc | Checked: 2013-11-28 (Catznip-3.6)
	LLChatEntry* getChatBox() { return mInputEditor; }
// [/SL:KB]
// [SL:KB] - Patch: Chat-IMPanel | Checked: 2014-02-02 (Catznip-3.6)
	const std::string getShowControlPanelControl() const;
// [/SL:KB

	// LLFloater overrides
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();
//	/*virtual*/ void setVisible(BOOL visible);
	/*virtual*/ void setFocus(BOOL focus);
// [SL:KB] - Patch: Chat-Misc | Checked: 2012-02-19 (Catznip-3.2)
	/*virtual*/ BOOL handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
// [/SL:KB]
	
	// Handle the left hand participant list widgets
	void addConversationViewParticipant(LLConversationItem* item, bool update_view = true);
	void removeConversationViewParticipant(const LLUUID& participant_id);
	void updateConversationViewParticipant(const LLUUID& participant_id);
	void refreshConversation();
	void buildConversationViewParticipant();

	void setSortOrder(const LLConversationSort& order);
	virtual void onTearOffClicked();
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
	LLToggleableMenu* getGearMenu() const { return mGearMenuHandle.get(); }
// [/SL:KB]
	void updateGearBtn();
	void initBtns();
	virtual void updateMessages() {}
	LLConversationItem* getCurSelectedViewModelItem();
	void forceReshape();
	virtual BOOL handleKeyHere( KEY key, MASK mask );
	bool isMessagePaneExpanded(){return mMessagePaneExpanded;}
	void setMessagePaneExpanded(bool expanded){mMessagePaneExpanded = expanded;}
	void restoreFloater();
	void saveCollapsedState();

	LLView* getChatHistory();

protected:
// [SL:KB] - Patch: Chat-IMSessionMenu | Checked: 2013-08-18 (Catznip-3.6)
	       bool onIMCheckNearbyChat();
	static void onIMSetChatBarType(const LLSD& sdParam);
	static bool onIMCheckChatBarType(const LLSD& sdParam);
	static void onIMSetFontSize(const LLSD& sdParam);
	static bool onIMCheckFontSize(const LLSD& sdParam);
// [/SL:KB]

// [SL:KB] - Patch: Chat-GroupModerators | Checked: Catznip-3.6
	void onToggleViewMenu(LLUICtrl* pCtrl, const LLSD& sdParam);
	void onMenuParticipantListItemClicked(const LLSD& sdParam);
	bool onMenuParticipantListItemEnable(const LLSD& sdParam);
// [/SL:KB]

	// callback for click on any items of the visual states menu
	void onIMSessionMenuItemClicked(const LLSD& userdata);

	// callback for check/uncheck of the expanded/collapse mode's switcher
	bool onIMCompactExpandedMenuItemCheck(const LLSD& userdata);

	//
	bool onIMShowModesMenuItemCheck(const LLSD& userdata);
	bool onIMShowModesMenuItemEnable(const LLSD& userdata);
	static void onSlide(LLFloaterIMSessionTab *self);
	static void onCollapseToLine(LLFloaterIMSessionTab *self);
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
	void onHistorySearchClicked();
	void onHistorySearchVisibilityChanged();
// [/SL:KB]
	void reshapeFloater(bool collapse);

	// refresh a visual state of the Call button
	void updateCallBtnState(bool callIsActive);

	void hideOrShowTitle(); // toggle the floater's drag handle
//	void hideAllStandardButtons();

// [SL:KB] - Patch: Chat-Refactor | Checked: 2013-08-28 (Catznip-3.6)
	void updateExpandCollapseBtn();
	void updateShowParticipantList();
// [/SL:KB]
//	/// Update floater header and toolbar buttons when hosted/torn off state is toggled.
//	void updateHeaderAndToolbar();

	// Update the input field help text and other places that need the session name
// [SL:KB] - Patch: Chat-Title | Checked: 2013-12-15 (Catznip-3.6)
	/*virtual*/ void updateSessionName();
// [/Sl:KB]
//	virtual void updateSessionName(const std::string& name);

	// set the enable/disable state for the Call button
	virtual void enableDisableCallBtn();

	// process focus events to set a currently active session
	/* virtual */ void onFocusLost();
	/* virtual */ void onFocusReceived();

	// prepare chat's params and out one message to chatHistory
	void appendMessage(const LLChat& chat, const LLSD &args = 0);

	std::string appendTime();
//	void assignResizeLimits();

//	S32  mFloaterExtraWidth;

	bool mIsNearbyChat;
	bool mIsP2PChat;

	bool mMessagePaneExpanded;
	bool mIsParticipantListExpanded;


	LLIMModel::LLIMSession* mSession;

	// Participants list: model and view
	LLConversationViewParticipant* createConversationViewParticipant(LLConversationItem* item);
	
	LLUUID mSessionID; 
	LLLayoutStack* mBodyStack;
	LLLayoutStack* mParticipantListAndHistoryStack;
	LLLayoutPanel* mParticipantListPanel;	// add the widgets to that see mConversationsListPanel
	LLLayoutPanel* mRightPartPanel;
	LLLayoutPanel* mContentPanel;
	LLLayoutPanel* mToolbarPanel;
	LLLayoutPanel* mInputButtonPanel;
	LLParticipantList* getParticipantList();
	conversations_widgets_map mConversationsWidgets;
	LLConversationViewModel mConversationViewModel;
	LLFolderView* mConversationsRoot;
	LLScrollContainer* mScroller;

    LLChatHistory* mChatHistory;
	LLChatEntry* mInputEditor;
	LLLayoutPanel * mChatLayoutPanel;
	LLLayoutStack * mInputPanels;
	
	LLButton* mExpandCollapseLineBtn;
	LLButton* mExpandCollapseBtn;
//	LLButton* mTearOffBtn;
//	LLButton* mCloseBtn;
//	LLButton* mGearBtn;
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2013-11-27 (Catznip-3.6)
	LLMenuButton* mGearBtn;
	LLHandle<LLToggleableMenu> mGearMenuHandle;
//	LLButton* mViewBtn;
// [/SL:KB]
// [SL:KB] - Patch: Chat-GroupModerators | Checked: Catznip-3.6
	LLMenuButton* mViewBtn;
// [/SL:KB]
	LLButton* mAddBtn;
    LLButton* mVoiceButton;
// [SL:KB] - Patch: Chat-Misc | Checked: 2014-03-22 (Catznip-3.6)
	LLButton* mSearchBtn;
	LLPanel* mExtendedButtonPanel = nullptr;
// [/SL:KB]
//    LLUICtrl* mTranslationCheckBox;

private:
	// Handling selection and contextual menu
    void doToSelected(const LLSD& userdata);
    bool enableContextMenuItem(const LLSD& userdata);
    bool checkContextMenuItem(const LLSD& userdata);
	
    void getSelectedUUIDs(uuid_vec_t& selected_uuids);
	
	/// Refreshes the floater at a constant rate.
	virtual void refresh() = 0;

	/**
	 * Adjusts chat history height to fit vertically with input chat field
	 * and avoid overlapping, since input chat field can be vertically expanded.
	 * Implementation: chat history bottom "follows" top+top_pad of input chat field
	 */
	void reshapeChatLayoutPanel();

	void onInputEditorClicked();

//	bool checkIfTornOff();
    bool mIsHostAttached;
    bool mHasVisibleBeenInitialized;

	LLTimer* mRefreshTimer; ///< Defines the rate at which refresh() is called.
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	LLParticipantList* mParticipantList;
// [/SL:KB]

	S32 mInputEditorPad;
	S32 mChatLayoutPanelHeight;
	S32 mFloaterHeight;
// [SL:KB] - Patch: Chat-NearbyChat | Checked: Catznip-5.3
	S32 mMinFloaterHeight = 0;
// [/SL:KB]
};


#endif /* LL_FLOATERIMSESSIONTAB_H */
