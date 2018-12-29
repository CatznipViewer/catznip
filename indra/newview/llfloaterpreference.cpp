/** 
 * @file llfloaterpreference.cpp
 * @brief Global preferences with and without persistence.
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

/*
 * App-wide preferences.  Note that these are not per-user,
 * because we need to load many preferences before we have
 * a login name.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterpreference.h"

#include "message.h"
#include "llfloaterautoreplacesettings.h"
#include "llagent.h"
#include "llagentcamera.h"
// [SL:KB] - Patch: Settings-ShapeHover | Checked: 2013-06-05 (Catznip-3.4)
#include "llagentwearables.h"
// [/SL:KB]
#include "llcheckboxctrl.h"
// [SL:KB] - Patch: Viewer-CrashLookup | Checked: 2012-05-26 (Catznip-3.3)
#include "llclipboard.h"
// [/SL:KB]
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "llcommandhandler.h"
#include "lldirpicker.h"
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2013-06-27 (Catznip-3.4)
#include "llerrorcontrol.h"
// [/SL:KB]
#include "lleventtimer.h"
#include "llfeaturemanager.h"
#include "llfocusmgr.h"
//#include "llfirstuse.h"
#include "llfloaterreg.h"
#include "llfloaterabout.h"
// [SL:KB] - Patch: World-RenderExceptions | Checked: Catznip-5.2
#include "llfloaterblocked.h"
// [/SL:KB]
#include "llfavoritesbar.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterimsession.h"
#include "llkeyboard.h"
#include "llmodaldialog.h"
#include "llnavigationbar.h"
#include "llfloaterimnearbychat.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llnotificationtemplate.h"
#include "llpanellogin.h"
#include "llpanelvoicedevicesettings.h"
#include "llradiogroup.h"
#include "llsearchcombobox.h"
#include "llsky.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsliderctrl.h"
#include "lltabcontainer.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llviewermessage.h"
#include "llviewershadermgr.h"
#include "llviewerthrottle.h"
#include "llvoavatarself.h"
#include "llvotree.h"
#include "llvosky.h"
#include "llfloaterpathfindingconsole.h"
// linden library includes
#include "llavatarnamecache.h"
#include "llerror.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llstring.h"

// project includes

#include "llbutton.h"
// [SL:KB] - Patch: Settings-Troubleshooting | Checked: Catznip-5.4
#include "llenvmanager.h"
// [/SL:KB]
#include "llflexibleobject.h"
#include "lllineeditor.h"
#include "llresmgr.h"
#include "llspinctrl.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llvovolume.h"
#include "llwindow.h"
#include "llworld.h"
#include "pipeline.h"
#include "lluictrlfactory.h"
#include "llviewermedia.h"
#include "llpluginclassmedia.h"
// [SL:KB] - Patch: Settings-ClearCache | Checked: 2011-10-01 (Catznip-2.8)
#include "llrecentpeople.h"
// [/SL:KB]
#include "llteleporthistorystorage.h"
#include "llproxy.h"
#include "llweb.h"
// [SL:KB] - Patch: Notification-Logging | Checked: 2012-02-01 (Catznip-3.2)
#include <boost/algorithm/string.hpp>
// [/SL:KB]
// [RLVa:KB] - Checked: 2010-03-18 (RLVa-1.2.0a)
#include "rlvactions.h"
#include "rlvhandler.h"
// [/RLVa:KB]

#include "lllogininstance.h"        // to check if logged in yet
#include "llsdserialize.h"
#include "llpresetsmanager.h"
#include "llviewercontrol.h"
#include "llpresetsmanager.h"
#include "llfeaturemanager.h"
#include "llviewertexturelist.h"

const F32 BANDWIDTH_UPDATER_TIMEOUT = 0.5f;
char const* const VISIBILITY_DEFAULT = "default";
char const* const VISIBILITY_HIDDEN = "hidden";

// [SL:KB] - Patch: Settings-ClearCache | Checked: 2010-08-03 (Catznip-2.1)
const U32 CLEAR_MASK_COOKIES	= 0x01;
const U32 CLEAR_MASK_NAVBAR		= 0x02;
const U32 CLEAR_MASK_SEARCH		= 0x04;
const U32 CLEAR_MASK_TELEPORT	= 0x08;
const U32 CLEAR_MASK_PEOPLE		= 0x10;
// [/SL:KB]

//control value for middle mouse as talk2push button
const static std::string MIDDLE_MOUSE_CV = "MiddleMouse";

/// This must equal the maximum value set for the IndirectMaxComplexity slider in panel_preferences_graphics1.xml
static const U32 INDIRECT_MAX_ARC_OFF = 101; // all the way to the right == disabled
static const U32 MIN_INDIRECT_ARC_LIMIT = 1; // must match minimum of IndirectMaxComplexity in panel_preferences_graphics1.xml
static const U32 MAX_INDIRECT_ARC_LIMIT = INDIRECT_MAX_ARC_OFF-1; // one short of all the way to the right...

/// These are the effective range of values for RenderAvatarMaxComplexity
static const F32 MIN_ARC_LIMIT =  20000.0f;
static const F32 MAX_ARC_LIMIT = 350000.0f;
static const F32 MIN_ARC_LOG = log(MIN_ARC_LIMIT);
static const F32 MAX_ARC_LOG = log(MAX_ARC_LIMIT);
static const F32 ARC_LIMIT_MAP_SCALE = (MAX_ARC_LOG - MIN_ARC_LOG) / (MAX_INDIRECT_ARC_LIMIT - MIN_INDIRECT_ARC_LIMIT);

class LLVoiceSetKeyDialog : public LLModalDialog
{
public:
	LLVoiceSetKeyDialog(const LLSD& key);
	~LLVoiceSetKeyDialog();
	
	/*virtual*/ BOOL postBuild();
	
	void setParent(LLFloaterPreference* parent) { mParent = parent; }
	
	BOOL handleKeyHere(KEY key, MASK mask);
	static void onCancel(void* user_data);
		
private:
	LLFloaterPreference* mParent;
};

LLVoiceSetKeyDialog::LLVoiceSetKeyDialog(const LLSD& key)
  : LLModalDialog(key),
	mParent(NULL)
{
}

//virtual
BOOL LLVoiceSetKeyDialog::postBuild()
{
	childSetAction("Cancel", onCancel, this);
	getChild<LLUICtrl>("Cancel")->setFocus(TRUE);
	
	gFocusMgr.setKeystrokesOnly(TRUE);
	
	return TRUE;
}

LLVoiceSetKeyDialog::~LLVoiceSetKeyDialog()
{
}

BOOL LLVoiceSetKeyDialog::handleKeyHere(KEY key, MASK mask)
{
	BOOL result = TRUE;
	
	if (key == 'Q' && mask == MASK_CONTROL)
	{
		result = FALSE;
	}
	else if (mParent)
	{
		mParent->setKey(key);
	}
	closeFloater();
	return result;
}

//static
void LLVoiceSetKeyDialog::onCancel(void* user_data)
{
	LLVoiceSetKeyDialog* self = (LLVoiceSetKeyDialog*)user_data;
	self->closeFloater();
}


// global functions 

// helper functions for getting/freeing the web browser media
// if creating/destroying these is too slow, we'll need to create
// a static member and update all our static callbacks

void handleNameTagOptionChanged(const LLSD& newvalue);	
void handleDisplayNamesOptionChanged(const LLSD& newvalue);	
bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response);
bool callback_clear_cache(const LLSD& notification, const LLSD& response);

// [SL:KB] - Patch: UI-Font | Checked: 2012-10-10 (Catznip-3.3)
void initEditorFont(LLUICtrl* pCtrl);
void refreshEditorFont(LLHandle<LLUICtrl> hCtrl);
void handleEditorFontToggle(LLUICtrl* pCheckCtrl);
// [/SL:KB]

//bool callback_skip_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);
//bool callback_reset_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);

void fractionFromDecimal(F32 decimal_val, S32& numerator, S32& denominator);

bool callback_clear_cache(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if ( option == 0 ) // YES
	{
		// flag client texture cache for clearing next time the client runs
		gSavedSettings.setBOOL("PurgeCacheOnNextStartup", TRUE);
		LLNotificationsUtil::add("CacheWillClear");
	}

	return false;
}

bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if ( option == 0 ) // YES
	{
// [SL:KB] - Patch: Settings-ClearCache | Checked: 2010-08-03 (Catznip-2.1)
		U32 nClearMask = gSavedSettings.getU32("ClearCacheMask");
// [/SL:KB]

		// clean web
		LLViewerMedia::clearAllCaches();
// [SL:KB] - Patch: Settings-ClearCache | Checked: 2010-08-03 (Catznip-2.1)
		if (nClearMask & CLEAR_MASK_COOKIES)
		{
			LLViewerMedia::clearAllCookies();
		}
// [/SL:KB]
//		LLViewerMedia::clearAllCookies();
		
		// clean nav bar history
// [SL:KB] - Patch: Settings-ClearCache | Checked: 2010-08-03 (Catznip-2.1)
		if (nClearMask & CLEAR_MASK_NAVBAR)
		{
			LLNavigationBar::getInstance()->clearHistoryCache();
		}
// [/SL:KB]
//		LLNavigationBar::getInstance()->clearHistoryCache();
		
		// flag client texture cache for clearing next time the client runs
		gSavedSettings.setBOOL("PurgeCacheOnNextStartup", TRUE);
		LLNotificationsUtil::add("CacheWillClear");

// [SL:KB] - Patch: Settings-ClearCache | Checked: 2010-08-03 (Catznip-2.1)
		if (nClearMask & CLEAR_MASK_SEARCH)
		{
			LLSearchHistory::getInstance()->clearHistory();
			LLSearchHistory::getInstance()->save();
			LLSearchComboBox* search_ctrl = LLNavigationBar::getInstance()->getChild<LLSearchComboBox>("search_combo_box");
			search_ctrl->clearHistory();
		}
// [/SL:KB]
//		LLSearchHistory::getInstance()->clearHistory();
//		LLSearchHistory::getInstance()->save();
//		LLSearchComboBox* search_ctrl = LLNavigationBar::getInstance()->getChild<LLSearchComboBox>("search_combo_box");
//		search_ctrl->clearHistory();

// [SL:KB] - Patch: Settings-ClearCache | Checked: 2010-08-03 (Catznip-2.1)
		if (nClearMask & CLEAR_MASK_TELEPORT)
		{
			LLTeleportHistoryStorage::getInstance()->purgeItems();
			LLTeleportHistoryStorage::getInstance()->save();
		}
// [/SL:KB]
//		LLTeleportHistoryStorage::getInstance()->purgeItems();
//		LLTeleportHistoryStorage::getInstance()->save();

// [SL:KB] - Patch: Settings-ClearCache | Checked: 2011-10-01 (Catznip-3.0)
		if (nClearMask & CLEAR_MASK_PEOPLE)
		{
			LLRecentPeople::getInstance()->purgeItems();
			LLRecentPeople::getInstance()->save();
		}
// [/SL:KB]
	}
	
	return false;
}

void handleNameTagOptionChanged(const LLSD& newvalue)
{
//	LLAvatarNameCache::setUseUsernames(gSavedSettings.getBOOL("NameTagShowUsernames"));
	LLVOAvatar::invalidateNameTags();
}

void handleDisplayNamesOptionChanged(const LLSD& newvalue)
{
	LLAvatarNameCache::setUseDisplayNames(newvalue.asBoolean());
	LLVOAvatar::invalidateNameTags();
}

void handleAppearanceCameraMovementChanged(const LLSD& newvalue)
{
	if(!newvalue.asBoolean() && gAgentCamera.getCameraMode() == CAMERA_MODE_CUSTOMIZE_AVATAR)
	{
		gAgentCamera.changeCameraToDefault();
		gAgentCamera.resetView();
	}
}
/*bool callback_skip_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option && floater )
	{
		if ( floater )
		{
			floater->setAllIgnored();
		//	LLFirstUse::disableFirstUse();
			floater->buildPopupLists();
		}
	}
	return false;
}

bool callback_reset_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if ( 0 == option && floater )
	{
		if ( floater )
		{
			floater->resetAllIgnored();
			//LLFirstUse::resetFirstUse();
			floater->buildPopupLists();
		}
	}
	return false;
}
*/

void fractionFromDecimal(F32 decimal_val, S32& numerator, S32& denominator)
{
	numerator = 0;
	denominator = 0;
	for (F32 test_denominator = 1.f; test_denominator < 30.f; test_denominator += 1.f)
	{
		if (fmodf((decimal_val * test_denominator) + 0.01f, 1.f) < 0.02f)
		{
			numerator = ll_round(decimal_val * test_denominator);
			denominator = ll_round(test_denominator);
			break;
		}
	}
}
// static
//std::string LLFloaterPreference::sSkin = "";
//////////////////////////////////////////////
// LLFloaterPreference

LLFloaterPreference::LLFloaterPreference(const LLSD& key)
	: LLFloater(key),
	mGotPersonalInfo(false),
	mOriginalIMViaEmail(false),
	mLanguageChanged(false),
	mAvatarDataInitialized(false),
	mClickActionDirty(false)
{
	LLConversationLog::instance().addObserver(this);

	//Build Floater is now Called from 	LLFloaterReg::add("preferences", "floater_preferences.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreference>);
	
	static bool registered_dialog = false;
	if (!registered_dialog)
	{
		LLFloaterReg::add("voice_set_key", "floater_select_key.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLVoiceSetKeyDialog>);
		registered_dialog = true;
	}
	
	mCommitCallbackRegistrar.add("Pref.Cancel",				boost::bind(&LLFloaterPreference::onBtnCancel, this, _2));
	mCommitCallbackRegistrar.add("Pref.OK",					boost::bind(&LLFloaterPreference::onBtnOK, this, _2));
// [SL:KB] - Patch: Preferences-General | Checked: Catznip-3.6)
	mCommitCallbackRegistrar.add("Pref.ShowPanel",			boost::bind(&LLFloaterPreference::onShowPanel, this, _2));
	mCommitCallbackRegistrar.add("Pref.ResetUIScale",		boost::bind(&LLFloaterPreference::onResetUIScale, this));
// [/SL:KB]
	
	mCommitCallbackRegistrar.add("Pref.ClearCache",				boost::bind(&LLFloaterPreference::onClickClearCache, this));
	mCommitCallbackRegistrar.add("Pref.WebClearCache",			boost::bind(&LLFloaterPreference::onClickBrowserClearCache, this));
	mCommitCallbackRegistrar.add("Pref.SetCache",				boost::bind(&LLFloaterPreference::onClickSetCache, this));
	mCommitCallbackRegistrar.add("Pref.ResetCache",				boost::bind(&LLFloaterPreference::onClickResetCache, this));
//	mCommitCallbackRegistrar.add("Pref.ClickSkin",				boost::bind(&LLFloaterPreference::onClickSkin, this,_1, _2));
//	mCommitCallbackRegistrar.add("Pref.SelectSkin",				boost::bind(&LLFloaterPreference::onSelectSkin, this));
	mCommitCallbackRegistrar.add("Pref.VoiceSetKey",			boost::bind(&LLFloaterPreference::onClickSetKey, this));
	mCommitCallbackRegistrar.add("Pref.VoiceSetMiddleMouse",	boost::bind(&LLFloaterPreference::onClickSetMiddleMouse, this));
	mCommitCallbackRegistrar.add("Pref.SetSounds",				boost::bind(&LLFloaterPreference::onClickSetSounds, this));
	mCommitCallbackRegistrar.add("Pref.ClickEnablePopup",		boost::bind(&LLFloaterPreference::onClickEnablePopup, this));
	mCommitCallbackRegistrar.add("Pref.ClickDisablePopup",		boost::bind(&LLFloaterPreference::onClickDisablePopup, this));	
	mCommitCallbackRegistrar.add("Pref.LogPath",				boost::bind(&LLFloaterPreference::onClickLogPath, this));
	mCommitCallbackRegistrar.add("Pref.RenderExceptions",       boost::bind(&LLFloaterPreference::onClickRenderExceptions, this));
// [SL:KB] - Patch: Settings-Snapshot | Checked: Catznip-3.2
	mCommitCallbackRegistrar.add("Pref.SnapshotPath",			boost::bind(&LLFloaterPreference::onClickSnapshotPath, this));
// [/SL:KB]
	mCommitCallbackRegistrar.add("Pref.HardwareDefaults",		boost::bind(&LLFloaterPreference::setHardwareDefaults, this));
	mCommitCallbackRegistrar.add("Pref.AvatarImpostorsEnable",	boost::bind(&LLFloaterPreference::onAvatarImpostorsEnable, this));
	mCommitCallbackRegistrar.add("Pref.UpdateIndirectMaxComplexity",	boost::bind(&LLFloaterPreference::updateMaxComplexity, this));
	mCommitCallbackRegistrar.add("Pref.VertexShaderEnable",		boost::bind(&LLFloaterPreference::onVertexShaderEnable, this));
	mCommitCallbackRegistrar.add("Pref.WindowedMod",			boost::bind(&LLFloaterPreference::onCommitWindowedMode, this));
	mCommitCallbackRegistrar.add("Pref.UpdateSliderText",		boost::bind(&LLFloaterPreference::refreshUI,this));
	mCommitCallbackRegistrar.add("Pref.QualityPerformance",		boost::bind(&LLFloaterPreference::onChangeQuality, this, _2));
	mCommitCallbackRegistrar.add("Pref.applyUIColor",			boost::bind(&LLFloaterPreference::applyUIColor, this ,_1, _2));
// [SL:KB] - Patch: Settings-NameTags | Checked: 2014-05-17 (Catznip-3.6)
	mCommitCallbackRegistrar.add("Pref.applyNameTagColor",		boost::bind(&LLFloaterPreference::applyNameTagColor, this ,_1, _2));
// [/SL:KB]
	mCommitCallbackRegistrar.add("Pref.getUIColor",				boost::bind(&LLFloaterPreference::getUIColor, this ,_1, _2));
	mCommitCallbackRegistrar.add("Pref.MaturitySettings",		boost::bind(&LLFloaterPreference::onChangeMaturity, this));
	mCommitCallbackRegistrar.add("Pref.BlockList",				boost::bind(&LLFloaterPreference::onClickBlockList, this));
	mCommitCallbackRegistrar.add("Pref.Proxy",					boost::bind(&LLFloaterPreference::onClickProxySettings, this));
//	mCommitCallbackRegistrar.add("Pref.TranslationSettings",	boost::bind(&LLFloaterPreference::onClickTranslationSettings, this));
//	mCommitCallbackRegistrar.add("Pref.AutoReplace",            boost::bind(&LLFloaterPreference::onClickAutoReplace, this));
	mCommitCallbackRegistrar.add("Pref.PermsDefault",           boost::bind(&LLFloaterPreference::onClickPermsDefault, this));
//	mCommitCallbackRegistrar.add("Pref.SpellChecker",           boost::bind(&LLFloaterPreference::onClickSpellChecker, this));
	mCommitCallbackRegistrar.add("Pref.Advanced",				boost::bind(&LLFloaterPreference::onClickAdvanced, this));
// [SL:KB] - Patch: UI-Font | Checked: 2012-10-10 (Catznip-3.3)
	mCommitCallbackRegistrar.add("Pref.InitEditorFont",			boost::bind(&initEditorFont, _1));
	mCommitCallbackRegistrar.add("Pref.ToggleEditorFont",		boost::bind(&handleEditorFontToggle, _1));
// [/SL:KB]

//	sSkin = gSavedSettings.getString("SkinCurrent");

	mCommitCallbackRegistrar.add("Pref.ClickActionChange",		boost::bind(&LLFloaterPreference::onClickActionChange, this));

	gSavedSettings.getControl("NameTagShowUsernames")->getCommitSignal()->connect(boost::bind(&handleNameTagOptionChanged,  _2));	
	gSavedSettings.getControl("NameTagShowFriends")->getCommitSignal()->connect(boost::bind(&handleNameTagOptionChanged,  _2));	
	gSavedSettings.getControl("UseDisplayNames")->getCommitSignal()->connect(boost::bind(&handleDisplayNamesOptionChanged,  _2));

	gSavedSettings.getControl("AppearanceCameraMovement")->getCommitSignal()->connect(boost::bind(&handleAppearanceCameraMovementChanged,  _2));

	LLAvatarPropertiesProcessor::getInstance()->addObserver( gAgent.getID(), this );

	mCommitCallbackRegistrar.add("Pref.ClearLog",				boost::bind(&LLConversationLog::onClearLog, &LLConversationLog::instance()));
	mCommitCallbackRegistrar.add("Pref.DeleteTranscripts",      boost::bind(&LLFloaterPreference::onDeleteTranscripts, this));
}

void LLFloaterPreference::processProperties( void* pData, EAvatarProcessorType type )
{
	if ( APT_PROPERTIES == type )
	{
		const LLAvatarData* pAvatarData = static_cast<const LLAvatarData*>( pData );
		if (pAvatarData && (gAgent.getID() == pAvatarData->avatar_id) && (pAvatarData->avatar_id != LLUUID::null))
		{
			storeAvatarProperties( pAvatarData );
			processProfileProperties( pAvatarData );
		}
	}	
}

void LLFloaterPreference::storeAvatarProperties( const LLAvatarData* pAvatarData )
{
	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		mAvatarProperties.avatar_id		= pAvatarData->avatar_id;
		mAvatarProperties.image_id		= pAvatarData->image_id;
		mAvatarProperties.fl_image_id   = pAvatarData->fl_image_id;
		mAvatarProperties.about_text	= pAvatarData->about_text;
		mAvatarProperties.fl_about_text = pAvatarData->fl_about_text;
		mAvatarProperties.profile_url   = pAvatarData->profile_url;
		mAvatarProperties.flags		    = pAvatarData->flags;
		mAvatarProperties.allow_publish	= pAvatarData->flags & AVATAR_ALLOW_PUBLISH;

		mAvatarDataInitialized = true;
	}
}

