/** 
 * @file llparticipantlist.cpp
 * @brief LLParticipantList : model of a conversation session with added speaker events handling
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

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
#include "llavatarlist.h"
#include "lltrans.h"
// [/SL:KB]
#include "llavatarnamecache.h"
#include "llerror.h"
#include "llimview.h"
#include "llfloaterimcontainer.h"
#include "llparticipantlist.h"
#include "llspeakers.h"
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2014-03-01 (Catznip-3.6)
#include "llpanelpeoplemenus.h"
#include "llviewercontrol.h"
// [/SL:KB]

//LLParticipantList retrieves add, clear and remove events and updates view accordingly 
#if LL_MSVC
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

// See EXT-4301.
/**
 * class LLAvalineUpdater - observe the list of voice participants in session and check
 *  presence of Avaline Callers among them.
 *
 * LLAvalineUpdater is a LLVoiceClientParticipantObserver. It provides two kinds of validation:
 *	- whether Avaline caller presence among participants;
 *	- whether watched Avaline caller still exists in voice channel.
 * Both validations have callbacks which will notify subscriber if any of event occur.
 *
 * @see findAvalineCaller()
 * @see checkIfAvalineCallersExist()
 */
class LLAvalineUpdater : public LLVoiceClientParticipantObserver
{
public:
	typedef boost::function<void(const LLUUID& speaker_id)> process_avaline_callback_t;

	LLAvalineUpdater(process_avaline_callback_t found_cb, process_avaline_callback_t removed_cb)
		: mAvalineFoundCallback(found_cb)
		, mAvalineRemovedCallback(removed_cb)
	{
		LLVoiceClient::getInstance()->addObserver(this);
	}
	~LLAvalineUpdater()
	{
		if (LLVoiceClient::instanceExists())
		{
			LLVoiceClient::getInstance()->removeObserver(this);
		}
	}

	/**
	 * Adds UUID of Avaline caller to watch.
	 *
	 * @see checkIfAvalineCallersExist().
	 */
	void watchAvalineCaller(const LLUUID& avaline_caller_id)
	{
		mAvalineCallers.insert(avaline_caller_id);
	}

	void onParticipantsChanged()
	{
		uuid_set_t participant_uuids;
		LLVoiceClient::getInstance()->getParticipantList(participant_uuids);


		// check whether Avaline caller exists among voice participants
		// and notify Participant List
		findAvalineCaller(participant_uuids);

		// check whether watched Avaline callers still present among voice participant
		// and remove if absents.
		checkIfAvalineCallersExist(participant_uuids);
	}

private:
	typedef std::set<LLUUID> uuid_set_t;

	/**
	 * Finds Avaline callers among voice participants and calls mAvalineFoundCallback.
	 *
	 * When Avatar is in group call with Avaline caller and then ends call Avaline caller stays
	 * in Group Chat floater (exists in LLSpeakerMgr). If Avatar starts call with that group again
	 * Avaline caller is added to voice channel AFTER Avatar is connected to group call.
	 * But Voice Control Panel (VCP) is filled from session LLSpeakerMgr and there is no information
	 * if a speaker is Avaline caller.
	 *
	 * In this case this speaker is created as avatar and will be recreated when it appears in
	 * Avatar's Voice session.
	 *
	 * @see LLParticipantList::onAvalineCallerFound()
	 */
	void findAvalineCaller(const uuid_set_t& participant_uuids)
	{
		uuid_set_t::const_iterator it = participant_uuids.begin(), it_end = participant_uuids.end();

		for(; it != it_end; ++it)
		{
			const LLUUID& participant_id = *it;
			if (!LLVoiceClient::getInstance()->isParticipantAvatar(participant_id))
			{
				LL_DEBUGS("Avaline") << "Avaline caller found among voice participants: " << participant_id << LL_ENDL;

				if (mAvalineFoundCallback)
				{
					mAvalineFoundCallback(participant_id);
				}
			}
		}
	}

