/**
 *
 * Copyright (c) 2016-2020, Kitty Barnett
 *
 * The source code in this file is provided to you under the terms of the
 * GNU Lesser General Public License, version 2.1, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. Terms of the LGPL can be found in doc/LGPL-licence.txt
 * in this distribution, or online at http://www.gnu.org/licenses/lgpl-2.1.txt
 *
 * By copying, modifying or distributing this software, you acknowledge that
 * you have read and understood your obligations described above, and agree to
 * abide by those obligations.
 *
 */

#pragma once

#include "llfloaterquickprefs.h"
#include "llinventorysettings.h"

// ====================================================================================
// Foward declarations
//

class LLCheckBoxCtrl;
class LLComboBox;
class LLRadioGroup;
class LLSliderCtrl;
class LLTextBox;
class LLViewerInventoryItem;

// ====================================================================================
// LLQuickPrefsWindlightPanel class
//

class LLQuickPrefsWindlightPanel : public LLQuickPrefsPanel
{
	friend class LLQuickPrefsWindInventoryObserver;
	struct EnvironmentSetting
	{
		EnvironmentSetting(const LLViewerInventoryItem* pItem, bool fIsLibrary);

		LLSettingsType::type_e m_Type = LLSettingsType::ST_INVALID;
		std::string            m_Name;
		std::string            m_Path;
		LLUUID                 m_InventoryId;
		LLUUID                 m_AssetId;
		bool                   m_IsLibrary = false;
	};
	typedef std::vector<EnvironmentSetting> env_setting_vec_t;

	LOG_CLASS(LLQuickPrefsWindlightPanel);
public:
	LLQuickPrefsWindlightPanel();
	~LLQuickPrefsWindlightPanel() override;

	/*
	 * LLPanel base class overrides
	 */
public:
	BOOL postBuild() override;
	void draw() override;
	void onVisibilityChange(BOOL fVisible) override;

	/*
	 * Member functions
	 */
protected:
	void refreshControls(bool fRefreshPresets);
	void refreshEnvironments();
	void switchToLocalEnv();
	void syncControls();
	static void populateSettingsList(LLComboBox* pComboBox, std::vector<EnvironmentSetting>& settingList);
	static void sortSettingsList(env_setting_vec_t& settingList);


	/*
	 * Event handlers
	 */
protected:
	void onAddEnvironment(const LLUUID& idItem);
	void onDayOffsetChanged();
	void onDayFreezeToggle();
	void onEditEnvSetting(LLComboBox* pComboBox);
	void onSelectEnvSetting(LLComboBox* pComboBox);
	void onSelectEnvSettingPrev(LLComboBox* pComboBox);
	void onSelectEnvSettingNext(LLComboBox* pComboBox);
	void onSkyAzimuthChanged();
	void onSkyElevationChanged();
	void onSkyGammaChanged();
	void onShowPersonalLightingClicked();
	void onUseSharedEnvClicked();

	/*
	 * Member variables
	 */
protected:
	LLInventoryObserver* m_pSettingsObserver = nullptr;
	bool                 m_fNeedsRefresh = false;
	LLTimer              m_RefreshTimer;
	env_setting_vec_t    m_DayCycles;
	std::string          m_strDayCycleFilter;
	env_setting_vec_t    m_Skies;
	std::string          m_strSkyFilter;
	env_setting_vec_t    m_Waters;
	std::string          m_strWaterFilter;
	boost::signals2::scoped_connection m_EnvironmentChangeConn;
	boost::signals2::scoped_connection m_EnvironmentUpdateConn;

	LLComboBox*     m_pDayCyclePresetCombo = nullptr;
	LLButton*       m_pDayCyclePrevButton = nullptr;
	LLButton*       m_pDayCycleNextButton = nullptr;
	LLButton*       m_pDayCycleEditButton = nullptr;
	LLTextBox*      m_pDayOffsetText = nullptr;
	LLSliderCtrl*   m_pDayOffsetSlider = nullptr;
	LLCheckBoxCtrl* m_pDayFreezeCheck = nullptr;
	LLComboBox*     m_pSkyPresetCombo = nullptr;
	LLButton*       m_pSkyPrevButton = nullptr;
	LLButton*       m_pSkyNextButton = nullptr;
	LLButton*       m_pSkyEditButton = nullptr;
	LLTextBox*      m_pSkyGammaText = nullptr;
	LLSliderCtrl*   m_pSkyGammaSlider = nullptr;
	LLTextBox*      m_pSkyCelestialBodyText = nullptr;
	LLRadioGroup*   m_pSkyCelestialBodyGroup = nullptr;
	LLTextBox*      m_pSkyAzimuthText = nullptr;
	LLSliderCtrl*   m_pSkyAzimuthSlider = nullptr;
	LLTextBox*      m_pSkyElevationText = nullptr;
	LLSliderCtrl*   m_pSkyElevationSlider = nullptr;
	LLComboBox*     m_pWaterPresetCombo = nullptr;
	LLButton*       m_pWaterPrevButton = nullptr;
	LLButton*       m_pWaterNextButton = nullptr;
	LLButton*       m_pWaterEditButton = nullptr;
	LLCheckBoxCtrl* m_pInterpolatePresetsCheck = nullptr;
};

// ====================================================================================