void LLFloaterPreference::processProfileProperties(const LLAvatarData* pAvatarData )
{
	getChild<LLUICtrl>("online_searchresults")->setValue( (bool)(pAvatarData->flags & AVATAR_ALLOW_PUBLISH) );	
}

void LLFloaterPreference::saveAvatarProperties( void )
{
	const BOOL allowPublish = getChild<LLUICtrl>("online_searchresults")->getValue();

	if (allowPublish)
	{
		mAvatarProperties.flags |= AVATAR_ALLOW_PUBLISH;
	}

	//
	// NOTE: We really don't want to send the avatar properties unless we absolutely
	//       need to so we can avoid the accidental profile reset bug, so, if we're
	//       logged in, the avatar data has been initialized and we have a state change
	//       for the "allow publish" flag, then set the flag to its new value and send
	//       the properties update.
	//
	// NOTE: The only reason we can not remove this update altogether is because of the
	//       "allow publish" flag, the last remaining profile setting in the viewer
	//       that doesn't exist in the web profile.
	//
	if ((LLStartUp::getStartupState() == STATE_STARTED) && mAvatarDataInitialized && (allowPublish != mAvatarProperties.allow_publish))
	{
		mAvatarProperties.allow_publish = allowPublish;

		LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesUpdate( &mAvatarProperties );
	}
}

BOOL LLFloaterPreference::postBuild()
{
//	gSavedSettings.getControl("ChatFontSize")->getSignal()->connect(boost::bind(&LLFloaterIMSessionTab::processChatHistoryStyleUpdate, false));

//	gSavedSettings.getControl("ChatFontSize")->getSignal()->connect(boost::bind(&LLViewerChat::signalChatFontChanged));

	gSavedSettings.getControl("ChatBubbleOpacity")->getSignal()->connect(boost::bind(&LLFloaterPreference::onNameTagOpacityChange, this, _2));

	gSavedSettings.getControl("PreferredMaturity")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeMaturity, this));

	gSavedPerAccountSettings.getControl("ModelUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeModelFolder, this));
	gSavedPerAccountSettings.getControl("TextureUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeTextureFolder, this));
	gSavedPerAccountSettings.getControl("SoundUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeSoundFolder, this));
	gSavedPerAccountSettings.getControl("AnimationUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeAnimationFolder, this));

	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	if (!tabcontainer->selectTab(gSavedSettings.getS32("LastPrefTab")))
		tabcontainer->selectFirstTab();

	getChild<LLUICtrl>("cache_location")->setEnabled(FALSE); // make it read-only but selectable (STORM-227)
	std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
	setCacheLocation(cache_location);

//	getChild<LLUICtrl>("log_path_string")->setEnabled(FALSE); // make it read-only but selectable

	getChild<LLComboBox>("language_combobox")->setCommitCallback(boost::bind(&LLFloaterPreference::onLanguageChange, this));

//	getChild<LLComboBox>("FriendIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"FriendIMOptions"));
//	getChild<LLComboBox>("NonFriendIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"NonFriendIMOptions"));
//	getChild<LLComboBox>("ConferenceIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"ConferenceIMOptions"));
//	getChild<LLComboBox>("GroupChatOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"GroupChatOptions"));
//	getChild<LLComboBox>("NearbyChatOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"NearbyChatOptions"));
//	getChild<LLComboBox>("ObjectIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"ObjectIMOptions"));

	// if floater is opened before login set default localized do not disturb message
	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		gSavedPerAccountSettings.setString("DoNotDisturbModeResponse", LLTrans::getString("DoNotDisturbModeResponseDefault"));
	}

	// set 'enable' property for 'Clear log...' button
	changed();

	LLLogChat::setSaveHistorySignal(boost::bind(&LLFloaterPreference::onLogChatHistorySaved, this));

	LLSliderCtrl* fov_slider = getChild<LLSliderCtrl>("camera_fov");
	fov_slider->setMinValue(LLViewerCamera::getInstance()->getMinView());
	fov_slider->setMaxValue(LLViewerCamera::getInstance()->getMaxView());

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2011-06-11 (Catznip-2.6)
#ifndef LL_SEND_CRASH_REPORTS
	// Hide the crash report tab if crash reporting isn't enabled
	LLTabContainer* pTabContainer = getChild<LLTabContainer>("pref core");
	if (pTabContainer)
	{
		LLPanel* pCrashReportPanel = pTabContainer->getPanelByName("crashreports");
		if (pCrashReportPanel)
			pTabContainer->removeTabPanel(pCrashReportPanel);
	}
#endif // LL_SEND_CRASH_REPORTS
// [/SL:KB]

	return TRUE;
}

void LLFloaterPreference::updateDeleteTranscriptsButton()
{
// [SL:KB] - Patch: Chat-Logs | Checked: 2014-03-05 (Catznip-3.6)
	getChild<LLButton>("delete_transcripts")->setEnabled(LLLogChat::hasTranscripts());
// [/SL:KB]
//	std::vector<std::string> list_of_transcriptions_file_names;
//	LLLogChat::getListOfTranscriptFiles(list_of_transcriptions_file_names);
//	getChild<LLButton>("delete_transcripts")->setEnabled(list_of_transcriptions_file_names.size() > 0);
}

void LLFloaterPreference::onDoNotDisturbResponseChanged()
{
	// set "DoNotDisturbResponseChanged" TRUE if user edited message differs from default, FALSE otherwise
	bool response_changed_flag =
			LLTrans::getString("DoNotDisturbModeResponseDefault")
					!= getChild<LLUICtrl>("do_not_disturb_response")->getValue().asString();

	gSavedPerAccountSettings.setBOOL("DoNotDisturbResponseChanged", response_changed_flag );
}

LLFloaterPreference::~LLFloaterPreference()
{
	LLConversationLog::instance().removeObserver(this);
}

void LLFloaterPreference::draw()
{
	BOOL has_first_selected = (getChildRef<LLScrollListCtrl>("disabled_popups").getFirstSelected()!=NULL);
	gSavedSettings.setBOOL("FirstSelectedDisabledPopups", has_first_selected);
	
	has_first_selected = (getChildRef<LLScrollListCtrl>("enabled_popups").getFirstSelected()!=NULL);
	gSavedSettings.setBOOL("FirstSelectedEnabledPopups", has_first_selected);
	
	LLFloater::draw();
}

// [SL:KB] - Patch: Preferences-General | Checked: Catznip-3.6
void LLFloaterPreference::registerPrefPanel(LLPanelPreference* pPrefPanel)
{
	if (mPreferencePanels.end() == std::find(mPreferencePanels.cbegin(), mPreferencePanels.cend(), pPrefPanel))
	{
		mPreferencePanels.push_back(pPrefPanel);
	}
}

void LLFloaterPreference::unregisterPrefpanel(LLPanelPreference* pPrefPanel)
{
	std::list<LLPanelPreference*>::const_iterator itPanel = std::find(mPreferencePanels.cbegin(), mPreferencePanels.cend(), pPrefPanel);
	if (mPreferencePanels.end() != itPanel)
	{
		mPreferencePanels.erase(itPanel);
	}
}

template<class T> T* LLFloaterPreference::getPanelByType() const
{
	T* pRetPanel = nullptr;
	for (LLPanelPreference* pPanel : mPreferencePanels)
	{
		pRetPanel = dynamic_cast<T*>(pPanel);
		if (pRetPanel)
			break;
	}
	return pRetPanel;
}

void LLFloaterPreference::showPanel(const std::string& strPanel)
{
	if (!strPanel.empty())
	{
		auto itPanel = std::find_if(mPreferencePanels.begin(), mPreferencePanels.end(), [&strPanel](const LLPanelPreference* x) { return x->getName() == strPanel; });
		if (mPreferencePanels.end() != itPanel)
		{
			LLPanel* pPanel = *itPanel;
			while (LLTabContainer* pParent = pPanel->getParentByType<LLTabContainer>())
			{
				pParent->selectTabPanel(pPanel);
				pPanel = pParent;
			}
		}
	}
}

void LLFloaterPreference::onShowPanel(const LLSD& sdParam)
{
	showPanel(sdParam.asString());
}

void LLFloaterPreference::onResetUIScale() const
{
	gSavedSettings.setF32("UIScaleFactor", 1.0);
	gViewerWindow->requestResolutionUpdate();
}
// [/SL:KB]

//void LLFloaterPreference::saveSettings()
//{
//	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
//	child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
//	child_list_t::const_iterator end = tabcontainer->getChildList()->end();
//	for ( ; iter != end; ++iter)
//	{
//		LLView* view = *iter;
//		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
//		if (panel)
//			panel->saveSettings();
//	}
//}	

void LLFloaterPreference::apply()
{
	LLAvatarPropertiesProcessor::getInstance()->addObserver( gAgent.getID(), this );
	
//	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");

//	if (sSkin != gSavedSettings.getString("SkinCurrent"))
//	{
//		LLNotificationsUtil::add("ChangeSkin");
//		refreshSkin(this);
//	}

// [SL:KB] - Patch: Preferences-General | Checked: Catznip-3.6
	// Call apply() on all registered preference panels
	for (LLPanelPreference* pPrefPanel : mPreferencePanels)
	{
		if ( (pPrefPanel) && (pPrefPanel->isInitialized()) && (pPrefPanel->isDirty()) )
		{
			pPrefPanel->apply();
			pPrefPanel->saveSettings();
		}
	}
// [/SL:KB]
//	// Call apply() on all panels that derive from LLPanelPreference
//	for (child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
//		 iter != tabcontainer->getChildList()->end(); ++iter)
//	{
//		LLView* view = *iter;
//		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
//		if (panel)
//			panel->apply();
//	}
	
	gViewerWindow->requestResolutionUpdate(); // for UIScaleFactor

	LLSliderCtrl* fov_slider = getChild<LLSliderCtrl>("camera_fov");
	fov_slider->setMinValue(LLViewerCamera::getInstance()->getMinView());
	fov_slider->setMaxValue(LLViewerCamera::getInstance()->getMaxView());
	
	std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
	setCacheLocation(cache_location);
	
	LLViewerMedia::setCookiesEnabled(getChild<LLUICtrl>("cookies_enabled")->getValue());
	
	if (hasChild("web_proxy_enabled", TRUE) &&hasChild("web_proxy_editor", TRUE) && hasChild("web_proxy_port", TRUE))
	{
		bool proxy_enable = getChild<LLUICtrl>("web_proxy_enabled")->getValue();
		std::string proxy_address = getChild<LLUICtrl>("web_proxy_editor")->getValue();
		int proxy_port = getChild<LLUICtrl>("web_proxy_port")->getValue();
		LLViewerMedia::setProxyConfig(proxy_enable, proxy_address, proxy_port);
	}
	
	if (mGotPersonalInfo)
	{ 
		bool new_im_via_email = getChild<LLUICtrl>("send_im_to_email")->getValue().asBoolean();
		bool new_hide_online = getChild<LLUICtrl>("online_visibility")->getValue().asBoolean();		
	
		if ((new_im_via_email != mOriginalIMViaEmail)
			||(new_hide_online != mOriginalHideOnlineStatus))
		{
			// This hack is because we are representing several different 	 
			// possible strings with a single checkbox. Since most users 	 
			// can only select between 2 values, we represent it as a 	 
			// checkbox. This breaks down a little bit for liaisons, but 	 
			// works out in the end. 	 
			if (new_hide_online != mOriginalHideOnlineStatus)
			{
				if (new_hide_online) mDirectoryVisibility = VISIBILITY_HIDDEN;
				else mDirectoryVisibility = VISIBILITY_DEFAULT;
			 //Update showonline value, otherwise multiple applys won't work
				mOriginalHideOnlineStatus = new_hide_online;
			}
			gAgent.sendAgentUpdateUserInfo(new_im_via_email,mDirectoryVisibility);
		}
	}

	saveAvatarProperties();

	if (mClickActionDirty)
	{
		updateClickActionSettings();
		mClickActionDirty = false;
	}
}

void LLFloaterPreference::cancel()
{
// [SL:KB] - Patch: Preferences-General | Checked: 2014-03-03 (Catznip-3.6)
	// Call cancel() on all panels that derive from LLPanelPreference
	for (std::list<LLPanelPreference*>::iterator itPanel = mPreferencePanels.begin(); itPanel != mPreferencePanels.end(); ++itPanel)
	{
		LLPanelPreference* pPrefPanel = *itPanel;
		if ( (pPrefPanel) && (pPrefPanel->isInitialized()) && (pPrefPanel->isDirty()) )
			pPrefPanel->cancel();
	}
// [/SL:KB]
//	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
//	// Call cancel() on all panels that derive from LLPanelPreference
//	for (child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
//		iter != tabcontainer->getChildList()->end(); ++iter)
//	{
//		LLView* view = *iter;
//		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
//		if (panel)
//			panel->cancel();
//	}
	// hide joystick pref floater
	LLFloaterReg::hideInstance("pref_joystick");

//	// hide translation settings floater
//	LLFloaterReg::hideInstance("prefs_translation");
	
//	// hide autoreplace settings floater
//	LLFloaterReg::hideInstance("prefs_autoreplace");
	
//	// hide spellchecker settings folder
//	LLFloaterReg::hideInstance("prefs_spellchecker");

	// hide advancede floater
	LLFloaterReg::hideInstance("prefs_graphics_advanced");
	
	// reverts any changes to current skin
//	gSavedSettings.setString("SkinCurrent", sSkin);

	if (mClickActionDirty)
	{
		updateClickActionControls();
		mClickActionDirty = false;
	}

	LLFloaterPreferenceProxy * advanced_proxy_settings = LLFloaterReg::findTypedInstance<LLFloaterPreferenceProxy>("prefs_proxy");
	if (advanced_proxy_settings)
	{
		advanced_proxy_settings->cancel();
	}
	//Need to reload the navmesh if the pathing console is up
	LLHandle<LLFloaterPathfindingConsole> pathfindingConsoleHandle = LLFloaterPathfindingConsole::getInstanceHandle();
	if ( !pathfindingConsoleHandle.isDead() )
	{
		LLFloaterPathfindingConsole* pPathfindingConsole = pathfindingConsoleHandle.get();
		pPathfindingConsole->onRegionBoundaryCross();
	}

	if (!mSavedGraphicsPreset.empty())
	{
		gSavedSettings.setString("PresetGraphicActive", mSavedGraphicsPreset);
		LLPresetsManager::getInstance()->triggerChangeSignal();
	}
}

void LLFloaterPreference::onOpen(const LLSD& key)
{

	// this variable and if that follows it are used to properly handle do not disturb mode response message
	static bool initialized = FALSE;
	// if user is logged in and we haven't initialized do not disturb mode response yet, do it
	if (!initialized && LLStartUp::getStartupState() == STATE_STARTED)
	{
		// Special approach is used for do not disturb response localization, because "DoNotDisturbModeResponse" is
		// in non-localizable xml, and also because it may be changed by user and in this case it shouldn't be localized.
		// To keep track of whether do not disturb response is default or changed by user additional setting DoNotDisturbResponseChanged
		// was added into per account settings.

		// initialization should happen once,so setting variable to TRUE
		initialized = TRUE;
		// this connection is needed to properly set "DoNotDisturbResponseChanged" setting when user makes changes in
		// do not disturb response message.
		gSavedPerAccountSettings.getControl("DoNotDisturbModeResponse")->getSignal()->connect(boost::bind(&LLFloaterPreference::onDoNotDisturbResponseChanged, this));
	}
	gAgent.sendAgentUserInfoRequest();

	/////////////////////////// From LLPanelGeneral //////////////////////////
	// if we have no agent, we can't let them choose anything
	// if we have an agent, then we only let them choose if they have a choice
	bool can_choose_maturity =
		gAgent.getID().notNull() &&
		(gAgent.isMature() || gAgent.isGodlike());
	
	LLComboBox* maturity_combo = getChild<LLComboBox>("maturity_desired_combobox");
	LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesRequest( gAgent.getID() );
	if (can_choose_maturity)
	{		
		// if they're not adult or a god, they shouldn't see the adult selection, so delete it
		if (!gAgent.isAdult() && !gAgent.isGodlikeWithoutAdminMenuFakery())
		{
			// we're going to remove the adult entry from the combo
			LLScrollListCtrl* maturity_list = maturity_combo->findChild<LLScrollListCtrl>("ComboBox");
			if (maturity_list)
			{
				maturity_list->deleteItems(LLSD(SIM_ACCESS_ADULT));
			}
		}
		getChildView("maturity_desired_combobox")->setEnabled( true);
		getChildView("maturity_desired_textbox")->setVisible( false);
	}
	else
	{
		getChild<LLUICtrl>("maturity_desired_textbox")->setValue(maturity_combo->getSelectedItemLabel());
		getChildView("maturity_desired_combobox")->setEnabled( false);
	}

	// Forget previous language changes.
	mLanguageChanged = false;

	// Display selected maturity icons.
	onChangeMaturity();

	onChangeModelFolder();
	onChangeTextureFolder();
	onChangeSoundFolder();
	onChangeAnimationFolder();

	// Load (double-)click to walk/teleport settings.
	updateClickActionControls();
	
	// Enabled/disabled popups, might have been changed by user actions
	// while preferences floater was closed.
	buildPopupLists();


//	//get the options that were checked
//	onNotificationsChange("FriendIMOptions");
//	onNotificationsChange("NonFriendIMOptions");
//	onNotificationsChange("ConferenceIMOptions");
//	onNotificationsChange("GroupChatOptions");
//	onNotificationsChange("NearbyChatOptions");
//	onNotificationsChange("ObjectIMOptions");

	LLPanelLogin::setAlwaysRefresh(true);
	refresh();
	
// [SL:KB] - Patch: Preferences-General | Checked: 2014-03-03 (Catznip-3.6)
	mCancelOnClose = true;
// [/SL:KB]
//	// Make sure the current state of prefs are saved away when
//	// when the floater is opened.  That will make cancel do its
//	// job
//	saveSettings();

	// Make sure there is a default preference file
	LLPresetsManager::getInstance()->createMissingDefault();

	bool started = (LLStartUp::getStartupState() == STATE_STARTED);

	LLButton* load_btn = findChild<LLButton>("PrefLoadButton");
	LLButton* save_btn = findChild<LLButton>("PrefSaveButton");
	LLButton* delete_btn = findChild<LLButton>("PrefDeleteButton");
	LLButton* exceptions_btn = findChild<LLButton>("RenderExceptionsButton");

	load_btn->setEnabled(started);
	save_btn->setEnabled(started);
	delete_btn->setEnabled(started);
	exceptions_btn->setEnabled(started);
}

void LLFloaterPreference::onVertexShaderEnable()
{
	refreshEnabledGraphics();
}

void LLFloaterPreferenceGraphicsAdvanced::onVertexShaderEnable()
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance)
	{
		instance->refresh();
	}

	refreshEnabledGraphics();
}