	/**
	 * Finds Avaline callers which are not anymore among voice participants and calls mAvalineRemovedCallback.
	 *
	 * The problem is when Avaline caller ends a call it is removed from Voice Client session but
	 * still exists in LLSpeakerMgr. Server does not send such information.
	 * This method implements a HUCK to notify subscribers that watched Avaline callers by class
	 * are not anymore in the call.
	 *
	 * @see LLParticipantList::onAvalineCallerRemoved()
	 */
	void checkIfAvalineCallersExist(const uuid_set_t& participant_uuids)
	{
		uuid_set_t::iterator it = mAvalineCallers.begin();
		uuid_set_t::const_iterator participants_it_end = participant_uuids.end();

		while (it != mAvalineCallers.end())
		{
			const LLUUID participant_id = *it;
			LL_DEBUGS("Avaline") << "Check avaline caller: " << participant_id << LL_ENDL;
			bool not_found = participant_uuids.find(participant_id) == participants_it_end;
			if (not_found)
			{
				LL_DEBUGS("Avaline") << "Watched Avaline caller is not found among voice participants: " << participant_id << LL_ENDL;

				// notify Participant List
				if (mAvalineRemovedCallback)
				{
					mAvalineRemovedCallback(participant_id);
				}

				// remove from the watch list
				mAvalineCallers.erase(it++);
			}
			else
			{
				++it;
			}
		}
	}

	process_avaline_callback_t mAvalineFoundCallback;
	process_avaline_callback_t mAvalineRemovedCallback;

	uuid_set_t mAvalineCallers;
};

//LLParticipantList::LLParticipantList(LLSpeakerMgr* data_source, LLFolderViewModelInterface& root_view_model) :
//	LLConversationItemSession(data_source->getSessionID(), root_view_model),
//	mSpeakerMgr(data_source),
//	mValidateSpeakerCallback(NULL)
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
LLParticipantList::LLParticipantList(LLSpeakerMgr* data_source)
	: mSpeakerMgr(data_source)
	, mValidateSpeakerCallback(NULL)
// [/SL:KB]
{

	mAvalineUpdater = new LLAvalineUpdater(boost::bind(&LLParticipantList::onAvalineCallerFound, this, _1),
										   boost::bind(&LLParticipantList::onAvalineCallerRemoved, this, _1));

	mSpeakerAddListener = new SpeakerAddListener(*this);
	mSpeakerRemoveListener = new SpeakerRemoveListener(*this);
	mSpeakerClearListener = new SpeakerClearListener(*this);
	mSpeakerModeratorListener = new SpeakerModeratorUpdateListener(*this);
	mSpeakerUpdateListener = new SpeakerUpdateListener(*this);
	mSpeakerMuteListener = new SpeakerMuteListener(*this);

	mSpeakerMgr->addListener(mSpeakerAddListener, "add");
	mSpeakerMgr->addListener(mSpeakerRemoveListener, "remove");
	mSpeakerMgr->addListener(mSpeakerClearListener, "clear");
	mSpeakerMgr->addListener(mSpeakerModeratorListener, "update_moderator");
	mSpeakerMgr->addListener(mSpeakerUpdateListener, "update_speaker");

//	setSessionID(mSpeakerMgr->getSessionID());

//	NOTE-Catznip: addAvatarIDExceptAgent() calls two (pure) virtual functions which aren't available since at this point we have a LLParticipantList vtable pointer
//                so we moved it to its own function which will need to be called from derived class' constructor
//	//Lets fill avatarList with existing speakers
//	LLSpeakerMgr::speaker_list_t speaker_list;
//	mSpeakerMgr->getSpeakerList(&speaker_list, true);
//	for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
//	{
//		const LLPointer<LLSpeaker>& speakerp = *it;
//
//		addAvatarIDExceptAgent(speakerp->mID);
//		if ( speakerp->mIsModerator )
//		{
//			mModeratorList.insert(speakerp->mID);
//		}
//		else
//		{
//			mModeratorToRemoveList.insert(speakerp->mID);
//		}
//	}
	
//	// Identify and store what kind of session we are
//	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(data_source->getSessionID());
//	if (im_session)
//	{
//		// By default, sessions that can't be identified as group or ad-hoc will be considered P2P (i.e. 1 on 1)
//		mConvType = CONV_SESSION_1_ON_1;
//		if (im_session->isAdHocSessionType())
//		{
//			mConvType = CONV_SESSION_AD_HOC;
//		}
//		else if (im_session->isGroupSessionType())
//		{
//			mConvType = CONV_SESSION_GROUP;
//		}
//	}
//	else 
//	{
//		// That's the only session that doesn't get listed in the LLIMModel as a session...
//		mConvType = CONV_SESSION_NEARBY;
//	}
}

