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

#include "llviewerprecompiledheaders.h"

#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llpanelquickprefssettings.h"
#include "llsliderctrl.h"

// Windlight panel
#include "lldaycyclemanager.h"
#include "llenvmanager.h"
#include "llfloaterenvironmentsettings.h"
#include "llwaterparammanager.h"
#include "llwlparammanager.h"

// ====================================================================================
// LLQuickPrefsWindlightPanel class
//

static LLPanelInjector<LLQuickPrefsWindlightPanel> t_quickprefs_windlight("quickprefs_windlight");

// From llfloatereditsky.cpp
F32 sun_pos_to_time24(F32 sun_pos);
F32 time24_to_sun_pos(F32 time24);

LLQuickPrefsWindlightPanel::LLQuickPrefsWindlightPanel()
	: LLQuickPrefsPanel()
{
}

LLQuickPrefsWindlightPanel::~LLQuickPrefsWindlightPanel()
{
}

// virtual
BOOL LLQuickPrefsWindlightPanel::postBuild()
{
	m_pUseRegionCheck = getChild<LLCheckBoxCtrl>("windlight_regiondefault_check");
	m_pUseRegionCheck->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onUseRegionSettings, this));

	m_pDayCyclePresetCombo = getChild<LLComboBox>("windlight_daycycle_combo");
	m_pDayCyclePresetCombo->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectDayCyclePreset, this));
	LLDayCycleManager::instance().setModifyCallback(boost::bind(&LLFloaterEnvironmentSettings::populateDayCyclePresetsList, m_pDayCyclePresetCombo));
	getChild<LLButton>("windlight_daycycle_prevbtn")->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectComboPrev, this, m_pDayCyclePresetCombo));
	getChild<LLButton>("windlight_daycycle_nextbtn")->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectComboNext, this, m_pDayCyclePresetCombo));

	m_pSkyPresetCombo = getChild<LLComboBox>("windlight_fixedsky_combo");
	m_pSkyPresetCombo->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectSkyPreset, this));
	LLWLParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterEnvironmentSettings::populateSkyPresetsList, m_pSkyPresetCombo));
	getChild<LLButton>("windlight_fixedsky_prevbtn")->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectComboPrev, this, m_pSkyPresetCombo));
	getChild<LLButton>("windlight_fixedsky_nextbtn")->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectComboNext, this, m_pSkyPresetCombo));

	m_pWaterPresetCombo = getChild<LLComboBox>("windlight_water_combo");
	m_pWaterPresetCombo->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectWaterPreset, this));
	LLWaterParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterEnvironmentSettings::populateWaterPresetsList, m_pWaterPresetCombo));
	getChild<LLButton>("windlight_water_prevbtn")->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectComboPrev, this, m_pWaterPresetCombo));
	getChild<LLButton>("windlight_water_nextbtn")->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectComboNext, this, m_pWaterPresetCombo));

	LLEnvManagerNew::instance().setPreferencesChangeCallback(boost::bind(&LLQuickPrefsWindlightPanel::refreshControls, this, false));

	m_pSunMoonSlider = getChild<LLSliderCtrl>("windlight_sunmoon_position");
	m_pSunMoonSlider->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSunPositionMoved, this));

	m_pSunMoonFreezeCheck = getChild<LLCheckBoxCtrl>("windlight_sunmoon_freeze_check");
	m_pSunMoonFreezeCheck->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSunPositionFreezeToggle, this));

	m_pInterpolatePresetsCheck = getChild<LLCheckBoxCtrl>("windlight_prefs_interpolate");

	getChild<LLButton>("windlight_reset_btn")->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onResetWindLight, this));

	return LLQuickPrefsPanel::postBuild();
}

// virtual
void LLQuickPrefsWindlightPanel::draw()
{
	LLQuickPrefsPanel::draw();

	float nTime24 = 0.f;
	if (LLEnvManagerNew::instance().getUseFixedSky())
		nTime24 = sun_pos_to_time24(LLWLParamManager::instance().mCurParams.getSunAngle() / F_TWO_PI);
	else
		nTime24 = (float)(LLWLParamManager::instance().mAnimator.getDayTime() * 24.f);
	m_pSunMoonSlider->setValue(nTime24, true);
	m_pSunMoonFreezeCheck->set(!LLWLParamManager::instance().mAnimator.getIsRunning());
}

// virtual
void LLQuickPrefsWindlightPanel::onVisibilityChange(BOOL fVisible)
{
	if (fVisible)
	{
		refreshControls(true);
	}
}

void LLQuickPrefsWindlightPanel::onResetWindLight()
{
	LLEnvManagerNew::resetUserPrefs();
}

void LLQuickPrefsWindlightPanel::onSelectComboPrev(LLComboBox* pComboBox)
{
	if ((pComboBox) && (pComboBox->selectPrevItem()))
	{
		pComboBox->onCommit();
	}
}