void LLFloaterPreferenceGraphicsAdvanced::refreshEnabledGraphics()
{
	refreshEnabledState();
}

void LLFloaterPreference::onAvatarImpostorsEnable()
{
	refreshEnabledGraphics();
}

//static
void LLFloaterPreference::initDoNotDisturbResponse()
	{
		if (!gSavedPerAccountSettings.getBOOL("DoNotDisturbResponseChanged"))
		{
			//LLTrans::getString("DoNotDisturbModeResponseDefault") is used here for localization (EXT-5885)
			gSavedPerAccountSettings.setString("DoNotDisturbModeResponse", LLTrans::getString("DoNotDisturbModeResponseDefault"));
		}
	}

//static 
void LLFloaterPreference::updateShowFavoritesCheckbox(bool val)
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance)
	{
		instance->getChild<LLUICtrl>("favorites_on_login_check")->setValue(val);
	}	
}

void LLFloaterPreference::setHardwareDefaults()
{
	std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");
	if (!preset_graphic_active.empty())
	{
// [SL:KB] - Patch: Settings-Preferences | Checked: Catznip-4.1
		saveGraphicsPreset(preset_graphic_active);
		if (LLPanelPreferenceGraphics* pGraphicsPanel = getPanelByType<LLPanelPreferenceGraphics>())
		{
			pGraphicsPanel->saveSettings(); // save here to be able to return to the previous preset by Cancel
		}
// [/SL:KB]
//		saveGraphicsPreset(preset_graphic_active);
//		saveSettings(); // save here to be able to return to the previous preset by Cancel
	}

	LLFeatureManager::getInstance()->applyRecommendedSettings();

	// reset indirects before refresh because we may have changed what they control
	LLAvatarComplexityControls::setIndirectControls(); 

	refreshEnabledGraphics();
	gSavedSettings.setString("PresetGraphicActive", "");
	LLPresetsManager::getInstance()->triggerChangeSignal();

	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
	child_list_t::const_iterator end = tabcontainer->getChildList()->end();
	for ( ; iter != end; ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
		{
			panel->setHardwareDefaults();
		}
	}
}

void LLFloaterPreference::getControlNames(std::vector<std::string>& names)
{
	LLView* view = findChild<LLView>("display");
	LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced");
	if (view && advanced)
	{
		std::list<LLView*> stack;
		stack.push_back(view);
		stack.push_back(advanced);
		while(!stack.empty())
		{
			// Process view on top of the stack
			LLView* curview = stack.front();
			stack.pop_front();

			LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
			if (ctrl)
			{
				LLControlVariable* control = ctrl->getControlVariable();
//				if (control)
// [SL:KB] - Patch: Settings-ControlPreset | Checked: Catznip-5.2)
				if ( (control) && (!control->isExcludedFromPreset()) )
// [/SL:KB]
				{
					std::string control_name = control->getName();
					if (std::find(names.begin(), names.end(), control_name) == names.end())
					{
						names.push_back(control_name);
					}
				}
			}

			for (child_list_t::const_iterator iter = curview->getChildList()->begin();
				iter != curview->getChildList()->end(); ++iter)
			{
				stack.push_back(*iter);
			}
		}
	}
}

//virtual
void LLFloaterPreference::onClose(bool app_quitting)
{
	gSavedSettings.setS32("LastPrefTab", getChild<LLTabContainer>("pref core")->getCurrentPanelIndex());
	LLPanelLogin::setAlwaysRefresh(false);
	if (!app_quitting)
// [SL:KB] - Patch: Preferences-General | Checked: 2014-03-03 (Catznip-3.6)
	// Don't call cancel() if the user closed the floater by clicking the "OK" or "Cancel" button
	if ( (!app_quitting) && (mCancelOnClose) )
// [/SL:KB]
	{
		cancel();
	}
}

// static 
void LLFloaterPreference::onBtnOK(const LLSD& userdata)
{
	// commit any outstanding text entry
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}

	if (canClose())
	{
//		saveSettings();
		apply();
// [SL:KB] - Patch: Preferences-General | Checked: 2014-03-03 (Catznip-3.6)
		mCancelOnClose = false;
// [/SL:KB]
		
		if (userdata.asString() == "closeadvanced")
		{
			LLFloaterReg::hideInstance("prefs_graphics_advanced");
		}
		else
		{
			closeFloater(false);
		}

		//Conversation transcript and log path changed so reload conversations based on new location
		if(mPriorInstantMessageLogPath.length())
		{
			if(moveTranscriptsAndLog())
			{
				//When floaters are empty but have a chat history files, reload chat history into them
				LLFloaterIMSessionTab::reloadEmptyFloaters();
			}
			//Couldn't move files so restore the old path and show a notification
			else
			{
				gSavedPerAccountSettings.setString("InstantMessageLogPath", mPriorInstantMessageLogPath);
				LLNotificationsUtil::add("PreferenceChatPathChanged");
			}
			mPriorInstantMessageLogPath.clear();
		}

		LLUIColorTable::instance().saveUserSettings();
		gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);
// [SL:KB] - Patch: Settings-Troubleshooting | Checked: 2013-08-11 (Catznip-3.6)
		gStartupSettings.saveToFile(gSavedSettings.getString("StartupSettingsFile"), TRUE);
// [/SL:KB]
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2011-10-02 (Catznip-2.8)
		// We need to save all crash settings, even if they're defaults [see LLCrashLogger::loadCrashBehaviorSetting()]
		gCrashSettings.saveToFile(gSavedSettings.getString("CrashSettingsFile"), FALSE);
// [/SL:KB]
		
		//Only save once logged in and loaded per account settings
		if(mGotPersonalInfo)
		{
			gSavedPerAccountSettings.saveToFile(gSavedSettings.getString("PerAccountSettingsFile"), TRUE);
	}
	}
	else
	{
		// Show beep, pop up dialog, etc.
		LL_INFOS() << "Can't close preferences!" << LL_ENDL;
	}

	LLPanelLogin::updateLocationSelectorsVisibility();	
	//Need to reload the navmesh if the pathing console is up
	LLHandle<LLFloaterPathfindingConsole> pathfindingConsoleHandle = LLFloaterPathfindingConsole::getInstanceHandle();
	if ( !pathfindingConsoleHandle.isDead() )
	{
		LLFloaterPathfindingConsole* pPathfindingConsole = pathfindingConsoleHandle.get();
		pPathfindingConsole->onRegionBoundaryCross();
	}
}

// static 
void LLFloaterPreference::onBtnCancel(const LLSD& userdata)
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
		refresh();
	}
	cancel();
// [SL:KB] - Patch: Preferences-General | Checked: 2014-03-03 (Catznip-3.6)
	mCancelOnClose = false;
// [/SL:KB]

	if (userdata.asString() == "closeadvanced")
	{
		LLFloaterReg::hideInstance("prefs_graphics_advanced");
	}
	else
	{
		closeFloater();
	}
}

// static 
void LLFloaterPreference::updateUserInfo(const std::string& visibility, bool im_via_email, bool is_verified_email)
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance)
	{
        instance->setPersonalInfo(visibility, im_via_email, is_verified_email);
	}
}

void LLFloaterPreference::refreshEnabledGraphics()
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance)
	{
		instance->refresh();
	}

	LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced");
	if (advanced)
	{
		advanced->refresh();
	}
}

void LLFloaterPreference::onClickClearCache()
{
	LLNotificationsUtil::add("ConfirmClearCache", LLSD(), LLSD(), callback_clear_cache);
}

void LLFloaterPreference::onClickBrowserClearCache()
{
	LLNotificationsUtil::add("ConfirmClearBrowserCache", LLSD(), LLSD(), callback_clear_browser_cache);
}

// Called when user changes language via the combobox.
void LLFloaterPreference::onLanguageChange()
{
	// Let the user know that the change will only take effect after restart.
	// Do it only once so that we're not too irritating.
	if (!mLanguageChanged)
	{
		LLNotificationsUtil::add("ChangeLanguage");
		mLanguageChanged = true;
	}
}

//void LLFloaterPreference::onNotificationsChange(const std::string& OptionName)
//{
//	mNotificationOptions[OptionName] = getChild<LLComboBox>(OptionName)->getSelectedItemLabel();
//
//	bool show_notifications_alert = true;
//	for (notifications_map::iterator it_notification = mNotificationOptions.begin(); it_notification != mNotificationOptions.end(); it_notification++)
//	{
//		if(it_notification->second != "No action")
//		{
//			show_notifications_alert = false;
//			break;
//		}
//	}
//
//	getChild<LLTextBox>("notifications_alert")->setVisible(show_notifications_alert);
//}

void LLFloaterPreference::onNameTagOpacityChange(const LLSD& newvalue)
{
	LLColorSwatchCtrl* color_swatch = findChild<LLColorSwatchCtrl>("background");
	if (color_swatch)
	{
		LLColor4 new_color = color_swatch->get();
		color_swatch->set( new_color.setAlpha(newvalue.asReal()) );
	}
}

void LLFloaterPreference::onClickSetCache()
{
	std::string cur_name(gSavedSettings.getString("CacheLocation"));
//	std::string cur_top_folder(gDirUtilp->getBaseFileName(cur_name));
	
	std::string proposed_name(cur_name);

	(new LLDirPickerThread(boost::bind(&LLFloaterPreference::changeCachePath, this, _1, _2), proposed_name))->getFile();
}

void LLFloaterPreference::changeCachePath(const std::vector<std::string>& filenames, std::string proposed_name)
{
	std::string dir_name = filenames[0];
	if (!dir_name.empty() && dir_name != proposed_name)
	{
		std::string new_top_folder(gDirUtilp->getBaseFileName(dir_name));
		LLNotificationsUtil::add("CacheWillBeMoved");
		gSavedSettings.setString("NewCacheLocation", dir_name);
		gSavedSettings.setString("NewCacheLocationTopFolder", new_top_folder);
	}
	else
	{
		std::string cache_location = gDirUtilp->getCacheDir();
		gSavedSettings.setString("CacheLocation", cache_location);
		std::string top_folder(gDirUtilp->getBaseFileName(cache_location));
		gSavedSettings.setString("CacheLocationTopFolder", top_folder);
	}
}

void LLFloaterPreference::onClickResetCache()
{
	if (gDirUtilp->getCacheDir(false) == gDirUtilp->getCacheDir(true))
	{
		// The cache location was already the default.
		return;
	}
	gSavedSettings.setString("NewCacheLocation", "");
	gSavedSettings.setString("NewCacheLocationTopFolder", "");
	LLNotificationsUtil::add("CacheWillBeMoved");
	std::string cache_location = gDirUtilp->getCacheDir(false);
	gSavedSettings.setString("CacheLocation", cache_location);
	std::string top_folder(gDirUtilp->getBaseFileName(cache_location));
	gSavedSettings.setString("CacheLocationTopFolder", top_folder);
}

//void LLFloaterPreference::onClickSkin(LLUICtrl* ctrl, const LLSD& userdata)
//{
//	gSavedSettings.setString("SkinCurrent", userdata.asString());
//	ctrl->setValue(userdata.asString());
//}
//
//void LLFloaterPreference::onSelectSkin()
//{
//	std::string skin_selection = getChild<LLRadioGroup>("skin_selection")->getValue().asString();
//	gSavedSettings.setString("SkinCurrent", skin_selection);
//}
//
//void LLFloaterPreference::refreshSkin(void* data)
//{
//	LLPanel*self = (LLPanel*)data;
//	sSkin = gSavedSettings.getString("SkinCurrent");
//	self->getChild<LLRadioGroup>("skin_selection", true)->setValue(sSkin);
//}

void LLFloaterPreference::buildPopupLists()
{
	LLScrollListCtrl& disabled_popups =
		getChildRef<LLScrollListCtrl>("disabled_popups");
	LLScrollListCtrl& enabled_popups =
		getChildRef<LLScrollListCtrl>("enabled_popups");
	
	disabled_popups.deleteAllItems();
	enabled_popups.deleteAllItems();
	
	for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd();
		 ++iter)
	{
		LLNotificationTemplatePtr templatep = iter->second;
		LLNotificationFormPtr formp = templatep->mForm;
		
		LLNotificationForm::EIgnoreType ignore = formp->getIgnoreType();
		if (ignore == LLNotificationForm::IGNORE_NO)
			continue;
		
		LLSD row;
		row["columns"][0]["value"] = formp->getIgnoreMessage();
		row["columns"][0]["font"] = "SANSSERIF_SMALL";
		row["columns"][0]["width"] = 400;
		
		LLScrollListItem* item = NULL;
		
		bool show_popup = !formp->getIgnored();
		if (!show_popup)
		{
			if (ignore == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
			{
				LLSD last_response = LLUI::sSettingGroups["config"]->getLLSD("Default" + templatep->mName);
				if (!last_response.isUndefined())
				{
					for (LLSD::map_const_iterator it = last_response.beginMap();
						 it != last_response.endMap();
						 ++it)
					{
						if (it->second.asBoolean())
						{
							row["columns"][1]["value"] = formp->getElement(it->first)["ignore"].asString();
							break;
						}
					}
				}
				row["columns"][1]["font"] = "SANSSERIF_SMALL";
				row["columns"][1]["width"] = 360;
			}
			item = disabled_popups.addElement(row);
		}
		else
		{
			item = enabled_popups.addElement(row);
		}
		
		if (item)
		{
			item->setUserdata((void*)&iter->first);
		}
	}
}

void LLFloaterPreference::refreshEnabledState()
{
	LLCheckBoxCtrl* ctrl_wind_light = getChild<LLCheckBoxCtrl>("WindLightUseAtmosShaders");
	LLCheckBoxCtrl* ctrl_deferred = getChild<LLCheckBoxCtrl>("UseLightShaders");

	// if vertex shaders off, disable all shader related products
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable") ||
		!LLFeatureManager::getInstance()->isFeatureAvailable("WindLightUseAtmosShaders"))
	{
		ctrl_wind_light->setEnabled(FALSE);
		ctrl_wind_light->setValue(FALSE);
	}
	else
	{
		ctrl_wind_light->setEnabled(gSavedSettings.getBOOL("VertexShaderEnable"));
	}

	//Deferred/SSAO/Shadows
	BOOL bumpshiny = gGLManager.mHasCubeMap && LLCubeMap::sUseCubeMaps && LLFeatureManager::getInstance()->isFeatureAvailable("RenderObjectBump") && gSavedSettings.getBOOL("RenderObjectBump");
	BOOL shaders = gSavedSettings.getBOOL("WindLightUseAtmosShaders") && gSavedSettings.getBOOL("VertexShaderEnable");
	BOOL enabled = LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred") &&
						bumpshiny &&
						shaders && 
						gGLManager.mHasFramebufferObject &&
						gSavedSettings.getBOOL("RenderAvatarVP") &&
						(ctrl_wind_light->get()) ? TRUE : FALSE;

	ctrl_deferred->setEnabled(enabled);

	// Cannot have floater active until caps have been received
	getChild<LLButton>("default_creation_permissions")->setEnabled(LLStartUp::getStartupState() < STATE_STARTED ? false : true);

	getChildView("block_list")->setEnabled(LLLoginInstance::getInstance()->authSuccess());
}

