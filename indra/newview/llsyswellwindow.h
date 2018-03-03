/** 
 * @file llsyswellwindow.h
 * @brief                                    // TODO
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLSYSWELLWINDOW_H
#define LL_LLSYSWELLWINDOW_H

#include "llimview.h"
#include "llnotifications.h"
#include "llscreenchannel.h"
#include "llsyswellitem.h"
#include "lltransientdockablefloater.h"
#include "llinitdestroyclass.h"

class LLAvatarName;
class LLChiclet;
class LLFlatListView;
class LLIMChiclet;
class LLScriptChiclet;
class LLSysWellChiclet;

class LLSysWellWindow : public LLTransientDockableFloater
{
public:
	LOG_CLASS(LLSysWellWindow);

    LLSysWellWindow(const LLSD& key);
    virtual ~LLSysWellWindow();
	BOOL postBuild();

	// other interface functions
	// check is window empty
	bool isWindowEmpty();

	// Operating with items
	void removeItemByID(const LLUUID& id);
	LLPanel * findItemByID(const LLUUID& id);

	// Operating with outfit
	virtual void setVisible(BOOL visible);
	void adjustWindowPosition();
	/*virtual*/ void	setDocked(bool docked, bool pop_on_undock = true);
	// override LLFloater's minimization according to EXT-1216
	/*virtual*/ void	setMinimized(BOOL minimize);
	/*virtual*/ void	handleReshape(const LLRect& rect, bool by_user);

	void onStartUpToastClick(S32 x, S32 y, MASK mask);

	void setSysWellChiclet(LLSysWellChiclet* chiclet);

	// size constants for the window and for its elements
	static const S32 MAX_WINDOW_HEIGHT		= 200;
	static const S32 MIN_WINDOW_WIDTH		= 318;

protected:
	// init Window's channel
	virtual void initChannel();

	const std::string NOTIFICATION_WELL_ANCHOR_NAME;
	const std::string IM_WELL_ANCHOR_NAME;
	virtual const std::string& getAnchorViewName() = 0;

	void reshapeWindow();
// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	void releaseNewMessagesState();
// [/SL:KB]

	// pointer to a corresponding channel's instance
	LLNotificationsUI::LLScreenChannel*	mChannel;
	LLFlatListView*	mMessageList;

	/**
	 * Reference to an appropriate Well chiclet to release "new message" state. EXT-3147
	 */
	LLSysWellChiclet* mSysWellChiclet;

	bool mIsReshapedByUser;
};

/**
 * Class intended to manage incoming messages in IM chats.
 * 
 * It contains a list list of all active IM sessions.
 */
//class LLIMWellWindow : public LLSysWellWindow, LLInitClass<LLIMWellWindow>
// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
class LLIMWellWindow : public LLSysWellWindow, LLInitClass<LLIMWellWindow>, LLIMSessionObserver
// [/SL:KB]
{
public:
	LLIMWellWindow(const LLSD& key);
	~LLIMWellWindow();

	static LLIMWellWindow* getInstance(const LLSD& key = LLSD());
	static LLIMWellWindow* findInstance(const LLSD& key = LLSD());
	static void initClass() { getInstance(); }

	/*virtual*/ BOOL postBuild();

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	// LLIMSessionObserver observe triggers
	/*virtual*/ void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id, BOOL has_offline_msg);
    /*virtual*/ void sessionActivated(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id) {}
	/*virtual*/ void sessionVoiceOrIMStarted(const LLUUID& session_id) {};
	/*virtual*/ void sessionRemoved(const LLUUID& session_id);
	/*virtual*/ void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id);
// [/SL:KB]

	void addObjectRow(const LLUUID& notification_id, bool new_message = false);
	void removeObjectRow(const LLUUID& notification_id);
	void closeAll();

protected:
	/*virtual*/ const std::string& getAnchorViewName() { return IM_WELL_ANCHOR_NAME; }

private:
// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	LLChiclet* findIMChiclet(const LLUUID& sessionId);
// [/SL:KB]
	LLChiclet* findObjectChiclet(const LLUUID& notification_id);

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	void addIMRow(const LLUUID& sessionId, S32 chicletCounter, const std::string& name, const LLUUID& otherParticipantId);
	void delIMRow(const LLUUID& sessionId);
// [/SL:KB]
	bool confirmCloseAll(const LLSD& notification, const LLSD& response);
	void closeAllImpl();

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	/**
	 * Scrolling row panel.
	 */
	class IMRowPanel: public LLPanel
	{
	public:
		IMRowPanel(const LLUUID& sessionId, S32 chicletCounter, const std::string& name, const LLUUID& otherParticipantId);
		/*virtual*/ ~IMRowPanel();
		void onMouseEnter(S32 x, S32 y, MASK mask);
		void onMouseLeave(S32 x, S32 y, MASK mask);
		BOOL handleMouseDown(S32 x, S32 y, MASK mask);
		BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

		void closeConversation();
	private:
		static const S32 CHICLET_HPAD = 10;
		void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);
		void onChicletSizeChanged(LLChiclet* ctrl, const LLSD& param);

	public:
		LLIMChiclet* mChiclet;
	private:
		LLButton*    mCloseBtn;
	};
// [/SL:KB]

	class ObjectRowPanel: public LLPanel
	{
	public:
		ObjectRowPanel(const LLUUID& notification_id, bool new_message = false);
		virtual ~ObjectRowPanel();
		/*virtual*/ void onMouseEnter(S32 x, S32 y, MASK mask);
		/*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);
		/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
		/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	private:
		void onClosePanel();
		void initChiclet(const LLUUID& notification_id, bool new_message = false);

	public:
		LLIMChiclet* mChiclet;
	private:
		LLButton*	mCloseBtn;
	};
};

#endif // LL_LLSYSWELLWINDOW_H