void LLQuickPrefsWindlightPanel::onSelectComboNext(LLComboBox* pComboBox)
{
	if ((pComboBox) && (pComboBox->selectNextItem()))
	{
		pComboBox->onCommit();
	}
}

void LLQuickPrefsWindlightPanel::onSelectDayCyclePreset()
{
	LLEnvManagerNew::instance().setUseDayCycle(m_pDayCyclePresetCombo->getValue().asString(), m_pInterpolatePresetsCheck->get());
	onCommit();
}

void LLQuickPrefsWindlightPanel::onSelectSkyPreset()
{
	LLEnvManagerNew::instance().setUseSkyPreset(m_pSkyPresetCombo->getValue().asString(), m_pInterpolatePresetsCheck->get());
	onCommit();
}

void LLQuickPrefsWindlightPanel::onSelectWaterPreset()
{
	LLEnvManagerNew::instance().setUseWaterPreset(m_pWaterPresetCombo->getValue().asString(), m_pInterpolatePresetsCheck->get());
	onCommit();
}

void LLQuickPrefsWindlightPanel::onSunPositionMoved()
{
	if (LLEnvManagerNew::instance().getUseFixedSky())
	{
		// Fixed sky so just change the sun angle
		LLWLParamManager::instance().mAnimator.deactivate();
		float nAngle = time24_to_sun_pos(m_pSunMoonSlider->getValueF32()) * F_TWO_PI;
		LLWLParamManager::instance().mCurParams.setSunAngle(nAngle);
	}
	else
	{
		// Select the proper time of day for the running day cycle
		LLWLParamManager* pParamMgr = LLWLParamManager::getInstance();
		float nTimeOfDay = m_pSunMoonSlider->getValueF32() / 24.0f;
		pParamMgr->mAnimator.setDayTime(nTimeOfDay);
		pParamMgr->mAnimator.deactivate();
		pParamMgr->mAnimator.update(pParamMgr->mCurParams);
	}
}

void LLQuickPrefsWindlightPanel::onSunPositionFreezeToggle()
{
	if (m_pSunMoonFreezeCheck->get())
	{
		LLWLParamManager::instance().mAnimator.deactivate();
	}
	else if (!LLEnvManagerNew::instance().getUseFixedSky())
	{
		float nTimeOfDay = m_pSunMoonSlider->getValueF32() / 24.0f;
		LLWLParamManager::instance().resetAnimator(nTimeOfDay, true);
	}
}

void LLQuickPrefsWindlightPanel::onUseRegionSettings()
{
	LLEnvManagerNew::instance().setUseRegionSettings(m_pUseRegionCheck->get(), m_pInterpolatePresetsCheck->get());
	onCommit();
}

// virtual
void LLQuickPrefsWindlightPanel::refreshControls(bool fRefreshPresets)
{
	if (!isInVisibleChain())
	{
		return;
	}

	if (fRefreshPresets)
	{
		LLFloaterEnvironmentSettings::populateDayCyclePresetsList(m_pDayCyclePresetCombo);
		LLFloaterEnvironmentSettings::populateSkyPresetsList(m_pSkyPresetCombo);
		LLFloaterEnvironmentSettings::populateWaterPresetsList(m_pWaterPresetCombo);
	}

	LLEnvManagerNew* pEnvMgr = LLEnvManagerNew::getInstance();
	bool fUseRegionSettings = pEnvMgr->getUseRegionSettings();
	bool fUseDayCycle = pEnvMgr->getUseDayCycle();
	bool fUseFixedSky = pEnvMgr->getUseFixedSky();

	m_pUseRegionCheck->set(pEnvMgr->getUseRegionSettings());

	if ((!fUseRegionSettings) && (fUseDayCycle))
		m_pDayCyclePresetCombo->selectByValue(pEnvMgr->getDayCycleName());
	else
		m_pDayCyclePresetCombo->setLabel(getString((fUseRegionSettings) ? "WINDLIGHT_REGION" : "WINDLIGHT_FIXEDSKY"));

	if ((!fUseRegionSettings) && (fUseFixedSky))
		m_pSkyPresetCombo->selectByValue(pEnvMgr->getSkyPresetName());
	else
		m_pSkyPresetCombo->setLabel(getString((fUseRegionSettings) ? "WINDLIGHT_REGION" : "WINDLIGHT_DAYCYCLE"));

	if (!fUseRegionSettings)
		m_pWaterPresetCombo->selectByValue(pEnvMgr->getWaterPresetName());
	else
		m_pWaterPresetCombo->setLabel(getString("WINDLIGHT_REGION"));

	m_pSunMoonFreezeCheck->setEnabled(!fUseFixedSky);
}

// ====================================================================================
