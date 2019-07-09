/** 
 * @file llavatarlistitem.cpp
 * @brief avatar list item source file
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


#include "llviewerprecompiledheaders.h"

#include <boost/signals2.hpp>

#include "llavataractions.h"
#include "llavatarlistitem.h"

#include "llbutton.h"
#include "llfloaterreg.h"
#include "lltextutil.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llavatariconctrl.h"
#include "lloutputmonitorctrl.h"
// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2010-11-04 (Catznip-3.0)
#include "llnotificationsutil.h"
#include "llslurl.h"
// [/SL:KB]
#include "lltooldraganddrop.h"

bool LLAvatarListItem::sStaticInitialized = false;
S32 LLAvatarListItem::sLeftPadding = 0;
S32 LLAvatarListItem::sNameRightPadding = 0;
S32 LLAvatarListItem::sChildrenWidths[LLAvatarListItem::ALIC_COUNT];

static LLWidgetNameRegistry::StaticRegistrar sRegisterAvatarListItemParams(&typeid(LLAvatarListItem::Params), "avatar_list_item");

LLAvatarListItem::Params::Params()
:	default_style("default_style"),
	voice_call_invited_style("voice_call_invited_style"),
	voice_call_joined_style("voice_call_joined_style"),
	voice_call_left_style("voice_call_left_style"),
	online_style("online_style"),
	offline_style("offline_style"),
	name_right_pad("name_right_pad", 0)
{};


LLAvatarListItem::LLAvatarListItem(bool not_from_ui_factory/* = true*/)
	: LLPanel(),
	LLFriendObserver(),
	mAvatarIcon(NULL),
	mAvatarName(NULL),
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
	mTextField(NULL),
// [/SL:KB]
//	mLastInteractionTime(NULL),
	mIconPermissionOnline(NULL),
	mIconPermissionMap(NULL),
	mIconPermissionEditMine(NULL),
	mIconPermissionEditTheirs(NULL),
	mSpeakingIndicator(NULL),
	mInfoBtn(NULL),
	mProfileBtn(NULL),
	mOnlineStatus(E_UNKNOWN),
	mShowInfoBtn(true),
	mShowProfileBtn(true),
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2013-06-03 (Catznip-3.4)
	mShowPermissions(SP_NEVER),
// [/SL:KB]
//	mShowPermissions(false),
//	mShowCompleteName(false),
	mHovered(false),
	mAvatarNameCacheConnection(),
	mGreyOutUsername("")
{
	if (not_from_ui_factory)
	{
		buildFromFile("panel_avatar_list_item.xml");
	}
	// *NOTE: mantipov: do not use any member here. They can be uninitialized here in case instance
	// is created from the UICtrlFactory
}

LLAvatarListItem::~LLAvatarListItem()
{
	if (mAvatarId.notNull())
	{
		LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarId, this);
	}

	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}
}