void LLFloaterPreferenceGraphicsAdvanced::refreshEnabledState()
{
	LLComboBox* ctrl_reflections = getChild<LLComboBox>("Reflections");
	LLTextBox* reflections_text = getChild<LLTextBox>("ReflectionsText");

// [RLVa:KB] - Checked: 2013-05-11 (RLVa-1.4.9)
	if (rlv_handler_t::isEnabled())
	{
		getChild<LLUICtrl>("do_not_disturb_response")->setEnabled(!RlvActions::hasBehaviour(RLV_BHVR_SENDIM));
	}
// [/RLVa:KB]

	// Reflections
	BOOL reflections = gSavedSettings.getBOOL("VertexShaderEnable") 
		&& gGLManager.mHasCubeMap
		&& LLCubeMap::sUseCubeMaps;
	ctrl_reflections->setEnabled(reflections);
	reflections_text->setEnabled(reflections);
	
	// Bump & Shiny	
	LLCheckBoxCtrl* bumpshiny_ctrl = getChild<LLCheckBoxCtrl>("BumpShiny");
	bool bumpshiny = gGLManager.mHasCubeMap && LLCubeMap::sUseCubeMaps && LLFeatureManager::getInstance()->isFeatureAvailable("RenderObjectBump");
	bumpshiny_ctrl->setEnabled(bumpshiny ? TRUE : FALSE);
    
	// Avatar Mode
	// Enable Avatar Shaders
	LLCheckBoxCtrl* ctrl_avatar_vp = getChild<LLCheckBoxCtrl>("AvatarVertexProgram");
	// Avatar Render Mode
	LLCheckBoxCtrl* ctrl_avatar_cloth = getChild<LLCheckBoxCtrl>("AvatarCloth");
	
	bool avatar_vp_enabled = LLFeatureManager::getInstance()->isFeatureAvailable("RenderAvatarVP");
	if (LLViewerShaderMgr::sInitialized)
	{
		S32 max_avatar_shader = LLViewerShaderMgr::instance()->mMaxAvatarShaderLevel;
		avatar_vp_enabled = (max_avatar_shader > 0) ? TRUE : FALSE;
	}

	ctrl_avatar_vp->setEnabled(avatar_vp_enabled);
	
	if (gSavedSettings.getBOOL("VertexShaderEnable") == FALSE || 
		gSavedSettings.getBOOL("RenderAvatarVP") == FALSE)
	{
		ctrl_avatar_cloth->setEnabled(FALSE);
	} 
	else
	{
		ctrl_avatar_cloth->setEnabled(TRUE);
	}
	
	// Vertex Shaders
	// Global Shader Enable
	LLCheckBoxCtrl* ctrl_shader_enable   = getChild<LLCheckBoxCtrl>("BasicShaders");
	LLSliderCtrl* terrain_detail = getChild<LLSliderCtrl>("TerrainDetail");   // can be linked with control var
	LLTextBox* terrain_text = getChild<LLTextBox>("TerrainDetailText");

//	ctrl_shader_enable->setEnabled(LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable"));
// [RLVa:KB] - Checked: 2010-03-18 (RLVa-1.2.0a) | Modified: RLVa-0.2.0a
	// "Basic Shaders" can't be disabled - but can be enabled - under @setenv=n
	bool fCtrlShaderEnable = LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable");
	ctrl_shader_enable->setEnabled(
		fCtrlShaderEnable && ((!gRlvHandler.hasBehaviour(RLV_BHVR_SETENV)) || (!gSavedSettings.getBOOL("VertexShaderEnable"))) );
// [/RLVa:KB]

	BOOL shaders = ctrl_shader_enable->get();
	if (shaders)
	{
		terrain_detail->setEnabled(FALSE);
		terrain_text->setEnabled(FALSE);
	}
	else
	{
		terrain_detail->setEnabled(TRUE);
		terrain_text->setEnabled(TRUE);
	}
	
	// WindLight
	LLCheckBoxCtrl* ctrl_wind_light = getChild<LLCheckBoxCtrl>("WindLightUseAtmosShaders");
	LLSliderCtrl* sky = getChild<LLSliderCtrl>("SkyMeshDetail");
	LLTextBox* sky_text = getChild<LLTextBox>("SkyMeshDetailText");

	// *HACK just checks to see if we can use shaders... 
	// maybe some cards that use shaders, but don't support windlight
//	ctrl_wind_light->setEnabled(ctrl_shader_enable->getEnabled() && shaders);
// [RLVa:KB] - Checked: 2010-03-18 (RLVa-1.2.0a) | Modified: RLVa-0.2.0a
	// "Atmospheric Shaders" can't be disabled - but can be enabled - under @setenv=n
	bool fCtrlWindLightEnable = fCtrlShaderEnable && shaders;
	ctrl_wind_light->setEnabled(
		fCtrlWindLightEnable && ((!gRlvHandler.hasBehaviour(RLV_BHVR_SETENV)) || (!gSavedSettings.getBOOL("WindLightUseAtmosShaders"))) );
// [/RLVa:KB]

	sky->setEnabled(ctrl_wind_light->get() && shaders);
	sky_text->setEnabled(ctrl_wind_light->get() && shaders);

	//Deferred/SSAO/Shadows
	LLCheckBoxCtrl* ctrl_deferred = getChild<LLCheckBoxCtrl>("UseLightShaders");
	
	BOOL enabled = LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred") &&
						((bumpshiny_ctrl && bumpshiny_ctrl->get()) ? TRUE : FALSE) &&
						shaders && 
						gGLManager.mHasFramebufferObject &&
						gSavedSettings.getBOOL("RenderAvatarVP") &&
						(ctrl_wind_light->get()) ? TRUE : FALSE;

	ctrl_deferred->setEnabled(enabled);

	LLCheckBoxCtrl* ctrl_ssao = getChild<LLCheckBoxCtrl>("UseSSAO");
	LLCheckBoxCtrl* ctrl_dof = getChild<LLCheckBoxCtrl>("UseDoF");
	LLComboBox* ctrl_shadow = getChild<LLComboBox>("ShadowDetail");
	LLTextBox* shadow_text = getChild<LLTextBox>("RenderShadowDetailText");

	// note, okay here to get from ctrl_deferred as it's twin, ctrl_deferred2 will alway match it
	enabled = enabled && LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferredSSAO") && (ctrl_deferred->get() ? TRUE : FALSE);
	
	ctrl_deferred->set(gSavedSettings.getBOOL("RenderDeferred"));

	ctrl_ssao->setEnabled(enabled);
	ctrl_dof->setEnabled(enabled);

	enabled = enabled && LLFeatureManager::getInstance()->isFeatureAvailable("RenderShadowDetail");

	ctrl_shadow->setEnabled(enabled);
	shadow_text->setEnabled(enabled);

	// Hardware settings
	F32 mem_multiplier = gSavedSettings.getF32("RenderTextureMemoryMultiple");
	S32Megabytes min_tex_mem = LLViewerTextureList::getMinVideoRamSetting();
// [SL:DP] - Patch: Viewer-TextureMemory | Checked: Catznip-5.3
	S32Megabytes max_tex_mem = LLViewerTextureList::getMaxVideoRamSetting(true, mem_multiplier);
// [/SL:DP]
//	S32Megabytes max_tex_mem = LLViewerTextureList::getMaxVideoRamSetting(false, mem_multiplier);
	getChild<LLSliderCtrl>("GraphicsCardTextureMemory")->setMinValue(min_tex_mem.value());
	getChild<LLSliderCtrl>("GraphicsCardTextureMemory")->setMaxValue(max_tex_mem.value());

	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderVBOEnable") ||
		!gGLManager.mHasVertexBufferObject)
	{
		getChildView("vbo")->setEnabled(FALSE);
	}

	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderCompressTextures") ||
		!gGLManager.mHasVertexBufferObject)
	{
		getChildView("texture compression")->setEnabled(FALSE);
	}

// [SL:TD] - Patch: Settings-Misc | Checked: 2012-08-23 (Catznip-3.3)
	// Setting should be disabled when basic shaders is unchecked or deferred rendering is enabled
	LLCheckBoxCtrl* ctrl_glow = getChild<LLCheckBoxCtrl>("UseGlow");
	ctrl_glow->setEnabled((ctrl_shader_enable->get()) && (!ctrl_deferred->get()));
// [/SL:TD]

	// if no windlight shaders, turn off nighttime brightness, gamma, and fog distance
	LLUICtrl* gamma_ctrl = getChild<LLUICtrl>("gamma");
	gamma_ctrl->setEnabled(!gPipeline.canUseWindLightShaders());
	getChildView("(brightness, lower is brighter)")->setEnabled(!gPipeline.canUseWindLightShaders());
	getChildView("fog")->setEnabled(!gPipeline.canUseWindLightShaders());
	getChildView("antialiasing restart")->setVisible(!LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred"));

	// now turn off any features that are unavailable
	disableUnavailableSettings();

// [SL:KB] - Patch: Settings-Sounds | Checked: 2014-04-28 (Catznip-3.6)
	getChild<LLPanel>("soundalerts")->setEnabled(LLStartUp::getStartupState() == STATE_STARTED);
// [/SL:KB]
}

// static
void LLAvatarComplexityControls::setIndirectControls()
{
	/*
	 * We have controls that have an indirect relationship between the control
	 * values and adjacent text and the underlying setting they influence.
	 * In each case, the control and its associated setting are named Indirect<something>
	 * This method interrogates the controlled setting and establishes the
	 * appropriate value for the indirect control. It must be called whenever the
	 * underlying setting may have changed other than through the indirect control,
	 * such as when the 'Reset all to recommended settings' button is used...
	 */
	setIndirectMaxNonImpostors();
	setIndirectMaxArc();
}

// static
void LLAvatarComplexityControls::setIndirectMaxNonImpostors()
{
	U32 max_non_impostors = gSavedSettings.getU32("RenderAvatarMaxNonImpostors");
	// for this one, we just need to make zero, which means off, the max value of the slider
	U32 indirect_max_non_impostors = (0 == max_non_impostors) ? LLVOAvatar::IMPOSTORS_OFF : max_non_impostors;
	gSavedSettings.setU32("IndirectMaxNonImpostors", indirect_max_non_impostors);
}

void LLAvatarComplexityControls::setIndirectMaxArc()
{
	U32 max_arc = gSavedSettings.getU32("RenderAvatarMaxComplexity");
	U32 indirect_max_arc;
	if (0 == max_arc)
	{
		// the off position is all the way to the right, so set to control max
		indirect_max_arc = INDIRECT_MAX_ARC_OFF;
	}
	else
	{
		// This is the inverse of the calculation in updateMaxComplexity
		indirect_max_arc = (U32)ll_round(((log(F32(max_arc)) - MIN_ARC_LOG) / ARC_LIMIT_MAP_SCALE)) + MIN_INDIRECT_ARC_LIMIT;
	}
	gSavedSettings.setU32("IndirectMaxComplexity", indirect_max_arc);
}

void LLFloaterPreferenceGraphicsAdvanced::disableUnavailableSettings()
{	
	LLComboBox* ctrl_reflections   = getChild<LLComboBox>("Reflections");
	LLTextBox* reflections_text = getChild<LLTextBox>("ReflectionsText");
	LLCheckBoxCtrl* ctrl_avatar_vp     = getChild<LLCheckBoxCtrl>("AvatarVertexProgram");
	LLCheckBoxCtrl* ctrl_avatar_cloth  = getChild<LLCheckBoxCtrl>("AvatarCloth");
	LLCheckBoxCtrl* ctrl_shader_enable = getChild<LLCheckBoxCtrl>("BasicShaders");
// [SL:TD] - Patch: Settings-Misc | Checked: 2012-08-23 (Catznip-3.3)
	LLCheckBoxCtrl* ctrl_glow = getChild<LLCheckBoxCtrl>("UseGlow");
// [/SL:TD]
	LLCheckBoxCtrl* ctrl_wind_light    = getChild<LLCheckBoxCtrl>("WindLightUseAtmosShaders");
	LLCheckBoxCtrl* ctrl_deferred = getChild<LLCheckBoxCtrl>("UseLightShaders");
	LLComboBox* ctrl_shadows = getChild<LLComboBox>("ShadowDetail");
	LLTextBox* shadows_text = getChild<LLTextBox>("RenderShadowDetailText");
	LLCheckBoxCtrl* ctrl_ssao = getChild<LLCheckBoxCtrl>("UseSSAO");
	LLCheckBoxCtrl* ctrl_dof = getChild<LLCheckBoxCtrl>("UseDoF");
	LLSliderCtrl* sky = getChild<LLSliderCtrl>("SkyMeshDetail");
	LLTextBox* sky_text = getChild<LLTextBox>("SkyMeshDetailText");

	// if vertex shaders off, disable all shader related products
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable"))
	{
		ctrl_shader_enable->setEnabled(FALSE);
		ctrl_shader_enable->setValue(FALSE);
		
// [SL:TD] - Patch: Settings-Misc | Checked: 2012-08-23 (Catznip-3.3)
		ctrl_glow->setEnabled(FALSE);
		ctrl_glow->setValue(FALSE);
// [/SL:TD]

		ctrl_wind_light->setEnabled(FALSE);
		ctrl_wind_light->setValue(FALSE);

		sky->setEnabled(FALSE);
		sky_text->setEnabled(FALSE);

		ctrl_reflections->setEnabled(FALSE);
		ctrl_reflections->setValue(0);
		reflections_text->setEnabled(FALSE);
		
		ctrl_avatar_vp->setEnabled(FALSE);
		ctrl_avatar_vp->setValue(FALSE);
		
		ctrl_avatar_cloth->setEnabled(FALSE);
		ctrl_avatar_cloth->setValue(FALSE);

		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		shadows_text->setEnabled(FALSE);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_dof->setEnabled(FALSE);
		ctrl_dof->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}
	
	// disabled windlight
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("WindLightUseAtmosShaders"))
	{
		ctrl_wind_light->setEnabled(FALSE);
		ctrl_wind_light->setValue(FALSE);

		sky->setEnabled(FALSE);
		sky_text->setEnabled(FALSE);

		//deferred needs windlight, disable deferred
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		shadows_text->setEnabled(FALSE);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_dof->setEnabled(FALSE);
		ctrl_dof->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}

	// disabled deferred
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred") ||
		!gGLManager.mHasFramebufferObject)
	{
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		shadows_text->setEnabled(FALSE);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_dof->setEnabled(FALSE);
		ctrl_dof->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}
	
	// disabled deferred SSAO
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferredSSAO"))
	{
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);
	}
	
	// disabled deferred shadows
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderShadowDetail"))
	{
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		shadows_text->setEnabled(FALSE);
	}

	// disabled reflections
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderReflectionDetail"))
	{
		ctrl_reflections->setEnabled(FALSE);
		ctrl_reflections->setValue(FALSE);
		reflections_text->setEnabled(FALSE);
	}
	
	// disabled av
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderAvatarVP"))
	{
		ctrl_avatar_vp->setEnabled(FALSE);
		ctrl_avatar_vp->setValue(FALSE);
		
		ctrl_avatar_cloth->setEnabled(FALSE);
		ctrl_avatar_cloth->setValue(FALSE);

		//deferred needs AvatarVP, disable deferred
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		shadows_text->setEnabled(FALSE);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_dof->setEnabled(FALSE);
		ctrl_dof->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}

	// disabled cloth
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderAvatarCloth"))
	{
		ctrl_avatar_cloth->setEnabled(FALSE);
		ctrl_avatar_cloth->setValue(FALSE);
	}
}

void LLFloaterPreference::refresh()
{
	LLPanel::refresh();
    LLAvatarComplexityControls::setText(
        gSavedSettings.getU32("RenderAvatarMaxComplexity"),
        getChild<LLTextBox>("IndirectMaxComplexityText", true));
	refreshEnabledState();
	LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced");
	if (advanced)
	{
		advanced->refresh();
	}
}

void LLFloaterPreferenceGraphicsAdvanced::refresh()
{
	getChild<LLUICtrl>("fsaa")->setValue((LLSD::Integer)  gSavedSettings.getU32("RenderFSAASamples"));

	// sliders and their text boxes
	//	mPostProcess = gSavedSettings.getS32("RenderGlowResolutionPow");
	// slider text boxes
	updateSliderText(getChild<LLSliderCtrl>("ObjectMeshDetail",		true), getChild<LLTextBox>("ObjectMeshDetailText",		true));
	updateSliderText(getChild<LLSliderCtrl>("FlexibleMeshDetail",	true), getChild<LLTextBox>("FlexibleMeshDetailText",	true));
	updateSliderText(getChild<LLSliderCtrl>("TreeMeshDetail",		true), getChild<LLTextBox>("TreeMeshDetailText",		true));
	updateSliderText(getChild<LLSliderCtrl>("AvatarMeshDetail",		true), getChild<LLTextBox>("AvatarMeshDetailText",		true));
	updateSliderText(getChild<LLSliderCtrl>("AvatarPhysicsDetail",	true), getChild<LLTextBox>("AvatarPhysicsDetailText",		true));
	updateSliderText(getChild<LLSliderCtrl>("TerrainMeshDetail",	true), getChild<LLTextBox>("TerrainMeshDetailText",		true));
	updateSliderText(getChild<LLSliderCtrl>("RenderPostProcess",	true), getChild<LLTextBox>("PostProcessText",			true));
	updateSliderText(getChild<LLSliderCtrl>("SkyMeshDetail",		true), getChild<LLTextBox>("SkyMeshDetailText",			true));
	updateSliderText(getChild<LLSliderCtrl>("TerrainDetail",		true), getChild<LLTextBox>("TerrainDetailText",			true));	
    LLAvatarComplexityControls::setIndirectControls();
	setMaxNonImpostorsText(
        gSavedSettings.getU32("RenderAvatarMaxNonImpostors"),
        getChild<LLTextBox>("IndirectMaxNonImpostorsText", true));
    LLAvatarComplexityControls::setText(
        gSavedSettings.getU32("RenderAvatarMaxComplexity"),
        getChild<LLTextBox>("IndirectMaxComplexityText", true));
	refreshEnabledState();
}

void LLFloaterPreference::onCommitWindowedMode()
{
	refresh();
}

void LLFloaterPreference::onChangeQuality(const LLSD& data)
{
	U32 level = (U32)(data.asReal());
	LLFeatureManager::getInstance()->setGraphicsLevel(level, true);
	refreshEnabledGraphics();
	refresh();
}

void LLFloaterPreference::onClickSetKey()
{
	LLVoiceSetKeyDialog* dialog = LLFloaterReg::showTypedInstance<LLVoiceSetKeyDialog>("voice_set_key", LLSD(), TRUE);
	if (dialog)
	{
		dialog->setParent(this);
	}
}

void LLFloaterPreference::setKey(KEY key)
{
	getChild<LLUICtrl>("modifier_combo")->setValue(LLKeyboard::stringFromKey(key));
	// update the control right away since we no longer wait for apply
	getChild<LLUICtrl>("modifier_combo")->onCommit();
}

