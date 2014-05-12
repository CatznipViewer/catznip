/** 
* @file llpanelprofileview.cpp
* @brief Side tray "Profile View" panel
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

#include "llavatarconstants.h"
// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2013-08-26 (Catznip-3.6)
#include "llavataractions.h"
// [/SL:KB]
#include "llclipboard.h"
#include "llpanelprofileview.h"
#include "lltextbox.h"
#include "lltooldraganddrop.h"

static LLPanelInjector<LLPanelProfileView> t_panel_target_profile("panel_profile_view");

static const std::string PANEL_NOTES = "panel_notes";
static const std::string PANEL_PROFILE = "panel_profile";
static const std::string PANEL_PICKS = "panel_picks";

// ----------------------------------------------------------------------------
// AvatarStatusObserver helper class
//

class AvatarStatusObserver : public LLAvatarPropertiesObserver
{
public:
	AvatarStatusObserver(LLPanelProfileView* profile_view)
	{
		mProfileView = profile_view;
	}

// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2010-11-28 (Catznip-2.4)
	~AvatarStatusObserver()
	{
		if (mAvatarId.notNull())
			LLAvatarPropertiesProcessor::instance().removeObserver(mAvatarId, this);
	}
// [/SL:KB]

	void processProperties(void* data, EAvatarProcessorType type)
	{
		if (APT_PROPERTIES != type)
			return;

		const LLAvatarData* avatar_data = static_cast<const LLAvatarData*>(data);
		if ( (avatar_data) && (mProfileView->getAvatarId() == avatar_data->avatar_id) )
		{
			mProfileView->processOnlineStatus(avatar_data->flags & AVATAR_ONLINE);
//			LLAvatarPropertiesProcessor::instance().removeObserver(mProfileView->getAvatarId(), this);
		}

// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2010-11-28 (Catznip-2.4)
		// Profile view may have switched to a new avatar already so this needs to be outside the check above
		LLAvatarPropertiesProcessor::instance().removeObserver(mAvatarId, this);
		mAvatarId.setNull();
// [/SL:KB]
	}

	void subscribe()
	{
// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2010-11-28 (Catznip-2.4)
		if (mAvatarId.notNull())
			LLAvatarPropertiesProcessor::instance().removeObserver(mProfileView->getAvatarId(), this);
		mAvatarId = mProfileView->getAvatarId();
		LLAvatarPropertiesProcessor::instance().addObserver(mAvatarId, this);
// [/SL:KB]
//		LLAvatarPropertiesProcessor::instance().addObserver(mProfileView->getAvatarId(), this);
	}

private:
	LLPanelProfileView* mProfileView;
// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2010-11-28 (Catznip-2.4)
	LLUUID				mAvatarId;
// [/SL:KB]
};

// ----------------------------------------------------------------------------
// LLPanelProfileView class
//

LLPanelProfileView::LLPanelProfileView()
	: LLPanelProfile()
	, mStatusText(NULL)
	, mAvatarStatusObserver(NULL)
{
	mAvatarStatusObserver = new AvatarStatusObserver(this);
}

LLPanelProfileView::~LLPanelProfileView(void)
{
	delete mAvatarStatusObserver;
}

/*virtual*/ 
BOOL LLPanelProfileView::postBuild()
{
	LLPanelProfile::postBuild();

	getTabContainer()[PANEL_NOTES] = findChild<LLPanelAvatarNotes>(PANEL_NOTES);
	
	mStatusText = getChild<LLTextBox>("status");
	mStatusText->setVisible(false);

// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2013-08-26 (Catznip-3.6)
	getChild<LLUICtrl>("copy_to_clipboard")->setCommitCallback(boost::bind(&LLPanelProfileView::onCopyToClipboard, this, _1));
// [/SL:KB]
//	childSetCommitCallback("copy_to_clipboard",boost::bind(&LLPanelProfileView::onCopyToClipboard,this),NULL);

// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2010-09-08 (Catznip-2.1)
	LLFloater* pParentView = getParentByType<LLFloater>();
	if (pParentView)
		pParentView->setTitle(getLabel());
// [/SL:KB]

	return TRUE;
}

/*virtual*/ 
void LLPanelProfileView::onOpen(const LLSD& key)
{
	LLUUID id;
	if (key.has("id"))
	{
		id = key["id"];
	}

	if ( (id.notNull()) && (getAvatarId() != id) )
	{
		setAvatarId(id);

		// clear name fields, which might have old data
		getChild<LLUICtrl>("user_name")->setValue(LLSD());
		getChild<LLUICtrl>("user_slid")->setValue(LLSD());
	}

	// Update the avatar name.
	LLAvatarNameCache::get(getAvatarId(), boost::bind(&LLPanelProfileView::onAvatarNameCache, this, _1, _2));

	updateOnlineStatus();

	LLPanelProfile::onOpen(key);
}