BOOL  LLAvatarListItem::postBuild()
{
	mAvatarIcon = getChild<LLAvatarIconCtrl>("avatar_icon");
	mAvatarName = getChild<LLTextBox>("avatar_name");
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
	mTextField = getChild<LLTextBox>("text_field");
	mTextField->setVisible(false);
	mTextField->setRightAlign();
// [/SL:KB]
//	mLastInteractionTime = getChild<LLTextBox>("last_interaction");

//	mIconPermissionOnline = getChild<LLIconCtrl>("permission_online_icon");
//	mIconPermissionMap = getChild<LLIconCtrl>("permission_map_icon");
//	mIconPermissionEditMine = getChild<LLIconCtrl>("permission_edit_mine_icon");
	mIconPermissionEditTheirs = getChild<LLIconCtrl>("permission_edit_theirs_icon");
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-10-26 (Catznip-2.3)
	// NOTE-Catznip: we're leaving the names unchanged even though they're buttons now because we change too much LL code otherwise
	mIconPermissionOnline = getChild<LLButton>("permission_online_icon");
	mIconPermissionMap = getChild<LLButton>("permission_map_icon");
	mIconPermissionEditMine = getChild<LLButton>("permission_edit_mine_icon");

	mIconPermissionOnline->setClickedCallback(boost::bind(&LLAvatarListItem::onPermissionBtnToggle, this, (S32)LLRelationship::GRANT_ONLINE_STATUS));
	mIconPermissionMap->setClickedCallback(boost::bind(&LLAvatarListItem::onPermissionBtnToggle, this, (S32)LLRelationship::GRANT_MAP_LOCATION));
	mIconPermissionEditMine->setClickedCallback(boost::bind(&LLAvatarListItem::onPermissionBtnToggle, this, (S32)LLRelationship::GRANT_MODIFY_OBJECTS));
// [/SL:KB]

	mIconPermissionOnline->setVisible(false);
	mIconPermissionMap->setVisible(false);
	mIconPermissionEditMine->setVisible(false);
	mIconPermissionEditTheirs->setVisible(false);

	mSpeakingIndicator = getChild<LLOutputMonitorCtrl>("speaking_indicator");
	mSpeakingIndicator->setChannelState(LLOutputMonitorCtrl::UNDEFINED_CHANNEL);
// [SL:KB] - Control-AvatarListSpeakingIndicator | Checked: 2012-06-03 (Catznip-3.3)
	mSpeakingIndicator->setVisible(false);
// [/SL:KB]

	mInfoBtn = getChild<LLButton>("info_btn");
	mProfileBtn = getChild<LLButton>("profile_btn");

	mInfoBtn->setVisible(false);
	mInfoBtn->setClickedCallback(boost::bind(&LLAvatarListItem::onInfoBtnClick, this));

	mProfileBtn->setVisible(false);
	mProfileBtn->setClickedCallback(boost::bind(&LLAvatarListItem::onProfileBtnClick, this));

	if (!sStaticInitialized)
	{
		// Remember children widths including their padding from the next sibling,
		// so that we can hide and show them again later.
		initChildrenWidths(this);

		// Right padding between avatar name text box and nearest visible child.
		sNameRightPadding = LLUICtrlFactory::getDefaultParams<LLAvatarListItem>().name_right_pad;

		sStaticInitialized = true;
	}

// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2011-05-13 (Catznip-2.6)
	// Disable all controls by default so we'll know which ones we can skip in updateChildren()
	mIconPermissionOnline->setEnabled(SP_NEVER != mShowPermissions);
	mIconPermissionMap->setEnabled(SP_NEVER != mShowPermissions);
	mIconPermissionEditMine->setEnabled(SP_NEVER != mShowPermissions);
	mIconPermissionEditTheirs->setEnabled(SP_NEVER != mShowPermissions);
	mSpeakingIndicator->setEnabled(false);
	mInfoBtn->setEnabled(mShowInfoBtn);
	mProfileBtn->setEnabled(mShowProfileBtn);
	mTextField->LLUICtrl::setEnabled(false);					// Disabled and invisible by default (see above)
// [/SL:KB]

	return TRUE;
}

void LLAvatarListItem::handleVisibilityChange ( BOOL new_visibility )
{
    //Adjust positions of icons (info button etc) when 
    //speaking indicator visibility was changed/toggled while panel was closed (not visible)
    if(new_visibility && mSpeakingIndicator->getIndicatorToggled())
    {
        updateChildren();
        mSpeakingIndicator->setIndicatorToggled(false);
    }
}

//void LLAvatarListItem::fetchAvatarName()
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
void LLAvatarListItem::fetchAvatarName(EAvatarListNameFormat name_format)
// [/SL:KB]
{
	if (mAvatarId.notNull())
	{
		if (mAvatarNameCacheConnection.connected())
		{
			mAvatarNameCacheConnection.disconnect();
		}
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
		mAvatarNameCacheConnection = LLAvatarNameCache::get(getAvatarId(), boost::bind(&LLAvatarListItem::onAvatarNameCache, this, _2, name_format));
// [/SL:KB]
//		mAvatarNameCacheConnection = LLAvatarNameCache::get(getAvatarId(), boost::bind(&LLAvatarListItem::onAvatarNameCache, this, _2));
	}
}

S32 LLAvatarListItem::notifyParent(const LLSD& info)
{
	if (info.has("visibility_changed"))
	{
		updateChildren();
		return 1;
	}
	return LLPanel::notifyParent(info);
}

void LLAvatarListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible( true);
	mInfoBtn->setVisible(mShowInfoBtn);
	mProfileBtn->setVisible(mShowProfileBtn);

	mHovered = true;
	LLPanel::onMouseEnter(x, y, mask);

//	showPermissions(mShowPermissions);
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-10-26 (Catznip-2.3)
	refreshPermissions();
// [/SL:KB]
	updateChildren();
}

void LLAvatarListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible( false);
	mInfoBtn->setVisible(false);
	mProfileBtn->setVisible(false);

	mHovered = false;
	LLPanel::onMouseLeave(x, y, mask);

//	showPermissions(false);
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-10-26 (Catznip-2.3)
	refreshPermissions();
// [/SL:KB]
	updateChildren();
}

// virtual, called by LLAvatarTracker
void LLAvatarListItem::changed(U32 mask)
{
	// no need to check mAvatarId for null in this case
	setOnline(LLAvatarTracker::instance().isBuddyOnline(mAvatarId));

	if (mask & LLFriendObserver::POWERS)
	{
//		showPermissions(mShowPermissions && mHovered);
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-10-26 (Catznip-2.3)
		refreshPermissions();
// [/SL:KB]
		updateChildren();
	}
}