LLParticipantList::~LLParticipantList()
{
	delete mAvalineUpdater;
}

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2014-02-24(Catznip-3.6)
void LLParticipantList::initInitialSpeakers()
{
	//Lets fill avatarList with existing speakers
	LLSpeakerMgr::speaker_list_t speaker_list;
	mSpeakerMgr->getSpeakerList(&speaker_list, true);
	for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
	{
		const LLPointer<LLSpeaker>& speakerp = *it;

		addAvatarIDExceptAgent(speakerp->mID);
		if ( speakerp->mIsModerator )
		{
			mModeratorList.insert(speakerp->mID);
		}
		else
		{
			mModeratorToRemoveList.insert(speakerp->mID);
		}
	}
}
// [/SL:KB]

/*
  Seems this method is not necessary after onAvalineCallerRemoved was implemented;

  It does nothing because list item is always created with correct class type for Avaline caller.
  For now Avaline Caller is removed from the LLSpeakerMgr List when it is removed from the Voice Client
  session.
  This happens in two cases: if Avaline Caller ends call itself or if Resident ends group call.

  Probably Avaline caller should be removed from the LLSpeakerMgr list ONLY if it ends call itself.
  Asked in EXT-4301.
*/
void LLParticipantList::onAvalineCallerFound(const LLUUID& participant_id)
{
//	LLConversationItemParticipant* participant = findParticipant(participant_id);
//	if (participant)
//	{
//		removeParticipant(participant);
//	}
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	removeParticipant(participant_id);
// [/SL:KB]

	// re-add avaline caller with a correct class instance.
	addAvatarIDExceptAgent(participant_id);
}

void LLParticipantList::onAvalineCallerRemoved(const LLUUID& participant_id)
{
	LL_DEBUGS("Avaline") << "Removing avaline caller from the list: " << participant_id << LL_ENDL;

	mSpeakerMgr->removeAvalineSpeaker(participant_id);
}

void LLParticipantList::setValidateSpeakerCallback(validate_speaker_callback_t cb)
{
	mValidateSpeakerCallback = cb;
}

void LLParticipantList::update()
{
	mSpeakerMgr->update(true);
}

bool LLParticipantList::onAddItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLUUID uu_id = event->getValue().asUUID();

	if (mValidateSpeakerCallback && !mValidateSpeakerCallback(uu_id))
	{
		return true;
	}

	addAvatarIDExceptAgent(uu_id);
	return true;
}

bool LLParticipantList::onRemoveItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLUUID avatar_id = event->getValue().asUUID();
	removeParticipant(avatar_id);
	return true;
}

bool LLParticipantList::onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	clearParticipants();
	return true;
}

bool LLParticipantList::onSpeakerUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	const LLSD& evt_data = event->getValue();
	if ( evt_data.has("id") )
	{
		LLUUID participant_id = evt_data["id"];
//		LLFloaterIMContainer* im_box = LLFloaterIMContainer::findInstance();
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-25 (Catznip-3.5)
		LLFloaterIMContainerBase* im_box = LLFloaterIMContainerBase::findInstance();
// [/SL:KB]
		if (im_box)
		{
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
			im_box->setTimeNow(getSessionID(), participant_id);
// [/SL:KB]
//			im_box->setTimeNow(mUUID,participant_id);
		}
	}
	return true;
}

bool LLParticipantList::onModeratorUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	const LLSD& evt_data = event->getValue();
	if ( evt_data.has("id") && evt_data.has("is_moderator") )
	{
		LLUUID id = evt_data["id"];
		bool is_moderator = evt_data["is_moderator"];
		if ( id.notNull() )
		{
			if ( is_moderator )
				mModeratorList.insert(id);
			else
			{
				std::set<LLUUID>::iterator it = mModeratorList.find (id);
				if ( it != mModeratorList.end () )
				{
					mModeratorToRemoveList.insert(id);
					mModeratorList.erase(id);
				}
			}
			// *TODO : do we have to fire an event so that LLFloaterIMSessionTab::refreshConversation() gets called
		}
	}
	return true;
}

