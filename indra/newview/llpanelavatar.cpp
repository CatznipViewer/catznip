/** 
 * @file llpanelavatar.cpp
 * @brief LLPanelAvatar and related class implementations
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llpanelavatar.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llavatarconstants.h"	// AVATAR_ONLINE
#include "llcallingcard.h"
#include "llcombobox.h"
#include "lldateutil.h"			// ageFromDate()
#include "llimview.h"
#include "llmenubutton.h"
#include "llnotificationsutil.h"
#include "llslurl.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltoggleablemenu.h"
#include "lltooldraganddrop.h"
#include "llscrollcontainer.h"
#include "llavatariconctrl.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llvoiceclient.h"
#include "lltextbox.h"
#include "lltrans.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLDropTarget : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<LLUUID> agent_id;
		Params()
		:	agent_id("agent_id")
		{
			changeDefault(mouse_opaque, false);
			changeDefault(follows.flags, FOLLOWS_ALL);
		}
	};

	LLDropTarget(const Params&);
	~LLDropTarget();

	void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	//
	// LLView functionality
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
	void setAgentID(const LLUUID &agent_id)		{ mAgentID = agent_id; }
protected:
	LLUUID mAgentID;
};

LLDropTarget::LLDropTarget(const LLDropTarget::Params& p) 
:	LLView(p),
	mAgentID(p.agent_id)
{}

LLDropTarget::~LLDropTarget()
{}

void LLDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "LLDropTarget::doDrop()" << llendl;
}

BOOL LLDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	if(getParent())
	{
		LLToolDragAndDrop::handleGiveDragAndDrop(mAgentID, LLUUID::null, drop,
												 cargo_type, cargo_data, accept);

		return TRUE;
	}

	return FALSE;
}

static LLDefaultChildRegistry::Register<LLDropTarget> r("drop_target");

// [SL:KB] - Patch: UI-ProfileFloaters (Legacy) | Checked: 2012-01-01 (Catznip-3.2)
static LLRegisterPanelClassWrapper<LLPanelAvatarProfile> t_panel_profile("panel_profile");
static LLRegisterPanelClassWrapper<LLPanelAvatarNotes> t_panel_notes("panel_notes");
// [/SL:KB]

// [SL:KB] - Patch: UI-ProfileFloaters (Legacy) | Checked: 2012-01-01 (Catznip-3.2)
//-----------------------------------------------------------------------------
// LLPanelAvatarNotes()
//-----------------------------------------------------------------------------
LLPanelAvatarNotes::LLPanelAvatarNotes()
	: LLPanelProfileTab()
{
}

void LLPanelAvatarNotes::updateData()
{
	LLAvatarPropertiesProcessor::getInstance()->sendAvatarNotesRequest(getAvatarId());
}

BOOL LLPanelAvatarNotes::postBuild()
{
	childSetCommitCallback("status_check", boost::bind(&LLPanelAvatarNotes::onCommitRights, this), NULL);
	childSetCommitCallback("map_check", boost::bind(&LLPanelAvatarNotes::onCommitRights, this), NULL);
	childSetCommitCallback("objects_check", boost::bind(&LLPanelAvatarNotes::onCommitRights, this), NULL);

	LLTextEditor* te = getChild<LLTextEditor>("notes_edit");
	te->setCommitCallback(boost::bind(&LLPanelAvatarNotes::onCommitNotes,this));
	te->setCommitOnFocusLost(TRUE);

	resetControls();
	resetData();

	LLVoiceClient::getInstance()->addObserver((LLVoiceClientStatusObserver*)this);

	return TRUE;
}

void LLPanelAvatarNotes::onOpen(const LLSD& key)
{
	LLPanelProfileTab::onOpen(key);

	fillRightsData();
}

void LLPanelAvatarNotes::fillRightsData()
{
	getChild<LLUICtrl>("status_check")->setValue(FALSE);
	getChild<LLUICtrl>("map_check")->setValue(FALSE);
	getChild<LLUICtrl>("objects_check")->setValue(FALSE);

	const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	// If true - we are viewing friend's profile, enable check boxes and set values.
	if(relation)
	{
		S32 rights = relation->getRightsGrantedTo();

		getChild<LLUICtrl>("status_check")->setValue(LLRelationship::GRANT_ONLINE_STATUS & rights ? TRUE : FALSE);
		getChild<LLUICtrl>("map_check")->setValue(LLRelationship::GRANT_MAP_LOCATION & rights ? TRUE : FALSE);
		getChild<LLUICtrl>("objects_check")->setValue(LLRelationship::GRANT_MODIFY_OBJECTS & rights ? TRUE : FALSE);

	}

	enableCheckboxes(NULL != relation);
}

void LLPanelAvatarNotes::onCommitNotes()
{
	std::string notes = getChild<LLUICtrl>("notes_edit")->getValue().asString();
	LLAvatarPropertiesProcessor::getInstance()-> sendNotes(getAvatarId(),notes);
}

void LLPanelAvatarNotes::rightsConfirmationCallback(const LLSD& notification,
		const LLSD& response, S32 rights)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(
				getAvatarId(), rights);
	}
	else
	{
		getChild<LLUICtrl>("objects_check")->setValue(
				getChild<LLUICtrl>("objects_check")->getValue().asBoolean() ? FALSE : TRUE);
	}
}

void LLPanelAvatarNotes::confirmModifyRights(bool grant, S32 rights)
{
	LLSD args;
	args["NAME"] = LLSLURL("agent", getAvatarId(), "displayname").getSLURLString();

	if (grant)
	{
		LLNotificationsUtil::add(
			"GrantModifyRights", args, LLSD(), boost::bind(&LLPanelAvatarNotes::rightsConfirmationCallback, this, _1, _2, rights));
	}
	else
	{
		LLNotificationsUtil::add(
			"RevokeModifyRights", args, LLSD(), boost::bind(&LLPanelAvatarNotes::rightsConfirmationCallback, this, _1, _2, rights));
	}
}

void LLPanelAvatarNotes::onCommitRights()
{
	const LLRelationship* buddy_relationship = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());

	if (NULL == buddy_relationship)
	{
		// Lets have a warning log message instead of having a crash. EXT-4947.
		llwarns << "Trying to modify rights for non-friend avatar. Skipped." << llendl;
		return;
	}

	S32 rights = 0;

	if(getChild<LLUICtrl>("status_check")->getValue().asBoolean())
		rights |= LLRelationship::GRANT_ONLINE_STATUS;
	if(getChild<LLUICtrl>("map_check")->getValue().asBoolean())
		rights |= LLRelationship::GRANT_MAP_LOCATION;
	if(getChild<LLUICtrl>("objects_check")->getValue().asBoolean())
		rights |= LLRelationship::GRANT_MODIFY_OBJECTS;

	bool allow_modify_objects = getChild<LLUICtrl>("objects_check")->getValue().asBoolean();

	// if modify objects checkbox clicked
	if (buddy_relationship->isRightGrantedTo(
			LLRelationship::GRANT_MODIFY_OBJECTS) != allow_modify_objects)
	{
		confirmModifyRights(allow_modify_objects, rights);
	}
	// only one checkbox can trigger commit, so store the rest of rights
	else
	{
		LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(getAvatarId(), rights);
	}
}

void LLPanelAvatarNotes::processProperties(void* data, EAvatarProcessorType type)
{
	if(APT_NOTES == type)
	{
		LLAvatarNotes* avatar_notes = static_cast<LLAvatarNotes*>(data);
		if(avatar_notes && getAvatarId() == avatar_notes->target_id)
		{
			getChild<LLUICtrl>("notes_edit")->setValue(avatar_notes->notes);
			getChildView("notes_edit")->setEnabled(true);

			LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
		}
	}
}

void LLPanelAvatarNotes::resetData()
{
	getChild<LLUICtrl>("notes_edit")->setValue(LLStringUtil::null);
	// Default value is TRUE
	getChild<LLUICtrl>("status_check")->setValue(TRUE);
}

void LLPanelAvatarNotes::resetControls()
{
	enableCheckboxes(false);
}

void LLPanelAvatarNotes::enableCheckboxes(bool enable)
{
	getChildView("status_check")->setEnabled(enable);
	getChildView("map_check")->setEnabled(enable);
	getChildView("objects_check")->setEnabled(enable);
}

LLPanelAvatarNotes::~LLPanelAvatarNotes()
{
	if(getAvatarId().notNull())
	{
		LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
	}

	if(LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->removeObserver((LLVoiceClientStatusObserver*)this);
	}
}

// virtual, called by LLAvatarTracker
void LLPanelAvatarNotes::changed(U32 mask)
{
	getChildView("teleport")->setEnabled(LLAvatarTracker::instance().isBuddyOnline(getAvatarId()));

	// update rights to avoid have checkboxes enabled when friendship is terminated. EXT-4947.
	fillRightsData();
}

// virtual
void LLPanelAvatarNotes::onChange(EStatusType status, const std::string &channelURI, bool proximal)
{
	if(status == STATUS_JOINING || status == STATUS_LEFT_CHANNEL)
	{
		return;
	}

	getChildView("call")->setEnabled(LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking());
}

void LLPanelAvatarNotes::setAvatarId(const LLUUID& id)
{
	if(id.notNull())
	{
		if(getAvatarId().notNull())
		{
			LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
		}
		LLPanelProfileTab::setAvatarId(id);
		LLAvatarTracker::instance().addParticularFriendObserver(getAvatarId(), this);
	}
}
// [/SL:KB]

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelProfileTab::LLPanelProfileTab()
: LLPanel()
, mAvatarId(LLUUID::null)
{
// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2013-08-26 (Catznip-3.6)
	mCommitCallbackRegistrar.add("Profile.AddFriend", boost::bind(&LLPanelProfileTab::onAvatarAction, this, static_cast<void (*)(const LLUUID&)>(&LLAvatarActions::requestFriendshipDialog)));
	mCommitCallbackRegistrar.add("Profile.RemoveFriend", boost::bind(&LLPanelProfileTab::onAvatarAction, this, LLAvatarActions::removeFriendDialog));
	mCommitCallbackRegistrar.add("Profile.IM", boost::bind(&LLPanelProfileTab::onAvatarAction, this, LLAvatarActions::startIM));
	mCommitCallbackRegistrar.add("Profile.Call", boost::bind(&LLPanelProfileTab::onAvatarAction, this, LLAvatarActions::startCall));
	mCommitCallbackRegistrar.add("Profile.OfferTeleport", boost::bind(&LLPanelProfileTab::onAvatarAction, this, static_cast<void (*)(const LLUUID&)>(&LLAvatarActions::offerTeleport)));
	mCommitCallbackRegistrar.add("Profile.ShowWebProfile", boost::bind(&LLPanelProfileTab::onAvatarAction, this, LLAvatarActions::showWebProfile));
	mCommitCallbackRegistrar.add("Profile.ShowOnMap", boost::bind(&LLPanelProfileTab::onAvatarAction, this, LLAvatarActions::showOnMap));
	mCommitCallbackRegistrar.add("Profile.Pay", boost::bind(&LLPanelProfileTab::onAvatarAction, this, LLAvatarActions::pay));
	mCommitCallbackRegistrar.add("Profile.Share", boost::bind(&LLPanelProfileTab::onAvatarAction, this, LLAvatarActions::share));
	mCommitCallbackRegistrar.add("Profile.BlockUnblock", boost::bind(&LLPanelProfileTab::onAvatarAction, this, LLAvatarActions::toggleBlock));

	mEnableCallbackRegistrar.add("Profile.EnableAddFriend", boost::bind(&LLPanelProfileTab::onEnableAddFriend, this));
	mEnableCallbackRegistrar.add("Profile.EnableRemoveFriend", boost::bind(&LLPanelProfileTab::onEnableRemoveFriend, this));
	mEnableCallbackRegistrar.add("Profile.EnableShowOnMap", boost::bind(&LLPanelProfileTab::onEnableShowOnMap, this));
	mEnableCallbackRegistrar.add("Profile.EnableBlock", boost::bind(&LLPanelProfileTab::onEnableBlock, this));
	mEnableCallbackRegistrar.add("Profile.EnableUnblock", boost::bind(&LLPanelProfileTab::onEnableUnblock, this));
// [/SL:KB]
}

LLPanelProfileTab::~LLPanelProfileTab()
{
	if(getAvatarId().notNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
	}
}

void LLPanelProfileTab::setAvatarId(const LLUUID& id)
{
	if(id.notNull())
	{
		if(getAvatarId().notNull())
		{
			LLAvatarPropertiesProcessor::getInstance()->removeObserver(mAvatarId,this);
		}
		mAvatarId = id;
		LLAvatarPropertiesProcessor::getInstance()->addObserver(getAvatarId(),this);
	}
}

void LLPanelProfileTab::onOpen(const LLSD& key)
{
	// Don't reset panel if we are opening it for same avatar.
	if(getAvatarId() != key.asUUID())
	{
		resetControls();
		resetData();

		scrollToTop();
	}

	// Update data even if we are viewing same avatar profile as some data might been changed.
	setAvatarId(key.asUUID());
	updateData();
	updateButtons();
}

void LLPanelProfileTab::scrollToTop()
{
	LLScrollContainer* scrollContainer = findChild<LLScrollContainer>("profile_scroll");
	if (scrollContainer)
		scrollContainer->goToTop();
}

void LLPanelProfileTab::onMapButtonClick()
{
	LLAvatarActions::showOnMap(getAvatarId());
}

void LLPanelProfileTab::updateButtons()
{
	bool is_buddy_online = LLAvatarTracker::instance().isBuddyOnline(getAvatarId());
	
	if(LLAvatarActions::isFriend(getAvatarId()))
	{
		getChildView("teleport")->setEnabled(is_buddy_online);
	}
	else
	{
		getChildView("teleport")->setEnabled(true);
	}
}

// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2013-08-26 (Catznip-3.6)
void LLPanelProfileTab::onAvatarAction(functor_t functor)
{
	functor(getAvatarId());
}

bool LLPanelProfileTab::onEnableAddFriend()
{
	return !LLAvatarActions::isFriend(getAvatarId());
}

bool LLPanelProfileTab::onEnableRemoveFriend()
{
	return LLAvatarActions::isFriend(getAvatarId());
}

bool LLPanelProfileTab::onEnableShowOnMap()
{
	bool is_buddy_online = LLAvatarTracker::instance().isBuddyOnline(getAvatarId());

	bool enable_map_btn = (is_buddy_online && is_agent_mappable(getAvatarId())) || gAgent.isGodlike();
	return enable_map_btn;
}

bool LLPanelProfileTab::onEnableBlock()
{
	return LLAvatarActions::canBlock(getAvatarId()) && !LLAvatarActions::isBlocked(getAvatarId());
}

bool LLPanelProfileTab::onEnableUnblock()
{
	return LLAvatarActions::isBlocked(getAvatarId());
}
// [/SL:KB]

// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2012-01-01 (Catznip-3.2)

// ----------------------------------------------------------------------------
// LLPanelAvatarProfile class
//

LLPanelAvatarProfile::LLPanelAvatarProfile()
	: LLPanelProfileTab()
{
}

BOOL LLPanelAvatarProfile::postBuild()
{
	LLVoiceClient::getInstance()->addObserver((LLVoiceClientStatusObserver*)this);

	resetControls();
	resetData();

	return TRUE;
}

void LLPanelAvatarProfile::onOpen(const LLSD& key)
{
	LLPanelProfileTab::onOpen(key);

	getChildView("call")->setEnabled(LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking());

	mGroups.clear();
}

void LLPanelAvatarProfile::updateData()
{
	if (getAvatarId().notNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesRequest(getAvatarId());
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarGroupsRequest(getAvatarId());
	}
}

void LLPanelAvatarProfile::resetControls()
{
	getChildView("profile_buttons_panel")->setVisible( true);
	getChildView("title_groups_text")->setVisible(true);
	getChildView("sl_groups")->setVisible(true);
}

void LLPanelAvatarProfile::resetData()
{
	mGroups.clear();
	getChild<LLUICtrl>("2nd_life_pic")->setValue(LLUUID::null);
	getChild<LLUICtrl>("real_world_pic")->setValue(LLUUID::null);
	getChild<LLUICtrl>("sl_description_edit")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("fl_description_edit")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("sl_groups")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("homepage_edit")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("register_date")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("acc_status_text")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("partner_text")->setValue(LLStringUtil::null);
}

void LLPanelAvatarProfile::processProperties(void* data, EAvatarProcessorType type)
{
	if (APT_PROPERTIES == type)
	{
		const LLAvatarData* avatar_data = static_cast<const LLAvatarData*>(data);
		if(avatar_data && getAvatarId() == avatar_data->avatar_id)
		{
			processProfileProperties(avatar_data);
		}
	}
	else if (APT_GROUPS == type)
	{
		LLAvatarGroups* avatar_groups = static_cast<LLAvatarGroups*>(data);
		if(avatar_groups && getAvatarId() == avatar_groups->avatar_id)
		{
			processGroupProperties(avatar_groups);
		}
	}
}

void LLPanelAvatarProfile::processProfileProperties(const LLAvatarData* avatar_data)
{
	fillCommonData(avatar_data);

	fillPartnerData(avatar_data);

	fillAccountStatus(avatar_data);
}

void LLPanelAvatarProfile::processGroupProperties(const LLAvatarGroups* avatar_groups)
{
	// *NOTE dzaporozhan
	// Group properties may arrive in two callbacks, we need to save them across
	// different calls. We can't do that in textbox as textbox may change the text.

	LLAvatarGroups::group_list_t::const_iterator it = avatar_groups->group_list.begin();
	const LLAvatarGroups::group_list_t::const_iterator it_end = avatar_groups->group_list.end();

	for(; it_end != it; ++it)
	{
		LLAvatarGroups::LLGroupData group_data = *it;
		mGroups[group_data.group_name] = group_data.group_id;
	}

	// Creating string, containing group list
	std::string groups = "";
	for (group_map_t::iterator it = mGroups.begin(); it != mGroups.end(); ++it)
	{
		if (it != mGroups.begin())
			groups += ", ";

		std::string group_name = LLURI::escape(it->first);
		std::string group_url= it->second.notNull()
				? "[secondlife:///app/group/" + it->second.asString() + "/about " + group_name + "]"
						: getString("no_group_text");

		groups += group_url;
	}

	getChild<LLUICtrl>("sl_groups")->setValue(groups);
}

void LLPanelAvatarProfile::fillCommonData(const LLAvatarData* avatar_data)
{
	//remove avatar id from cache to get fresh info
	LLAvatarIconIDCache::getInstance()->remove(avatar_data->avatar_id);

	LLStringUtil::format_map_t args;
	{
		std::string birth_date = LLTrans::getString("AvatarBirthDateFormat");
		LLStringUtil::format(birth_date, LLSD().with("datetime", (S32) avatar_data->born_on.secondsSinceEpoch()));
		args["[REG_DATE]"] = birth_date;
	}

	args["[AGE]"] = LLDateUtil::ageFromDate( avatar_data->born_on, LLDate::now());
	std::string register_date = getString("RegisterDateFormat", args);
	getChild<LLUICtrl>("register_date")->setValue(register_date );
	getChild<LLUICtrl>("sl_description_edit")->setValue(avatar_data->about_text);
	getChild<LLUICtrl>("fl_description_edit")->setValue(avatar_data->fl_about_text);
	getChild<LLUICtrl>("2nd_life_pic")->setValue(avatar_data->image_id);
	getChild<LLUICtrl>("real_world_pic")->setValue(avatar_data->fl_image_id);
	getChild<LLUICtrl>("homepage_edit")->setValue(avatar_data->profile_url);

	// Hide home page textbox if no page was set to fix "homepage URL appears clickable without URL - EXT-4734"
	getChildView("homepage_edit")->setVisible( !avatar_data->profile_url.empty());
}

void LLPanelAvatarProfile::fillPartnerData(const LLAvatarData* avatar_data)
{
	LLTextBox* partner_text = getChild<LLTextBox>("partner_text");
	if (avatar_data->partner_id.notNull())
	{
		partner_text->setText(LLSLURL("agent", avatar_data->partner_id, "inspect").getSLURLString());
	}
	else
	{
		partner_text->setText(getString("no_partner_text"));
	}
}

void LLPanelAvatarProfile::fillAccountStatus(const LLAvatarData* avatar_data)
{
	LLStringUtil::format_map_t args;
	args["[ACCTTYPE]"] = LLAvatarPropertiesProcessor::accountType(avatar_data);
	args["[PAYMENTINFO]"] = LLAvatarPropertiesProcessor::paymentInfo(avatar_data);
	// *NOTE: AVATAR_AGEVERIFIED not currently getting set in 
	// dataserver/lldataavatar.cpp for privacy considerations
	args["[AGEVERIFICATION]"] = "";
	std::string caption_text = getString("CaptionTextAcctInfo", args);
	getChild<LLUICtrl>("acc_status_text")->setValue(caption_text);
}

LLPanelAvatarProfile::~LLPanelAvatarProfile()
{
	if(getAvatarId().notNull())
	{
		LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
	}

	if(LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->removeObserver((LLVoiceClientStatusObserver*)this);
	}
}

// virtual, called by LLAvatarTracker
void LLPanelAvatarProfile::changed(U32 mask)
{
	getChildView("teleport")->setEnabled(LLAvatarTracker::instance().isBuddyOnline(getAvatarId()));
}

// virtual
void LLPanelAvatarProfile::onChange(EStatusType status, const std::string &channelURI, bool proximal)
{
	if ( (status == STATUS_JOINING) || (status == STATUS_LEFT_CHANNEL) )
	{
		return;
	}

	getChildView("call")->setEnabled(LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking());
}

void LLPanelAvatarProfile::setAvatarId(const LLUUID& id)
{
	if(id.notNull())
	{
		if(getAvatarId().notNull())
		{
			LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
		}
		LLPanelProfileTab::setAvatarId(id);
		LLAvatarTracker::instance().addParticularFriendObserver(getAvatarId(), this);
	}
}
// [/SL:KB]