void LLAvatarListItem::setOnline(bool online)
{
	// *FIX: setName() overrides font style set by setOnline(). Not an issue ATM.

	if (mOnlineStatus != E_UNKNOWN && (bool) mOnlineStatus == online)
		return;

	mOnlineStatus = (EOnlineStatus) online;

	// Change avatar name font style depending on the new online status.
	setState(online ? IS_ONLINE : IS_OFFLINE);
}

void LLAvatarListItem::setAvatarName(const std::string& name)
{
	setNameInternal(name, mHighlihtSubstring);
}

void LLAvatarListItem::setAvatarToolTip(const std::string& tooltip)
{
	mAvatarName->setToolTip(tooltip);
}

void LLAvatarListItem::setHighlight(const std::string& highlight)
{
	setNameInternal(mAvatarName->getText(), mHighlihtSubstring = highlight);
}

void LLAvatarListItem::setState(EItemState item_style)
{
	const LLAvatarListItem::Params& params = LLUICtrlFactory::getDefaultParams<LLAvatarListItem>();

	switch(item_style)
	{
	default:
	case IS_DEFAULT:
		mAvatarNameStyle = params.default_style();
		break;
	case IS_VOICE_INVITED:
		mAvatarNameStyle = params.voice_call_invited_style();
		break;
	case IS_VOICE_JOINED:
		mAvatarNameStyle = params.voice_call_joined_style();
		break;
	case IS_VOICE_LEFT:
		mAvatarNameStyle = params.voice_call_left_style();
		break;
	case IS_ONLINE:
		mAvatarNameStyle = params.online_style();
		break;
	case IS_OFFLINE:
		mAvatarNameStyle = params.offline_style();
		break;
	}

	// *NOTE: You cannot set the style on a text box anymore, you must
	// rebuild the text.  This will cause problems if the text contains
	// hyperlinks, as their styles will be wrong.
	setNameInternal(mAvatarName->getText(), mHighlihtSubstring);

	icon_color_map_t& item_icon_color_map = getItemIconColorMap();
	mAvatarIcon->setColor(item_icon_color_map[item_style]);
}

//void LLAvatarListItem::setAvatarId(const LLUUID& id, const LLUUID& session_id, bool ignore_status_changes/* = false*/, bool is_resident/* = true*/)
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
void LLAvatarListItem::setAvatarId(const LLUUID& id, const LLUUID& session_id, EAvatarListNameFormat name_format, bool ignore_status_changes /*= false*/, bool is_resident /*= true*/)
// [/SL:KB]
{
	if (mAvatarId.notNull())
		LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarId, this);

	mAvatarId = id;
// [SL:KB] - Control-AvatarListSpeakingIndicator | Checked: 2012-06-03 (Catznip-3.3)
	// Only set the speaker if it's currently non-null
	if (mSpeakingIndicator->getSpeakerId().notNull())
		mSpeakingIndicator->setSpeakerId(id, session_id);
	mSessionId = session_id;
// [/SL:KB]
//	mSpeakingIndicator->setSpeakerId(id, session_id);

	// We'll be notified on avatar online status changes
	if (!ignore_status_changes && mAvatarId.notNull())
		LLAvatarTracker::instance().addParticularFriendObserver(mAvatarId, this);

	if (is_resident)
	{
		mAvatarIcon->setValue(id);

		// Set avatar name.
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
		fetchAvatarName(name_format);
// [/SL:KB]
//		fetchAvatarName();
	}
}

// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-10-24 (Catznip-2.3)
void LLAvatarListItem::setShowPermissions(EShowPermissionType spType)
{
	mShowPermissions = spType;

// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2011-05-13 (Catznip-2.6)
	// Reenable the controls for updateChildren()
	mIconPermissionOnline->setEnabled(SP_NEVER != mShowPermissions);
	mIconPermissionMap->setEnabled(SP_NEVER != mShowPermissions);
	mIconPermissionEditMine->setEnabled(SP_NEVER != mShowPermissions);
	mIconPermissionEditTheirs->setEnabled(SP_NEVER != mShowPermissions);
// [/SL:KB]
	
	refreshPermissions();
	updateChildren();
}
// [/SL:KB]

void LLAvatarListItem::showTextField(bool show)
{
//	mLastInteractionTime->setVisible(show);
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
	mTextField->setVisible(show);
// [/SL:KB]
// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2011-05-13 (Catznip-2.6)
	// Reenable for updateChildren()
	mTextField->LLUICtrl::setEnabled(show);
// [/SL:KB]
	updateChildren();
}

// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
void LLAvatarListItem::setTextField(const std::string& text)
{
	mTextField->setValue(text);
}

void LLAvatarListItem::setTextFieldDistance(F32 distance)
{
	mTextField->setValue(llformat("%3.1fm", distance));
}

void LLAvatarListItem::setTextFieldSeconds(U32 secs_since)
{
	mTextField->setValue(formatSeconds(secs_since));
}
// [/SL:KB]
//void LLAvatarListItem::setLastInteractionTime(U32 secs_since)
//{
//	mLastInteractionTime->setValue(formatSeconds(secs_since));
//}

