/** 
 * @file llchiclet.h
 * @brief LLChiclet class header file
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

#ifndef LL_LLCHICLET_H
#define LL_LLCHICLET_H

#include "llavatariconctrl.h"
#include "llbutton.h"
// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
#include "llgroupmgr.h"
#include "llimview.h"
#include "lloutputmonitorctrl.h"
// [/SL:KB]
#include "llnotifications.h"
#include "lltextbox.h"

class LLMenuGL;
class LLFloaterIMSession;

/**
 * Class for displaying amount of messages/notifications(unread).
 */
class LLChicletNotificationCounterCtrl : public LLTextBox
{
public:

	struct Params :	public LLInitParam::Block<Params, LLTextBox::Params>
	{
		/**
		 * Contains maximum displayed count of unread messages. Default value is 9.
		 *
		 * If count is less than "max_unread_count" will be displayed as is.
		 * Otherwise 9+ will be shown (for default value).
		 */
		Optional<S32> max_displayed_count;

		Params();
	};

	/**
	 * Sets number of notifications
	 */
	virtual void setCounter(S32 counter);

	/**
	 * Returns number of notifications
	 */
	virtual S32 getCounter() const { return mCounter; }

	/**
	 * Returns width, required to display amount of notifications in text form.
	 * Width is the only valid value.
	 */
	/*virtual*/ LLRect getRequiredRect();

	/**
	 * Sets number of notifications using LLSD
	 */
	/*virtual*/ void setValue(const LLSD& value);

	/**
	 * Returns number of notifications wrapped in LLSD
	 */
	/*virtual*/ LLSD getValue() const;

protected:

	LLChicletNotificationCounterCtrl(const Params& p);
	friend class LLUICtrlFactory;

private:

	S32 mCounter;
	S32 mInitialWidth;
	S32 mMaxDisplayedCount;
};

/**
 * Class for displaying avatar's icon in P2P chiclet.
 */
class LLChicletAvatarIconCtrl : public LLAvatarIconCtrl
{
public:

	struct Params :	public LLInitParam::Block<Params, LLAvatarIconCtrl::Params>
	{
		Params()
		{
			changeDefault(draw_tooltip, FALSE);
			changeDefault(mouse_opaque, FALSE);
			changeDefault(default_icon_name, "Generic_Person");
		};
	};

protected:

	LLChicletAvatarIconCtrl(const Params& p);
	friend class LLUICtrlFactory;
};

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
/**
 * Class for displaying group's icon in Group chiclet.
 */
class LLChicletGroupIconCtrl : public LLIconCtrl
{
	friend class LLUICtrlFactory;
public:
	struct Params :	public LLInitParam::Block<Params, LLIconCtrl::Params>
	{
		Optional<std::string> default_icon;

		Params()
			: default_icon("default_icon", "Generic_Group")
		{}
	};

protected:
	LLChicletGroupIconCtrl(const Params& p);

public:
	/**
	 * Sets icon, if value is LLUUID::null - default icon will be set.
	 */
	/*virtual*/ void setValue(const LLSD& value);

private:
	std::string mDefaultIcon;
};
// [/SL:KB]

/**
 * Class for displaying icon in inventory offer chiclet.
 */
class LLChicletInvOfferIconCtrl : public LLChicletAvatarIconCtrl
{
public:

	struct Params :
		public LLInitParam::Block<Params, LLChicletAvatarIconCtrl::Params>
	{
		Optional<std::string> default_icon;

		Params()
		:	default_icon("default_icon", "Generic_Object_Small")
		{
			changeDefault(avatar_id, LLUUID::null);
		};
	};

	/**
	 * Sets icon, if value is LLUUID::null - default icon will be set.
	 */
	virtual void setValue(const LLSD& value );

protected:

	LLChicletInvOfferIconCtrl(const Params& p);
	friend class LLUICtrlFactory;

private:
	std::string mDefaultIcon;
};

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
/**
 * Class for displaying of speaker's voice indicator 
 */
class LLChicletSpeakerCtrl : public LLOutputMonitorCtrl
{
	friend class LLUICtrlFactory;
public:
	struct Params : public LLInitParam::Block<Params, LLOutputMonitorCtrl::Params>
	{
		Params(){};
	};

protected:
	LLChicletSpeakerCtrl(const Params&p);
};
// [/SL:KB]

/**
 * Base class for all chiclets.
 */
