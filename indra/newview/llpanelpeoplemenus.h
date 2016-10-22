/** 
 * @file llpanelpeoplemenus.h
 * @brief Menus used by the side tray "People" panel
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

#ifndef LL_LLPANELPEOPLEMENUS_H
#define LL_LLPANELPEOPLEMENUS_H

#include "lllistcontextmenu.h"

// [SL:KB] - Patch: Control-ParticipantList | Checked: 2014-03-02 (Catznip-3.6)
class LLParticipantList;
// [/SL:KB]

namespace LLPanelPeopleMenus
{

/**
 * Menu used in the people lists.
 */
class PeopleContextMenu : public LLListContextMenu
{
public:
// [RLVa:KB] - Checked: RLVa-1.5.0
	PeopleContextMenu() : m_fRlvCheck(false) {}
// [/RLVa:KB]
	/*virtual*/ LLContextMenu* createMenu();

protected:
	virtual void buildContextMenu(class LLMenuGL& menu, U32 flags);

private:
	bool enableContextMenuItem(const LLSD& userdata);
	bool checkContextMenuItem(const LLSD& userdata);
	bool enableFreezeEject(const LLSD& userdata);
	void offerTeleport();
//	void eject();
	void startConference();
	void requestTeleport();

// [RLVa:KB] - Checked: RLVa-1.5.0
protected:
	bool m_fRlvCheck;
// [/RLVa:KB]
};

/**
 * Menu used in the nearby people list.
 */
class NearbyPeopleContextMenu : public PeopleContextMenu
{
// [RLVa:KB] - Checked: RLVa-1.5.0
public:
	NearbyPeopleContextMenu() : PeopleContextMenu() { m_fRlvCheck = true; }
// [/RLVa:KB]
protected:
	/*virtual*/ void buildContextMenu(class LLMenuGL& menu, U32 flags);
};

/**
 * Menu used in the suggested friends list.
 */
class SuggestedFriendsContextMenu : public PeopleContextMenu
{
public:
	/*virtual*/ LLContextMenu * createMenu();

protected:
	/*virtual*/ void buildContextMenu(class LLMenuGL& menu, U32 flags);
};

// [SL:KB] - Patch: Control-ParticipantList | Checked: 2014-03-02 (Catznip-3.6)

/**
 * Menu used in the participant list
 */
class ParticipantContextMenu : public PeopleContextMenu
{
public:
	ParticipantContextMenu(LLParticipantList* pParticipantList);
	virtual ~ParticipantContextMenu() { }

public:
	/*virtual*/ LLContextMenu* createMenu();
protected:
	/*virtual*/ void buildContextMenu(class LLMenuGL& menu, U32 flags);

protected:
	bool canEjectAvatar(const LLUUID& idAgent) const;
	void ejectAvatar(const LLUUID& idAgent);
	bool hasBlockedText(const LLUUID& idAgent) const;
	bool hasBlockedVoice(const LLUUID& idAgent) const;
	bool isModerator() const;
	void toggleBlockText(const LLUUID& idAgent);
	void toggleBlockVoice(const LLUUID& idAgent);

protected:
	LLParticipantList* m_pParticipantList;
};

// [/SL:KB]

extern PeopleContextMenu gPeopleContextMenu;
extern NearbyPeopleContextMenu gNearbyPeopleContextMenu;
extern SuggestedFriendsContextMenu gSuggestedFriendsContextMenu;

} // namespace LLPanelPeopleMenus

#endif // LL_LLPANELPEOPLEMENUS_H