void LLAvatarListItem::setShowInfoBtn(bool show)
{
	mShowInfoBtn = show;
// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2011-05-13 (Catznip-3.0.0a) | Added: Catznip-2.6.0a
	// Reenable for updateChildren()
	mInfoBtn->setEnabled(show);
// [/SL:KB]
}

void LLAvatarListItem::setShowProfileBtn(bool show)
{
	mShowProfileBtn = show;
// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2011-05-13 (Catznip-3.0.0a) | Added: Catznip-2.6.0a
	// Reenable for updateChildren()
	mProfileBtn->setEnabled(show);
// [/SL:KB]
}

// [SL:KB] - Control-AvatarListSpeakingIndicator | Checked: 2012-06-03 (Catznip-3.3.0)
void LLAvatarListItem::showSpeakingIndicator(bool visible)
{
	// Already done? Then do nothing.
	if (mSpeakingIndicator->getEnabled() == (BOOL)visible)
		return;

	if (visible)
		mSpeakingIndicator->setSpeakerId(mAvatarId, mSessionId);
	else
		mSpeakingIndicator->setSpeakerId(LLUUID::null);
	mSpeakingIndicator->setEnabled(visible);
	updateChildren();
}
// [/SL:KB]
//void LLAvatarListItem::showSpeakingIndicator(bool visible)
//{
//	// Already done? Then do nothing.
//	if (mSpeakingIndicator->getVisible() == (BOOL)visible)
//		return;
//// Disabled to not contradict with SpeakingIndicatorManager functionality. EXT-3976
//// probably this method should be totally removed.
////	mSpeakingIndicator->setVisible(visible);
////	updateChildren();
//}

// [SL:KB] - Patch: UI-AvatarListVolumeSlider | Checked: 2012-06-03 (Catznip-3.3)
void LLAvatarListItem::showVolumeSlider(bool visible)
{
	// Already done? Then do nothing.
	if (mSpeakingIndicator->getShowVolumeSlider() == visible)
		return;

	// TODO-Catznip: fix hard-coded width values here, updateChildren() and LLOutputMonitorCtrl::showVolumeSlider()
	const LLRect& rctIndicator = mSpeakingIndicator->getRect();
	S32 nWidthDelta = (visible) ? 90 : -90;
	mSpeakingIndicator->reshape(rctIndicator.getWidth() + nWidthDelta, rctIndicator.getHeight());
	mSpeakingIndicator->showVolumeSlider(visible);

	updateChildren();
}
// [/SL:KB]

void LLAvatarListItem::setAvatarIconVisible(bool visible)
{
	// Already done? Then do nothing.
	if (mAvatarIcon->getVisible() == (BOOL)visible)
	{
		return;
	}

	// Show/hide avatar icon.
	mAvatarIcon->setVisible(visible);
	updateChildren();
}

void LLAvatarListItem::onInfoBtnClick()
{
	LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", mAvatarId));
}

void LLAvatarListItem::onProfileBtnClick()
{
	LLAvatarActions::showProfile(mAvatarId);
}

// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-11-04 (Catznip-2.3)
void LLAvatarListItem::onPermissionBtnToggle(S32 toggleRight)
{
	LLRelationship* pRelationship = const_cast<LLRelationship*>(LLAvatarTracker::instance().getBuddyInfo(mAvatarId));
	if (!pRelationship)
		return;

	if (LLRelationship::GRANT_MODIFY_OBJECTS != toggleRight)
	{
		S32 rights = pRelationship->getRightsGrantedTo();
		if ( (rights & toggleRight) == toggleRight)
		{
			rights &= ~toggleRight;
			// Revoke the permission locally until we hear back from the region
			pRelationship->revokeRights(toggleRight, LLRelationship::GRANT_NONE);
		}
		else
		{
			rights |= toggleRight;
			// Grant the permission locally until we hear back from the region
			pRelationship->grantRights(toggleRight, LLRelationship::GRANT_NONE);
		}

		LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(mAvatarId, rights);
		refreshPermissions();
		updateChildren();
	}
	else
	{
		LLSD args;
		args["NAME"] = LLSLURL("agent", mAvatarId, "fullname").getSLURLString();

		if (!pRelationship->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS))
		{
			LLNotificationsUtil::add("GrantModifyRights", args, LLSD(), 
				boost::bind(&LLAvatarListItem::onModifyRightsConfirmationCallback, this, _1, _2, true));
		}
		else
		{
			LLNotificationsUtil::add("RevokeModifyRights", args, LLSD(),
				boost::bind(&LLAvatarListItem::onModifyRightsConfirmationCallback, this, _1, _2, false));
		}
	}
}

