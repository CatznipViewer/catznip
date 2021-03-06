/** 
 * @file llavatarlistitem.h
 * @brief avatar list item header file
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

#ifndef LL_LLAVATARLISTITEM_H
#define LL_LLAVATARLISTITEM_H

#include <boost/signals2.hpp>

#include "llpanel.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "llstyle.h"

#include "llcallingcard.h" // for LLFriendObserver

class LLAvatarIconCtrl;
class LLOutputMonitorCtrl;
class LLAvatarName;
class LLIconCtrl;

// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
typedef enum
{
	NF_DISPLAYNAME = 0,
	NF_USERNAME = 1,
	NF_COMPLETENAME = 2
} EAvatarListNameFormat;
// [/SL:KB]

// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2013-06-03 (Catznip-3.4)
typedef enum
{
	SP_NEVER = 0,			// Never show permission icons
	SP_HOVER = 1,			// Only show permission icons on hover
	SP_NONDEFAULT = 2,		// Show permissions different from default
	SP_COUNT
} EShowPermissionType;
// [/SL:KB]

class LLAvatarListItem : public LLPanel, public LLFriendObserver
{
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<LLStyle::Params>	default_style,
									voice_call_invited_style,
									voice_call_joined_style,
									voice_call_left_style,
									online_style,
// [SL:KB] - Patch: Chat-GroupModerators | Checked: Catznip-3.3
									offline_style,
									moderator_style;
// [/SL:KB]
//									offline_style;

		Optional<S32>				name_right_pad;

		Params();
	};

	typedef enum e_item_state_type {
		IS_DEFAULT,
		IS_VOICE_INVITED,
		IS_VOICE_JOINED,
		IS_VOICE_LEFT,
		IS_ONLINE,
		IS_OFFLINE,
// [SL:KB] - Patch: Chat-GroupModerators | Checked: Catznip-3.3
		IS_MODERATOR,
// [/SL:KB]
	} EItemState;

	/**
	 * Creates an instance of LLAvatarListItem.
	 *
	 * It is not registered with LLDefaultChildRegistry. It is built via LLUICtrlFactory::buildPanel
	 * or via registered LLCallbackMap depend on passed parameter.
	 * 
	 * @param not_from_ui_factory if true instance will be build with LLUICtrlFactory::buildPanel 
	 * otherwise it should be registered via LLCallbackMap before creating.
	 */
	LLAvatarListItem(bool not_from_ui_factory = true);
	virtual ~LLAvatarListItem();

	virtual BOOL postBuild();

	/**
	 * Processes notification from speaker indicator to update children when indicator's visibility is changed.
	 */
    virtual void handleVisibilityChange ( BOOL new_visibility );
	virtual S32	notifyParent(const LLSD& info);
	virtual void onMouseLeave(S32 x, S32 y, MASK mask);
	virtual void onMouseEnter(S32 x, S32 y, MASK mask);
	virtual void setValue(const LLSD& value);
	virtual void changed(U32 mask); // from LLFriendObserver

	void setOnline(bool online);
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
	void updateAvatarName(EAvatarListNameFormat name_format); // re-query the name cache
// [/SL:KB]
//	void updateAvatarName(); // re-query the name cache
	void setAvatarName(const std::string& name);
	void setAvatarToolTip(const std::string& tooltip);
	void setHighlight(const std::string& highlight);
	void setState(EItemState item_style);
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
	void setAvatarId(const LLUUID& id, const LLUUID& session_id, EAvatarListNameFormat name_format, bool ignore_status_changes = false, bool is_resident = true);
	static std::string formatAvatarName(const LLAvatarName& avName, EAvatarListNameFormat eNameFormat, bool* pfShowUsername = nullptr);
// [/SL:KB]
//	void setAvatarId(const LLUUID& id, const LLUUID& session_id, bool ignore_status_changes = false, bool is_resident = true);
//	void setLastInteractionTime(U32 secs_since);
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
	void setTextField(const std::string& text);
	void setTextFieldDistance(F32 distance);
	void setTextFieldSeconds(U32 secs_since);
// [/SL:KB]
	//Show/hide profile/info btn, translating speaker indicator and avatar name coordinates accordingly
	void setShowProfileBtn(bool show);
	void setShowInfoBtn(bool show);
	void showSpeakingIndicator(bool show);
// [SL:KB] - Patch: UI-AvatarListVolumeSlider | Checked: 2012-06-03 (Catznip-3.3)
	void showVolumeSlider(bool show);
// [/SL:KB]
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2013-06-03 (Catznip-3.4)
	void setShowPermissions(EShowPermissionType spType);
// [/SL:KB]
//	void setShowPermissions(bool show) { mShowPermissions = show; };
//	void showLastInteractionTime(bool show);
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
	void showTextField(bool show);
// [/SL:KB]
	void setAvatarIconVisible(bool visible);
//	void setShowCompleteName(bool show) { mShowCompleteName = show;};
// [RLVa:KB] - Checked: RLVa-1.2.0
	void setRlvCheckShowNames(bool fRlvCheckShowNames) { mRlvCheckShowNames = fRlvCheckShowNames; }
// [/RLVa:KB]
	
	const LLUUID& getAvatarId() const;
	std::string getAvatarName() const;
// [SL:KB] - Patch: UI-SidepanelPeopleSort | Checked: Catznip-6.5
	const std::string& getAvatarUsername() const { return mAvatarUsername; }
// [/SL:KB]
	std::string getAvatarToolTip() const;

	void onInfoBtnClick();
	void onProfileBtnClick();
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-11-04 (Catznip-2.3)
	void onPermissionBtnToggle(S32 toggleRight);
	void onModifyRightsConfirmationCallback(const LLSD& notification, const LLSD& response, bool fGrant);