bool LLParticipantList::onSpeakerMuteEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLPointer<LLSpeaker> speakerp = (LLSpeaker*)event->getSource();
	if (speakerp.isNull()) return false;

	// update UI on confirmation of moderator mutes
	if (event->getValue().asString() == "voice")
	{
		setParticipantIsMuted(speakerp->mID, speakerp->mModeratorMutedVoice);
	}
	return true;
}

void LLParticipantList::addAvatarIDExceptAgent(const LLUUID& avatar_id)
{
	// Do not add if already in there, is the session id (hence not an avatar) or excluded for some reason
//	if (findParticipant(avatar_id) || (avatar_id == mUUID))
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	if ( (isParticipant(avatar_id)) || ((avatar_id == getSessionID())) )
// [/SL:KB]
	{
		return;
	}

	bool is_avatar = LLVoiceClient::getInstance()->isParticipantAvatar(avatar_id);

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	if (is_avatar)
	{
		addAvatarParticipant(avatar_id);
	}
	else
	{
		mAvalineUpdater->watchAvalineCaller(avatar_id);
		addAvalineParticipant(avatar_id);
	}
// [/SL:KB]
//	LLConversationItemParticipant* participant = NULL;
//	
//	if (is_avatar)
//	{
//		// Create a participant view model instance
//		LLAvatarName avatar_name;
//		bool has_name = LLAvatarNameCache::get(avatar_id, &avatar_name);
//		participant = new LLConversationItemParticipant(!has_name ? LLTrans::getString("AvatarNameWaiting") : avatar_name.getDisplayName() , avatar_id, mRootViewModel);
//		participant->fetchAvatarName();
//	}
//	else
//	{
//		std::string display_name = LLVoiceClient::getInstance()->getDisplayName(avatar_id);
//		// Create a participant view model instance
//		participant = new LLConversationItemParticipant(display_name.empty() ? LLTrans::getString("AvatarNameWaiting") : display_name, avatar_id, mRootViewModel);
//		mAvalineUpdater->watchAvalineCaller(avatar_id);
//	}
//
//	// *TODO : Need to update the online/offline status of the participant
//	// Hack for this: LLAvatarTracker::instance().isBuddyOnline(avatar_id))
//	
//	// Add the participant model to the session's children list
//	addParticipant(participant);

	adjustParticipant(avatar_id);
}

static LLFastTimer::DeclareTimer FTM_FOLDERVIEW_TEST("add test avatar agents");

void LLParticipantList::adjustParticipant(const LLUUID& speaker_id)
{
	LLPointer<LLSpeaker> speakerp = mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp.isNull()) return;

	// add listener to process moderation changes
	speakerp->addListener(mSpeakerMuteListener);
}

//
// LLParticipantList::SpeakerAddListener
//
bool LLParticipantList::SpeakerAddListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	/**
	 * We need to filter speaking objects. These objects shouldn't appear in the list.
	 * @see LLFloaterChat::addChat() in llviewermessage.cpp to get detailed call hierarchy
	 */
	const LLUUID& speaker_id = event->getValue().asUUID();
	LLPointer<LLSpeaker> speaker = mParent.mSpeakerMgr->findSpeaker(speaker_id);
	if (speaker.isNull() || (speaker->mType == LLSpeaker::SPEAKER_OBJECT))
	{
		return false;
	}
	return mParent.onAddItemEvent(event, userdata);
}

//
// LLParticipantList::SpeakerRemoveListener
//
bool LLParticipantList::SpeakerRemoveListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onRemoveItemEvent(event, userdata);
}

//
// LLParticipantList::SpeakerClearListener
//
bool LLParticipantList::SpeakerClearListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onClearListEvent(event, userdata);
}

//
// LLParticipantList::SpeakerUpdateListener
//
bool LLParticipantList::SpeakerUpdateListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onSpeakerUpdateEvent(event, userdata);
}

//
// LLParticipantList::SpeakerModeratorListener
//
bool LLParticipantList::SpeakerModeratorUpdateListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onModeratorUpdateEvent(event, userdata);
}