void LLAvatarListItem::onModifyRightsConfirmationCallback(const LLSD& notification, const LLSD& response, bool fGrant)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLRelationship* pRelationship = const_cast<LLRelationship*>(LLAvatarTracker::instance().getBuddyInfo(mAvatarId));
		if (!pRelationship)
			return;

		S32 rights = pRelationship->getRightsGrantedTo();
		if (!fGrant)
		{
			rights &= ~LLRelationship::GRANT_MODIFY_OBJECTS;
			// Revoke the permission locally until we hear back from the region
			pRelationship->revokeRights(LLRelationship::GRANT_MODIFY_OBJECTS, LLRelationship::GRANT_NONE);
		}
		else
		{
			rights |= LLRelationship::GRANT_MODIFY_OBJECTS;
			// Grant the permission locally until we hear back from the region
			pRelationship->grantRights(LLRelationship::GRANT_MODIFY_OBJECTS, LLRelationship::GRANT_NONE);
		}

		LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(mAvatarId, rights);
		refreshPermissions();
		updateChildren();
	}
}
// [/SL:KB]

BOOL LLAvatarListItem::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if(mInfoBtn->getRect().pointInRect(x, y))
	{
		onInfoBtnClick();
		return TRUE;
	}
	if(mProfileBtn->getRect().pointInRect(x, y))
	{
		onProfileBtnClick();
		return TRUE;
	}
	return LLPanel::handleDoubleClick(x, y, mask);
}

void LLAvatarListItem::setValue( const LLSD& value )
{
	if (!value.isMap()) return;;
	if (!value.has("selected")) return;
	getChildView("selected_icon")->setVisible( value["selected"]);
}

const LLUUID& LLAvatarListItem::getAvatarId() const
{
	return mAvatarId;
}

std::string LLAvatarListItem::getAvatarName() const
{
	return mAvatarName->getValue();
}

std::string LLAvatarListItem::getAvatarToolTip() const
{
	return mAvatarName->getToolTip();
}

// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
void LLAvatarListItem::updateAvatarName(EAvatarListNameFormat name_format)
{
	fetchAvatarName(name_format);
}
// [/SL:KB]
//void LLAvatarListItem::updateAvatarName()
//{
//	fetchAvatarName();
//}

//== PRIVATE SECITON ==========================================================

void LLAvatarListItem::setNameInternal(const std::string& name, const std::string& highlight)
{
//    if(mShowCompleteName && highlight.empty())
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
	if ( (!mGreyOutUsername.empty()) && (highlight.empty()) )
// [/SL:KB]
    {
        LLTextUtil::textboxSetGreyedVal(mAvatarName, mAvatarNameStyle, name, mGreyOutUsername);
    }
    else
    {
        LLTextUtil::textboxSetHighlightedVal(mAvatarName, mAvatarNameStyle, name, highlight);
    }
}

// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
std::string LLAvatarListItem::formatAvatarName(const LLAvatarName& avName, EAvatarListNameFormat eNameFormat, bool* pfShowUsername)
{
	switch (eNameFormat)
	{
		case NF_USERNAME:
			if (pfShowUsername)
				*pfShowUsername = false;
			return (!avName.getAccountName().empty()) ? avName.getAccountName() : avName.getDisplayName();
		case NF_COMPLETENAME:
			if (pfShowUsername)
				*pfShowUsername = !avName.isDisplayNameDefault();
			return avName.getCompleteName(true/*, LLAvatarName::SHOW_MISMATCH*/);
		case NF_DISPLAYNAME:
		default:
			if (pfShowUsername)
				*pfShowUsername = false;
			return avName.getDisplayName();
	}
}
// [/SL:KB]

//void LLAvatarListItem::onAvatarNameCache(const LLAvatarName& av_name)
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
void LLAvatarListItem::onAvatarNameCache(const LLAvatarName& av_name, EAvatarListNameFormat name_format)
// [/SL:KB]
{
	mAvatarNameCacheConnection.disconnect();

// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
	bool fVisibleUsername;
	setAvatarName(formatAvatarName(av_name, name_format, &fVisibleUsername));
	mGreyOutUsername = (fVisibleUsername) ? llformat("(%s)", av_name.getAccountName().c_str()) : LLStringUtil::null;
// [/SL:KB]
//	mGreyOutUsername = "";
//	std::string name_string = mShowCompleteName? av_name.getCompleteName(false) : av_name.getDisplayName();
//	if(av_name.getCompleteName() != av_name.getUserName())
//	{
//	    mGreyOutUsername = "[ " + av_name.getUserName(true) + " ]";
//	    LLStringUtil::toLower(mGreyOutUsername);
//	}
//	setAvatarName(name_string);
	setAvatarToolTip(av_name.getUserName());

	//requesting the list to resort
	notifyParent(LLSD().with("sort", LLSD()));
}