BOOL LLPanelProfileView::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type,
                                           void *cargo_data, EAcceptance *accept, std::string& tooltip_msg)
{
// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2010-09-08 (Catznip-2.1)
	LLToolDragAndDrop::handleGiveDragAndDrop(getAvatarId(), LLUUID::null, drop, cargo_type, cargo_data, accept);
// [/SL:KB]
//	LLToolDragAndDrop::handleGiveDragAndDrop(getAvatarId(), gAgent.getSessionID(), drop, cargo_type, cargo_data, accept);

	return TRUE;
}

// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2013-08-26 (Catznip-3.6)
void LLPanelProfileView::onCopyToClipboard(LLUICtrl* ctrl)
{
	LLAvatarActions::copyToClipboard(getAvatarId(), ctrl->getValue());
}
// [/SL:KB]
//void LLPanelProfileView::onCopyToClipboard()
//{
//	std::string name = getChild<LLUICtrl>("user_name")->getValue().asString() + " (" + getChild<LLUICtrl>("user_slid")->getValue().asString() + ")";
//	LLClipboard::instance().copyToClipboard(utf8str_to_wstring(name), 0, name.length());
//}

bool LLPanelProfileView::isGrantedToSeeOnlineStatus()
{
	const LLRelationship* relationship = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	if (NULL == relationship)
		return false;

	// *NOTE: GRANT_ONLINE_STATUS is always set to false while changing any other status.
	// When avatar disallow me to see her online status processOfflineNotification Message is received by the viewer
	// see comments for ChangeUserRights template message. EXT-453.
	// If GRANT_ONLINE_STATUS flag is changed it will be applied when viewer restarts. EXT-3880
	return relationship->isRightGrantedFrom(LLRelationship::GRANT_ONLINE_STATUS);
}

// method was disabled according to EXT-2022. Re-enabled & improved according to EXT-3880
void LLPanelProfileView::updateOnlineStatus()
{
	// set text box visible to show online status for non-friends who has not set in Preferences
	// "Only Friends & Groups can see when I am online"
	mStatusText->setVisible(TRUE);

	const LLRelationship* relationship = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	if (NULL == relationship)
	{
		// this is non-friend avatar. Status will be updated from LLAvatarPropertiesProcessor.
		// in LLPanelProfileView::processOnlineStatus()

		// subscribe observer to get online status. Request will be sent by LLPanelAvatarProfile itself.
		// do not subscribe for friend avatar because online status can be wrong overridden
		// via LLAvatarData::flags if Preferences: "Only Friends & Groups can see when I am online" is set.
		mAvatarStatusObserver->subscribe();
		return;
	}
	// For friend let check if he allowed me to see his status

	// status should only show if viewer has permission to view online/offline. EXT-453, EXT-3880
	mStatusText->setVisible(isGrantedToSeeOnlineStatus());

	bool online = relationship->isOnline();
	processOnlineStatus(online);
}

void LLPanelProfileView::processOnlineStatus(bool online)
{
	std::string status = getString(online ? "status_online" : "status_offline");

	mStatusText->setValue(status);
}

void LLPanelProfileView::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	LLTextBox* agent_name = getChild<LLTextBox>("user_name");
	LLTextBox* agent_name_small = getChild<LLTextBox>("user_name_small");

	agent_name->setValue( av_name.getDisplayName() );
	agent_name_small->setValue( av_name.getDisplayName() );
	getChild<LLUICtrl>("user_slid")->setValue( av_name.getAccountName() );

	// Show smaller display name if too long to display in regular size
	if (agent_name->getTextPixelWidth() > agent_name->getRect().getWidth())
	{
		agent_name_small->setVisible(true);
		agent_name->setVisible(false);
	}
	else
	{
		agent_name_small->setVisible(false);
		agent_name->setVisible(true);
	}

	if (LLAvatarName::useDisplayNames())
	{
		getChild<LLUICtrl>("user_label")->setVisible( true );
		getChild<LLUICtrl>("user_slid")->setVisible( true );
		getChild<LLUICtrl>("display_name_label")->setVisible( true );
		getChild<LLUICtrl>("copy_to_clipboard")->setVisible( true );
		getChild<LLUICtrl>("copy_to_clipboard")->setEnabled( true );
		getChild<LLUICtrl>("solo_username_label")->setVisible( false );
	}
	else
	{
		getChild<LLUICtrl>("user_label")->setVisible( false );
		getChild<LLUICtrl>("user_slid")->setVisible( false );
		getChild<LLUICtrl>("display_name_label")->setVisible( false );
		getChild<LLUICtrl>("copy_to_clipboard")->setVisible( false );
		getChild<LLUICtrl>("copy_to_clipboard")->setEnabled( false );
		getChild<LLUICtrl>("solo_username_label")->setVisible( true );
	}

// [SL:KB] - Patch: UI-ProfileFloaters | Modified: 2010-11-07 (Catznip-2.3)
	LLFloater* pParentView = getParentByType<LLFloater>();
	if (pParentView)
		pParentView->setTitle(av_name.getCompleteName() + " - " + getLabel());
// [/SL:KB]
}

// ----------------------------------------------------------------------------