bool LLParticipantList::SpeakerMuteListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onSpeakerMuteEvent(event, userdata);
}

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)

//
// LLParticipantModelList class
//
LLParticipantModelList::LLParticipantModelList(LLSpeakerMgr* data_source, LLFolderViewModelInterface& root_view_model)
	: LLConversationItemSession(data_source->getSessionID(), root_view_model)
	, LLParticipantList(data_source)
{
	LLConversationItemSession::setSessionID(data_source->getSessionID());
	initInitialSpeakers();
	
	// Identify and store what kind of session we are
	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(data_source->getSessionID());
	if (im_session)
	{
		// By default, sessions that can't be identified as group or ad-hoc will be considered P2P (i.e. 1 on 1)
		mConvType = CONV_SESSION_1_ON_1;
		if (im_session->isAdHocSessionType())
		{
			mConvType = CONV_SESSION_AD_HOC;
		}
		else if (im_session->isGroupSessionType())
		{
			mConvType = CONV_SESSION_GROUP;
		}
	}
	else 
	{
		// That's the only session that doesn't get listed in the LLIMModel as a session...
		mConvType = CONV_SESSION_NEARBY;
	}
}

LLParticipantModelList::~LLParticipantModelList()
{
}

void LLParticipantModelList::addAvatarParticipant(const LLUUID& particpant_id)
{
	// Create a participant view model instance
	LLAvatarName avatar_name;
	bool has_name = LLAvatarNameCache::get(particpant_id, &avatar_name);
	LLConversationItemParticipant* participant =  new LLConversationItemParticipant(!has_name ? LLTrans::getString("AvatarNameWaiting") : avatar_name.getDisplayName() , particpant_id, mRootViewModel);
	participant->fetchAvatarName();

	LLConversationItemSession::addParticipant(participant);
}

void LLParticipantModelList::addAvalineParticipant(const LLUUID& particpant_id)
{
	// Create a participant view model instance
	std::string display_name = LLVoiceClient::getInstance()->getDisplayName(particpant_id);
	LLConversationItemParticipant* participant = new LLConversationItemParticipant(display_name.empty() ? LLTrans::getString("AvatarNameWaiting") : display_name, particpant_id, mRootViewModel);
	LLConversationItemSession::addParticipant(participant);
}

// [SL:KB] - Patch: Chat-GroupModerators | Checked: 2012-05-30 (Catznip-3.3)
//
// LLParticipantAvatarList helper classes
//

/**
 * Comparator for comparing avatar items by status and then name
 */
class LLAvatarItemStatusAndNameComparator : public LLAvatarItemNameComparator, public LLRefCount
{
	LOG_CLASS(LLAvatarItemStatusAndNameComparator);

public:
	LLAvatarItemStatusAndNameComparator(LLParticipantList& parent) : mParent(parent) { }
	virtual ~LLAvatarItemStatusAndNameComparator() { }

protected:
	/*virtual*/ bool doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const
	{
		const LLSpeakerMgr* pSpeakerMgr = mParent.getSpeakerManager();
		if (pSpeakerMgr)
		{
			LLPointer<LLSpeaker> lhs = pSpeakerMgr->findSpeaker(avatar_item1->getAvatarId());
			LLPointer<LLSpeaker> rhs = pSpeakerMgr->findSpeaker(avatar_item2->getAvatarId());
			if ( (lhs.notNull()) && (rhs.notNull()) )
			{
				// Sort moderators before non-moderators, then sort by name
				if (lhs->mIsModerator == rhs->mIsModerator)
					return LLAvatarItemNameComparator::doCompare(avatar_item1, avatar_item2);
				else if (lhs->mIsModerator)
					return true;
				else if (rhs->mIsModerator)
					return false;
			}
			else if (lhs.notNull())
			{
				// True if only avatar_item1 speaker info available
				return true;
			}
			else if (rhs.notNull())
			{
				// False if only avatar_item2 speaker info available
				return false;
			}
		}

		// By default compare by name.
		return LLAvatarItemNameComparator::doCompare(avatar_item1, avatar_item2);
	}

private:
	LLParticipantList& mParent;
};

// [/SL:KB]