// Convert given number of seconds to a string like "23 minutes", "15 hours" or "3 years",
// taking i18n into account. The format string to use is taken from the panel XML.
std::string LLAvatarListItem::formatSeconds(U32 secs)
{
	static const U32 LL_ALI_MIN		= 60;
	static const U32 LL_ALI_HOUR	= LL_ALI_MIN	* 60;
	static const U32 LL_ALI_DAY		= LL_ALI_HOUR	* 24;
	static const U32 LL_ALI_WEEK	= LL_ALI_DAY	* 7;
	static const U32 LL_ALI_MONTH	= LL_ALI_DAY	* 30;
	static const U32 LL_ALI_YEAR	= LL_ALI_DAY	* 365;

	std::string fmt; 
	U32 count = 0;

	if (secs >= LL_ALI_YEAR)
	{
		fmt = "FormatYears"; count = secs / LL_ALI_YEAR;
	}
	else if (secs >= LL_ALI_MONTH)
	{
		fmt = "FormatMonths"; count = secs / LL_ALI_MONTH;
	}
	else if (secs >= LL_ALI_WEEK)
	{
		fmt = "FormatWeeks"; count = secs / LL_ALI_WEEK;
	}
	else if (secs >= LL_ALI_DAY)
	{
		fmt = "FormatDays"; count = secs / LL_ALI_DAY;
	}
	else if (secs >= LL_ALI_HOUR)
	{
		fmt = "FormatHours"; count = secs / LL_ALI_HOUR;
	}
	else if (secs >= LL_ALI_MIN)
	{
		fmt = "FormatMinutes"; count = secs / LL_ALI_MIN;
	}
	else
	{
		fmt = "FormatSeconds"; count = secs;
	}

	LLStringUtil::format_map_t args;
	args["[COUNT]"] = llformat("%u", count);
	return getString(fmt, args);
}

// static
LLAvatarListItem::icon_color_map_t& LLAvatarListItem::getItemIconColorMap()
{
	static icon_color_map_t item_icon_color_map;
	if (!item_icon_color_map.empty()) return item_icon_color_map;

	item_icon_color_map.insert(
		std::make_pair(IS_DEFAULT,
		LLUIColorTable::instance().getColor("AvatarListItemIconDefaultColor", LLColor4::white)));

	item_icon_color_map.insert(
		std::make_pair(IS_VOICE_INVITED,
		LLUIColorTable::instance().getColor("AvatarListItemIconVoiceInvitedColor", LLColor4::white)));

	item_icon_color_map.insert(
		std::make_pair(IS_VOICE_JOINED,
		LLUIColorTable::instance().getColor("AvatarListItemIconVoiceJoinedColor", LLColor4::white)));

	item_icon_color_map.insert(
		std::make_pair(IS_VOICE_LEFT,
		LLUIColorTable::instance().getColor("AvatarListItemIconVoiceLeftColor", LLColor4::white)));

	item_icon_color_map.insert(
		std::make_pair(IS_ONLINE,
		LLUIColorTable::instance().getColor("AvatarListItemIconOnlineColor", LLColor4::white)));

	item_icon_color_map.insert(
		std::make_pair(IS_OFFLINE,
		LLUIColorTable::instance().getColor("AvatarListItemIconOfflineColor", LLColor4::white)));

	return item_icon_color_map;
}

// static
void LLAvatarListItem::initChildrenWidths(LLAvatarListItem* avatar_item)
{
	//speaking indicator width + padding
	S32 speaking_indicator_width = avatar_item->getRect().getWidth() - avatar_item->mSpeakingIndicator->getRect().mLeft;

// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2011-05-13 (Catznip-2.6)
	// Text field textbox width + padding
	S32 text_field_width = avatar_item->mSpeakingIndicator->getRect().mLeft - avatar_item->mTextField->getRect().mLeft;
// [/SL:KB]

	//profile btn width + padding
//	S32 profile_btn_width = avatar_item->mSpeakingIndicator->getRect().mLeft - avatar_item->mProfileBtn->getRect().mLeft;
// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2011-05-13 (Catznip-3.0.0a) | Added: Catznip-2.6.0a
	S32 profile_btn_width = avatar_item->mTextField->getRect().mLeft - avatar_item->mProfileBtn->getRect().mLeft;
// [/SL:KB]

	//info btn width + padding
	S32 info_btn_width = avatar_item->mProfileBtn->getRect().mLeft - avatar_item->mInfoBtn->getRect().mLeft;

	// online permission icon width + padding
	S32 permission_online_width = avatar_item->mInfoBtn->getRect().mLeft - avatar_item->mIconPermissionOnline->getRect().mLeft;

	// map permission icon width + padding
	S32 permission_map_width = avatar_item->mIconPermissionOnline->getRect().mLeft - avatar_item->mIconPermissionMap->getRect().mLeft;

	// edit my objects permission icon width + padding
	S32 permission_edit_mine_width = avatar_item->mIconPermissionMap->getRect().mLeft - avatar_item->mIconPermissionEditMine->getRect().mLeft;

	// edit their objects permission icon width + padding
	S32 permission_edit_theirs_width = avatar_item->mIconPermissionEditMine->getRect().mLeft - avatar_item->mIconPermissionEditTheirs->getRect().mLeft;

	// last interaction time textbox width + padding
//	S32 last_interaction_time_width = avatar_item->mIconPermissionEditTheirs->getRect().mLeft - avatar_item->mLastInteractionTime->getRect().mLeft;
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
//	S32 text_field_width = avatar_item->mIconPermissionEditTheirs->getRect().mLeft - avatar_item->mTextField->getRect().mLeft;
// [/SL:KB]

	// avatar icon width + padding
	S32 icon_width = avatar_item->mAvatarName->getRect().mLeft - avatar_item->mAvatarIcon->getRect().mLeft;

	sLeftPadding = avatar_item->mAvatarIcon->getRect().mLeft;

	S32 index = ALIC_COUNT;
	sChildrenWidths[--index] = icon_width;
	sChildrenWidths[--index] = 0; // for avatar name we don't need its width, it will be calculated as "left available space"
//	sChildrenWidths[--index] = last_interaction_time_width;
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
//	sChildrenWidths[--index] = text_field_width;
// [/SL:KB]
	sChildrenWidths[--index] = permission_edit_theirs_width;
	sChildrenWidths[--index] = permission_edit_mine_width;
	sChildrenWidths[--index] = permission_map_width;
	sChildrenWidths[--index] = permission_online_width;
	sChildrenWidths[--index] = info_btn_width;
	sChildrenWidths[--index] = profile_btn_width;
// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2011-05-13 (Catznip-2.6)
	sChildrenWidths[--index] = text_field_width;
// [/SL:KB]
	sChildrenWidths[--index] = speaking_indicator_width;
	llassert(index == 0);
}