void LLFloaterPreference::onClickSetMiddleMouse()
{
	LLUICtrl* p2t_line_editor = getChild<LLUICtrl>("modifier_combo");

	// update the control right away since we no longer wait for apply
	p2t_line_editor->setControlValue(MIDDLE_MOUSE_CV);

	//push2talk button "middle mouse" control value is in English, need to localize it for presentation
	LLPanel* advanced_preferences = dynamic_cast<LLPanel*>(p2t_line_editor->getParent());
	if (advanced_preferences)
	{
		p2t_line_editor->setValue(advanced_preferences->getString("middle_mouse"));
	}
}

void LLFloaterPreference::onClickSetSounds()
{
	// Disable Enable gesture sounds checkbox if the master sound is disabled 
	// or if sound effects are disabled.
	getChild<LLCheckBoxCtrl>("gesture_audio_play_btn")->setEnabled(!gSavedSettings.getBOOL("MuteSounds"));
}

/*
void LLFloaterPreference::onClickSkipDialogs()
{
	LLNotificationsUtil::add("SkipShowNextTimeDialogs", LLSD(), LLSD(), boost::bind(&callback_skip_dialogs, _1, _2, this));
}

void LLFloaterPreference::onClickResetDialogs()
{
	LLNotificationsUtil::add("ResetShowNextTimeDialogs", LLSD(), LLSD(), boost::bind(&callback_reset_dialogs, _1, _2, this));
}
 */

void LLFloaterPreference::onClickEnablePopup()
{	
	LLScrollListCtrl& disabled_popups = getChildRef<LLScrollListCtrl>("disabled_popups");
	
	std::vector<LLScrollListItem*> items = disabled_popups.getAllSelected();
	std::vector<LLScrollListItem*>::iterator itor;
	for (itor = items.begin(); itor != items.end(); ++itor)
	{
		LLNotificationTemplatePtr templatep = LLNotifications::instance().getTemplate(*(std::string*)((*itor)->getUserdata()));
		//gSavedSettings.setWarning(templatep->mName, TRUE);
		std::string notification_name = templatep->mName;
		LLUI::sSettingGroups["ignores"]->setBOOL(notification_name, TRUE);
	}
	
	buildPopupLists();
}

void LLFloaterPreference::onClickDisablePopup()
{	
	LLScrollListCtrl& enabled_popups = getChildRef<LLScrollListCtrl>("enabled_popups");
	
	std::vector<LLScrollListItem*> items = enabled_popups.getAllSelected();
	std::vector<LLScrollListItem*>::iterator itor;
	for (itor = items.begin(); itor != items.end(); ++itor)
	{
		LLNotificationTemplatePtr templatep = LLNotifications::instance().getTemplate(*(std::string*)((*itor)->getUserdata()));
		templatep->mForm->setIgnored(true);
	}
	
	buildPopupLists();
}

void LLFloaterPreference::resetAllIgnored()
{
	for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd();
		 ++iter)
	{
		if (iter->second->mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
		{
			iter->second->mForm->setIgnored(false);
		}
	}
}

void LLFloaterPreference::setAllIgnored()
{
	for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd();
		 ++iter)
	{
		if (iter->second->mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
		{
			iter->second->mForm->setIgnored(true);
		}
	}
}

void LLFloaterPreference::onClickLogPath()
{
	std::string proposed_name(gSavedPerAccountSettings.getString("InstantMessageLogPath"));	 
	mPriorInstantMessageLogPath.clear();
	

	(new LLDirPickerThread(boost::bind(&LLFloaterPreference::changeLogPath, this, _1, _2), proposed_name))->getFile();
}

void LLFloaterPreference::changeLogPath(const std::vector<std::string>& filenames, std::string proposed_name)
{
	//Path changed
	if (proposed_name != filenames[0])
	{
		gSavedPerAccountSettings.setString("InstantMessageLogPath", filenames[0]);
		mPriorInstantMessageLogPath = proposed_name;

		// enable/disable 'Delete transcripts button
		updateDeleteTranscriptsButton();
	}
}

// [SL:KB] - Patch: Settings-Snapshot | Checked: Catznip-3.2
void LLFloaterPreference::onClickSnapshotPath()
{
	std::string proposed_name(gSavedSettings.getString("SnapshotLocalPath"));
	(new LLDirPickerThread(boost::bind(&LLFloaterPreference::onClickSnapshotPathCallback, this, _1, _2), proposed_name))->getFile();
}

void LLFloaterPreference::onClickSnapshotPathCallback(const std::vector<std::string>& filenames, std::string proposed_name)
{
	// Path changed
	if (proposed_name != filenames[0])
	{
		gSavedSettings.setString("SnapshotLocalPath", filenames[0]);
		gDirUtilp->setSnapshotDir(filenames[0]);
	}
}
// [/SL:KB]

bool LLFloaterPreference::moveTranscriptsAndLog()
{
	std::string instantMessageLogPath(gSavedPerAccountSettings.getString("InstantMessageLogPath"));
	std::string chatLogPath = gDirUtilp->add(instantMessageLogPath, gDirUtilp->getUserName());

	bool madeDirectory = false;

	//Does the directory really exist, if not then make it
	if(!LLFile::isdir(chatLogPath))
	{
		//mkdir success is defined as zero
		if(LLFile::mkdir(chatLogPath) != 0)
		{
			return false;
		}
		madeDirectory = true;
	}
	
	std::string originalConversationLogDir = LLConversationLog::instance().getFileName();
	std::string targetConversationLogDir = gDirUtilp->add(chatLogPath, "conversation.log");
	//Try to move the conversation log
	if(!LLConversationLog::instance().moveLog(originalConversationLogDir, targetConversationLogDir))
	{
		//Couldn't move the log and created a new directory so remove the new directory
		if(madeDirectory)
		{
			LLFile::rmdir(chatLogPath);
		}
		return false;
	}

	//Attempt to move transcripts
	std::vector<std::string> listOfTranscripts;
	std::vector<std::string> listOfFilesMoved;

	LLLogChat::getListOfTranscriptFiles(listOfTranscripts);

	if(!LLLogChat::moveTranscripts(gDirUtilp->getChatLogsDir(), 
									instantMessageLogPath, 
									listOfTranscripts,
									listOfFilesMoved))
	{
		//Couldn't move all the transcripts so restore those that moved back to their old location
		LLLogChat::moveTranscripts(instantMessageLogPath, 
			gDirUtilp->getChatLogsDir(), 
			listOfFilesMoved);

		//Move the conversation log back
		LLConversationLog::instance().moveLog(targetConversationLogDir, originalConversationLogDir);

		if(madeDirectory)
		{
			LLFile::rmdir(chatLogPath);
		}

		return false;
	}

	gDirUtilp->setChatLogsDir(instantMessageLogPath);
	gDirUtilp->updatePerAccountChatLogsDir();

	return true;
}

void LLFloaterPreference::setPersonalInfo(const std::string& visibility, bool im_via_email, bool is_verified_email)
{
	mGotPersonalInfo = true;
	mOriginalIMViaEmail = im_via_email;
	mDirectoryVisibility = visibility;
	
	if (visibility == VISIBILITY_DEFAULT)
	{
		mOriginalHideOnlineStatus = false;
		getChildView("online_visibility")->setEnabled(TRUE); 	 
	}
	else if (visibility == VISIBILITY_HIDDEN)
	{
		mOriginalHideOnlineStatus = true;
		getChildView("online_visibility")->setEnabled(TRUE); 	 
	}
	else
	{
		mOriginalHideOnlineStatus = true;
	}
	
	getChild<LLUICtrl>("online_searchresults")->setEnabled(TRUE);
	getChildView("friends_online_notify_checkbox")->setEnabled(TRUE);
	getChild<LLUICtrl>("online_visibility")->setValue(mOriginalHideOnlineStatus); 	 
	getChild<LLUICtrl>("online_visibility")->setLabelArg("[DIR_VIS]", mDirectoryVisibility);
	getChildView("send_im_to_email")->setEnabled(is_verified_email);

    std::string tooltip;
    if (!is_verified_email)
        tooltip = getString("email_unverified_tooltip");

    getChildView("send_im_to_email")->setToolTip(tooltip);

    // *TODO: Show or hide verify email text here based on is_verified_email
    getChild<LLUICtrl>("send_im_to_email")->setValue(im_via_email);
	getChildView("favorites_on_login_check")->setEnabled(TRUE);
	getChildView("log_path_button")->setEnabled(TRUE);
// [SL:KB] - Patch: Chat-Logs | Checked: 2014-03-05 (Catznip-3.6)
	getChildView("log_nearby_chat")->setEnabled(TRUE);
	getChildView("log_instant_messages")->setEnabled(TRUE);
	getChildView("log_unread_messages")->setEnabled(TRUE);
// [/SL:KB]
// [SL:KB] - Patch: Preferences-Chat | Checked: 2014-07-02 (Catznip-3.6)
	getChildView("logfile_name_datestamp")->setEnabled(TRUE);
// [/SL:KB]
//	getChildView("chat_font_size")->setEnabled(TRUE);
	getChildView("conversation_log_combo")->setEnabled(TRUE);
}


void LLFloaterPreference::refreshUI()
{
	refresh();
}

void LLFloaterPreferenceGraphicsAdvanced::updateSliderText(LLSliderCtrl* ctrl, LLTextBox* text_box)
{
	if (text_box == NULL || ctrl== NULL)
		return;

	// get range and points when text should change
	F32 value = (F32)ctrl->getValue().asReal();
	F32 min = ctrl->getMinValue();
	F32 max = ctrl->getMaxValue();
	F32 range = max - min;
	llassert(range > 0);
	F32 midPoint = min + range / 3.0f;
	F32 highPoint = min + (2.0f * range / 3.0f);

	// choose the right text
	if (value < midPoint)
	{
		text_box->setText(LLTrans::getString("GraphicsQualityLow"));
	} 
	else if (value < highPoint)
	{
		text_box->setText(LLTrans::getString("GraphicsQualityMid"));
	}
	else
	{
		text_box->setText(LLTrans::getString("GraphicsQualityHigh"));
	}
}

// [SL:KB] - Patch: Appearance-Complexity | Checked: Catznip-4.1
void LLFloaterPreferenceGraphicsAdvanced::onMaxNonImpostorsChange()
{
	// Called when the IndirectMaxNonImpostors control changes
	// Responsible for fixing the slider label (IndirectMaxNonImpostorsText) and setting RenderAvatarMaxNonImpostors
	LLSliderCtrl* ctrl = getChild<LLSliderCtrl>("IndirectMaxNonImpostors",true);
	U32 value = ctrl->getValue().asInteger();

	updateMaxNonImpostors(value);

	setMaxNonImpostorsText(value, getChild<LLTextBox>("IndirectMaxNonImpostorsText"));
}

void LLFloaterPreferenceGraphicsAdvanced::updateMaxNonImpostors(U32 value)
{
	if (0 == value || LLVOAvatar::IMPOSTORS_OFF <= value)
	{
		value = 0;
	}
	gSavedSettings.setU32("RenderAvatarMaxNonImpostors", value);
	LLVOAvatar::updateImpostorRendering(value); // make it effective immediately
}
// [/SL:KB]
//void LLFloaterPreferenceGraphicsAdvanced::updateMaxNonImpostors()
//{
//	// Called when the IndirectMaxNonImpostors control changes
//	// Responsible for fixing the slider label (IndirectMaxNonImpostorsText) and setting RenderAvatarMaxNonImpostors
//	LLSliderCtrl* ctrl = getChild<LLSliderCtrl>("IndirectMaxNonImpostors", true);
//	U32 value = ctrl->getValue().asInteger();
//
//	if (0 == value || LLVOAvatar::IMPOSTORS_OFF <= value)
//	{
//		value = 0;
//	}
//	gSavedSettings.setU32("RenderAvatarMaxNonImpostors", value);
//	LLVOAvatar::updateImpostorRendering(value); // make it effective immediately
//	setMaxNonImpostorsText(value, getChild<LLTextBox>("IndirectMaxNonImpostorsText"));
//}

void LLFloaterPreferenceGraphicsAdvanced::setMaxNonImpostorsText(U32 value, LLTextBox* text_box)
{
	if (0 == value)
	{
		text_box->setText(LLTrans::getString("no_limit"));
	}
	else
	{
		text_box->setText(llformat("%d", value));
	}
}

void LLAvatarComplexityControls::updateMax(LLSliderCtrl* slider, LLTextBox* value_label)
{
	// Called when the IndirectMaxComplexity control changes
	// Responsible for fixing the slider label (IndirectMaxComplexityText) and setting RenderAvatarMaxComplexity
	U32 indirect_value = slider->getValue().asInteger();
	U32 max_arc;
	
	if (INDIRECT_MAX_ARC_OFF == indirect_value)
	{
		// The 'off' position is when the slider is all the way to the right, 
		// which is a value of INDIRECT_MAX_ARC_OFF,
		// so it is necessary to set max_arc to 0 disable muted avatars.
		max_arc = 0;
	}
	else
	{
		// if this is changed, the inverse calculation in setIndirectMaxArc
		// must be changed to match
		max_arc = (U32)ll_round(exp(MIN_ARC_LOG + (ARC_LIMIT_MAP_SCALE * (indirect_value - MIN_INDIRECT_ARC_LIMIT))));
	}

// [SL:KB] - Patch: Appearance-Complexity | Checked: Catznip-4.1
	// Make OCD people happy
	max_arc = (U32)((max_arc + 500) / 1000) * 1000;
// [/SL:KB]
	gSavedSettings.setU32("RenderAvatarMaxComplexity", (U32)max_arc);
	setText(max_arc, value_label);
}

void LLAvatarComplexityControls::setText(U32 value, LLTextBox* text_box)
{
	if (0 == value)
	{
		text_box->setText(LLTrans::getString("no_limit"));
	}
	else
	{
		text_box->setText(llformat("%d", value));
	}
}

void LLFloaterPreference::updateMaxComplexity()
{
	// Called when the IndirectMaxComplexity control changes
    LLAvatarComplexityControls::updateMax(
        getChild<LLSliderCtrl>("IndirectMaxComplexity"),
        getChild<LLTextBox>("IndirectMaxComplexityText"));
}

void LLFloaterPreferenceGraphicsAdvanced::updateMaxComplexity()
{
	// Called when the IndirectMaxComplexity control changes
    LLAvatarComplexityControls::updateMax(
        getChild<LLSliderCtrl>("IndirectMaxComplexity"),
        getChild<LLTextBox>("IndirectMaxComplexityText"));
}

void LLFloaterPreference::onChangeMaturity()
{
	U8 sim_access = gSavedSettings.getU32("PreferredMaturity");

	getChild<LLIconCtrl>("rating_icon_general")->setVisible(sim_access == SIM_ACCESS_PG
															|| sim_access == SIM_ACCESS_MATURE
															|| sim_access == SIM_ACCESS_ADULT);

	getChild<LLIconCtrl>("rating_icon_moderate")->setVisible(sim_access == SIM_ACCESS_MATURE
															|| sim_access == SIM_ACCESS_ADULT);

	getChild<LLIconCtrl>("rating_icon_adult")->setVisible(sim_access == SIM_ACCESS_ADULT);
}

std::string get_category_path(LLUUID cat_id)
{
    LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
    std::string localized_cat_name;
    if (!LLTrans::findString(localized_cat_name, "InvFolder " + cat->getName()))
    {
        localized_cat_name = cat->getName();
    }

    if (cat->getParentUUID().notNull())
    {
        return get_category_path(cat->getParentUUID()) + " > " + localized_cat_name;
    }
    else
    {
        return localized_cat_name;
    }
}

std::string get_category_path(LLFolderType::EType cat_type)
{
    LLUUID cat_id = gInventory.findUserDefinedCategoryUUIDForType(cat_type);
    return get_category_path(cat_id);
}

void LLFloaterPreference::onChangeModelFolder()
{
    if (gInventory.isInventoryUsable())
    {
        getChild<LLTextBox>("upload_models")->setText(get_category_path(LLFolderType::FT_OBJECT));
    }
}

void LLFloaterPreference::onChangeTextureFolder()
{
    if (gInventory.isInventoryUsable())
    {
        getChild<LLTextBox>("upload_textures")->setText(get_category_path(LLFolderType::FT_TEXTURE));
    }
}

void LLFloaterPreference::onChangeSoundFolder()
{
    if (gInventory.isInventoryUsable())
    {
        getChild<LLTextBox>("upload_sounds")->setText(get_category_path(LLFolderType::FT_SOUND));
    }
}

void LLFloaterPreference::onChangeAnimationFolder()
{
    if (gInventory.isInventoryUsable())
    {
        getChild<LLTextBox>("upload_animation")->setText(get_category_path(LLFolderType::FT_ANIMATION));
    }
}

// FIXME: this will stop you from spawning the sidetray from preferences dialog on login screen
// but the UI for this will still be enabled
void LLFloaterPreference::onClickBlockList()
{
// [SL:KB] - Patch: World-Derender | Checked: Catznip-3.2
	LLFloaterReg::showInstance("blocked");
// [/SL:KB]
//	LLFloaterSidePanelContainer::showPanel("people", "panel_people",
//		LLSD().with("people_panel_tab_name", "blocked_panel"));
}

void LLFloaterPreference::onClickProxySettings()
{
	LLFloaterReg::showInstance("prefs_proxy");
}

//void LLFloaterPreference::onClickTranslationSettings()
//{
//	LLFloaterReg::showInstance("prefs_translation");
//}

//void LLFloaterPreference::onClickAutoReplace()
//{
//	LLFloaterReg::showInstance("prefs_autoreplace");
//}

//void LLFloaterPreference::onClickSpellChecker()
//{
//    LLFloaterReg::showInstance("prefs_spellchecker");
//}

void LLFloaterPreference::onClickRenderExceptions()
{
// [SL:KB] - Patch: World-RenderExceptions | Checked: Catznip-5.2
	LLFloaterReg::showInstance("blocked", LLSD("avatar_rendering_tab"));
// [/SL:KB]
//    LLFloaterReg::showInstance("avatar_render_settings");
}

void LLFloaterPreference::onClickAdvanced()
{
	LLFloaterReg::showInstance("prefs_graphics_advanced");

	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	for (child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
		 iter != tabcontainer->getChildList()->end(); ++iter)
	{
		LLView* view = *iter;
		LLPanelPreferenceGraphics* panel = dynamic_cast<LLPanelPreferenceGraphics*>(view);
		if (panel)
		{
			panel->resetDirtyChilds();
		}
	}
}

void LLFloaterPreference::onClickActionChange()
{
	mClickActionDirty = true;
}

void LLFloaterPreference::onClickPermsDefault()
{
	LLFloaterReg::showInstance("perms_default");
}

void LLFloaterPreference::onDeleteTranscripts()
{
	LLSD args;
	args["FOLDER"] = gDirUtilp->getUserName();

	LLNotificationsUtil::add("PreferenceChatDeleteTranscripts", args, LLSD(), boost::bind(&LLFloaterPreference::onDeleteTranscriptsResponse, this, _1, _2));
}