class LLChiclet : public LLUICtrl
{
public:

	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<bool> show_counter,
					   enable_counter;

		Params();
	};

	virtual ~LLChiclet() {}

	/**
	 * Associates chat session id with chiclet.
	 */
	virtual void setSessionId(const LLUUID& session_id) { mSessionId = session_id; }

	/**
	 * Returns associated chat session.
	 */
	virtual const LLUUID& getSessionId() const { return mSessionId; }

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	/*
	 * Sets number of unread notifications.
	 */
	virtual void setCounter(S32 counter) = 0;

	/**
	 * Returns number of unread notifications.
	 */
	virtual S32 getCounter() const = 0;
// [/SL:KB]

	/**
	 * Sets show counter state.
	 */
	virtual void setShowCounter(bool show) { mShowCounter = show; }

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	/**
	 * Returns show counter state.
	 */
	virtual bool getShowCounter() const { return mShowCounter; };
// [/SL:KB]

	/**
	 * Connects chiclet clicked event with callback.
	 */
	/*virtual*/ boost::signals2::connection setLeftButtonClickCallback(
		const commit_callback_t& cb);

	typedef boost::function<void (LLChiclet* ctrl, const LLSD& param)> 
		chiclet_size_changed_callback_t;

	/**
	 * Connects chiclets size changed event with callback.
	 */
	virtual boost::signals2::connection setChicletSizeChangedCallback(
		const chiclet_size_changed_callback_t& cb);

	/**
	 * Sets IM Session id using LLSD
	 */
	/*virtual*/ LLSD getValue() const;

	/**
	 * Returns IM Session id using LLSD
	 */
	/*virtual*/ void setValue(const LLSD& value);

protected:

	friend class LLUICtrlFactory;
	LLChiclet(const Params& p);

	/**
	 * Notifies subscribers about click on chiclet.
	 */
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);

	/**
	 * Notifies subscribers about chiclet size changed event.
	 */
	virtual void onChicletSizeChanged();

private:

	LLUUID mSessionId;

	bool mShowCounter;

	typedef boost::signals2::signal<void (LLChiclet* ctrl, const LLSD& param)> 
		chiclet_size_changed_signal_t;

	chiclet_size_changed_signal_t mChicletSizeChangedSignal;
};


/**
 * Base class for Instant Message chiclets.
 * IMChiclet displays icon, number of unread messages(optional)
 * and voice chat status(optional).
 */
class LLIMChiclet : public LLChiclet
{
public:
	enum EType {
		TYPE_UNKNOWN,
		TYPE_IM,
		TYPE_GROUP,
		TYPE_AD_HOC
	};
	struct Params : public LLInitParam::Block<Params, LLChiclet::Params>
	{};

	
// [SL:KB] - Patch: Chat-ChicletContextMenu | Checked: 2013-08-21 (Catznip-3.6)
	/*virtual*/ ~LLIMChiclet();
// [/SL:KB]
//	virtual ~LLIMChiclet() {};

	/**
	 * It is used for default setting up of chicklet:click handler, etc.  
	 */
	BOOL postBuild();

	/**
	 * Sets IM session name. This name will be displayed in chiclet tooltip.
	 */
	virtual void setIMSessionName(const std::string& name) { setToolTip(name); }

	/**
	 * Sets id of person/group user is chatting with.
	 * Session id should be set before calling this
	 */
	virtual void setOtherParticipantId(const LLUUID& other_participant_id) { mOtherParticipantId = other_participant_id; }

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	/**
	 * Gets id of person/group user is chatting with.
	 */
	virtual const LLUUID& getOtherParticipantId() const { return mOtherParticipantId; }

	/**
	 * Init Speaker Control with speaker's ID
	 */
	virtual void initSpeakerControl();

	/**
	 * set status (Shows/Hide) for voice control.
	 */
	virtual void setShowSpeaker(bool show);

	/**
	 * Returns voice chat status control visibility.
	 */
	virtual bool getShowSpeaker() { return mShowSpeaker; }

	/**
	 * Shows/Hides for voice control for a chiclet.
	 */
	virtual void toggleSpeakerControl();

	/**
	* Sets number of unread messages. Will update chiclet's width if number text 
	* exceeds size of counter and notify it's parent about size change.
	*/
	/*virtual*/ void setCounter(S32);
// [/SL:KB]

	/**
	* Enables/disables the counter control for a chiclet.
	*/
	virtual void enableCounterControl(bool enable);

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	/**
	* Sets show counter state.
	*/
	virtual void setShowCounter(bool show);