void LLAvatarListItem::updateChildren()
{
	LL_DEBUGS("AvatarItemReshape") << LL_ENDL;
	LL_DEBUGS("AvatarItemReshape") << "Updating for: " << getAvatarName() << LL_ENDL;

//	S32 name_new_width = getRect().getWidth();
//	S32 ctrl_new_left = name_new_width;
	S32 name_new_left = sLeftPadding;
// [SL:KB] - Patch: Control-AvatarList | Checked: 2012-07-04 (Catznip-3.3.0)
	S32 name_new_right = getRect().getWidth();
	S32 ctrl_new_left = name_new_right;
// [/SL:KB]

	// iterate through all children and set them into correct position depend on each child visibility
	// assume that child indexes are in back order: the first in Enum is the last (right) in the item
	// iterate & set child views starting from right to left
	for (S32 i = 0; i < ALIC_COUNT; ++i)
	{
		// skip "name" textbox, it will be processed out of loop later
		if (ALIC_NAME == i) continue;

		LLView* control = getItemChildView((EAvatarListItemChildIndex)i);

		LL_DEBUGS("AvatarItemReshape") << "Processing control: " << control->getName() << LL_ENDL;
		// skip invisible views
//		if (!control->getVisible()) continue;
// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2011-05-13 (Catznip-3.0.0a) | Added: Catznip-2.6.0a
		if (!control->getEnabled())
			continue;
// [/SL:KB]

		S32 ctrl_width = sChildrenWidths[i]; // including space between current & left controls
// [SL:KB] - Patch: Control-OutputMonitor | Checked: 2012-09-04 (Catznip-3.3)
		// HACK-Catznip: sChildrenWidths is static so shared among all instances so we need to selective adjust the size here
		if ( (ALIC_SPEAKER_INDICATOR == i) && (mSpeakingIndicator->getShowVolumeSlider()) )
		{
			ctrl_width += 90;
		}
// [/SL:KB]

//		// decrease available for 
//		name_new_width -= ctrl_width;
//		LL_DEBUGS("AvatarItemReshape") << "width: " << ctrl_width << ", name_new_width: " << name_new_width << LL_ENDL;

		LLRect control_rect = control->getRect();
		LL_DEBUGS("AvatarItemReshape") << "rect before: " << control_rect << LL_ENDL;

		if (ALIC_ICON == i)
		{
			// assume that this is the last iteration,
			// so it is not necessary to save "ctrl_new_left" value calculated on previous iterations
			ctrl_new_left = sLeftPadding;
// [SL:KB] - Patch: Control-AvatarList | Checked: 2012-07-04 (Catznip-3.3.0)
			if (mAvatarIcon->getVisible())
				name_new_left = ctrl_new_left + ctrl_width;
// [/SL:KB]
//			name_new_left = ctrl_new_left + ctrl_width;
		}
		else
		{
			ctrl_new_left -= ctrl_width;
// [SL:KB] - Patch: Control-AvatarList | Checked: 2012-07-04 (Catznip-3.3.0)
			if (control->getVisible())
				name_new_right = ctrl_new_left;
// [/SL:KB]
		}

		LL_DEBUGS("AvatarItemReshape") << "ctrl_new_left: " << ctrl_new_left << LL_ENDL;

		control_rect.setLeftTopAndSize(
			ctrl_new_left,
			control_rect.mTop,
			control_rect.getWidth(),
			control_rect.getHeight());

		LL_DEBUGS("AvatarItemReshape") << "rect after: " << control_rect << LL_ENDL;
		control->setShape(control_rect);
	}

	// set size and position of the "name" child
	LLView* name_view = getItemChildView(ALIC_NAME);
	LLRect name_view_rect = name_view->getRect();
	LL_DEBUGS("AvatarItemReshape") << "name rect before: " << name_view_rect << LL_ENDL;

	// apply paddings
//	name_new_width -= sLeftPadding;
//	name_new_width -= sNameRightPadding;
// [SL:KB] - Patch: Control-AvatarList | Checked: 2012-07-04 (Catznip-3.3.0)
	name_new_right -= sNameRightPadding;
// [/SL:KB]

	name_view_rect.setLeftTopAndSize(
		name_new_left,
		name_view_rect.mTop,
// [SL:KB] - Patch: Control-AvatarList | Checked: 2012-07-04 (Catznip-3.3.0)
		name_new_right - name_new_left,
// [/SL:KB]
//		name_new_width,
		name_view_rect.getHeight());

	name_view->setShape(name_view_rect);

	LL_DEBUGS("AvatarItemReshape") << "name rect after: " << name_view_rect << LL_ENDL;
}

