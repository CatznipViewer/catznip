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

#include "llpanel.h"
#include "llcombobox.h"
#include "llgesturemgr.h"
#include "llchat.h"
#include "llvoiceclient.h"
#include "lloutputmonitorctrl.h"
#include "llspeakers.h"
#include "llbottomtray.h"


class LLGestureComboList
	: public LLGestureManagerObserver
	, public LLUICtrl
{
public:
	struct Params :	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLBottomtrayButton::Params>			combo_button;
		Optional<LLScrollListCtrl::Params>	combo_list;
		Optional<bool>						get_more,
											view_all;
		
		Params();
	};

protected:
	
	friend class LLUICtrlFactory;
	LLGestureComboList(const Params&);
	std::vector<LLMultiGesture*> mGestures;
	std::string mLabel;
	bool			mShowViewAll;
	bool			mShowGetMore;
	LLSD::Integer mViewAllItemIndex;
	LLSD::Integer mGetMoreItemIndex;

public:

	~LLGestureComboList();

	LLCtrlListInterface* getListInterface();
	virtual void	showList();
	virtual void	hideList();
	virtual BOOL	handleKeyHere(KEY key, MASK mask);

	virtual void	draw();

	S32				getCurrentIndex() const;
	void			onItemSelected(const LLSD& data);
	void			sortByName(bool ascending = true);
	void refreshGestures();
	void onCommitGesture();
	void onButtonCommit();
	virtual LLSD	getValue() const;

	// LLGestureManagerObserver trigger
	virtual void changed() { refreshGestures(); }

private:

	LLButton*			mButton;
	LLScrollListCtrl*	mList;
	S32                 mLastSelectedIndex;
};

class LLNearbyChatBar
:	public LLPanel
{
// [SL:KB] - Patch: Chat-NearbyToastWidth | Checked: 2010-11-10 (Catznip-2.5.0a) | Added: Catznip-2.4.0a
public:
	// Right now we only have two instances of the chatbar, but that might change if we ever embed one into the chat history floater as well
	typedef enum e_nearby_chatbar_type {
		CHATBAR_BOTTOMTRAY,			// Bottom tray chatbar
		CHATBAR_BOTTOMTRAY_LITE		// Bottom tray chatbar (mouselook)
	} EChatBarType;
// [/SL:KB]
public:
	// constructor for inline chat-bars (e.g. hosted in chat history window)
	LLNearbyChatBar();
//	~LLNearbyChatBar() {}
// [SL:KB] - Patch: Chat-NearbyToastWidth | Checked: 2010-11-10 (Catznip-2.5.0a) | Added: Catznip-2.4.0a
	~LLNearbyChatBar();
// [/SL:KB]

	virtual BOOL postBuild();

	static LLNearbyChatBar* getInstance();

	static bool instanceExists();

// [SL:KB] - Patch: Chat-NearbyToastWidth | Checked: 2010-11-10 (Catznip-2.5.0a) | Added: Catznip-2.4.0a
	static LLNearbyChatBar* getInstance(EChatBarType typeChatBar);
	static bool instanceExists(EChatBarType typeChatBar);

	// TODO-Catznip: find a better way to do this? The old way had calls embedded in LLBottomTray but that didn't seem very nice either
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	typedef boost::signals2::signal<void (LLUICtrl* ctrl, S32 width, S32 height)> reshape_signal_t;
	boost::signals2::connection setReshapeCallback(const reshape_signal_t::slot_type& cb);
// [/SL:KB]

	LLLineEditor* getChatBox() { return mChatBox; }

	virtual void draw();

	std::string getCurrentChat();
	virtual BOOL handleKeyHere( KEY key, MASK mask );

	static void startChat(const char* line);
	static void stopChat();

	static void sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate);
	static void sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate);

protected:
	static BOOL matchChatTypeTrigger(const std::string& in_str, std::string* out_str);
	static void onChatBoxKeystroke(LLLineEditor* caller, void* userdata);
	static void onChatBoxFocusLost(LLFocusableElement* caller, void* userdata);
	void onChatBoxFocusReceived();

	void sendChat( EChatType type );
	void onChatBoxCommit();

	static LLWString stripChannelNumber(const LLWString &mesg, S32* channel);
	EChatType processChatTypeTriggers(EChatType type, std::string &str);

	void displaySpeakingIndicator();

	// Which non-zero channel did we last chat on?
	static S32 sLastSpecialChatChannel;

	LLLineEditor*		mChatBox;
	LLOutputMonitorCtrl* mOutputMonitor;
	LLLocalSpeakerMgr*  mSpeakerMgr;

// [SL:KB] - Patch: Chat-NearbyToastWidth | Checked: 2010-11-10 (Catznip-2.5.0a) | Added: Catznip-2.4.0a
	reshape_signal_t*	mReshapeSignal;
// [/SL:KB]
};

#endif
