/** 
 * @file llpanelavatar.h
 * @brief LLPanelAvatar and related class definitions
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

#ifndef LL_LLPANELAVATAR_H
#define LL_LLPANELAVATAR_H

#include "llpanel.h"
#include "llavatarpropertiesprocessor.h"
#include "llcallingcard.h"
#include "llvoiceclient.h"
#include "llavatarnamecache.h"

class LLComboBox;
class LLLineEditor;

/**
* Base class for any Profile View.
*/
class LLPanelProfileTab
	: public LLPanel
	, public LLAvatarPropertiesObserver
{
public:

	/**
	 * Sets avatar ID, sets panel as observer of avatar related info replies from server.
	 */
	virtual void setAvatarId(const LLUUID& id);

	/**
	 * Returns avatar ID.
	 */
	virtual const LLUUID& getAvatarId() { return mAvatarId; }

	/**
	 * Sends update data request to server.
	 */
	virtual void updateData() = 0;

	/**
	 * Clears panel data if viewing avatar info for first time and sends update data request.
	 */
	virtual void onOpen(const LLSD& key);

	/**
	 * Profile tabs should close any opened panels here.
	 *
	 * Called from LLPanelProfile::onOpen() before opening new profile.
	 * See LLPanelPicks::onClosePanel for example. LLPanelPicks closes picture info panel
	 * before new profile is displayed, otherwise new profile will 
	 * be hidden behind picture info panel.
	 */
	virtual void onClosePanel() {}

	/**
	 * Resets controls visibility, state, etc.
	 */
	virtual void resetControls(){};

	/**
	 * Clears all data received from server.
	 */
	virtual void resetData(){};

	/*virtual*/ ~LLPanelProfileTab();

protected:

	LLPanelProfileTab();

	/**
	 * Scrolls panel to top when viewing avatar info for first time.
	 */
	void scrollToTop();

	virtual void onMapButtonClick();

	virtual void updateButtons();

// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2013-08-26 (Catznip-3.6)
	typedef boost::function<void (const LLUUID&)> functor_t;

	void onAvatarAction(functor_t functor);
	bool onEnableAddFriend();
	bool onEnableRemoveFriend();
	bool onEnableShowOnMap();
	bool onEnableBlock();
	bool onEnableUnblock();
// [/SL:KB]

private:

	LLUUID mAvatarId;
};

// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2012-01-01 (Catznip-3.2)
/**
 * Panel for displaying Avatar's first and second life related info.
 */
class LLPanelAvatarProfile
	: public LLPanelProfileTab
	, public LLFriendObserver
	, public LLVoiceClientStatusObserver
{
public:
	LLPanelAvatarProfile();
	/*virtual*/ ~LLPanelAvatarProfile();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	/**
	 * LLFriendObserver trigger
	 */
	virtual void changed(U32 mask);

	// Implements LLVoiceClientStatusObserver::onChange() to enable the call
	// button when voice is available
	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

	/*virtual*/ void setAvatarId(const LLUUID& id);

	/**
	 * Processes data received from server.
	 */
	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	/*virtual*/ void updateData();

	/*virtual*/ void resetControls();

	/*virtual*/ void resetData();

protected:
	/**
	 * Process profile related data received from server.
	 */
	virtual void processProfileProperties(const LLAvatarData* avatar_data);

	/**
	 * Processes group related data received from server.
	 */
	virtual void processGroupProperties(const LLAvatarGroups* avatar_groups);

	/**
	 * Fills common for Avatar profile and My Profile fields.
	 */
	virtual void fillCommonData(const LLAvatarData* avatar_data);

	/**
	 * Fills partner data.
	 */
	virtual void fillPartnerData(const LLAvatarData* avatar_data);

	/**
	 * Fills account status.
	 */
	virtual void fillAccountStatus(const LLAvatarData* avatar_data);

private:
	typedef std::map< std::string,LLUUID>	group_map_t;
	group_map_t 			mGroups;
};
// [/SL:KB]

// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2012-01-01 (Catznip-3.2)
/**
 * Panel for displaying Avatar's notes and modifying friend's rights.
 */
class LLPanelAvatarNotes 
	: public LLPanelProfileTab
	, public LLFriendObserver
	, public LLVoiceClientStatusObserver
{
public:
	LLPanelAvatarNotes();
	/*virtual*/ ~LLPanelAvatarNotes();

	virtual void setAvatarId(const LLUUID& id);

	/** 
	 * LLFriendObserver trigger
	 */
	virtual void changed(U32 mask);

	// Implements LLVoiceClientStatusObserver::onChange() to enable the call
	// button when voice is available
	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	/*virtual*/ void updateData();

protected:

	/*virtual*/ void resetControls();

	/*virtual*/ void resetData();

	/**
	 * Fills rights data for friends.
	 */
	void fillRightsData();

	void rightsConfirmationCallback(const LLSD& notification,
			const LLSD& response, S32 rights);
	void confirmModifyRights(bool grant, S32 rights);
	void onCommitRights();
	void onCommitNotes();

	void enableCheckboxes(bool enable);
};
// [/SL:KB]

#endif // LL_LLPANELAVATAR_H