// [/SL:KB]

	/*virtual*/ BOOL handleDoubleClick(S32 x, S32 y, MASK mask);
// [SL:KB] - Patch: UI-AvatarListDndShare | Checked: 2011-06-19 (Catznip-2.6)
	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type, void *cargo_data, EAcceptance *accept, std::string& tooltip_msg);
// [/SL:KB]

protected:
	/**
	 * Contains indicator to show voice activity. 
	 */
	LLOutputMonitorCtrl* mSpeakingIndicator;

	LLAvatarIconCtrl* mAvatarIcon;

	/// Indicator for permission to see me online.
//	LLIconCtrl* mIconPermissionOnline;
	/// Indicator for permission to see my position on the map.
//	LLIconCtrl* mIconPermissionMap;
	/// Indicator for permission to edit my objects.
//	LLIconCtrl* mIconPermissionEditMine;
	/// Indicator for permission to edit their objects.
	LLIconCtrl* mIconPermissionEditTheirs;
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-10-26 (Catznip-2.3)
	LLButton* mIconPermissionOnline;
	LLButton* mIconPermissionMap;
	LLButton* mIconPermissionEditMine;
// [/SL:KB]

private:

	typedef enum e_online_status {
		E_OFFLINE,
		E_ONLINE,
		E_UNKNOWN,
	} EOnlineStatus;

	/**
	 * Enumeration of item elements in order from right to left.
	 * 
	 * updateChildren() assumes that indexes are in the such order to process avatar icon easier.
	 *
	 * @see updateChildren()
	 */
	typedef enum e_avatar_item_child {
		ALIC_SPEAKER_INDICATOR,
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2011-05-13 (Catznip-2.6)
		ALIC_TEXT_FIELD,
// [/SL:KB]
		ALIC_PROFILE_BUTTON,
		ALIC_INFO_BUTTON,
		ALIC_PERMISSION_ONLINE,
		ALIC_PERMISSION_MAP,
		ALIC_PERMISSION_EDIT_MINE,
		ALIC_PERMISSION_EDIT_THEIRS,
//		ALIC_INTERACTION_TIME,
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
//		ALIC_TEXT_FIELD,
// [/SL:KB]
		ALIC_NAME,
		ALIC_ICON,
		ALIC_COUNT,
	} EAvatarListItemChildIndex;

	void setNameInternal(const std::string& name, const std::string& highlight);
//	void onAvatarNameCache(const LLAvatarName& av_name);
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
	void onAvatarNameCache(const LLAvatarName& av_name, EAvatarListNameFormat name_format);
// [/SL:KB]

	std::string formatSeconds(U32 secs);

	typedef std::map<EItemState, LLColor4> icon_color_map_t;
	static icon_color_map_t& getItemIconColorMap();

	/**
	 * Initializes widths of all children to use them while changing visibility of any of them.
	 *
	 * @see updateChildren()
	 */
	static void initChildrenWidths(LLAvatarListItem* self);

	/**
	 * Updates position and rectangle of visible children to fit all available item's width.
	 */
	void updateChildren();

	/**
	 * Update visibility of active permissions icons.
	 *
	 * Need to call updateChildren() afterwards to sort out their layout.
	 */
//	bool showPermissions(bool visible);
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-10-26 (Catznip-2.3)
	bool refreshPermissions();
// [/SL:KB]

	/**
	 * Gets child view specified by index.
	 *
	 * This method implemented via switch by all EAvatarListItemChildIndex values.
	 * It is used to not store children in array or vector to avoid of increasing memory usage.
	 */
	LLView* getItemChildView(EAvatarListItemChildIndex child_index);

	LLTextBox* mAvatarName;
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
	LLTextBox* mTextField;
// [/SL:KB]
//	LLTextBox* mLastInteractionTime;
	LLStyle::Params mAvatarNameStyle;
	
	LLButton* mInfoBtn;
	LLButton* mProfileBtn;

	LLUUID mAvatarId;
// [SL:KB] - Control-AvatarListSpeakingIndicator | Checked: 2012-06-03 (Catznip-3.3.0)
	LLUUID mSessionId;
// [/SL:KB]
	std::string mHighlihtSubstring; // substring to highlight
	EOnlineStatus mOnlineStatus;
	//Flag indicating that info/profile button shouldn't be shown at all.
	//Speaker indicator and avatar name coords are translated accordingly
	bool mShowInfoBtn;
	bool mShowProfileBtn;
// [RLVa:KB] - Checked: RLVa-1.2.0
	bool mRlvCheckShowNames;
// [/RLVa:KB]

	/// indicates whether to show icons representing permissions granted
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2013-06-03 (Catznip-3.4)
	EShowPermissionType mShowPermissions;
// [/SL:KB]
//	bool mShowPermissions;

	/// true when the mouse pointer is hovering over this item
	bool mHovered;
	
//	bool mShowCompleteName;
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: Catnzip-6.5
	bool mShowUsername = true;
	std::string mAvatarUsername;
// [/SL:KB]
//	std::string mGreyOutUsername;

//	void fetchAvatarName();
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
	void fetchAvatarName(EAvatarListNameFormat name_format);
// [/SL:KB]
	boost::signals2::connection mAvatarNameCacheConnection;

	static bool	sStaticInitialized; // this variable is introduced to improve code readability
	static S32  sLeftPadding; // padding to first left visible child (icon or name)
	static S32  sNameRightPadding; // right padding from name to next visible child

	/**
	 * Contains widths of each child specified by EAvatarListItemChildIndex
	 * including padding to the next right one.
	 *
	 * @see initChildrenWidths()
	 */
	static S32 sChildrenWidths[ALIC_COUNT];

};

#endif //LL_LLAVATARLISTITEM_H
