/** 
 * @file llfloaterpreference.h
 * @brief LLPreferenceCore class definition
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

#ifndef LL_LLFLOATERPREFERENCE_H
#define LL_LLFLOATERPREFERENCE_H

#include "llfloater.h"
#include "llavatarpropertiesprocessor.h"
#include "llconversationlog.h"
#include "llsearcheditor.h"

class LLConversationLogObserver;
class LLPanelPreference;
class LLPanelLCD;
class LLPanelDebug;
class LLMessageSystem;
class LLScrollListCtrl;
class LLSliderCtrl;
class LLSD;
class LLTextBox;
// [SL:KB] - Patch: Viewer-Skins | Checked: 2010-10-21 (Catznip-2.2)
class LLComboBox;
// [/SL:KB]

namespace ll
{
	namespace prefs
	{
		struct SearchData;
	}
}

typedef std::map<std::string, std::string> notifications_map;

typedef enum
	{
		GS_LOW_GRAPHICS,
		GS_MID_GRAPHICS,
		GS_HIGH_GRAPHICS,
		GS_ULTRA_GRAPHICS
		
	} EGraphicsSettings;

// Floater to control preferences (display, audio, bandwidth, general.
class LLFloaterPreference : public LLFloater, public LLAvatarPropertiesObserver, public LLConversationLogObserver
{
public: 
	LLFloaterPreference(const LLSD& key);
	~LLFloaterPreference();

	void apply();
	void cancel();
	/*virtual*/ void draw();
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);
	/*virtual*/ void changed();
	/*virtual*/ void changed(const LLUUID& session_id, U32 mask) {};

	// static data update, called from message handler
	static void updateUserInfo(const std::string& visibility, bool im_via_email, bool is_verified_email);

	// refresh all the graphics preferences menus
	static void refreshEnabledGraphics();
	
	// translate user's do not disturb response message according to current locale if message is default, otherwise do nothing
	static void initDoNotDisturbResponse();

	// update Show Favorites checkbox
	static void updateShowFavoritesCheckbox(bool val);

	void processProperties( void* pData, EAvatarProcessorType type );
	void processProfileProperties(const LLAvatarData* pAvatarData );
	void storeAvatarProperties( const LLAvatarData* pAvatarData );
	void saveAvatarProperties( void );
	void selectPrivacyPanel();
	void selectChatPanel();
	void getControlNames(std::vector<std::string>& names);

// [SL:KB] - Patch: Preferences-General | Checked: Catznip-3.6
	void registerPrefPanel(LLPanelPreference* pPrefPanel);
	void unregisterPrefpanel(LLPanelPreference* pPrefPanel);

	template<class T> T* getPanelByType() const;
	void                 showPanel(const std::string& strPanel);
// [/SL:KB]

protected:	
	void		onBtnOK(const LLSD& userdata);
	void		onBtnCancel(const LLSD& userdata);
// [SL:KB] - Patch: Preferences-General | Checked: Catznip-3.6
	void		onShowPanel(const LLSD& sdParam);
	void		onResetUIScale() const;
// [/SL:KB]

	void		onClickClearCache();			// Clear viewer texture cache, vfs, and VO cache on next startup
	void		onClickBrowserClearCache();		// Clear web history and caches as well as viewer caches above
	void		onLanguageChange();
//	void		onNotificationsChange(const std::string& OptionName);
	void		onNameTagOpacityChange(const LLSD& newvalue);

	// set value of "DoNotDisturbResponseChanged" in account settings depending on whether do not disturb response
	// string differs from default after user changes.
	void onDoNotDisturbResponseChanged();
	// if the custom settings box is clicked
	void onChangeCustom();
	void updateMeterText(LLUICtrl* ctrl);
	// callback for defaults
	void setHardwareDefaults();
	void setRecommended();
	// callback for when client modifies a render option
    void onRenderOptionEnable();
	// callback for when client turns on impostors
	void onAvatarImpostorsEnable();

	// callback for commit in the "Single click on land" and "Double click on land" comboboxes.
	void onClickActionChange();
	// updates click/double-click action settings depending on controls values
	void updateClickActionSettings();
	// updates click/double-click action controls depending on values from settings.xml
	void updateClickActionControls();

public:
//	// This function squirrels away the current values of the controls so that
//	// cancel() can restore them.	
//	void saveSettings();

	void setCacheLocation(const LLStringExplicit& location);

	void onClickSetCache();
	void changeCachePath(const std::vector<std::string>& filenames, std::string proposed_name);
	void onClickResetCache();
//	void onClickSkin(LLUICtrl* ctrl,const LLSD& userdata);
//	void onSelectSkin();
	void onClickSetKey();
	void setKey(KEY key);
	void setMouse(LLMouseHandler::EClickType click);
	void onClickSetMiddleMouse();
	void onClickSetSounds();
	void onClickEnablePopup();
	void onClickDisablePopup();	
	void resetAllIgnored();
	void setAllIgnored();
	void onClickLogPath();
	void changeLogPath(const std::vector<std::string>& filenames, std::string proposed_name);
	bool moveTranscriptsAndLog();