void LLFloaterPreference::onDeleteTranscriptsResponse(const LLSD& notification, const LLSD& response)
{
	if (0 == LLNotificationsUtil::getSelectedOption(notification, response))
	{
		LLLogChat::deleteTranscripts();
		updateDeleteTranscriptsButton();
	}
}

void LLFloaterPreference::onLogChatHistorySaved()
{
	LLButton * delete_transcripts_buttonp = getChild<LLButton>("delete_transcripts");

	if (!delete_transcripts_buttonp->getEnabled())
	{
		delete_transcripts_buttonp->setEnabled(true);
	}
}

void LLFloaterPreference::updateClickActionSettings()
{
	const int single_clk_action = getChild<LLComboBox>("single_click_action_combo")->getValue().asInteger();
	const int double_clk_action = getChild<LLComboBox>("double_click_action_combo")->getValue().asInteger();

	gSavedSettings.setBOOL("ClickToWalk",			single_clk_action == 1);
	gSavedSettings.setBOOL("DoubleClickAutoPilot",	double_clk_action == 1);
	gSavedSettings.setBOOL("DoubleClickTeleport",	double_clk_action == 2);
}

void LLFloaterPreference::updateClickActionControls()
{
	const bool click_to_walk = gSavedSettings.getBOOL("ClickToWalk");
	const bool dbl_click_to_walk = gSavedSettings.getBOOL("DoubleClickAutoPilot");
	const bool dbl_click_to_teleport = gSavedSettings.getBOOL("DoubleClickTeleport");

	getChild<LLComboBox>("single_click_action_combo")->setValue((int)click_to_walk);
	getChild<LLComboBox>("double_click_action_combo")->setValue(dbl_click_to_teleport ? 2 : (int)dbl_click_to_walk);
}

void LLFloaterPreference::applyUIColor(LLUICtrl* ctrl, const LLSD& param)
{
	LLUIColorTable::instance().setColor(param.asString(), LLColor4(ctrl->getValue()));
}

// [SL:KB] - Patch: Settings-NameTags | Checked: 2014-05-17 (Catznip-3.6)
void LLFloaterPreference::applyNameTagColor(LLUICtrl* ctrl, const LLSD& param)
{
	applyUIColor(ctrl, param);
	LLVOAvatar::invalidateNameTags();
}
// [/SL:KB]

void LLFloaterPreference::getUIColor(LLUICtrl* ctrl, const LLSD& param)
{
	LLColorSwatchCtrl* color_swatch = (LLColorSwatchCtrl*) ctrl;
	color_swatch->setOriginal(LLUIColorTable::instance().getColor(param.asString()));
}

void LLFloaterPreference::setCacheLocation(const LLStringExplicit& location)
{
	LLUICtrl* cache_location_editor = getChild<LLUICtrl>("cache_location");
	cache_location_editor->setValue(location);
	cache_location_editor->setToolTip(location);
}

//void LLFloaterPreference::selectPanel(const LLSD& name)
//{
//	LLTabContainer * tab_containerp = getChild<LLTabContainer>("pref core");
//	LLPanel * panel = tab_containerp->getPanelByName(name);
//	if (NULL != panel)
//	{
//		tab_containerp->selectTabPanel(panel);
//	}
//}

void LLFloaterPreference::selectPrivacyPanel()
{
// [SL:KB] - Patch: Settings-Preferences | Checked: 2014-04-12 (Catznip-3.6)
	showPanel("im");
// [/SL:KB]
//	selectPanel("im");
}

void LLFloaterPreference::selectChatPanel()
{
// [SL:KB] - Patch: Settings-Preferences | Checked: 2014-04-12 (Catznip-3.6)
	showPanel("chat");
// [/SL:KB]
//	selectPanel("chat");
}

void LLFloaterPreference::changed()
{
//	getChild<LLButton>("clear_log")->setEnabled(LLConversationLog::instance().getConversations().size() > 0);

	// set 'enable' property for 'Delete transcripts...' button
	updateDeleteTranscriptsButton();

}

void LLFloaterPreference::saveGraphicsPreset(std::string& preset)
{
	mSavedGraphicsPreset = preset;
}

// [SL:KB] - Patch: UI-Font | Checked: 2012-10-10 (Catznip-3.3)
void initEditorFont(LLUICtrl* pCtrl)
{
	LLControlVariable* pControl = gSavedSettings.getControl("FontOverrideEditor");
	if ( (pControl) && (pCtrl) )
	{
		// This should be safe since the preferences floater is single instance and isn't destroyed until the viewer is shutting down
		LLHandle<LLUICtrl> hCtrl = pCtrl->getHandle();
		pControl->getSignal()->connect(boost::bind(&refreshEditorFont, hCtrl));
		refreshEditorFont(hCtrl);
	}
}

void refreshEditorFont(LLHandle<LLUICtrl> hCtrl)
{
	if (hCtrl.isDead())
		return;

	LLControlVariable* pControl = gSavedSettings.getControl("FontOverrideEditor");

	LLCheckBoxCtrl* pCheckCtrl = dynamic_cast<LLCheckBoxCtrl*>(hCtrl.get());
	if (pCheckCtrl)
	{
		pCheckCtrl->set(!pControl->isDefault());
	}

	LLComboBox* pComboCtrl = dynamic_cast<LLComboBox*>(hCtrl.get());
	if (pComboCtrl)
	{
		if (pControl->isDefault())
		{
			pComboCtrl->setEnabled(false);
			pComboCtrl->clear();
		}
		else
		{
			pComboCtrl->setEnabled(true);
		}
	}
}

void handleEditorFontToggle(LLUICtrl* pCheckCtrl)
{
	LLControlVariable* pControl = gSavedSettings.getControl("FontOverrideEditor");

	LLComboBox* pEditorCombo = pCheckCtrl->getParent()->findChild<LLComboBox>("font_editor_combo");
	if (!pCheckCtrl->getValue().asBoolean())
	{
		// When unchecked, revert to default (= use FontOverrideMain)
		pEditorCombo->setEnabled(false);
		pEditorCombo->clear();
		pControl->resetToDefault(true);
	}
	else
	{
		// When checked, use the current FontOverrideMain setting as a starting point
		pEditorCombo->setEnabled(true);
		std::string strValue = gSavedSettings.getString("FontOverrideMain");
		LLStringUtil::replaceString(strValue, "main", "editor");
		if (!pEditorCombo->setSelectedByValue(strValue, TRUE))
			pEditorCombo->selectFirstItem();
		pEditorCombo->onCommit();
	}
}
// [/SL:KB]

//------------------------------Updater---------------------------------------

static bool handleBandwidthChanged(const LLSD& newvalue)
{
	gViewerThrottle.setMaxBandwidth((F32) newvalue.asReal());
	return true;
}

class LLPanelPreference::Updater : public LLEventTimer
{

public:

	typedef boost::function<bool(const LLSD&)> callback_t;

	Updater(callback_t cb, F32 period)
	:LLEventTimer(period),
	 mCallback(cb)
	{
		mEventTimer.stop();
	}

	virtual ~Updater(){}

	void update(const LLSD& new_value)
	{
		mNewValue = new_value;
		mEventTimer.start();
	}

protected:

	BOOL tick()
	{
		mCallback(mNewValue);
		mEventTimer.stop();

		return FALSE;
	}

private:

	LLSD mNewValue;
	callback_t mCallback;
};
//----------------------------------------------------------------------------
static LLPanelInjector<LLPanelPreference> t_places("panel_preference");
LLPanelPreference::LLPanelPreference()
: LLPanel(),
  mBandWidthUpdater(NULL)
{
	mCommitCallbackRegistrar.add("Pref.setControlFalse",	boost::bind(&LLPanelPreference::setControlFalse,this, _2));
	mCommitCallbackRegistrar.add("Pref.updateMediaAutoPlayCheckbox",	boost::bind(&LLPanelPreference::updateMediaAutoPlayCheckbox, this, _1));
	mCommitCallbackRegistrar.add("Pref.PrefDelete",	boost::bind(&LLPanelPreference::deletePreset, this, _2));
	mCommitCallbackRegistrar.add("Pref.PrefSave",	boost::bind(&LLPanelPreference::savePreset, this, _2));
	mCommitCallbackRegistrar.add("Pref.PrefLoad",	boost::bind(&LLPanelPreference::loadPreset, this, _2));
}

//virtual
BOOL LLPanelPreference::postBuild()
{
	////////////////////// PanelGeneral ///////////////////
	if (hasChild("display_names_check", TRUE))
	{
		BOOL use_people_api = gSavedSettings.getBOOL("UsePeopleAPI");
		LLCheckBoxCtrl* ctrl_display_name = getChild<LLCheckBoxCtrl>("display_names_check");
		ctrl_display_name->setEnabled(use_people_api);
		if (!use_people_api)
		{
			ctrl_display_name->setValue(FALSE);
		}
	}

	////////////////////// PanelVoice ///////////////////
	if (hasChild("voice_unavailable", TRUE))
	{
		BOOL voice_disabled = gSavedSettings.getBOOL("CmdLineDisableVoice");
		getChildView("voice_unavailable")->setVisible( voice_disabled);
		getChildView("enable_voice_check")->setVisible( !voice_disabled);
	}
	
	//////////////////////PanelSkins ///////////////////
	
//	if (hasChild("skin_selection", TRUE))
//	{
//		LLFloaterPreference::refreshSkin(this);
//
//		// if skin is set to a skin that no longer exists (silver) set back to default
//		if (getChild<LLRadioGroup>("skin_selection")->getSelectedIndex() < 0)
//		{
//			gSavedSettings.setString("SkinCurrent", "default");
//			LLFloaterPreference::refreshSkin(this);
//		}
//
//	}

	//////////////////////PanelPrivacy ///////////////////
	if (hasChild("media_enabled", TRUE))
	{
		bool media_enabled = gSavedSettings.getBOOL("AudioStreamingMedia");
		
		getChild<LLCheckBoxCtrl>("media_enabled")->set(media_enabled);
		getChild<LLCheckBoxCtrl>("autoplay_enabled")->setEnabled(media_enabled);
	}
	if (hasChild("music_enabled", TRUE))
	{
		getChild<LLCheckBoxCtrl>("music_enabled")->set(gSavedSettings.getBOOL("AudioStreamingMusic"));
	}
	if (hasChild("voice_call_friends_only_check", TRUE))
	{
		getChild<LLCheckBoxCtrl>("voice_call_friends_only_check")->setCommitCallback(boost::bind(&showFriendsOnlyWarning, _1, _2));
	}
	if (hasChild("allow_multiple_viewer_check", TRUE))
	{
		getChild<LLCheckBoxCtrl>("allow_multiple_viewer_check")->setCommitCallback(boost::bind(&showMultipleViewersWarning, _1, _2));
	}
	if (hasChild("favorites_on_login_check", TRUE))
	{
		getChild<LLCheckBoxCtrl>("favorites_on_login_check")->setCommitCallback(boost::bind(&handleFavoritesOnLoginChanged, _1, _2));
		bool show_favorites_at_login = LLPanelLogin::getShowFavorites();
		getChild<LLCheckBoxCtrl>("favorites_on_login_check")->setValue(show_favorites_at_login);
	}
	if (hasChild("mute_chb_label", TRUE))
	{
		getChild<LLTextBox>("mute_chb_label")->setShowCursorHand(false);
		getChild<LLTextBox>("mute_chb_label")->setClickedCallback(boost::bind(&toggleMuteWhenMinimized));
	}

	//////////////////////PanelAdvanced ///////////////////
	if (hasChild("modifier_combo", TRUE))
	{
		//localizing if push2talk button is set to middle mouse
		if (MIDDLE_MOUSE_CV == getChild<LLUICtrl>("modifier_combo")->getValue().asString())
		{
			getChild<LLUICtrl>("modifier_combo")->setValue(getString("middle_mouse"));
		}
	}

	//////////////////////PanelSetup ///////////////////
	if (hasChild("max_bandwidth"), TRUE)
	{
		mBandWidthUpdater = new LLPanelPreference::Updater(boost::bind(&handleBandwidthChanged, _1), BANDWIDTH_UPDATER_TIMEOUT);
		gSavedSettings.getControl("ThrottleBandwidthKBPS")->getSignal()->connect(boost::bind(&LLPanelPreference::Updater::update, mBandWidthUpdater, _2));
	}

#ifdef EXTERNAL_TOS
	LLRadioGroup* ext_browser_settings = getChild<LLRadioGroup>("preferred_browser_behavior");
	if (ext_browser_settings)
	{
		// turn off ability to set external/internal browser
		ext_browser_settings->setSelectedByValue(LLWeb::BROWSER_EXTERNAL_ONLY, true);
		ext_browser_settings->setEnabled(false);
	}
#endif

// [SL:KB] - Checked: 2011-06-27 (Catznip-2.6)
	//////////////////////PanelCatznip ///////////////////
	if (hasChild("windowedfullscreen_check", TRUE))
	{
#ifdef CATZNIP
		getChild<LLCheckBoxCtrl>("windowedfullscreen_check")->setEnabled( (gViewerWindow) && (gViewerWindow->canFullscreenWindow()) );
		getChild<LLCheckBoxCtrl>("windowedfullscreen_check")->setValue( (gViewerWindow) && (gViewerWindow->getFullscreenWindow()) );
#endif // CATZNIP
	}
// [/SL:KB]

// [SL:KB] - Patch: Preferences-General | Checked: Catznip-3.6
	LLFloaterPreference* pFloater = getParentByType<LLFloaterPreference>();
	if (pFloater)
	{
		pFloater->registerPrefPanel(this);
		// We're using the close callback since onVisibilityChanged() will fire before the open callback does causing the currently selected panel to not
		// have refresh() called. Setting things up on close (and the default to true) causes 'mRefreshOnOpen' to be true on the panel visibility change
		pFloater->setCloseCallback(boost::bind(&LLPanelPreference::onParentFloaterClose, this));
	}
// [/SL:KB]
//	apply();
	return true;
}

LLPanelPreference::~LLPanelPreference()
{
	if (mBandWidthUpdater)
	{
		delete mBandWidthUpdater;
	}
}

// [SL:KB] - Patch: Preferences-General | Checked: Catznip-3.6
void LLPanelPreference::onVisibilityChange(BOOL new_visibility)
{
	LLPanel::onVisibilityChange(new_visibility);

	if (new_visibility)
	{
		if (!mInitialized)
		{
			init();
			saveSettings();
			mInitialized = true;
		}

		if (mRefreshOnOpen)
		{
			refresh();
			mRefreshOnOpen = false;
		}
		onOpen(LLSD());
	}
	else if (mInitialized)
	{
		onClose();
	}
}
// [/SL:KB]

// [SL:KB] - Patch: Preferences-Chat | Checked: 2014-03-04 (Catznip-3.6)
class LLPanelPreferenceChat : public LLPanelPreference
{
public:
	LLPanelPreferenceChat()
		: LLPanelPreference()
	{
		mCommitCallbackRegistrar.add("PrefChat.InitLogNotificationChat",boost::bind(&LLPanelPreferenceChat::onInitLogNotification, _1, _2, "chat"));
		mCommitCallbackRegistrar.add("PrefChat.InitLogNotificationIM",  boost::bind(&LLPanelPreferenceChat::onInitLogNotification, _1, _2, "im"));
		mCommitCallbackRegistrar.add("PrefChat.LogNotificationChat",    boost::bind(&LLPanelPreferenceChat::onToggleLogNotification, _1, _2, "chat"));
		mCommitCallbackRegistrar.add("PrefChat.LogNotificationIM",      boost::bind(&LLPanelPreferenceChat::onToggleLogNotification, _1, _2, "im"));
	}

	/*virtual*/ BOOL postBuild()
	{
		LLPanelPreference::postBuild();

		LLUICtrl* pCtrl = findChild<LLUICtrl>("nearbychattornoff_check", TRUE);
		if (pCtrl)
		{
			pCtrl->setCommitCallback(boost::bind(LLPanelPreferenceChat::onNearbyChatTornOffChanged));
		}

		return TRUE;
	}

protected:
	static void onInitLogNotification(LLUICtrl* pCtrl, const LLSD& sdParam, const char* pstrScope)
	{
#ifdef CATZNIP
		std::vector<std::string> notifications; const std::string strParam = sdParam.asString();
		boost::split(notifications, strParam, boost::is_any_of(std::string(",")));

		for (std::vector<std::string>::const_iterator itNotif = notifications.begin(); itNotif != notifications.end(); ++itNotif)
		{
			LLNotificationTemplatePtr templ = LLNotifications::instance().getTemplate(*itNotif);
			if (*itNotif != templ->mName)
				continue;

			if (0 == strcmp(pstrScope, "chat"))
				pCtrl->setValue(templ->canLogToNearbyChat());
			else if (0 == strcmp(pstrScope, "im"))
				pCtrl->setValue(templ->canLogToIM(true));
		}
#endif // CATZNIP
	}

	static void onToggleLogNotification(LLUICtrl* pCtrl, const LLSD& sdParam, const char* pstrScope)
	{
#ifdef CATZNIP
		std::vector<std::string> notifications; const std::string strParam = sdParam.asString();
		boost::split(notifications, strParam, boost::is_any_of(std::string(",")));

		for (std::vector<std::string>::const_iterator itNotif = notifications.begin(); itNotif != notifications.end(); ++itNotif)
		{
			LLNotificationTemplatePtr templ = LLNotifications::instance().getTemplate(*itNotif);
			if (*itNotif != templ->mName)
				continue;

			if (0 == strcmp(pstrScope, "chat"))
				templ->setLogToNearbyChat(pCtrl->getValue().asBoolean());
			else if (0 == strcmp(pstrScope, "im"))
				templ->setLogToIM(pCtrl->getValue().asBoolean());
		}
#endif // CATZNIP
	}

	static void onNearbyChatTornOffChanged()
	{
#ifdef CATZNIP
		LLFloater* pNearbyChat = LLFloaterReg::findInstance("nearby_chat");
		LLFloater* pConversations = LLFloaterReg::findInstance("im_container");

		if ( (pNearbyChat) && (pConversations) )
		{
			bool fNearbyVisible = pNearbyChat->isInVisibleChain();
			bool fConvVisible = pConversations->isInVisibleChain();

			bool fTornOff = !gSavedPerAccountSettings.getBOOL("NearbyChatIsNotTornOff");
			if ( (fTornOff) && (!pNearbyChat->isTornOff()) )
			{
				pNearbyChat->onTearOffClicked();
				pNearbyChat->setVisible(fNearbyVisible);
				pConversations->setVisible(fConvVisible);
			}
			else if ( (!fTornOff) && (pNearbyChat->isTornOff()) )
			{
				pNearbyChat->onTearOffClicked();
				pConversations->setVisible(fConvVisible || fNearbyVisible);
			}
		}
#endif // CATZNIP
	}
};