	/**
	* Shows/Hides for counter control for a chiclet.
	*/
	virtual void toggleCounterControl();
// [/SL:KB]

	/**
	* Sets required width for a chiclet according to visible controls.
	*/
	virtual void setRequiredWidth();

	/**
	 * Shows/hides overlay icon concerning new unread messages.
	 */
	virtual void setShowNewMessagesIcon(bool show);

	/**
	 * Returns visibility of overlay icon concerning new unread messages.
	 */
	virtual bool getShowNewMessagesIcon();

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	/**
	 * Determine whether given ID refers to a group or an IM chat session.
	 * 
	 * This is used when we need to chose what IM chiclet (P2P/group)
	 * class to instantiate.
	 * 
	 * @param session_id session ID.
	 * @return TYPE_GROUP in case of group chat session,
	 *         TYPE_IM in case of P2P session,
	 *         TYPE_UNKNOWN otherwise.
	 */
	static EType getIMSessionType(const LLUUID& session_id);
// [/SL:KB]

	/**
	 * The action taken on mouse down event.
	 * 
	 * Made public so that it can be triggered from outside
	 * (more specifically, from the Active IM window).
	 */
	virtual void onMouseDown();

	virtual void setToggleState(bool toggle);

	/**
	 * Displays popup menu.
	 */
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	void hidePopupMenu();

protected:

	LLIMChiclet(const LLIMChiclet::Params& p);

protected:

	/**
	 * Creates chiclet popup menu.
	 */
	virtual void createPopupMenu() = 0;

	/** 
	 * Enables/disables menus.
	 */
	virtual void updateMenuItems() {};

	bool canCreateMenu();

// [SL:KB] - Patch: Chat-ChicletContextMenu | Checked: 2013-08-21 (Catznip-3.6)
	LLHandle<LLContextMenu> mContextMenuHandle;
// [/SL:KB]
//	LLMenuGL* mPopupMenu;

	bool mShowSpeaker;
	bool mCounterEnabled;
	/* initial width of chiclet, should not include counter or speaker width */
	S32 mDefaultWidth;

	LLIconCtrl* mNewMessagesIcon;
	LLButton* mChicletButton;
// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	LLChicletNotificationCounterCtrl* mCounterCtrl;
	LLChicletSpeakerCtrl* mSpeakerCtrl;
// [/SL:KB]

	/** the id of another participant, either an avatar id or a group id*/
	LLUUID mOtherParticipantId;

	template<typename Container>
	struct CollectChicletCombiner {
		typedef Container result_type;

		template<typename InputIterator>
		Container operator()(InputIterator first, InputIterator last) const {
			Container c = Container();
			for (InputIterator iter = first; iter != last; iter++) {
				if (*iter != NULL) {
					c.push_back(*iter);
				}
			}
			return c;
		}
	};

public:
	static boost::signals2::signal<LLChiclet* (const LLUUID&),
			CollectChicletCombiner<std::list<LLChiclet*> > >
			sFindChicletsSignal;
};

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
/**
 * Implements P2P chiclet
 */
class LLIMP2PChiclet : public LLIMChiclet
{
	friend class LLUICtrlFactory;
public:
	struct Params : public LLInitParam::Block<Params, LLIMChiclet::Params>
	{
		Optional<LLButton::Params> chiclet_button;

		Optional<LLChicletAvatarIconCtrl::Params> avatar_icon;

		Optional<LLChicletNotificationCounterCtrl::Params> unread_notifications;

		Optional<LLChicletSpeakerCtrl::Params> speaker;

		Optional<LLIconCtrl::Params> new_message_icon;

		Optional<bool> show_speaker;

		Params();
	};

protected:
	LLIMP2PChiclet(const Params& p);
public:
	virtual ~LLIMP2PChiclet() {}

	/*
	 * LLIMChiclet overrides
	 */
public:
	/*virtual*/ void createPopupMenu();
	/*virtual*/ S32 getCounter() const { return mCounterCtrl->getCounter(); }
	/*virtual*/ void initSpeakerControl();
	/*virtual*/ void setOtherParticipantId(const LLUUID& other_participant_id);

	/*
	 * Event handlers
	 */
protected:
	void onMenuItemClicked(const LLSD& sdParam);
	bool onMenuItemEnable(const LLSD& sdParam);

