/** 
 * @file llparticipantlist.h
 * @brief LLParticipantList : model of a conversation session with added speaker events handling
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * Copyright (C) 2012-2017, Kitty Barnett
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

#ifndef LL_PARTICIPANTLIST_H
#define LL_PARTICIPANTLIST_H

#include "llviewerprecompiledheaders.h"
#include "llconversationmodel.h"

class LLSpeakerMgr;
class LLUICtrl;
class LLAvalineUpdater;
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
class LLAvatarList;
class LLListContextMenu;
// [/SL:KB]

//class LLParticipantList : public LLConversationItemSession
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
class LLParticipantList
// [/SL:KB]
{
	LOG_CLASS(LLParticipantList);
public:

	typedef boost::function<bool (const LLUUID& speaker_id)> validate_speaker_callback_t;

//	LLParticipantList(LLSpeakerMgr* data_source, LLFolderViewModelInterface& root_view_model);
//	~LLParticipantList();
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	LLParticipantList(LLSpeakerMgr* data_source);
	virtual ~LLParticipantList();
// [/SL:KB]

	/**
	 * Adds specified avatar ID to the existing list if it is not Agent's ID
	 *
	 * @param[in] avatar_id - Avatar UUID to be added into the list
	 */
	void addAvatarIDExceptAgent(const LLUUID& avatar_id);

// [SL:KB] - Patch: Chat-ParticipantList | Checked: Catznip-3.6)
	LLSpeakerMgr* getSpeakerManager() const { return mSpeakerMgr; }
// [/SL:KB]

	/**
	 * Refreshes the participant list.
	 */
// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	virtual void update();
// [/SL:KB]
//	void update();

	/**
	 * Set a callback to be called before adding a speaker. Invalid speakers will not be added.
	 *
	 * If the callback is unset all speakers are considered as valid.
	 *
	 * @see onAddItemEvent()
	 */
	void setValidateSpeakerCallback(validate_speaker_callback_t cb);

protected:
	/**
	 * LLSpeakerMgr event handlers
	 */
	bool onAddItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onRemoveItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onModeratorUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onSpeakerUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onSpeakerMuteEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
	std::set<LLUUID>& getModeratorList()         { return mModeratorList; }
	std::set<LLUUID>& getModeratorToRemoveList() { return mModeratorToRemoveList; }

	virtual const LLUUID& getSessionID() const = 0;

	virtual void addAvatarParticipant(const LLUUID& particpant_id) = 0;
	virtual void addAvalineParticipant(const LLUUID& particpant_id) = 0;
	virtual void clearParticipants() = 0;
	virtual bool isParticipant(const LLUUID& particpant_id) = 0;
	virtual void removeParticipant(const LLUUID& particpant_id) = 0;
	virtual void setParticipantIsMuted(const LLUUID& particpant_id, bool is_muted) = 0;