// [SL:KB] - Patch: Settings-Snapshot | Checked: Catznip-3.2
	void onClickSnapshotPath();	
	void onClickSnapshotPathCallback(const std::vector<std::string>& filenames, std::string proposed_name);
// [/SL:KB]
	void enableHistory();
	void setPersonalInfo(const std::string& visibility, bool im_via_email, bool is_verified_email);
	void refreshEnabledState();
	void onCommitWindowedMode();
	void refresh();	// Refresh enable/disable
	// if the quality radio buttons are changed
	void onChangeQuality(const LLSD& data);
	
	void refreshUI();

// [SL:KB] - Patch: Notification-Logging | Checked: 2012-02-01 (Catznip-3.2)
	void onInitLogNotification(LLUICtrl* pCtrl, const LLSD& sdParam, const char* pstrScope);
	void onToggleLogNotification(LLUICtrl* pCtrl, const LLSD& sdParam, const char* pstrScope);
// [/SL:KB]

	void onCommitMediaEnabled();
	void onCommitMusicEnabled();
	void applyResolution();
	void onChangeMaturity();
	void onChangeModelFolder();
// [SL:KB] - Patch: Settings-RenderResolutionScale | Checked: Catznip-6.5
	void onChangeRenderResolutionScale(LLUICtrl* pCtrl);
// [/SL:KB]
	void onChangeTextureFolder();
	void onChangeSoundFolder();
	void onChangeAnimationFolder();
	void onClickBlockList();
	void onClickProxySettings();
//	void onClickTranslationSettings();
	void onClickPermsDefault();
	void onClickRememberedUsernames();
//	void onClickAutoReplace();
//	void onClickSpellChecker();
	void onClickRenderExceptions();
	void onClickAdvanced();
	void applyUIColor(LLUICtrl* ctrl, const LLSD& param);
// [SL:KB] - Patch: Settings-NameTags | Checked: 2014-05-17 (Catznip-3.6)
	void applyNameTagColor(LLUICtrl* ctrl, const LLSD& param);
// [/SL:KB]
	void getUIColor(LLUICtrl* ctrl, const LLSD& param);
	void onLogChatHistorySaved();	
	void buildPopupLists();
//	static void refreshSkin(void* data);
//	void selectPanel(const LLSD& name);
	void saveCameraPreset(std::string& preset);
	void saveGraphicsPreset(std::string& preset);

private:

	void onDeleteTranscripts();
	void onDeleteTranscriptsResponse(const LLSD& notification, const LLSD& response);
	void updateDeleteTranscriptsButton();
	void updateMaxComplexity();
	static bool loadFromFilename(const std::string& filename, std::map<std::string, std::string> &label_map);

//	static std::string sSkin;
//	notifications_map mNotificationOptions;
	bool mClickActionDirty; ///< Set to true when the click/double-click options get changed by user.
	bool mGotPersonalInfo;
	bool mOriginalIMViaEmail;
	bool mLanguageChanged;
	bool mAvatarDataInitialized;
	std::string mPriorInstantMessageLogPath;
	
	bool mOriginalHideOnlineStatus;
	std::string mDirectoryVisibility;
	
	LLAvatarData mAvatarProperties;
	std::string mSavedCameraPreset;
	std::string mSavedGraphicsPreset;
	LOG_CLASS(LLFloaterPreference);

	LLSearchEditor *mFilterEdit;
	std::unique_ptr< ll::prefs::SearchData > mSearchData;

	void onUpdateFilterTerm( bool force = false );
	void collectSearchableItems();

// [SL:KB] - Patch: Preferences-General | Checked: 2014-03-03 (Catznip-3.6)
	bool mCancelOnClose; // If TRUE then onClose() will call cancel(); set by apply() and cancel()
	std::list<LLPanelPreference*> mPreferencePanels;
// [/SL:KB]
};

class LLPanelPreference : public LLPanel
{
public:
	LLPanelPreference();
	/*virtual*/ BOOL postBuild();
	
	virtual ~LLPanelPreference();

// [SL:KB] - Patch: Preferences-General | Checked: Catznip-3.6
	// Calls onOpen and onClose on derived panels as appropriate
	void onVisibilityChange(BOOL new_visibility) override;

	// Called only once when the panel becomes visible for the first time
	virtual void init() {}
	// Called the first time the panel becomes visible in the currently preferences floater session
	virtual void refresh() {}
	//virtual void onOpen(const LLSD& sdKey) {}
	virtual void onClose() {}

	// Returns TRUE if the user may have made any chances to the state of this panel (currently we return "was opened this session")
	BOOL isDirty() const override { return !mRefreshOnOpen; }
	// Returns TRUE if the panel has been initialized (been visible at least once)
	bool isInitialized() const { return mInitialized; }
// [/SL:KB]

	virtual void apply();
	virtual void cancel();
	void setControlFalse(const LLSD& user_data);
	virtual void setHardwareDefaults();

	// Disables "Allow Media to auto play" check box only when both
	// "Streaming Music" and "Media" are unchecked. Otherwise enables it.
	void updateMediaAutoPlayCheckbox(LLUICtrl* ctrl);