	/*
	 * Member variables
	 */
private:
	LLChicletAvatarIconCtrl* mChicletIconCtrl;
};
// [/SL:KB]

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
/**
 * Implements ad-hoc chiclet.
 */
class LLAdHocChiclet : public LLIMChiclet
{
	friend class LLUICtrlFactory;
public:
	struct Params : public LLInitParam::Block<Params, LLIMChiclet::Params>
	{
		Optional<LLButton::Params> chiclet_button;

		Optional<LLChicletAvatarIconCtrl::Params> avatar_icon;

		Optional<LLChicletNotificationCounterCtrl::Params> unread_notifications;

		Optional<LLChicletSpeakerCtrl::Params> speaker;

		Optional<LLIconCtrl::Params> new_message_icon;

		Optional<bool>	show_speaker;

		Optional<LLColor4>	avatar_icon_color;

		Params();
	};

protected:
	LLAdHocChiclet(const Params& p);

public:
	/**
	 * Sets session id.
	 * Session ID for group chat is actually Group ID.
	 */
	/*virtual*/ void setSessionId(const LLUUID& session_id);

	/**
	 * Keep Speaker Control with actual speaker's ID
	 */
	/*virtual*/ void draw();

	/**
	 * Init Speaker Control with speaker's ID
	 */
	/*virtual*/ void initSpeakerControl();

	/**
	 * Returns number of unread messages.
	 */
	/*virtual*/ S32 getCounter() const { return mCounterCtrl->getCounter(); }

	/**
	 * Creates chiclet popup menu. Will create AdHoc Chat menu 
	 * based on other participant's id.
	 */
	virtual void createPopupMenu();

	/**
	 * Processes clicks on chiclet popup menu.
	 */
	virtual void onMenuItemClicked(const LLSD& user_data);

	/**
	 * Finds a current speaker and resets the SpeakerControl with speaker's ID
	 */
	/*virtual*/ void switchToCurrentSpeaker();

private:
	LLChicletAvatarIconCtrl* mChicletIconCtrl;
};
// [/SL:KB]

/**
 * Chiclet for script floaters.
 */
class LLScriptChiclet : public LLIMChiclet
{
public:

	struct Params : public LLInitParam::Block<Params, LLIMChiclet::Params>
	{
		Optional<LLButton::Params> chiclet_button;

		Optional<LLIconCtrl::Params> icon;

		Optional<LLIconCtrl::Params> new_message_icon;

		Params();
	};

	/*virtual*/ void setSessionId(const LLUUID& session_id);

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	/*virtual*/ void setCounter(S32 counter);

	/*virtual*/ S32 getCounter() const { return 0; }
// [/SL:KB]

	/**
	 * Toggle script floater
	 */
	/*virtual*/ void onMouseDown();

protected:

	LLScriptChiclet(const Params&);
	friend class LLUICtrlFactory;

	/**
	 * Creates chiclet popup menu.
	 */
	virtual void createPopupMenu();

// [SL:KB] - Patch: Notification-ScriptDialogBlock | Checked: 2011-11-22 (Catznip-3.2)
	/**
	 * Enables chiclet menu items.
	 */
	static bool enableMenuItem(const LLSD& user_data, const LLUUID& idSession);
// [/SL:KB]

	/**
	 * Processes clicks on chiclet popup menu.
	 */
	virtual void onMenuItemClicked(const LLSD& user_data);

private:

	LLIconCtrl* mChicletIconCtrl;
};

/**
 * Chiclet for inventory offer script floaters.
 */
class LLInvOfferChiclet: public LLIMChiclet
{
public:

	struct Params : public LLInitParam::Block<Params, LLIMChiclet::Params>
	{
		Optional<LLButton::Params> chiclet_button;

		Optional<LLChicletInvOfferIconCtrl::Params> icon;

		Optional<LLIconCtrl::Params> new_message_icon;

		Params();
	};

	/*virtual*/ void setSessionId(const LLUUID& session_id);

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	/*virtual*/ void setCounter(S32 counter);

	/*virtual*/ S32 getCounter() const { return 0; }
// [/SL:KB]

	/**
	 * Toggle script floater
	 */
	/*virtual*/ void onMouseDown();

protected:
	LLInvOfferChiclet(const Params&);
	friend class LLUICtrlFactory;