//bool LLAvatarListItem::showPermissions(bool visible)
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-10-26 (Catznip-2.3)
bool LLAvatarListItem::refreshPermissions()
// [/SL:KB]
{
	static const std::string strUngrantedOverlay = "Permission_Ungranted";

	const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
//	if(relation && visible)
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-10-26 (Catznip-2.3)
	if( (relation) && (((SP_HOVER == mShowPermissions) && (mHovered)) || (SP_NONDEFAULT == mShowPermissions)) )
// [/SL:KB]
	{
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2010-10-26 (Catznip-2.3)
		bool fGrantedOnline = relation->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS);
		mIconPermissionOnline->setVisible( (!fGrantedOnline) || (mHovered) );
		mIconPermissionOnline->setImageOverlay( (fGrantedOnline) ? "" : strUngrantedOverlay);

		bool fGrantedMap = relation->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION);
		mIconPermissionMap->setVisible( (fGrantedMap) || (mHovered) );
		mIconPermissionMap->setImageOverlay( (fGrantedMap) ? "" : strUngrantedOverlay);

		bool fGrantedEditMine = relation->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS);
		mIconPermissionEditMine->setVisible( (fGrantedEditMine) || (mHovered) );
		mIconPermissionEditMine->setImageOverlay( (fGrantedEditMine) ? "" : strUngrantedOverlay);
// [/SL:KB]
//		mIconPermissionOnline->setVisible(relation->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS));
//		mIconPermissionMap->setVisible(relation->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION));
//		mIconPermissionEditMine->setVisible(relation->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS));
		mIconPermissionEditTheirs->setVisible(relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS));
	}
	else
	{
		mIconPermissionOnline->setVisible(false);
		mIconPermissionMap->setVisible(false);
		mIconPermissionEditMine->setVisible(false);
		mIconPermissionEditTheirs->setVisible(false);
	}

	return NULL != relation;
}

LLView* LLAvatarListItem::getItemChildView(EAvatarListItemChildIndex child_view_index)
{
	LLView* child_view = mAvatarName;

	switch (child_view_index)
	{
	case ALIC_ICON:
		child_view = mAvatarIcon;
		break;
	case ALIC_NAME:
		child_view = mAvatarName;
		break;
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
	case ALIC_TEXT_FIELD:
		child_view = mTextField;
		break;
// [/SL:KB]
//	case ALIC_INTERACTION_TIME:
//		child_view = mLastInteractionTime;
//		break;
	case ALIC_SPEAKER_INDICATOR:
		child_view = mSpeakingIndicator;
		break;
	case ALIC_PERMISSION_ONLINE:
		child_view = mIconPermissionOnline;
		break;
	case ALIC_PERMISSION_MAP:
		child_view = mIconPermissionMap;
		break;
	case ALIC_PERMISSION_EDIT_MINE:
		child_view = mIconPermissionEditMine;
		break;
	case ALIC_PERMISSION_EDIT_THEIRS:
		child_view = mIconPermissionEditTheirs;
		break;
	case ALIC_INFO_BUTTON:
		child_view = mInfoBtn;
		break;
	case ALIC_PROFILE_BUTTON:
		child_view = mProfileBtn;
		break;
	default:
		LL_WARNS("AvatarItemReshape") << "Unexpected child view index is passed: " << child_view_index << LL_ENDL;
		// leave child_view untouched
	}
	
	return child_view;
}

// EOF
