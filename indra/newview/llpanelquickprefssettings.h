/**
 *
 * Copyright (c) 2016, Kitty Barnett
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

// ====================================================================================
// Foward declarations
//

class LLCheckBoxCtrl;
class LLComboBox;
class LLSliderCtrl;

// ====================================================================================
// LLQuickPrefsWindlightPanel class
//

class LLQuickPrefsWindlightPanel : public LLQuickPrefsPanel
{
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
	void syncControls();

	/*
	 * Event handlers
	 */
protected:
	void onEastAngleChanged();
	void onEditDayCycle();
	void onEditSkyPreset();
	void onEditWaterPreset();
	void onResetWindLight();
	void onSelectComboPrev(LLComboBox* pComboBox);
	void onSelectComboNext(LLComboBox* pComboBox);
	void onSelectDayCyclePreset();
	void onSelectSkyPreset();
	void onSelectWaterPreset();
	void onSunPositionMoved();
	void onSunPositionFreezeToggle();
	void onUseRegionSettings();

	/*
	 * Member variables
	 */
protected:
	LLCheckBoxCtrl* m_pUseRegionCheck = nullptr;
	LLComboBox*     m_pDayCyclePresetCombo = nullptr;
	LLButton*       m_pDayCycleEditButton = nullptr;
	LLComboBox*     m_pSkyPresetCombo = nullptr;
	LLButton*       m_pSkyEditButton = nullptr;
	LLComboBox*     m_pWaterPresetCombo = nullptr;
	LLButton*       m_pWaterEditButton = nullptr;
	LLTextBox*      m_pSceneGammaText = nullptr;
	LLSliderCtrl*   m_pSceneGammaSlider = nullptr;
	LLTextBox*      m_pEastAngleText = nullptr;
	LLSliderCtrl*   m_pEastAngleSlider = nullptr;
	LLCheckBoxCtrl* m_pInterpolatePresetsCheck = nullptr;
	LLSliderCtrl*   m_pSunMoonSlider = nullptr;
	LLCheckBoxCtrl* m_pSunMoonFreezeCheck = nullptr;
};

// ====================================================================================