	/**
	 * Creates chiclet popup menu.
	 */
	virtual void createPopupMenu();

// [SL:KB] - Patch: Notification-ScriptDialogBlock | Checked: 2011-11-22 (Catznip-3.2)
	/**
	 * Enables chiclet menu items.
	 */
	static bool enableMenuItem(const LLSD& user_data, bool fIsTaskOffer, const LLUUID& idSession);
// [/SL:KB]

	/**
	 * Processes clicks on chiclet popup menu.
	 */
	virtual void onMenuItemClicked(const LLSD& user_data);

private:
// [SL:KB] - Patch: Notification-ScriptDialogBlock | Checked: 2011-11-22 (Catznip-3.2)
	bool mIsTaskOffer;
// [/SL:KB]
	LLChicletInvOfferIconCtrl* mChicletIconCtrl;
};

/**
 * Implements notification chiclet. Used to display total amount of unread messages 
 * across all IM sessions, total amount of system notifications. See EXT-3147 for details
 */
class LLSysWellChiclet : public LLChiclet
{
public:
		
	struct Params : public LLInitParam::Block<Params, LLChiclet::Params>
	{
		Optional<LLButton::Params> button;

		Optional<LLChicletNotificationCounterCtrl::Params> unread_notifications;

		/**
		 * Contains maximum displayed count of unread messages. Default value is 9.
		 *
		 * If count is less than "max_unread_count" will be displayed as is.
		 * Otherwise 9+ will be shown (for default value).
		 */
		Optional<S32> max_displayed_count;

		Params();
	};

	/*virtual*/ void setCounter(S32 counter);

	// *TODO: mantipov: seems getCounter is not necessary for LLNotificationChiclet
	// but inherited interface requires it to implement. 
	// Probably it can be safe removed.
// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	/*virtual*/ S32 getCounter() const { return mCounter; }
// [/SL:KB]
//	/*virtual*/S32 getCounter() { return mCounter; }

	boost::signals2::connection setClickCallback(const commit_callback_t& cb);

	/*virtual*/ ~LLSysWellChiclet();

	void setToggleState(BOOL toggled);

	void setNewMessagesState(bool new_messages);
	//this method should change a widget according to state of the SysWellWindow 
	virtual void updateWidget(bool is_window_empty);

protected:

	LLSysWellChiclet(const Params& p);
	friend class LLUICtrlFactory;

	/**
	 * Change Well 'Lit' state from 'Lit' to 'Unlit' and vice-versa.
	 *
	 * There is an assumption that it will be called 2*N times to do not change its start state.
	 * @see FlashToLitTimer
	 */
	void changeLitState(bool blink);

	/**
	 * Displays menu.
	 */
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	virtual void createMenu() = 0;

protected:
	class FlashToLitTimer;
	LLButton* mButton;
	S32 mCounter;
	S32 mMaxDisplayedCount;
	bool mIsNewMessagesState;

	LLFlashTimer* mFlashToLitTimer;
// [SL:KB] - Patch: Chat-ChicletContextMenu | Checked: 2013-08-21 (Catznip-3.6)
	LLHandle<LLContextMenu> mContextMenuHandle;
// [/SL:KB]
//	LLContextMenu* mContextMenu;
};

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
/**
 * Class represented a chiclet for IM Well Icon.
 *
 * It displays a count of unread messages from other participants in all IM sessions.
 */
class LLIMWellChiclet : public LLSysWellChiclet, LLIMSessionObserver
{
	friend class LLUICtrlFactory;
protected:
	LLIMWellChiclet(const Params& p);
public:
	~LLIMWellChiclet();

	// LLIMSessionObserver observe triggers
	/*virtual*/ void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id, BOOL has_offline_msg) {}
	/*virtual*/ void sessionActivated(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id) {}
	/*virtual*/ void sessionVoiceOrIMStarted(const LLUUID& session_id) {};
	/*virtual*/ void sessionRemoved(const LLUUID& session_id) { messageCountChanged(LLSD()); }
	/*virtual*/ void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id) {}

	/**
	 * Processes clicks on chiclet popup menu.
	 */
	virtual void onMenuItemClicked(const LLSD& user_data);

	/**
	 * Enables chiclet menu items.
	 */
	bool enableMenuItem(const LLSD& user_data);

	/**
	 * Creates menu.
	 */
	/*virtual*/ void createMenu();

	/**
	 * Handles changes in a session (message was added, messages were read, etc.)
	 *
	 * It get total count of unread messages from a LLIMMgr in all opened sessions and display it.
	 *
	 * @param[in] session_data contains session related data, is not used now
	 *		["session_id"] - id of an appropriate session
	 *		["participant_unread"] - count of unread messages from "real" participants.
	 *
	 * @see LLIMMgr::getNumberOfUnreadParticipantMessages()
	 */
	void messageCountChanged(const LLSD& session_data);
};
// [/SL:KB]

