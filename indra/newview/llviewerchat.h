/** 
 * @file llviewerchat.h
 * @brief wrapper of LLChat in viewer
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLVIEWERCHAT_H
#define LL_LLVIEWERCHAT_H

#include "llchat.h"
#include "llfontgl.h"
#include "v4color.h"


class LLViewerChat 
{
public:
	typedef boost::signals2::signal<void (LLFontGL*)> font_change_signal_t;

	static void getChatColor(const LLChat& chat, LLColor4& r_color);
	static void getChatColor(const LLChat& chat, std::string& r_color_name, F32& r_color_alpha);
	static LLFontGL* getChatFont();
	static S32 getChatFontSize();
	static void formatChatMsg(const LLChat& chat, std::string& formated_msg);
	static std::string getSenderSLURL(const LLChat& chat, const LLSD& args);

	static boost::signals2::connection setFontChangedCallback(const font_change_signal_t::slot_type& cb);
	static void signalChatFontChanged();

// [SL:KB] - Patch: Chat-GroupModerators | Checked: 2012-06-01 (Catznip-3.3)
	static U8 getChatNameFontStyle(EChatFlags chat_flags);
// [/SL:KB]

// [SL:KB] - Patch: Settings-Sounds | Checked: 2013-12-20 (Catznip-3.6)
	enum EChatEvent
	{
		SND_CHAT_AGENT = 0,  // An avatar said something in nearby chat
		SND_CONV_FRIEND,     // New friend conversation
		SND_CONV_NONFRIEND,  // New non-friend conversation
		SND_CONV_CONFERENCE, // New conference conversation
		SND_CONV_GROUP,      // New group conversation
		SND_IM_FRIEND,       // Incoming IM from a friend
		SND_IM_NONFRIEND,    // Incoming IM from a non-friend
		SND_IM_CONFERENCE,   // Incoming IM from a conference
		SND_IM_GROUP,        // Incoming IM from a group
		SND_COUNT,
		SND_NONE = -1
	};
	static LLUUID getUISoundFromChatEvent(EChatEvent eEvent);
	static LLUUID getUISoundFromSetting(const std::string& strSetting);
	static LLUUID getUISoundFromSettingsString(const std::string& strSetting);
// [/SL:KB]

private:
	static std::string getObjectImSLURL(const LLChat& chat, const LLSD& args);
	static font_change_signal_t sChatFontChangedSignal;

// [SL:KB] - Patch: Settings-Sounds | Checked: 2013-12-20 (Catznip-3.6)
	static std::string SOUND_LOOKUP_SETTINGS[SND_COUNT];
// [/SL:KB]
};

#endif