	// This function squirrels away the current values of the controls so that
	// cancel() can restore them.
	virtual void saveSettings();

	void deletePreset(const LLSD& user_data);
	void savePreset(const LLSD& user_data);
	void loadPreset(const LLSD& user_data);

	class Updater;

protected:
	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;
// [SL:KB] - Patch: Preferences-General | Checked: Catznip-3.6
	bool mInitialized = false;
	bool mRefreshOnOpen = true;

	void onParentFloaterClose() { mRefreshOnOpen = true; }
// [/SL:KB]

private:
	//for "Only friends and groups can call or IM me"
	static void showFriendsOnlyWarning(LLUICtrl*, const LLSD&);
    //for  "Allow Multiple Viewers"
    static void showMultipleViewersWarning(LLUICtrl*, const LLSD&);
	//for "Show my Favorite Landmarks at Login"
	static void handleFavoritesOnLoginChanged(LLUICtrl* checkbox, const LLSD& value);

	static void toggleMuteWhenMinimized();
	typedef std::map<std::string, LLColor4> string_color_map_t;
	string_color_map_t mSavedColors;

	Updater* mBandWidthUpdater;
	LOG_CLASS(LLPanelPreference);
};

class LLPanelPreferenceGraphics : public LLPanelPreference
{
public:
	BOOL postBuild();
	void draw();
	void cancel();
	void saveSettings();
	void resetDirtyChilds();
	void setHardwareDefaults();
	void setPresetText();

	static const std::string getPresetsPath();

protected:
	bool hasDirtyChilds();

private:
	void onPresetsListChange();
	LOG_CLASS(LLPanelPreferenceGraphics);
};

class LLFloaterPreferenceGraphicsAdvanced : public LLFloater
{
  public: 
	LLFloaterPreferenceGraphicsAdvanced(const LLSD& key);
	~LLFloaterPreferenceGraphicsAdvanced();
	/*virtual*/ BOOL postBuild();
	void onOpen(const LLSD& key);
	void onClickCloseBtn(bool app_quitting);
	void disableUnavailableSettings();
	void refreshEnabledGraphics();
	void refreshEnabledState();
	void updateSliderText(LLSliderCtrl* ctrl, LLTextBox* text_box);
// [SL:KB] - Patch: Appearance-Complexity | Checked: Catznip-4.1
	void onMaxNonImpostorsChange();
	static void updateMaxNonImpostors(U32 value);
// [/SL:KB]
//	void updateMaxNonImpostors();
	void setMaxNonImpostorsText(U32 value, LLTextBox* text_box);
	void updateMaxComplexity();
	void setMaxComplexityText(U32 value, LLTextBox* text_box);
	static void setIndirectControls();
	static void setIndirectMaxNonImpostors();
	static void setIndirectMaxArc();
	void refresh();
	// callback for when client modifies a render option
	void onRenderOptionEnable();
    void onAdvancedAtmosphericsEnable();
	LOG_CLASS(LLFloaterPreferenceGraphicsAdvanced);
};

class LLAvatarComplexityControls
{
  public: 
	static void updateMax(LLSliderCtrl* slider, LLTextBox* value_label);
	static void setText(U32 value, LLTextBox* text_box);
	static void setIndirectControls();
	static void setIndirectMaxNonImpostors();
	static void setIndirectMaxArc();
	LOG_CLASS(LLAvatarComplexityControls);
};

class LLFloaterPreferenceProxy : public LLFloater
{
public: 
	LLFloaterPreferenceProxy(const LLSD& key);
	~LLFloaterPreferenceProxy();

	/// show off our menu
	static void show();
	void cancel();
	
protected:
	BOOL postBuild();
	void onOpen(const LLSD& key);
	void onClose(bool app_quitting);
	void saveSettings();
	void onBtnOk();
	void onBtnCancel();
	void onClickCloseBtn(bool app_quitting = false);

	void onChangeSocksSettings();

private:
	
	bool mSocksSettingsDirty;
	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;
	LOG_CLASS(LLFloaterPreferenceProxy);
};

// [SL:KB] - Patch: Viewer-Skins | Checked: 2010-10-21 (Catznip-2.2)
class LLPanelPreferenceSkins : public LLPanelPreference
{
public:
	LLPanelPreferenceSkins();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void apply();
	/*virtual*/ void cancel();
protected:
	void onSkinChanged();
	void onSkinThemeChanged();
	void refreshSkinList();
	void refreshSkinThemeList();

protected:
	std::string m_Skin;
	LLComboBox* m_pSkinCombo;
	std::string m_SkinTheme;
	LLComboBox* m_pSkinThemeCombo;
	LLSD        m_SkinsInfo;
};
// [/SL:KB]

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-10-21 (Catznip-2.2)
class LLPanelPreferenceCrashReports : public LLPanelPreference
{
public:
	LLPanelPreferenceCrashReports();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void apply();
	/*virtual*/ void cancel();

	/*virtual*/ void refresh();

protected:
	void onCopySelection();
	void onClearAll();

	static const std::string s_strLogFile;
};
// [/SL:KB]

#endif  // LL_LLPREFERENCEFLOATER_H