//
// LLParticipantAvatarList
//

LLParticipantAvatarList::LLParticipantAvatarList(LLSpeakerMgr* data_source, LLAvatarList* pAvatarList)
	: LLParticipantList(data_source)
	, m_pAvatarList(pAvatarList)
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2014-03-01 (Catznip-3.6)
	, m_pContextMenu(NULL)
// [/SL:KB]
{
	initInitialSpeakers();

	m_pAvatarList->setNoItemsCommentText(LLTrans::getString("LoadingData"));
	m_pAvatarList->setSessionID(data_source->getSessionID());
	m_AvatarListRefreshConn = m_pAvatarList->setRefreshCompleteCallback(boost::bind(&LLParticipantAvatarList::onAvatarListRefreshed, this));

// [SL:KB] - Patch: Control-ParticipantList | Checked: 2012-06-10 (Catznip-3.3)
	m_AvatarListSortOrderConn = gSavedSettings.getControl("SpeakerParticipantDefaultOrder")->getSignal()->connect(boost::bind(&LLParticipantAvatarList::sort, this));
	sort();

	m_pContextMenu = new LLPanelPeopleMenus::ParticipantContextMenu(this);
	m_pAvatarList->setContextMenu(m_pContextMenu);
// [/SL:KB]
}

LLParticipantAvatarList::~LLParticipantAvatarList()
{
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2014-03-01 (Catznip-3.6)
	if (m_pContextMenu)
	{
		m_pAvatarList->setContextMenu(NULL);

		delete m_pContextMenu;
		m_pContextMenu = NULL;
	}
// [/SL:KB]
}

void LLParticipantAvatarList::update()
{
	LLParticipantList::update();

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2014-03-01 (Catznip-3.6)
	// Refresh the sort if the mouse isn't hovering over us
	if ( (E_SORT_BY_RECENT_SPEAKERS == getSortOrder()) && (m_pAvatarList) )
	{
		S32 x, y;
		LLUI::getMousePositionScreen(&x, &y);
		if (!m_pAvatarList->calcScreenRect().pointInRect(x, y))
		{
			sort();
		}
	}
// [/SL:KB]
}

void LLParticipantAvatarList::getSelectedUUIDs(uuid_vec_t& idsSelected)
{
	if (m_pAvatarList)
	{
		m_pAvatarList->getSelectedUUIDs(idsSelected);
	}
}

const LLUUID& LLParticipantAvatarList::getSessionID() const
{
	return m_pAvatarList->getSessionID();
}

void LLParticipantAvatarList::addAvatarParticipant(const LLUUID& particpant_id)
{
	m_pAvatarList->getIDs().push_back(particpant_id);
	m_pAvatarList->setDirty();

	//sort();
}

void LLParticipantAvatarList::addAvalineParticipant(const LLUUID& particpant_id)
{
	std::string display_name = LLVoiceClient::getInstance()->getDisplayName(particpant_id);
	m_pAvatarList->addAvalineItem(particpant_id, m_pAvatarList->getSessionID(), display_name.empty() ? LLTrans::getString("AvatarNameWaiting") : display_name);

	//sort();
}

void LLParticipantAvatarList::clearParticipants()
{
	m_pAvatarList->getIDs().clear();
	m_pAvatarList->setDirty();
}

bool LLParticipantAvatarList::isParticipant(const LLUUID& particpant_id)
{
	return m_pAvatarList->contains(particpant_id);
}

void LLParticipantAvatarList::removeParticipant(const LLUUID& particpant_id)
{
	uuid_vec_t& lParticipantIds = m_pAvatarList->getIDs();

	uuid_vec_t::iterator itParticipant = std::find(lParticipantIds.begin(), lParticipantIds.end(), particpant_id);
	if (lParticipantIds.end() != itParticipant)
	{
		lParticipantIds.erase(itParticipant);
		m_pAvatarList->setDirty();
	}
}

void LLParticipantAvatarList::setParticipantIsMuted(const LLUUID& particpant_id, bool is_muted)
{
}