class LLNotificationChiclet : public LLSysWellChiclet
{
	LOG_CLASS(LLNotificationChiclet);
			
	friend class LLUICtrlFactory;
public:
	struct Params : public LLInitParam::Block<Params, LLSysWellChiclet::Params>{};
		
protected:
	struct ChicletNotificationChannel : public LLNotificationChannel
	{
		ChicletNotificationChannel(LLNotificationChiclet* chiclet) 
			: LLNotificationChannel(LLNotificationChannel::Params().filter(filterNotification).name(chiclet->getSessionId().asString()))
			, mChiclet(chiclet)
		{
			// connect counter handlers to the signals
			connectToChannel("Group Notifications");
			connectToChannel("Offer");
			connectToChannel("Notifications");
		}
				
		static bool filterNotification(LLNotificationPtr notify);
		// connect counter updaters to the corresponding signals
		/*virtual*/ void onAdd(LLNotificationPtr p) { mChiclet->setCounter(++mChiclet->mUreadSystemNotifications); }
		/*virtual*/ void onDelete(LLNotificationPtr p) { mChiclet->setCounter(--mChiclet->mUreadSystemNotifications); }
				
		LLNotificationChiclet* const mChiclet;
	};
				
	boost::scoped_ptr<ChicletNotificationChannel> mNotificationChannel;
				
	LLNotificationChiclet(const Params& p);
				
	/**
	 * Processes clicks on chiclet menu.
	 */
	void onMenuItemClicked(const LLSD& user_data);
				
	/**
	 * Enables chiclet menu items.
	 */
	bool enableMenuItem(const LLSD& user_data);
				
	/**
	 * Creates menu.
	 */
	/*virtual*/ void createMenu();

	/*virtual*/ void setCounter(S32 counter);
	S32 mUreadSystemNotifications;
};

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
/**
 * Implements Group chat chiclet.
 */
class LLIMGroupChiclet : public LLIMChiclet, public LLGroupMgrObserver
{
	friend class LLUICtrlFactory;
public:
	struct Params : public LLInitParam::Block<Params, LLIMChiclet::Params>
	{
		Optional<LLButton::Params> chiclet_button;

		Optional<LLChicletGroupIconCtrl::Params> group_icon;

		Optional<LLChicletNotificationCounterCtrl::Params> unread_notifications;

		Optional<LLChicletSpeakerCtrl::Params> speaker;

		Optional<LLIconCtrl::Params> new_message_icon;

		Optional<bool>	show_speaker;

		Params();
	};

protected:
	LLIMGroupChiclet(const Params& p);
public:
	/*virtual*/ ~LLIMGroupChiclet();

	/**
	 * Sets session id.
	 * Session ID for group chat is actually Group ID.
	 */
	/*virtual*/ void setSessionId(const LLUUID& session_id);

	/**
	 * Keep Speaker Control with actual speaker's ID
	 */
	/*virtual*/ void draw();

	/**
	 * Callback for LLGroupMgrObserver, we get this when group data is available or changed.
	 * Sets group icon.
	 */
	/*virtual*/ void changed(LLGroupChange gc);

	/**
	 * Init Speaker Control with speaker's ID
	 */
	/*virtual*/ void initSpeakerControl();

	/**
	 * Returns number of unread messages.
	 */
	/*virtual*/ S32 getCounter() const { return mCounterCtrl->getCounter(); }

	/**
	 * Finds a current speaker and resets the SpeakerControl with speaker's ID
	 */
	/*virtual*/ void switchToCurrentSpeaker();

	/**
	 * Creates chiclet popup menu. Will create P2P or Group IM Chat menu 
	 * based on other participant's id.
	 */
	/*virtual*/ void createPopupMenu();

	/**
	 * Processes clicks on chiclet popup menu.
	 */
	/*virtual*/ void onMenuItemClicked(const LLSD& user_data);

	/**
	 * Enables/disables "show session" menu item depending on visible IM floater existence.
	 */
	/*virtual*/ void updateMenuItems();

private:
	LLChicletGroupIconCtrl* mChicletIconCtrl;
};
// [/SL:KB]

