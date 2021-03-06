/** 
 * @file llfloaterimnearbychat.h
 * @brief LLFloaterIMNearbyChat class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * Copyright (C) 2010-2016, Kitty Barnett
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

#ifndef LL_LLFLOATERIMNEARBYCHAT_H
#define LL_LLFLOATERIMNEARBYCHAT_H

#include "llfloaterimsessiontab.h"
#include "llcombobox.h"
#include "llgesturemgr.h"
#include "llchat.h"
#include "llvoiceclient.h"
#include "lloutputmonitorctrl.h"
#include "llspeakers.h"
#include "llscrollbar.h"
#include "llviewerchat.h"
#include "llpanel.h"

class LLResizeBar;

class LLFloaterIMNearbyChat
	:	public LLFloaterIMSessionTab
{
public:
	// constructor for inline chat-bars (e.g. hosted in chat history window)
	LLFloaterIMNearbyChat(const LLSD& key = LLSD(LLUUID()));
// [SL:KB] - Patch: Chat-NearbyToastWidth | Checked: 2010-11-10 (Catznip-2.4)
	~LLFloaterIMNearbyChat();
// [/SL:KB]
//	~LLFloaterIMNearbyChat() {}

	static LLFloaterIMNearbyChat* buildFloater(const LLSD& key);

	/*virtual*/ BOOL postBuild();
//	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);
// [SL:KB] - Patch: Chat-BaseConversationsBtn | Checked: 2013-11-27 (Catznip-3.6)
	/*virtual*/ void setMinimized(BOOL b);
// [/SL:KB]
	/*virtual*/ void setVisible(BOOL visible);
	/*virtual*/ void setVisibleAndFrontmost(BOOL take_focus=TRUE, const LLSD& key = LLSD());
	/*virtual*/ void closeHostedFloater();

	void loadHistory();
    void reloadMessages(bool clean_messages = false);
	void removeScreenChat();

	void show();
//	bool isChatVisible() const;

	/** @param archive true - to save a message to the chat history log */
	void	addMessage			(const LLChat& message,bool archive = true, const LLSD &args = LLSD());
// [SL:KB] - Patch: Notification-Logging | Checked: 2012-07-03 (Catznip-3.3)
	void	logMessage			(const LLChat& message);	
// [/SL:KB]

//	LLChatEntry* getChatBox() { return mInputEditor; }

	std::string getCurrentChat();
	S32 getMessageArchiveLength() {return mMessageArchive.size();}

	virtual BOOL handleKeyHere( KEY key, MASK mask );

	static void startChat(const char* line);
	static void stopChat();

	static void sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate);
	static void sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate);

	static bool isWordsName(const std::string& name);

// [SL:KB] - Patch: Chat-NearbyToastWidth | Checked: 2010-11-10 (Catznip-2.4)
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	typedef boost::signals2::signal<void (LLUICtrl* ctrl, S32 width, S32 height)> reshape_signal_t;
	boost::signals2::connection setReshapeCallback(const reshape_signal_t::slot_type& cb);
// [/SL:KB]

	void showHistory();

protected:
	static BOOL matchChatTypeTrigger(const std::string& in_str, std::string* out_str);
	void onChatBoxKeystroke();
	void onChatBoxFocusLost();
	void onChatBoxFocusReceived();

	void sendChat( EChatType type );
	void onChatBoxCommit();
	void onChatFontChange(LLFontGL* fontp);

	/*virtual*/ void onTearOffClicked();
	/*virtual*/ void onClickCloseBtn(bool app_qutting = false);

	static LLWString stripChannelNumber(const LLWString &mesg, S32* channel);
	EChatType processChatTypeTriggers(EChatType type, std::string &str);

	void displaySpeakingIndicator();

	// Which non-zero channel did we last chat on?
	static S32 sLastSpecialChatChannel;

	LLOutputMonitorCtrl*	mOutputMonitor;
	LLLocalSpeakerMgr*		mSpeakerMgr;

// [SL:KB] - Patch: Chat-NearbyToastWidth | Checked: 2010-11-10 (Catznip-2.4)
	reshape_signal_t*		mReshapeSignal;
// [/SL:KB]
	S32 mExpandedHeight;

private:
	/*virtual*/ void refresh();

	std::vector<LLChat> mMessageArchive;
};

#endif // LL_LLFLOATERIMNEARBYCHAT_H