// [/SL:KB]

	/**
	 * List of listeners implementing LLOldEvents::LLSimpleListener.
	 * There is no way to handle all the events in one listener as LLSpeakerMgr registers
	 * listeners in such a way that one listener can handle only one type of event
	 **/
	class BaseSpeakerListener : public LLOldEvents::LLSimpleListener
	{
	public:
		BaseSpeakerListener(LLParticipantList& parent) : mParent(parent) {}
	protected:
		LLParticipantList& mParent;
	};

	class SpeakerAddListener : public BaseSpeakerListener
	{
	public:
		SpeakerAddListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

	class SpeakerRemoveListener : public BaseSpeakerListener
	{
	public:
		SpeakerRemoveListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

	class SpeakerClearListener : public BaseSpeakerListener
	{
	public:
		SpeakerClearListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

	class SpeakerUpdateListener : public BaseSpeakerListener
	{
	public:
		SpeakerUpdateListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
	
	class SpeakerModeratorUpdateListener : public BaseSpeakerListener
	{
	public:
		SpeakerModeratorUpdateListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
		
	class SpeakerMuteListener : public BaseSpeakerListener
	{
	public:
		SpeakerMuteListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}

		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

private:
	void onAvalineCallerFound(const LLUUID& participant_id);
	void onAvalineCallerRemoved(const LLUUID& participant_id);

	/**
	 * Adjusts passed participant to work properly.
	 *
	 * Adds SpeakerMuteListener to process moderation actions.
	 */
	void adjustParticipant(const LLUUID& speaker_id);

	LLSpeakerMgr*		mSpeakerMgr;

	std::set<LLUUID>	mModeratorList;
	std::set<LLUUID>	mModeratorToRemoveList;

	LLPointer<SpeakerAddListener>				mSpeakerAddListener;
	LLPointer<SpeakerRemoveListener>			mSpeakerRemoveListener;
	LLPointer<SpeakerClearListener>				mSpeakerClearListener;
	LLPointer<SpeakerUpdateListener>	        mSpeakerUpdateListener;
	LLPointer<SpeakerModeratorUpdateListener>	mSpeakerModeratorListener;
	LLPointer<SpeakerMuteListener>				mSpeakerMuteListener;

	validate_speaker_callback_t mValidateSpeakerCallback;
	LLAvalineUpdater* mAvalineUpdater;
};

// [SL:KB] - Patch: Chat-ParticipantList | Checked: 2013-11-21 (Catznip-3.6)
class LLParticipantModelList : public LLConversationItemSession, public LLParticipantList
{
	LOG_CLASS(LLParticipantModelList);
public:
	LLParticipantModelList(LLSpeakerMgr* data_source, LLFolderViewModelInterface& root_view_model);
	/*virtual*/ ~LLParticipantModelList();

public:
	// Bit of a hack here since in LL's viewer LLParticipantList::update() would override LLConversationItemSession::update()
	/*virtual*/ void update() { LLParticipantList::update(); }
protected:
	/*virtual*/ const LLUUID& getSessionID() const                   { return mUUID; }

	/*virtual*/ void addAvatarParticipant(const LLUUID& particpant_id);
	/*virtual*/ void addAvalineParticipant(const LLUUID& particpant_id);
	/*virtual*/ void clearParticipants()                            { LLConversationItemSession::clearParticipants(); }
	/*virtual*/ bool isParticipant(const LLUUID& particpant_id)     { return NULL != LLConversationItemSession::findParticipant(particpant_id); }
	/*virtual*/ void removeParticipant(const LLUUID& particpant_id) { LLConversationItemSession::removeParticipant(particpant_id); }
	/*virtual*/ void setParticipantIsMuted(const LLUUID& particpant_id, bool is_muted) { LLConversationItemSession::setParticipantIsMuted(particpant_id, is_muted); }
};

class LLParticipantAvatarList : public LLParticipantList
{
	LOG_CLASS(LLParticipantAvatarList);

	/*
	 * Constructor
	 */
public:
	LLParticipantAvatarList(LLSpeakerMgr* pDataSource, LLAvatarList* pAvatarList);
	~LLParticipantAvatarList() override;

	/*
	 * Base class overrides
	 */
protected:
	void addAvatarParticipant(const LLUUID& particpant_id) override;
	void addAvalineParticipant(const LLUUID& particpant_id) override;
	void clearParticipants() override;
	const LLUUID& getSessionID() const override;
	bool isParticipant(const LLUUID& particpant_id) override;
	void removeParticipant(const LLUUID& particpant_id) override;
	void setParticipantIsMuted(const LLUUID& particpant_id, bool is_muted) override;
	void update() override;

	/*
	 * Member functions
	 */
public:
	void getSelectedUUIDs(uuid_vec_t& idsSelected);

// [SL:KB] - Patch: Chat-ParticipantList | Checked: Catznip-3.6
public:
	enum ESortOrder
	{
		E_SORT_BY_NAME = 0,
		E_SORT_BY_RECENT_SPEAKERS = 1,
	};
	static ESortOrder getSortOrder();
	static void       setSortOrder(ESortOrder eSortOrder);
protected:
	void sort();
// [/SL:KB]

	/*
	 * Event handlers
	 */
public:
	void onAvatarListRefreshed();
	void onStartIM();

	/*
	 * Member variables
	 */
protected:
	LLAvatarList* m_pAvatarList;
// [SL:KB] - Patch: Chat-ParticipantList | Checked: Catznip-3.6
	LLListContextMenu* m_pContextMenu = nullptr;
	LLPointer<class LLAvatarItemStatusAndNameComparator> m_SortByStatusAndName;
	LLPointer<class LLAvatarItemRecentSpeakerComparator> m_SortByRecentSpeakers;
// [/SL:KB]

	boost::signals2::scoped_connection m_AvatarListRefreshConn;
// [SL:KB] - Patch: Chat-ParticipantList | Checked: Catznip-3.6
	boost::signals2::scoped_connection m_AvatarListSortOrderConn;
// [/SL:KB]
};
// [/SL:KB]

#endif // LL_PARTICIPANTLIST_H