void LLParticipantAvatarList::onAvatarListRefreshed()
{
	static const std::string s_strModIndicator(LLTrans::getString("IM_moderator_label")); 
	static const std::size_t s_lenModIndicator = s_strModIndicator.length();

	// Firstly remove moderators indicator
	std::set<LLUUID>& lModeratorsToRemove = getModeratorToRemoveList();
	for (std::set<LLUUID>::const_iterator itModRemove = lModeratorsToRemove.begin(); itModRemove != lModeratorsToRemove.end(); ++itModRemove)
	{
		LLAvatarListItem* pItem = dynamic_cast<LLAvatarListItem*>(m_pAvatarList->getItemByValue(*itModRemove));
		if (pItem)
		{
			std::string strName = pItem->getAvatarName();
			size_t idxModIndicator = strName.find(s_strModIndicator);
			if (std::string::npos != idxModIndicator)
			{
				strName.erase(idxModIndicator, s_lenModIndicator);
				pItem->setAvatarName(strName);
			}

			std::string strTooltip = pItem->getAvatarToolTip();
			idxModIndicator = strTooltip.find(s_strModIndicator);
			if (std::string::npos != idxModIndicator)
			{
				strTooltip.erase(idxModIndicator, s_lenModIndicator);
				pItem->setAvatarToolTip(strTooltip);
			}

// [SL:KB] - Patch: Chat-GroupModerators | Checked: 2012-05-30 (Catznip-3.3)
			pItem->setState(LLAvatarListItem::IS_ONLINE);
// [/SL:KB]
		}
	}
	lModeratorsToRemove.clear();

	// Add moderators indicator
	std::set<LLUUID>& lModerators = getModeratorList();
	for (std::set<LLUUID>::const_iterator itMod = lModerators.begin(); itMod != lModerators.end(); ++itMod)
	{
		LLAvatarListItem* pItem = dynamic_cast<LLAvatarListItem*>(m_pAvatarList->getItemByValue(*itMod));
		if (pItem)
		{
			std::string strName = pItem->getAvatarName();
			size_t idxModIndicator = strName.find(s_strModIndicator);
			if (std::string::npos == idxModIndicator)
			{
				strName += " ";
				strName += s_strModIndicator;
				pItem->setAvatarName(strName);
			}

			std::string strTooltip = pItem->getAvatarToolTip();
			idxModIndicator = strTooltip.find(s_strModIndicator);
			if (std::string::npos == idxModIndicator)
			{
				strTooltip += " ";
				strTooltip += s_strModIndicator;
				pItem->setAvatarToolTip(strTooltip);
			}

// [SL:KB] - Patch: Chat-GroupModerators | Checked: 2012-05-30 (Catznip-3.3)
			pItem->setState(LLAvatarListItem::IS_MODERATOR);
// [/SL:KB]
		}
	}
}
// [/SL:KB]

// [SL:KB] - Patch: Control-ParticipantList | Checked: 2012-06-10 (Catznip-3.3)

// static
LLParticipantAvatarList::ESortOrder LLParticipantAvatarList::getSortOrder()
{
	return (ESortOrder)gSavedSettings.getU32("SpeakerParticipantDefaultOrder");
}

// static 
void LLParticipantAvatarList::setSortOrder(ESortOrder eSortOrder)
{
	gSavedSettings.setU32("SpeakerParticipantDefaultOrder", (U32)eSortOrder);
}

void LLParticipantAvatarList::sort()
{
	if (!m_pAvatarList)
		return;

	switch (getSortOrder()) 
	{
		case E_SORT_BY_NAME :
			if (m_SortByStatusAndName.isNull())
				m_SortByStatusAndName = new LLAvatarItemStatusAndNameComparator(*this);
			m_pAvatarList->setComparator(m_SortByStatusAndName.get());
			m_pAvatarList->sort();
			break;
		case E_SORT_BY_RECENT_SPEAKERS:
			if (m_SortByRecentSpeakers.isNull())
				m_SortByRecentSpeakers = new LLAvatarItemRecentSpeakerComparator(getSpeakerManager());
			m_pAvatarList->setComparator(m_SortByRecentSpeakers.get());
			m_pAvatarList->sort();
			break;
		default:
			LL_WARNS() << "Unrecognized sort order for " << m_pAvatarList->getName() << LL_ENDL;
			return;
	}
}
// [/SL:KB]

//EOF