/**
 * Storage class for all IM chiclets. Provides mechanism to display, 
 * scroll, create, remove chiclets.
 */
class LLChicletPanel : public LLPanel
{
public:

	struct Params :	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<S32> chiclet_padding,
					  scrolling_offset,
					  scroll_button_hpad,
					  scroll_ratio;

		Optional<S32> min_width;

		Params();
	};

	virtual ~LLChicletPanel();

	/**
	 * Creates chiclet and adds it to chiclet list at specified index.
	 */
	template<class T> T* createChiclet(const LLUUID& session_id, S32 index);

	/**
	 * Creates chiclet and adds it to chiclet list at right.
	 */
	template<class T> T* createChiclet(const LLUUID& session_id);

	/**
	 * Returns pointer to chiclet of specified type at specified index.
	 */
	template<class T> T* getChiclet(S32 index);

	/**
	 * Returns pointer to LLChiclet at specified index.
	 */
	LLChiclet* getChiclet(S32 index) { return getChiclet<LLChiclet>(index); }

	/**
	 * Searches a chiclet using IM session id.
	 */
	template<class T> T* findChiclet(const LLUUID& im_session_id);

	/**
	 * Returns number of hosted chiclets.
	 */
	S32 getChicletCount() {return mChicletList.size();};

	/**
	 * Returns index of chiclet in list.
	 */
	S32 getChicletIndex(const LLChiclet* chiclet);

// [SL:KB] - Patch: UI-TabRearrange | Checked: 2012-05-05 (Catznip-3.3)
	enum EChicletOrder { START, END, LEFT_OF_SESSION, RIGHT_OF_SESSION };

	/**
	 * Sets the index of the specified chiclet in the list.
	 */
	void setChicletIndex(const LLChiclet* chiclet, EChicletOrder eOrder, const LLUUID& idSession = LLUUID::null);
// [/SL:KB]

	/**
	 * Removes chiclet by index.
	 */
	void removeChiclet(S32 index);

	/**
	 * Removes chiclet by pointer.
	 */
	void removeChiclet(LLChiclet* chiclet);

	/**
	 * Removes chiclet by IM session id.
	 */
	void removeChiclet(const LLUUID& im_session_id);

	/**
	 * Removes all chiclets.
	 */
	void removeAll();

	/**
	 * Scrolls the panel to the specified chiclet
	 */
	void scrollToChiclet(const LLChiclet* chiclet);

	boost::signals2::connection setChicletClickedCallback(
		const commit_callback_t& cb);

	/*virtual*/ BOOL postBuild();

	/**
	 * Handler for the Voice Client's signal. Finds a corresponding chiclet and toggles its SpeakerControl
	 */
	void onCurrentVoiceChannelChanged(const LLUUID& session_id);

	/**
	 * Reshapes controls and rearranges chiclets if needed.
	 */
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE );

	/*virtual*/ void draw();

	S32 getMinWidth() const { return mMinWidth; }

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	S32 getTotalUnreadIMCount();
// [/SL:KB]

	/*virtual*/ S32	notifyParent(const LLSD& info);

	/**
	 * Toggle chiclet by session id ON and toggle OFF all other chiclets.
	 */
	void setChicletToggleState(const LLUUID& session_id, bool toggle);