static LLPanelInjector<LLPanelPreferenceChat> t_pref_chat("panel_preference_chat");
// [/SL:KB]

void LLPanelPreference::apply()
{
	// no-op
}

void LLPanelPreference::saveSettings()
{
	LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced");

	// Save the value of all controls in the hierarchy
	mSavedValues.clear();
	std::list<LLView*> view_stack;
	view_stack.push_back(this);
	if (advanced)
	{
		view_stack.push_back(advanced);
	}
	while(!view_stack.empty())
	{
		// Process view on top of the stack
		LLView* curview = view_stack.front();
		view_stack.pop_front();

		LLColorSwatchCtrl* color_swatch = dynamic_cast<LLColorSwatchCtrl *>(curview);
		if (color_swatch)
		{
			mSavedColors[color_swatch->getName()] = color_swatch->get();
		}
		else
		{
			LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
			if (ctrl)
			{
				LLControlVariable* control = ctrl->getControlVariable();
				if (control)
				{
					mSavedValues[control] = control->getValue();
				}
			}
		}
			
		// Push children onto the end of the work stack
		for (child_list_t::const_iterator iter = curview->getChildList()->begin();
			 iter != curview->getChildList()->end(); ++iter)
		{
			view_stack.push_back(*iter);
		}
	}	
}

void LLPanelPreference::showMultipleViewersWarning(LLUICtrl* checkbox, const LLSD& value)
{
    if (checkbox && checkbox->getValue())
    {
        LLNotificationsUtil::add("AllowMultipleViewers");
    }
}

void LLPanelPreference::showFriendsOnlyWarning(LLUICtrl* checkbox, const LLSD& value)
{
	if (checkbox && checkbox->getValue())
	{
		LLNotificationsUtil::add("FriendsAndGroupsOnly");
	}
}

void LLPanelPreference::handleFavoritesOnLoginChanged(LLUICtrl* checkbox, const LLSD& value)
{
	if (checkbox)
	{
		LLFavoritesOrderStorage::instance().showFavoritesOnLoginChanged(checkbox->getValue().asBoolean());
		if(checkbox->getValue())
		{
			LLNotificationsUtil::add("FavoritesOnLogin");
		}
	}
}

void LLPanelPreference::toggleMuteWhenMinimized()
{
	std::string mute("MuteWhenMinimized");
	gSavedSettings.setBOOL(mute, !gSavedSettings.getBOOL(mute));
}

void LLPanelPreference::cancel()
{
	for (control_values_map_t::iterator iter =  mSavedValues.begin();
		 iter !=  mSavedValues.end(); ++iter)
	{
		LLControlVariable* control = iter->first;
		LLSD ctrl_value = iter->second;

		if((control->getName() == "InstantMessageLogPath") && (ctrl_value.asString() == ""))
		{
			continue;
		}

		control->set(ctrl_value);
	}

	for (string_color_map_t::iterator iter = mSavedColors.begin();
		 iter != mSavedColors.end(); ++iter)
	{
		LLColorSwatchCtrl* color_swatch = findChild<LLColorSwatchCtrl>(iter->first);
		if (color_swatch)
		{
			color_swatch->set(iter->second);
			color_swatch->onCommit();
		}
	}
}

void LLPanelPreference::setControlFalse(const LLSD& user_data)
{
	std::string control_name = user_data.asString();
	LLControlVariable* control = findControl(control_name);
	
	if (control)
		control->set(LLSD(FALSE));
}

void LLPanelPreference::updateMediaAutoPlayCheckbox(LLUICtrl* ctrl)
{
	std::string name = ctrl->getName();

	// Disable "Allow Media to auto play" only when both
	// "Streaming Music" and "Media" are unchecked. STORM-513.
	if ((name == "enable_music") || (name == "enable_media"))
	{
		bool music_enabled = getChild<LLCheckBoxCtrl>("enable_music")->get();
		bool media_enabled = getChild<LLCheckBoxCtrl>("enable_media")->get();

		getChild<LLCheckBoxCtrl>("media_auto_play_btn")->setEnabled(music_enabled || media_enabled);
	}
}

void LLPanelPreference::deletePreset(const LLSD& user_data)
{
	std::string subdirectory = user_data.asString();
	LLFloaterReg::showInstance("delete_pref_preset", subdirectory);
}

void LLPanelPreference::savePreset(const LLSD& user_data)
{
	std::string subdirectory = user_data.asString();
	LLFloaterReg::showInstance("save_pref_preset", subdirectory);
}

void LLPanelPreference::loadPreset(const LLSD& user_data)
{
	std::string subdirectory = user_data.asString();
	LLFloaterReg::showInstance("load_pref_preset", subdirectory);
}

void LLPanelPreference::setHardwareDefaults()
{
}

class LLPanelPreferencePrivacy : public LLPanelPreference
{
public:
	LLPanelPreferencePrivacy()
	{
// [SL:KB] - Patch: Settings-ClearCache | Checked: 2010-08-03 (Catznip-2.2)
		mCommitCallbackRegistrar.add("Privacy.ToggleClearSetting", boost::bind(&LLPanelPreferencePrivacy::onToggleClearSetting, this, _1, _2));
// [/SL:KB]

		mAccountIndependentSettings.push_back("VoiceCallsFriendsOnly");
		mAccountIndependentSettings.push_back("AutoDisengageMic");
	}

// [SL:KB] - Patch: Settings-ClearCache | Checked: 2010-08-03 (Catznip-2.1)
	/*virtual*/ void refresh()
	{
		U32 nClearMask = gSavedSettings.getU32("ClearCacheMask");
		getChild<LLCheckBoxCtrl>("clear_web_cookies")->set(nClearMask & CLEAR_MASK_COOKIES);
		getChild<LLCheckBoxCtrl>("clear_navbar_history")->set(nClearMask & CLEAR_MASK_NAVBAR);
		getChild<LLCheckBoxCtrl>("clear_search_history")->set(nClearMask & CLEAR_MASK_SEARCH);
		getChild<LLCheckBoxCtrl>("clear_teleport_history")->set(nClearMask & CLEAR_MASK_TELEPORT);
		getChild<LLCheckBoxCtrl>("clear_people_history")->set(nClearMask & CLEAR_MASK_PEOPLE);
	}
// [/SL:KB]

	/*virtual*/ void saveSettings()
	{
		LLPanelPreference::saveSettings();

		// Don't save (=erase from the saved values map) per-account privacy settings
		// if we're not logged in, otherwise they will be reset to defaults on log off.
		if (LLStartUp::getStartupState() != STATE_STARTED)
		{
			// Erase only common settings, assuming there are no color settings on Privacy page.
			for (control_values_map_t::iterator it = mSavedValues.begin(); it != mSavedValues.end(); )
			{
				const std::string setting = it->first->getName();
				if (find(mAccountIndependentSettings.begin(),
					mAccountIndependentSettings.end(), setting) == mAccountIndependentSettings.end())
				{
					mSavedValues.erase(it++);
				}
				else
				{
					++it;
				}
			}
		}

// [SL:KB] - Patch: Settings-ClearCache | Checked: 2011-01-20 (Catznip-2.5)
		LLControlVariablePtr pControl = gSavedSettings.getControl("ClearCacheMask");
		if (pControl.notNull())
		{
			mSavedValues[pControl] = pControl->getValue();
		}
// [/SL:KB]
	}

// [SL:KB] - Patch: Settings-ClearCache | Checked: 2010-08-03 (Catznip-2.1)
protected:
	void onToggleClearSetting(LLUICtrl* pUICtrl, const LLSD& sdParam)
	{
		/*const*/ LLCheckBoxCtrl* pCheckCtrl = dynamic_cast<LLCheckBoxCtrl*>(pUICtrl);
		if (pCheckCtrl)
		{
			U32 nClearToggle = 0; const std::string strParam = sdParam.asString();
			if ("cookies" == strParam)
				nClearToggle = CLEAR_MASK_COOKIES;
			else if ("navbar" == strParam)
				nClearToggle = CLEAR_MASK_NAVBAR;
			else if ("search" == strParam)
				nClearToggle = CLEAR_MASK_SEARCH;
			else if ("teleport" == strParam)
				nClearToggle = CLEAR_MASK_TELEPORT;
			else if ("people" == strParam)
				nClearToggle = CLEAR_MASK_PEOPLE;

			U32 nClearMask = gSavedSettings.getU32("ClearCacheMask");
			if (pCheckCtrl->get())
				nClearMask |= nClearToggle;
			else
				nClearMask &= ~nClearToggle;
			gSavedSettings.setU32("ClearCacheMask", nClearMask);
		}
	}
// [/SL:KB]

private:
	std::list<std::string> mAccountIndependentSettings;
};

static LLPanelInjector<LLPanelPreferenceGraphics> t_pref_graph("panel_preference_graphics");
static LLPanelInjector<LLPanelPreferencePrivacy> t_pref_privacy("panel_preference_privacy");

// [SL:KB] - Patch: Settings-Troubleshooting | Checked: 2014-04-03 (Catznip-3.6)
class LLPanelPreferenceTroubleshooting : public LLPanelPreference
{
public:
	LLPanelPreferenceTroubleshooting()
	{
		mCommitCallbackRegistrar.add("Pref.ClearSettings", boost::bind(&LLPanelPreferenceTroubleshooting::onClearSettings, _2));
		mCommitCallbackRegistrar.add("Pref.ForceInitialOutfit", boost::bind(&LLPanelPreferenceTroubleshooting::onForceInitialOutfit));
		mCommitCallbackRegistrar.add("Pref.ResetWindLight", boost::bind(&LLPanelPreferenceTroubleshooting::onResetWindLight));
	}

	/*virtual*/ void refresh()
	{
		const EStartupState eStartupState = LLStartUp::getStartupState();

		getChild<LLUICtrl>("catznip_group_btn")->setEnabled(eStartupState == STATE_STARTED);
		getChild<LLUICtrl>("reset_avatar_btn")->setEnabled(eStartupState == STATE_STARTED);
		getChild<LLUICtrl>("clear_account_settings_btn")->setEnabled(eStartupState == STATE_STARTED);
	}

	static void onClearSettings(const LLSD& sdParam)
	{
		const std::string strParam = sdParam.asString();

		std::string strNotifName;
		if ("user" == strParam)
			strNotifName = "ConfirmResetUserSettings";
		else if ("account" == strParam)
			strNotifName = "ConfirmResetAccountSettings";

		if (!strNotifName.empty())
		{
			LLNotificationsUtil::add(strNotifName, LLSD(), sdParam, onClearSettingsCallback);
		}
	}

	static void onClearSettingsCallback(const LLSD& sdNotification, const LLSD& sdResponse)
	{
		S32 nOption = LLNotificationsUtil::getSelectedOption(sdNotification, sdResponse);
		if (nOption == 0) // YES
		{
			const std::string strParam = sdNotification["payload"].asString();
			if ("user" == strParam)
			{
				gStartupSettings.setBOOL("PurgeUserSettingsOnNextStartup", TRUE);
			}
			else if ("account" == strParam)
			{
				gStartupSettings.setBOOL("PurgeAccountSettingsOnNextLogin", TRUE);
			}
			LLNotificationsUtil::add("SettingsWillReset");
		}
	}

	static void onForceInitialOutfit()
	{
		LLNotificationsUtil::add("ConfirmResetOutfit", LLSD(), LLSD(), onForceInitialOutfitCallback);
	}

	static void onForceInitialOutfitCallback(const LLSD& sdNotification, const LLSD& sdResponse)
	{
		S32 nOption = LLNotificationsUtil::getSelectedOption(sdNotification, sdResponse);
		if (nOption == 0) // YES
		{
			gSavedPerAccountSettings.setBOOL("WearInitialOutfit", TRUE);
			LLNotificationsUtil::add("OutfitWillReset");
		}
	}

	static void onResetWindLight()
	{
		LLEnvManagerNew::resetUserPrefs();
	}
};

static LLPanelInjector<LLPanelPreferenceTroubleshooting> t_pref_troubleshooting("panel_preference_troubleshooting");
// [SL:KB]

BOOL LLPanelPreferenceGraphics::postBuild()
{
	LLFloaterReg::showInstance("prefs_graphics_advanced");
	LLFloaterReg::hideInstance("prefs_graphics_advanced");

	resetDirtyChilds();
	setPresetText();

	LLPresetsManager* presetsMgr = LLPresetsManager::getInstance();
    presetsMgr->setPresetListChangeCallback(boost::bind(&LLPanelPreferenceGraphics::onPresetsListChange, this));
    presetsMgr->createMissingDefault(); // a no-op after the first time, but that's ok
    
	return LLPanelPreference::postBuild();
}

void LLPanelPreferenceGraphics::draw()
{
	setPresetText();
	LLPanelPreference::draw();
}

void LLPanelPreferenceGraphics::onPresetsListChange()
{
	resetDirtyChilds();
	setPresetText();

	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance && !gSavedSettings.getString("PresetGraphicActive").empty())
	{
// [SL:KB] - Patch: Preferences-General | Checked: Catznip-4.1
		saveSettings(); //make cancel work correctly after changing the preset
// [/SL:KB]
//		instance->saveSettings(); //make cancel work correctly after changing the preset
	}
}

void LLPanelPreferenceGraphics::setPresetText()
{
	LLTextBox* preset_text = getChild<LLTextBox>("preset_text");

	std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");

	if (!preset_graphic_active.empty() && preset_graphic_active != preset_text->getText())
	{
		LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
		if (instance)
		{
			instance->saveGraphicsPreset(preset_graphic_active);
		}
	}

	if (hasDirtyChilds() && !preset_graphic_active.empty())
	{
		gSavedSettings.setString("PresetGraphicActive", "");
		preset_graphic_active.clear();
		// This doesn't seem to cause an infinite recursion.  This trigger is needed to cause the pulldown
		// panel to update.
		LLPresetsManager::getInstance()->triggerChangeSignal();
	}

	if (!preset_graphic_active.empty())
	{
		if (preset_graphic_active == PRESETS_DEFAULT)
		{
			preset_graphic_active = LLTrans::getString(PRESETS_DEFAULT);
		}
		preset_text->setText(preset_graphic_active);
	}
	else
	{
		preset_text->setText(LLTrans::getString("none_paren_cap"));
	}

	preset_text->resetDirty();
}

bool LLPanelPreferenceGraphics::hasDirtyChilds()
{
	LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced");
	std::list<LLView*> view_stack;
	view_stack.push_back(this);
	if (advanced)
	{
		view_stack.push_back(advanced);
	}
	while(!view_stack.empty())
	{
		// Process view on top of the stack
		LLView* curview = view_stack.front();
		view_stack.pop_front();

		LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
		if (ctrl)
		{
			if (ctrl->isDirty())
			{
				LLControlVariable* control = ctrl->getControlVariable();
				if (control)
				{
					std::string control_name = control->getName();
					if (!control_name.empty())
					{
						return true;
					}
				}
			}
		}
		// Push children onto the end of the work stack
		for (child_list_t::const_iterator iter = curview->getChildList()->begin();
			 iter != curview->getChildList()->end(); ++iter)
		{
			view_stack.push_back(*iter);
		}
	}

	return false;
}

void LLPanelPreferenceGraphics::resetDirtyChilds()
{
	LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced");
	std::list<LLView*> view_stack;
	view_stack.push_back(this);
	if (advanced)
	{
		view_stack.push_back(advanced);
	}
	while(!view_stack.empty())
	{
		// Process view on top of the stack
		LLView* curview = view_stack.front();
		view_stack.pop_front();

		LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
		if (ctrl)
		{
			ctrl->resetDirty();
		}
		// Push children onto the end of the work stack
		for (child_list_t::const_iterator iter = curview->getChildList()->begin();
			 iter != curview->getChildList()->end(); ++iter)
		{
			view_stack.push_back(*iter);
		}
	}	
}

void LLPanelPreferenceGraphics::cancel()
{
	LLPanelPreference::cancel();
}
void LLPanelPreferenceGraphics::saveSettings()
{
	resetDirtyChilds();
	std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");
	if (preset_graphic_active.empty())
	{
		LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
		if (instance)
		{
			//don't restore previous preset after closing Preferences
			instance->saveGraphicsPreset(preset_graphic_active);
		}
	}
	LLPanelPreference::saveSettings();
}
void LLPanelPreferenceGraphics::setHardwareDefaults()
{
	resetDirtyChilds();
}

LLFloaterPreferenceGraphicsAdvanced::LLFloaterPreferenceGraphicsAdvanced(const LLSD& key)
	: LLFloater(key)
{
	mCommitCallbackRegistrar.add("Pref.VertexShaderEnable",		boost::bind(&LLFloaterPreferenceGraphicsAdvanced::onVertexShaderEnable, this));
// [SL:KB] - Patch: Appearance-Complexity | Checked: Catznip-4.1
	mCommitCallbackRegistrar.add("Pref.UpdateIndirectMaxNonImpostors", boost::bind(&LLFloaterPreferenceGraphicsAdvanced::onMaxNonImpostorsChange, this));
// [/SL:KB]
//	mCommitCallbackRegistrar.add("Pref.UpdateIndirectMaxNonImpostors", boost::bind(&LLFloaterPreferenceGraphicsAdvanced::updateMaxNonImpostors,this));
	mCommitCallbackRegistrar.add("Pref.UpdateIndirectMaxComplexity",   boost::bind(&LLFloaterPreferenceGraphicsAdvanced::updateMaxComplexity,this));
}

LLFloaterPreferenceGraphicsAdvanced::~LLFloaterPreferenceGraphicsAdvanced()
{
}

LLFloaterPreferenceProxy::LLFloaterPreferenceProxy(const LLSD& key)
	: LLFloater(key),
	  mSocksSettingsDirty(false)
{
	mCommitCallbackRegistrar.add("Proxy.OK",                boost::bind(&LLFloaterPreferenceProxy::onBtnOk, this));
	mCommitCallbackRegistrar.add("Proxy.Cancel",            boost::bind(&LLFloaterPreferenceProxy::onBtnCancel, this));
	mCommitCallbackRegistrar.add("Proxy.Change",            boost::bind(&LLFloaterPreferenceProxy::onChangeSocksSettings, this));
}

BOOL LLFloaterPreferenceGraphicsAdvanced::postBuild()
{
    // Don't do this on Mac as their braindead GL versioning
    // sets this when 8x and 16x are indeed available
    //
#if !LL_DARWIN
    if (gGLManager.mIsIntel || gGLManager.mGLVersion < 3.f)
    { //remove FSAA settings above "4x"
        LLComboBox* combo = getChild<LLComboBox>("fsaa");
        combo->remove("8x");
        combo->remove("16x");
    }
#endif

    return TRUE;
}

