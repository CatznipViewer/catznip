/** 
 * @file llnearbychatbar.h
 * @brief LLNearbyChatBar class definition
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

#ifndef LL_LLNEARBYCHATBAR_H
#define LL_LLNEARBYCHATBAR_H

#include "llfloater.h"
#include "llcombobox.h"
#include "llgesturemgr.h"
#include "llchat.h"
#include "llvoiceclient.h"
#include "lloutputmonitorctrl.h"
#include "llspeakers.h"
// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2011-08-20 (Catznip-3.2.0a)
class LLNearbyChat;
// [/SL:KB]

class LLNearbyChatBarListener;

class LLNearbyChatBar :	public LLFloater
{
	LOG_CLASS(LLNearbyChatBar);

public:
	// constructor for inline chat-bars (e.g. hosted in chat history window)
	LLNearbyChatBar(const LLSD& key);
	~LLNearbyChatBar() {}

	virtual BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	static LLNearbyChatBar* getInstance();

	LLLineEditor* getChatBox() { return mChatBox; }

	virtual void draw();

	std::string getCurrentChat();
	virtual BOOL handleKeyHere( KEY key, MASK mask );

	static void startChat(const char* line);
	static void stopChat();

	static void sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate);
	static void sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate);

	void showHistory();
	void showTranslationCheckbox(BOOL show);
//	/*virtual*/void setMinimized(BOOL b);
// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2011-11-17 (Catznip-3.2.0a) | Added: Catznip-3.2.0a
	/*virtual*/ BOOL canClose();
// [/SL:KB]

protected:
	static BOOL matchChatTypeTrigger(const std::string& in_str, std::string* out_str);
	static void onChatBoxKeystroke(LLLineEditor* caller, void* userdata);
	static void onChatBoxFocusLost(LLFocusableElement* caller, void* userdata);
	void onChatBoxFocusReceived();
// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2011-11-12 (Catznip-3.2.0a) | Added: Catznip-3.2.0a
	bool onNewNearbyChatMsg(const LLSD& sdEvent);
	void onTearOff(const LLSD& sdData);
// [/SL:KB]

	void sendChat( EChatType type );
	void onChatBoxCommit();
	void onChatFontChange(LLFontGL* fontp);

	/* virtual */ bool applyRectControl();

	void showNearbyChatPanel(bool show);
	void onToggleNearbyChatPanel();

	static LLWString stripChannelNumber(const LLWString &mesg, S32* channel);
	EChatType processChatTypeTriggers(EChatType type, std::string &str);

	void displaySpeakingIndicator();

	// Which non-zero channel did we last chat on?
	static S32 sLastSpecialChatChannel;

	LLLineEditor*			mChatBox;
//	LLView*					mNearbyChat;
	LLOutputMonitorCtrl*	mOutputMonitor;
	LLLocalSpeakerMgr*		mSpeakerMgr;
// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2011-10-26 (Catznip-3.2.0a) | Added: Catznip-3.2.0a
	LLPanel*			 mChatHistoryContainer;		// "nearby_chat_container" is the parent panel containing "nearby_chat"
	LLNearbyChat*		 mChatHistory;				// "nearby_chat"
// [/SL:KB]

	S32 mExpandedHeight;
// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2012-02-02 (Catznip-3.2.1) | Added: Catznip-3.2.1
	S32 mExpandedHeightMin;
// [/SL:KB]

	boost::shared_ptr<LLNearbyChatBarListener> mListener;
};

#endif