protected:
	LLChicletPanel(const Params&p);
	friend class LLUICtrlFactory;

	/**
	 * Adds chiclet to list and rearranges all chiclets. 
	 * They should be right aligned, most recent right. See EXT-1293
	 *
	 * It calculates position of the first chiclet in the list. Other chiclets are placed in arrange().
	 *
	 * @see arrange()
	 */
	bool addChiclet(LLChiclet*, S32 index);

	/**
	 * Arranges chiclets to have them in correct positions.
	 *
	 * Method bases on assumption that first chiclet has correct rect and starts from the its position.
	 *
	 * @see addChiclet()
	 */
	void arrange();

	/**
	 * Returns true if chiclets can be scrolled right.
	 */
	bool canScrollRight();

	/**
	 * Returns true if we need to show scroll buttons
	 */
	bool needShowScroll();

	/**
	 * Returns true if chiclets can be scrolled left.
	 */
	bool canScrollLeft();

	/**
	 * Shows or hides chiclet scroll buttons if chiclets can or can not be scrolled.
	 */
	void showScrollButtonsIfNeeded();

	/**
	 * Shifts chiclets left or right.
	 */
	void shiftChiclets(S32 offset, S32 start_index = 0);

	/**
	 * Removes gaps between first chiclet and scroll area left side,
	 * last chiclet and scroll area right side.
	 */
	void trimChiclets();

	/**
	 * Scrolls chiclets to right or left.
	 */
	void scroll(S32 offset);

	/**
	 * Verifies that chiclets can be scrolled left, then calls scroll()
	 */
	void scrollLeft();

	/**
	 * Verifies that chiclets can be scrolled right, then calls scroll()
	 */
	void scrollRight();

	/**
	 * Callback for left scroll button clicked
	 */
	void onLeftScrollClick();

	/**
	 * Callback for right scroll button clicked
	 */
	void onRightScrollClick();

	/**
	 * Callback for right scroll button held down event
	 */
	void onLeftScrollHeldDown();

	/**
	 * Callback for left scroll button held down event
	 */
	void onRightScrollHeldDown();

	/**
	 * Callback for mouse wheel scrolled, calls scrollRight() or scrollLeft()
	 */
	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);

	/**
	 * Notifies subscribers about click on chiclet.
	 * Do not place any code here, instead subscribe on event (see setChicletClickedCallback).
	 */
	void onChicletClick(LLUICtrl*ctrl,const LLSD&param);

	/**
	 * Callback for chiclet size changed event, rearranges chiclets.
	 */
	void onChicletSizeChanged(LLChiclet* ctrl, const LLSD& param);

	void onMessageCountChanged(const LLSD& data);

	void objectChicletCallback(const LLSD& data);

	typedef std::vector<LLChiclet*> chiclet_list_t;

	/**
	 * Removes chiclet from scroll area and chiclet list.
	 */
	void removeChiclet(chiclet_list_t::iterator it);

	S32 getChicletPadding() { return mChicletPadding; }

	S32 getScrollingOffset() { return mScrollingOffset; }

	bool isAnyIMFloaterDoked();

protected:

	chiclet_list_t mChicletList;
	LLButton* mLeftScrollButton;
	LLButton* mRightScrollButton;
	LLPanel* mScrollArea;

	S32 mChicletPadding;
	S32 mScrollingOffset;
	S32 mScrollButtonHPad;
	S32 mScrollRatio;
	S32 mMinWidth;
	bool mShowControls;
	static const S32 s_scroll_ratio;
};

template<class T> 
T* LLChicletPanel::createChiclet(const LLUUID& session_id, S32 index)
{
	typename T::Params params;
	T* chiclet = LLUICtrlFactory::create<T>(params);
	if(!chiclet)
	{
		LL_WARNS() << "Could not create chiclet" << LL_ENDL;
		return NULL;
	}
	if(!addChiclet(chiclet, index))
	{
		delete chiclet;
		LL_WARNS() << "Could not add chiclet to chiclet panel" << LL_ENDL;
		return NULL;
	}

	if (!isAnyIMFloaterDoked())
	{
		scrollToChiclet(chiclet);
	}

	chiclet->setSessionId(session_id);

	return chiclet;
}

template<class T>
T* LLChicletPanel::createChiclet(const LLUUID& session_id)
{
	return createChiclet<T>(session_id, mChicletList.size());
}

template<class T>
T* LLChicletPanel::findChiclet(const LLUUID& im_session_id)
{
	if(im_session_id.isNull())
	{
		return NULL;
	}

	chiclet_list_t::const_iterator it = mChicletList.begin();
	for( ; mChicletList.end() != it; ++it)
	{
		LLChiclet* chiclet = *it;

		llassert(chiclet);
		if (!chiclet) continue;
		if(chiclet->getSessionId() == im_session_id)
		{
			T* result = dynamic_cast<T*>(chiclet);
			if(!result)
			{
				LL_WARNS() << "Found chiclet but of wrong type " << LL_ENDL;
				continue;
			}
			return result;
		}
	}
	return NULL;
}

template<class T> T* LLChicletPanel::getChiclet(S32 index)
{
	if(index < 0 || index >= getChicletCount())
	{
		return NULL;
	}

	LLChiclet* chiclet = mChicletList[index];
	T*result = dynamic_cast<T*>(chiclet);
	if(!result && chiclet)
	{
		LL_WARNS() << "Found chiclet but of wrong type " << LL_ENDL;
	}
	return result;
}

#endif // LL_LLCHICLET_H