void LLFloaterPreferenceGraphicsAdvanced::onOpen(const LLSD& key)
{
    refresh();
}

void LLFloaterPreferenceGraphicsAdvanced::onClickCloseBtn(bool app_quitting)
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance)
	{
		instance->cancel();
	}
}

LLFloaterPreferenceProxy::~LLFloaterPreferenceProxy()
{
}

BOOL LLFloaterPreferenceProxy::postBuild()
{
	LLRadioGroup* socksAuth = getChild<LLRadioGroup>("socks5_auth_type");
	if (!socksAuth)
	{
		return FALSE;
	}
	if (socksAuth->getSelectedValue().asString() == "None")
	{
		getChild<LLLineEditor>("socks5_username")->setEnabled(false);
		getChild<LLLineEditor>("socks5_password")->setEnabled(false);
	}
	else
	{
		// Populate the SOCKS 5 credential fields with protected values.
		LLPointer<LLCredential> socks_cred = gSecAPIHandler->loadCredential("SOCKS5");
		getChild<LLLineEditor>("socks5_username")->setValue(socks_cred->getIdentifier()["username"].asString());
		getChild<LLLineEditor>("socks5_password")->setValue(socks_cred->getAuthenticator()["creds"].asString());
	}

	return TRUE;
}

void LLFloaterPreferenceProxy::onOpen(const LLSD& key)
{
	saveSettings();
}

void LLFloaterPreferenceProxy::onClose(bool app_quitting)
{
	if(app_quitting)
	{
		cancel();
	}

	if (mSocksSettingsDirty)
	{

		// If the user plays with the Socks proxy settings after login, it's only fair we let them know
		// it will not be updated until next restart.
		if (LLStartUp::getStartupState()>STATE_LOGIN_WAIT)
		{
			LLNotifications::instance().add("ChangeProxySettings", LLSD(), LLSD());
			mSocksSettingsDirty = false; // we have notified the user now be quiet again
		}
	}
}

void LLFloaterPreferenceProxy::saveSettings()
{
	// Save the value of all controls in the hierarchy
	mSavedValues.clear();
	std::list<LLView*> view_stack;
	view_stack.push_back(this);
	while(!view_stack.empty())
	{
		// Process view on top of the stack
		LLView* curview = view_stack.front();
		view_stack.pop_front();

		LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
		if (ctrl)
		{
			LLControlVariable* control = ctrl->getControlVariable();
			if (control)
			{
				mSavedValues[control] = control->getValue();
			}
		}

		// Push children onto the end of the work stack
		for (child_list_t::const_iterator iter = curview->getChildList()->begin();
				iter != curview->getChildList()->end(); ++iter)
		{
			view_stack.push_back(*iter);
		}
	}
}

void LLFloaterPreferenceProxy::onBtnOk()
{
	// commit any outstanding text entry
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}

	// Save SOCKS proxy credentials securely if password auth is enabled
	LLRadioGroup* socksAuth = getChild<LLRadioGroup>("socks5_auth_type");
	if (socksAuth->getSelectedValue().asString() == "UserPass")
	{
		LLSD socks_id = LLSD::emptyMap();
		socks_id["type"] = "SOCKS5";
		socks_id["username"] = getChild<LLLineEditor>("socks5_username")->getValue().asString();

		LLSD socks_authenticator = LLSD::emptyMap();
		socks_authenticator["type"] = "SOCKS5";
		socks_authenticator["creds"] = getChild<LLLineEditor>("socks5_password")->getValue().asString();

		// Using "SOCKS5" as the "grid" argument since the same proxy
		// settings will be used for all grids and because there is no
		// way to specify the type of credential.
		LLPointer<LLCredential> socks_cred = gSecAPIHandler->createCredential("SOCKS5", socks_id, socks_authenticator);
		gSecAPIHandler->saveCredential(socks_cred, true);
	}
	else
	{
		// Clear SOCKS5 credentials since they are no longer needed.
		LLPointer<LLCredential> socks_cred = new LLCredential("SOCKS5");
		gSecAPIHandler->deleteCredential(socks_cred);
	}

	closeFloater(false);
}

void LLFloaterPreferenceProxy::onBtnCancel()
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
		refresh();
	}

	cancel();
}

void LLFloaterPreferenceProxy::onClickCloseBtn(bool app_quitting)
{
	cancel();
}

void LLFloaterPreferenceProxy::cancel()
{

	for (control_values_map_t::iterator iter =  mSavedValues.begin();
			iter !=  mSavedValues.end(); ++iter)
	{
		LLControlVariable* control = iter->first;
		LLSD ctrl_value = iter->second;
		control->set(ctrl_value);
	}
	mSocksSettingsDirty = false;
	closeFloater();
}

void LLFloaterPreferenceProxy::onChangeSocksSettings() 
{
	mSocksSettingsDirty = true;

	LLRadioGroup* socksAuth = getChild<LLRadioGroup>("socks5_auth_type");
	if (socksAuth->getSelectedValue().asString() == "None")
	{
		getChild<LLLineEditor>("socks5_username")->setEnabled(false);
		getChild<LLLineEditor>("socks5_password")->setEnabled(false);
	}
	else
	{
		getChild<LLLineEditor>("socks5_username")->setEnabled(true);
		getChild<LLLineEditor>("socks5_password")->setEnabled(true);
	}

	// Check for invalid states for the other HTTP proxy radio
	LLRadioGroup* otherHttpProxy = getChild<LLRadioGroup>("other_http_proxy_type");
	if ((otherHttpProxy->getSelectedValue().asString() == "Socks" &&
			getChild<LLCheckBoxCtrl>("socks_proxy_enabled")->get() == FALSE )||(
					otherHttpProxy->getSelectedValue().asString() == "Web" &&
					getChild<LLCheckBoxCtrl>("web_proxy_enabled")->get() == FALSE ) )
	{
		otherHttpProxy->selectFirstItem();
	}

}

// [SL:KB] - Patch: Viewer-Skins | Checked: 2010-10-21 (Catznip-2.2)
static LLPanelInjector<LLPanelPreferenceSkins> t_pref_skins("panel_preference_skins");

LLPanelPreferenceSkins::LLPanelPreferenceSkins()
	: LLPanelPreference()
	, m_pSkinCombo(NULL)
	, m_pSkinThemeCombo(NULL)
{
	m_Skin = gSavedSettings.getString("SkinCurrent");
	m_SkinTheme = gSavedSettings.getString("SkinCurrentTheme");

	const std::string strSkinsPath = gDirUtilp->getSkinBaseDir() + gDirUtilp->getDirDelimiter() + "skins.xml";
	llifstream fileSkins(strSkinsPath, std::ios::binary);
	if (fileSkins.is_open())
	{
		LLSDSerialize::fromXMLDocument(m_SkinsInfo, fileSkins);
	}
}

BOOL LLPanelPreferenceSkins::postBuild()
{
	m_pSkinCombo = getChild<LLComboBox>("skin_combobox");
	if (m_pSkinCombo)
	{
		m_pSkinCombo->setCommitCallback(boost::bind(&LLPanelPreferenceSkins::onSkinChanged, this));
	}

	m_pSkinThemeCombo = getChild<LLComboBox>("theme_combobox");
	if (m_pSkinThemeCombo)
	{
		m_pSkinThemeCombo->setCommitCallback(boost::bind(&LLPanelPreferenceSkins::onSkinThemeChanged, this));
	}

	refreshSkinList();

	return LLPanelPreference::postBuild();
}

void LLPanelPreferenceSkins::apply()
{
	if ( (m_Skin != gSavedSettings.getString("SkinCurrent")) || (m_SkinTheme != gSavedSettings.getString("SkinCurrentTheme")) )
	{
		gSavedSettings.setString("SkinCurrent", m_Skin);
		gSavedSettings.setString("SkinCurrentTheme", m_SkinTheme);

		LLNotificationsUtil::add("ChangeSkin");
	}
}

void LLPanelPreferenceSkins::cancel()
{
	m_Skin = gSavedSettings.getString("SkinCurrent");
	m_SkinTheme = gSavedSettings.getString("SkinCurrentTheme");
	refreshSkinList();
}

void LLPanelPreferenceSkins::onSkinChanged()
{
	m_Skin = (m_pSkinCombo) ? m_pSkinCombo->getSelectedValue().asString() : "default";
	refreshSkinThemeList();
	m_SkinTheme = (m_pSkinThemeCombo) ? m_pSkinThemeCombo->getSelectedValue().asString() : "";
}

void LLPanelPreferenceSkins::onSkinThemeChanged()
{
	m_SkinTheme = (m_pSkinThemeCombo) ? m_pSkinThemeCombo->getSelectedValue().asString() : "";
}

void LLPanelPreferenceSkins::refreshSkinList()
{
	if (!m_pSkinCombo)
		return;

	m_pSkinCombo->clearRows();
	for (LLSD::array_const_iterator itSkinInfo = m_SkinsInfo.beginArray(), endSkinInfo = m_SkinsInfo.endArray();
			itSkinInfo != endSkinInfo; ++itSkinInfo)
	{
		const LLSD& sdSkin = *itSkinInfo;
		std::string strPath = gDirUtilp->getSkinBaseDir();
		gDirUtilp->append(strPath, sdSkin["folder"].asString());
		if (gDirUtilp->fileExists(strPath))
		{
			m_pSkinCombo->add(sdSkin["name"].asString(), sdSkin["folder"]);
		}
	}

	BOOL fFound = m_pSkinCombo->setSelectedByValue(m_Skin, TRUE);
	if (!fFound)
	{
		m_pSkinCombo->setSelectedByValue("default", TRUE);
	}

	refreshSkinThemeList();
}

void LLPanelPreferenceSkins::refreshSkinThemeList()
{
	if (!m_pSkinThemeCombo)
		return;

	m_pSkinThemeCombo->clearRows();
	for (LLSD::array_const_iterator itSkinInfo = m_SkinsInfo.beginArray(), endSkinInfo = m_SkinsInfo.endArray(); 
			itSkinInfo != endSkinInfo; ++itSkinInfo)
	{
		const LLSD& sdSkin = *itSkinInfo;
		if (sdSkin["folder"].asString() == m_Skin)
		{
			const LLSD& sdThemes = sdSkin["themes"];
			for (LLSD::array_const_iterator itTheme = sdThemes.beginArray(), endTheme = sdThemes.endArray(); itTheme != endTheme; ++itTheme)
			{
				const LLSD& sdTheme = *itTheme;
				std::string strPath = gDirUtilp->getSkinBaseDir();
				gDirUtilp->append(strPath, sdSkin["folder"].asString());
				gDirUtilp->append(strPath, "themes");
				gDirUtilp->append(strPath, sdTheme["folder"].asString());
				if ( (gDirUtilp->fileExists(strPath)) || (sdTheme["folder"].asString().empty()) )
				{
					m_pSkinThemeCombo->add(sdTheme["name"].asString(), sdTheme["folder"]);
				}
			}
			break;
		}
	}

	BOOL fFound = m_pSkinThemeCombo->setSelectedByValue(m_SkinTheme, TRUE);
	if (!fFound)
	{
		m_pSkinThemeCombo->selectFirstItem();
	}
}
// [/SL:KB]

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2012-07-05 (Catznip-3.3)
static LLPanelInjector<LLPanelPreferenceCrashReports> t_pref_crashreports("panel_preference_crashreports");

const std::string LLPanelPreferenceCrashReports::s_strLogFile = "crash.log";

LLPanelPreferenceCrashReports::LLPanelPreferenceCrashReports()
	: LLPanelPreference()
{
}

BOOL LLPanelPreferenceCrashReports::postBuild()
{
	S32 nCrashSubmitBehavior = gCrashSettings.getS32("CrashSubmitBehavior");

	LLCheckBoxCtrl* pSendCrashReports = getChild<LLCheckBoxCtrl>("send_reports_check");
	pSendCrashReports->set(CRASH_BEHAVIOR_NEVER_SEND != nCrashSubmitBehavior);
	pSendCrashReports->setCommitCallback(boost::bind(&LLPanelPreferenceCrashReports::refresh, this));

	getChild<LLCheckBoxCtrl>("send_reportsask_check")->set(CRASH_BEHAVIOR_ASK == nCrashSubmitBehavior);
	getChild<LLCheckBoxCtrl>("send_name_check")->set(gCrashSettings.getBOOL("CrashSubmitName"));
	getChild<LLCheckBoxCtrl>("checkSendSettings")->set(gCrashSettings.getBOOL("CrashSubmitSettings"));
	getChild<LLCheckBoxCtrl>("send_log_check")->set(gCrashSettings.getBOOL("CrashSubmitLog"));

	//
	// Populate recent crashes
	//
	LLSD sdCrashLog; std::string strCrashLog = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, s_strLogFile);
	if (gDirUtilp->fileExists(strCrashLog))
	{
		llifstream fileCrashLogIn;
		fileCrashLogIn.open(strCrashLog);
		if (fileCrashLogIn.is_open())
			LLSDSerialize::fromXML(sdCrashLog, fileCrashLogIn);
		fileCrashLogIn.close();
	}

	LLScrollListCtrl* pCrashList = getChild<LLScrollListCtrl>("crashes_list");

	LLSD sdRow; LLSD& sdColumns = sdRow["columns"];
	sdColumns[0]["column"] = "crash_timestamp"; sdColumns[0]["type"] = "text";
	sdColumns[1]["column"] = "crash_module";    sdColumns[1]["type"] = "text";
	sdColumns[2]["column"] = "crash_id";        sdColumns[2]["type"] = "text";
	for (LLSD::array_const_iterator itCrash = sdCrashLog.beginArray(), endCrash = sdCrashLog.endArray();
			itCrash != endCrash; ++itCrash)
	{
		const LLSD& sdCrash = *itCrash;
		sdRow["value"] = sdCrash;
		sdColumns[0]["value"] = sdCrash["timestamp"].asDate().toHTTPDateString(std::string ("%Y-%m-%d %H:%M GMT"));
		if (!sdCrash["crash_freeze"].asBoolean())
			sdColumns[1]["value"] = sdCrash["crash_module"].asString() + "!" + sdCrash["crash_offset"].asString();
		else
			sdColumns[1]["value"] = "(freeze)";
		sdColumns[2]["value"] = sdCrash["crash_id"];
		pCrashList->addElement(sdRow, ADD_BOTTOM);
	}
	pCrashList->sortByColumnIndex(0, false);

	LLUICtrl* pCopySelBtn = getChild<LLUICtrl>("crashes_copysel_btn");
	pCopySelBtn->setEnabled(sdCrashLog.size() > 0);
	pCopySelBtn->setCommitCallback(boost::bind(&LLPanelPreferenceCrashReports::onCopySelection, this));
	LLUICtrl* pClearAllBtn = getChild<LLUICtrl>("crashes_clearall_btn");
	pClearAllBtn->setEnabled(sdCrashLog.size() > 0);
	pClearAllBtn->setCommitCallback(boost::bind(&LLPanelPreferenceCrashReports::onClearAll, this));

	return LLPanelPreference::postBuild();
}

void LLPanelPreferenceCrashReports::refresh()
{
	bool fEnable = getChild<LLCheckBoxCtrl>("send_reports_check")->get();
	getChild<LLUICtrl>("send_reportsask_check")->setEnabled(fEnable);
	getChild<LLUICtrl>("send_name_check")->setEnabled(fEnable);
	getChild<LLUICtrl>("checkSendSettings")->setEnabled(fEnable);
	getChild<LLUICtrl>("send_log_check")->setEnabled(fEnable);
}

void LLPanelPreferenceCrashReports::apply()
{
	LLCheckBoxCtrl* pSendCrashReports = getChild<LLCheckBoxCtrl>("send_reports_check");
	LLCheckBoxCtrl* pSendAlwaysAsk = getChild<LLCheckBoxCtrl>("send_reportsask_check");
	if (pSendCrashReports->get())
		gCrashSettings.setS32("CrashSubmitBehavior", (pSendAlwaysAsk->get()) ? CRASH_BEHAVIOR_ASK : CRASH_BEHAVIOR_ALWAYS_SEND);
	else
		gCrashSettings.setS32("CrashSubmitBehavior", CRASH_BEHAVIOR_NEVER_SEND);

	LLCheckBoxCtrl* pSendName = getChild<LLCheckBoxCtrl>("send_name_check");
	gCrashSettings.setBOOL("CrashSubmitName", pSendName->get());

	LLCheckBoxCtrl* pSendSettings = getChild<LLCheckBoxCtrl>("checkSendSettings");
	gCrashSettings.setBOOL("CrashSubmitSettings", pSendSettings->get());

	LLCheckBoxCtrl* pSendLog = getChild<LLCheckBoxCtrl>("send_log_check");
	bool fSendLog = pSendLog->get();
	// Only include the log if the user consented
	if (fSendLog)
		gDebugInfo["SLLog"] = LLError::logFileName();
	else
		gDebugInfo.erase("SLLog");
	gCrashSettings.setBOOL("CrashSubmitLog", fSendLog);
}

void LLPanelPreferenceCrashReports::cancel()
{
}

void LLPanelPreferenceCrashReports::onCopySelection()
{
	const LLScrollListCtrl* pCrashList = getChild<LLScrollListCtrl>("crashes_list");
	if (pCrashList)
	{
		std::string strCrash;

		const std::vector<LLScrollListItem*> selRows = (pCrashList->getNumSelected() > 0) ? pCrashList->getAllSelected() : pCrashList->getAllData();
		for (std::vector<LLScrollListItem*>::const_iterator itRow = selRows.begin(); itRow != selRows.end(); ++itRow)
		{
			const LLScrollListItem* pSelectedRow = *itRow;

			LLSD sdValues[3];
			for (int idxValue = 0; idxValue < 3; idxValue++)
			{
				const LLScrollListCell* pColumn = pSelectedRow->getColumn(0);
				if (pColumn)
				{
					sdValues[idxValue] = pSelectedRow->getColumn(idxValue)->getValue();
				}
			}

			if (!strCrash.empty())
				strCrash.append("\r\n");
			strCrash += llformat("%s - %s - %s", sdValues[0].asString().c_str(), sdValues[1].asString().c_str(), sdValues[2].asString().c_str());
		}

		if (!strCrash.empty())
		{
			LLClipboard::instance().copyToClipboard(utf8str_to_wstring(strCrash), 0, strCrash.length());
		}
	}
}

void LLPanelPreferenceCrashReports::onClearAll()
{
	std::string strCrashLog = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, s_strLogFile);
	if (gDirUtilp->fileExists(strCrashLog))
		LLFile::remove(strCrashLog);
	getChild<LLScrollListCtrl>("crashes_list")->clearRows();
}
// [/SL:KB]
