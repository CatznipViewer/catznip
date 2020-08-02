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

#include "llviewerprecompiledheaders.h"

#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llenvironment.h"
#include "llfloaterreg.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llpanelquickprefswindlight.h"
#include "llradiogroup.h"
#include "llsliderctrl.h"
#include "llviewermenu.h"

#include <boost/algorithm/string.hpp>

 // Defined in rlvenvironment.cpp
F32 rlvGetAzimuthFromDirectionVector(const LLVector3& vecDir, bool fStrict);
F32 rlvGetElevationFromDirectionVector(const LLVector3& vecDir, bool fStrict);

// Defined in llsettingssky.cpp
LLQuaternion convert_azimuth_and_altitude_to_quat(F32 azimuth, F32 altitude);

// ====================================================================================
// LLQuickPrefsWindInventoryObserver class
//

class LLQuickPrefsWindInventoryObserver : public LLInventoryObserver
{
public:
	LLQuickPrefsWindInventoryObserver(LLHandle<LLQuickPrefsWindlightPanel> hQuickPrefsPanel)
		: m_hQuickPrefsPanel(hQuickPrefsPanel)
		, LLInventoryObserver()
	{
	}

	void changed(U32 mask) override
	{
		LLQuickPrefsWindlightPanel* pPanel = m_hQuickPrefsPanel.get();
		if ( (pPanel) && (mask & LLInventoryObserver::ADD) )
		{
			const auto& idItemIDs = gInventory.getAddedIDs();
			for (const auto& idItem : idItemIDs)
			{
				const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
				if (LLAssetType::AT_SETTINGS != pItem->getActualType())
					continue;
				pPanel->onAddEnvironment(idItem);
			}
		}
	}

protected:
	LLHandle<LLQuickPrefsWindlightPanel> m_hQuickPrefsPanel;
};

// ====================================================================================
// LLQuickPrefsWindlightPanel class
//

static LLPanelInjector<LLQuickPrefsWindlightPanel> t_quickprefs_windlight("quickprefs_windlight");

LLQuickPrefsWindlightPanel::EnvironmentSetting::EnvironmentSetting(const LLViewerInventoryItem* pItem, bool fIsLibrary)
{
	m_Type = pItem->getSettingsType();
	m_Name = pItem->getName();
	m_InventoryId = pItem->getUUID();
	m_Path = get_item_path(m_InventoryId, false);
	m_AssetId = pItem->getAssetUUID();
	m_IsLibrary = fIsLibrary;
}

LLQuickPrefsWindlightPanel::LLQuickPrefsWindlightPanel()
	: LLQuickPrefsPanel()
{
}

LLQuickPrefsWindlightPanel::~LLQuickPrefsWindlightPanel()
{
	if (m_pSettingsObserver)
	{
		gInventory.removeObserver(m_pSettingsObserver);
		delete m_pSettingsObserver;
		m_pSettingsObserver = nullptr;
	}
}

// override
BOOL LLQuickPrefsWindlightPanel::postBuild()
{
	getChild<LLButton>("windlight_usesharedenv_btn")->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onUseSharedEnvClicked, this));
	getChild<LLButton>("windlight_personaledit_btn")->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onShowPersonalLightingClicked, this));
	m_pInterpolatePresetsCheck = getChild<LLCheckBoxCtrl>("windlight_prefs_interpolate");

	m_pDayCyclePresetCombo = getChild<LLComboBox>("windlight_daycycle_combo");
	m_pDayCyclePresetCombo->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectEnvSetting, this, m_pDayCyclePresetCombo));
	m_pDayCyclePrevButton = getChild<LLButton>("windlight_daycycle_prevbtn");
	m_pDayCyclePrevButton->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectEnvSettingPrev, this, m_pDayCyclePresetCombo));
	m_pDayCycleNextButton = getChild<LLButton>("windlight_daycycle_nextbtn");
	m_pDayCycleNextButton->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectEnvSettingNext, this, m_pDayCyclePresetCombo));
	m_pDayCycleEditButton = getChild<LLButton>("windlight_daycycle_editbtn");
	m_pDayCycleEditButton->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onEditEnvSetting, this, m_pDayCyclePresetCombo));
	m_pDayOffsetText = getChild<LLTextBox>("windlight_dayoffset_text");
	m_pDayOffsetSlider = getChild<LLSliderCtrl>("windlight_dayoffset_slider");
	m_pDayOffsetSlider->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onDayOffsetChanged, this));
	m_pDayFreezeCheck = getChild<LLCheckBoxCtrl>("windlight_freezetime_check");
	m_pDayFreezeCheck->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onDayFreezeToggle, this));

	m_pSkyPresetCombo = getChild<LLComboBox>("windlight_fixedsky_combo");
	m_pSkyPresetCombo->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectEnvSetting, this, m_pSkyPresetCombo));
	m_pSkyPrevButton = getChild<LLButton>("windlight_fixedsky_prevbtn");
	m_pSkyPrevButton->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectEnvSettingPrev, this, m_pSkyPresetCombo));
	m_pSkyNextButton = getChild<LLButton>("windlight_fixedsky_nextbtn");
	m_pSkyNextButton->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectEnvSettingNext, this, m_pSkyPresetCombo));
	m_pSkyEditButton = getChild<LLButton>("windlight_fixedsky_editbtn");
	m_pSkyEditButton->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onEditEnvSetting, this, m_pSkyPresetCombo));
	m_pSkyGammaText = getChild<LLTextBox>("windlight_skygamma_text");
	m_pSkyGammaSlider = getChild<LLSliderCtrl>("windlight_skygamma_slider");
	m_pSkyGammaSlider->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSkyGammaChanged, this));
	m_pSkyCelestialBodyText = getChild<LLTextBox>("windlight_celestialbody_text");
	m_pSkyCelestialBodyGroup = getChild<LLRadioGroup>("windlight_celestialbody_group");
	m_pSkyAzimuthText = getChild<LLTextBox>("windlight_azimuth_text");
	m_pSkyAzimuthSlider = getChild<LLSliderCtrl>("windlight_azimuth_slider");
	m_pSkyAzimuthSlider->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSkyAzimuthChanged, this));
	m_pSkyElevationText = getChild<LLTextBox>("windlight_elevation_text");
	m_pSkyElevationSlider = getChild<LLSliderCtrl>("windlight_elevation_slider");
	m_pSkyElevationSlider->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSkyElevationChanged, this));

	m_pWaterPresetCombo = getChild<LLComboBox>("windlight_water_combo");
	m_pWaterPresetCombo->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectEnvSetting, this, m_pWaterPresetCombo));
	m_pWaterPrevButton = getChild<LLButton>("windlight_water_prevbtn");
	m_pWaterPrevButton->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectEnvSettingPrev, this, m_pWaterPresetCombo));
	m_pWaterNextButton = getChild<LLButton>("windlight_water_nextbtn");
	m_pWaterNextButton->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onSelectEnvSettingNext, this, m_pWaterPresetCombo));
	m_pWaterEditButton = getChild<LLButton>("windlight_water_editbtn");
	m_pWaterEditButton->setCommitCallback(boost::bind(&LLQuickPrefsWindlightPanel::onEditEnvSetting, this, m_pWaterPresetCombo));

	m_EnvironmentChangeConn = LLEnvironment::instance().setEnvironmentChanged(boost::bind(&LLQuickPrefsWindlightPanel::refreshControls, this, false));
	m_EnvironmentUpdateConn = LLEnvironment::instance().setEnvironmentUpdated(boost::bind(&LLQuickPrefsWindlightPanel::refreshControls, this, false));

	return LLQuickPrefsPanel::postBuild();
}

// override
void LLQuickPrefsWindlightPanel::draw()
{
	LLQuickPrefsPanel::draw();
	if (m_fNeedsRefresh && m_RefreshTimer.hasExpired())
	{
		refreshControls(true);
		m_fNeedsRefresh = false;
	}
	syncControls();
}

// override
void LLQuickPrefsWindlightPanel::onVisibilityChange(BOOL fVisible)
{
	if (fVisible)
	{
		if (!m_pSettingsObserver)
		{
			refreshEnvironments();

			m_pSettingsObserver = new LLQuickPrefsWindInventoryObserver(getDerivedHandle<LLQuickPrefsWindlightPanel>());
			gInventory.addObserver(m_pSettingsObserver);
		}
		refreshControls(true);
	}
}

void LLQuickPrefsWindlightPanel::onAddEnvironment(const LLUUID& idItem)
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);

	env_setting_vec_t* pSettings = nullptr;
	switch (pItem->getSettingsType())
	{
		case LLSettingsType::ST_DAYCYCLE:
			pSettings = &m_DayCycles;
			break;
		case LLSettingsType::ST_SKY:
			pSettings = &m_Skies;
			break;
		case LLSettingsType::ST_WATER:
			pSettings = &m_Waters;
			break;
		default:
			return;
	}

	// Unlike refreshEnvironments we have to check for dupes
	if (pSettings->end() == std::find_if(pSettings->begin(), pSettings->end(), [idItem](const EnvironmentSetting& envSetting) {  return idItem == envSetting.m_InventoryId; }))
	{
		pSettings->push_back(EnvironmentSetting(pItem, gInventory.isObjectDescendentOf(idItem, gInventory.getLibraryRootFolderID())));
		m_fNeedsRefresh = true;
		m_RefreshTimer.setTimerExpirySec(0.5);
	}
}

void LLQuickPrefsWindlightPanel::onDayOffsetChanged()
{
	F32 fracCurProgress = LLEnvironment::instance().getProgress();
	F32 fracNewProgress = m_pDayOffsetSlider->getValueF32() / 100;
	LLSettingsDay::Seconds secDayOffset = LLEnvironment::instance().getCurrentDayOffset() + (LLSettingsDay::Seconds)((fracNewProgress - fracCurProgress) * LLEnvironment::instance().getCurrentDayLength());
	LLEnvironment::instance().setCurrentDayOffset(secDayOffset);
}

void LLQuickPrefsWindlightPanel::onDayFreezeToggle()
{
	LLEnvironment::instance().setCurrentDayRunning(!m_pDayFreezeCheck->get());
}

void LLQuickPrefsWindlightPanel::onEditEnvSetting(/*const*/ LLComboBox* pComboBox)
{
	const LLUUID idInventory = pComboBox->getSelectedValue().asUUID();
	if (idInventory.notNull())
	{
		LLInvFVBridgeAction::doAction(LLAssetType::AT_SETTINGS, idInventory, &gInventory);
	}
}

void LLQuickPrefsWindlightPanel::onShowPersonalLightingClicked()
{
	LLFloaterReg::showInstance("env_adjust_snapshot");
}

void LLQuickPrefsWindlightPanel::onSelectEnvSetting(/*const*/ LLComboBox* pComboBox)
{
	const LLUUID idInventory = pComboBox->getSelectedValue().asUUID();
	if (const LLViewerInventoryItem* pItem = (idInventory.notNull()) ? gInventory.getItem(idInventory) : nullptr)
	{
		LLSettingsBase::Seconds transitionDuration = (m_pInterpolatePresetsCheck->get()) ? LLEnvironment::TRANSITION_FAST : LLEnvironment::TRANSITION_INSTANT;
		LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, pItem->getAssetUUID(), transitionDuration);
		LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL, transitionDuration);
	}
}

void LLQuickPrefsWindlightPanel::onSelectEnvSettingPrev(LLComboBox* pComboBox)
{
	if ((pComboBox) && (pComboBox->selectPrevItem()))
	{
		pComboBox->onCommit();
	}
}

void LLQuickPrefsWindlightPanel::onSelectEnvSettingNext(LLComboBox* pComboBox)
{
	if ((pComboBox) && (pComboBox->selectNextItem()))
	{
		pComboBox->onCommit();
	}
}

void LLQuickPrefsWindlightPanel::onSkyAzimuthChanged()
{
	switchToLocalEnv();
	if (LLSettingsSky::ptr_t pCurSky = LLEnvironment::instance().getLocalSky())
	{
		bool isSun = (m_pSkyCelestialBodyGroup->getValue().asInteger() == 0);
		const LLVector3 dirCelestialBody = LLVector3::x_axis * (isSun ? pCurSky->getSunRotation() : pCurSky->getMoonRotation());
		const LLQuaternion rotCelestialBody = convert_azimuth_and_altitude_to_quat(m_pSkyAzimuthSlider->getValueF32() * DEG_TO_RAD, rlvGetElevationFromDirectionVector(dirCelestialBody, true));
		if (isSun)
			pCurSky->setSunRotation(rotCelestialBody);
		else
			pCurSky->setMoonRotation(rotCelestialBody);
		pCurSky->update();
	}
}

void LLQuickPrefsWindlightPanel::onSkyElevationChanged()
{
	switchToLocalEnv();
	if (LLSettingsSky::ptr_t pCurSky = LLEnvironment::instance().getCurrentSky())
	{
		bool isSun = (m_pSkyCelestialBodyGroup->getValue().asInteger() == 0);
		const LLVector3 dirCelestialBody = LLVector3::x_axis * ((m_pSkyCelestialBodyGroup->getValue().asInteger() == 0) ? pCurSky->getSunRotation() : pCurSky->getMoonRotation());
		const LLQuaternion rotCelestialBody = convert_azimuth_and_altitude_to_quat(rlvGetAzimuthFromDirectionVector(dirCelestialBody, true), m_pSkyElevationSlider->getValueF32() * DEG_TO_RAD);
		if (isSun)
			pCurSky->setSunRotation(rotCelestialBody);
		else
			pCurSky->setMoonRotation(rotCelestialBody);
		pCurSky->update();
	}
}

void LLQuickPrefsWindlightPanel::onSkyGammaChanged()
{
	switchToLocalEnv();

	LLSettingsSky::ptr_t pCurSky = LLEnvironment::instance().getCurrentSky();
	pCurSky->setGamma(m_pSkyGammaSlider->getValueF32());
	pCurSky->update();
}

void LLQuickPrefsWindlightPanel::onUseSharedEnvClicked()
{
	LLSettingsBase::Seconds transitionDuration = (m_pInterpolatePresetsCheck->get()) ? LLEnvironment::TRANSITION_FAST : LLEnvironment::TRANSITION_INSTANT;
	LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_LOCAL);
	LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL, transitionDuration);
	LLEnvironment::instance().resetCurrentDayOffset();
	LLEnvironment::instance().setCurrentDayRunning(true);
	defocus_env_floaters();
}

void LLQuickPrefsWindlightPanel::refreshControls(bool fRefreshPresets)
{
	static const std::string s_SharedLabel = getString("WINDLIGHT_SHARED");
	static const std::string s_ParcelLabel = getString("WINDLIGHT_PARCEL");
	static const std::string s_RegionLabel = getString("WINDLIGHT_REGION");
	static const std::string s_CustomLabel = getString("WINDLIGHT_CUSTOM");
	static const std::string s_DayCycleLabel = getString("WINDLIGHT_DAYCYCLE");
	static const std::string s_FixedSkyLabel = getString("WINDLIGHT_FIXEDSKY");
	static auto getSharedLabel = [](const LLEnvironment::EnvSelection_t& env)
	{
		switch (env)
		{
			case LLEnvironment::ENV_PARCEL: return s_ParcelLabel;
			case LLEnvironment::ENV_REGION: return s_RegionLabel;
			default: return s_SharedLabel;
		}
	};

	if (!isInVisibleChain())
	{
		return;
	}

	if (fRefreshPresets)
	{
		populateSettingsList(m_pDayCyclePresetCombo, m_DayCycles);
		populateSettingsList(m_pSkyPresetCombo, m_Skies);
		populateSettingsList(m_pWaterPresetCombo, m_Waters);
	}

	LLEnvironment* pEnvMgr = LLEnvironment::getInstance();
	LLEnvironment::EnvSelection_t curEnv = pEnvMgr->getCurrentSelection();
	bool fUseSharedEnv = !pEnvMgr->getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL);
	bool fIsCurSkyFixed = pEnvMgr->isCurrentSkyFixed();

	auto updatePresetCombo = [fUseSharedEnv](LLComboBox* pComboBox, const std::vector<EnvironmentSetting>& settingList, LLSettingsBase::ptr_t pEnvSettings, const std::string& strSharedLabel, LLButton* pPrevButton, LLButton* pNextButton, LLButton* pEditButton)
		{
			pComboBox->clear();
			if ( (!fUseSharedEnv) && (pEnvSettings) )
			{
				const LLUUID idLocalAsset = pEnvSettings->getBaseAssetId();
				if (idLocalAsset.notNull())
				{
					const LLUUID idSelItem = pComboBox->getSelectedValue().asUUID();
					// Favour the currently selected item if possible (failing that favour local inventory over library)
					auto itSelSetting = (idSelItem.notNull()) ? std::find_if(settingList.begin(), settingList.end(), [&idSelItem](const EnvironmentSetting& s) { return s.m_InventoryId == idSelItem; })
															  : settingList.end();
					if ( (settingList.end() == itSelSetting) || (itSelSetting->m_AssetId != idLocalAsset) )
						itSelSetting = std::find_if(settingList.begin(), settingList.end(), [&idLocalAsset](const EnvironmentSetting& s) { return s.m_AssetId == idLocalAsset; });
					if ( (settingList.end() == itSelSetting) || (!pComboBox->selectByValue(itSelSetting->m_InventoryId)) )
						pComboBox->setLabel(s_CustomLabel);
				}
				else
				{
					pComboBox->setLabel(s_CustomLabel);
				}
			}
			else
			{
				pComboBox->setLabel(strSharedLabel);
			}
			bool fHasSel = pComboBox->getCurrentIndex() != -1;
			pPrevButton->setEnabled(fHasSel);
			pNextButton->setEnabled(fHasSel);
			pEditButton->setEnabled(fHasSel);
		};

	updatePresetCombo(m_pDayCyclePresetCombo, m_DayCycles, pEnvMgr->getLocalDay(), (!fIsCurSkyFixed ? getSharedLabel(curEnv) : s_FixedSkyLabel), m_pDayCycleEditButton, m_pDayCycleEditButton, m_pDayCycleEditButton);
	m_pDayOffsetText->setEnabled(!fIsCurSkyFixed);
	m_pDayOffsetSlider->setEnabled(!fIsCurSkyFixed);
	m_pDayFreezeCheck->setEnabled(!fIsCurSkyFixed);

	updatePresetCombo(m_pSkyPresetCombo, m_Skies, pEnvMgr->getLocalSky(), (fIsCurSkyFixed ? getSharedLabel(curEnv) : s_DayCycleLabel), m_pSkyPrevButton, m_pSkyNextButton, m_pSkyEditButton);

	updatePresetCombo(m_pWaterPresetCombo, m_Waters, pEnvMgr->getLocalWater(), (fUseSharedEnv ? getSharedLabel(curEnv) : s_DayCycleLabel), m_pWaterPrevButton, m_pWaterNextButton, m_pWaterEditButton);
}

void LLQuickPrefsWindlightPanel::refreshEnvironments()
{
	auto processSettingsItems = [this](const LLInventoryModel::item_array_t& items, bool isLibrary)
	{
		for (const LLViewerInventoryItem* pItem : items)
		{
			switch (pItem->getSettingsType())
			{
				case LLSettingsType::ST_DAYCYCLE:
					m_DayCycles.push_back(EnvironmentSetting(pItem, isLibrary));
					break;
				case LLSettingsType::ST_SKY:
					m_Skies.push_back(EnvironmentSetting(pItem, isLibrary));
					break;
				case LLSettingsType::ST_WATER:
					m_Waters.push_back(EnvironmentSetting(pItem, isLibrary));
					break;
			}
		}
	};

	m_DayCycles.erase(std::remove_if(m_DayCycles.begin(), m_DayCycles.end(), [](const EnvironmentSetting& envSetting) { return !envSetting.m_IsLibrary; }), m_DayCycles.end());
	m_Skies.erase(std::remove_if(m_Skies.begin(), m_Skies.end(), [](const EnvironmentSetting& envSetting) { return !envSetting.m_IsLibrary; }), m_Skies.end());
	m_Waters.erase(std::remove_if(m_Waters.begin(), m_Waters.end(), [](const EnvironmentSetting& envSetting) { return !envSetting.m_IsLibrary; }), m_Waters.end());

	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLIsTypeActual f(LLAssetType::AT_SETTINGS);
	if (m_Skies.empty()) {
		gInventory.collectDescendentsIf(gInventory.getLibraryRootFolderID(), cats, items, LLInventoryModel::EXCLUDE_TRASH, f);
		processSettingsItems(items, true);
		items.clear();
	}

	gInventory.collectDescendentsIf(gInventory.getRootFolderID(), cats, items, LLInventoryModel::EXCLUDE_TRASH, f);
	processSettingsItems(items, false);
	items.clear();
}

// static
void LLQuickPrefsWindlightPanel::sortSettingsList(env_setting_vec_t& settingList)
{
	std::sort(settingList.begin(), settingList.end(), [](const LLQuickPrefsWindlightPanel::EnvironmentSetting& lhs, const LLQuickPrefsWindlightPanel::EnvironmentSetting& rhs)
	{
		if (lhs.m_IsLibrary != rhs.m_IsLibrary)
			return !lhs.m_IsLibrary;
		int cmp = lhs.m_Path.compare(rhs.m_Path);
		if (cmp == 0)
			return lhs.m_Name < rhs.m_Name;
		return cmp < 0;
	});
}

// static
void LLQuickPrefsWindlightPanel::populateSettingsList(LLComboBox* pComboBox, std::vector<EnvironmentSetting>& settingList)
{
	LLSD sdFolderElement;
	sdFolderElement["enabled"] = false;
	sdFolderElement["columns"][0] = LLSD().with("type", "text").with("font", LLSD().with("style", "BOLD"));

	LLSD sdSettingsElement;
	sdSettingsElement["columns"][0] = LLSD().with("type", "text").with("pad_left", 12);

	const LLUUID idSelItem = pComboBox->getSelectedValue().asUUID();
	pComboBox->clearRows();

	std::string strCurPath;
	sortSettingsList(settingList);
	for (const EnvironmentSetting& env : settingList)
	{
		if (strCurPath != env.m_Path)
		{
			sdFolderElement["columns"][0]["value"] = (!env.m_IsLibrary) ? env.m_Path : "Library/" + env.m_Path;
			pComboBox->addElement(sdFolderElement);
			strCurPath = env.m_Path;
		}

		sdSettingsElement["value"] = env.m_InventoryId;
		sdSettingsElement["columns"][0]["value"] = env.m_Name;
		pComboBox->addElement(sdSettingsElement);
	}

	pComboBox->selectByValue(idSelItem);
}

void LLQuickPrefsWindlightPanel::switchToLocalEnv()
{
	LLEnvironment* pEnvMgr = LLEnvironment::getInstance();
	if (!pEnvMgr->getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL))
	{
		// Copy shared environment to local (*TODO-Catznip: shouldn't this be ENV_PUSH?)
		pEnvMgr->setEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::instance().getEnvironmentFixedSky(LLEnvironment::ENV_PARCEL, true)->buildClone());
		pEnvMgr->setEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::instance().getEnvironmentFixedWater(LLEnvironment::ENV_PARCEL, true)->buildClone());
		pEnvMgr->setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_INSTANT);
		pEnvMgr->updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
	}
}

void LLQuickPrefsWindlightPanel::syncControls()
{
	LLEnvironment* pEnvMgr = LLEnvironment::getInstance();
	LLSettingsSky::ptr_t pCurSky = pEnvMgr->getCurrentSky();

	if (!pEnvMgr->isCurrentSkyFixed())
	{
		m_pDayFreezeCheck->set(pEnvMgr->getCurrentDayRunning());
		if (!m_pDayFreezeCheck->get())
		{
			// Bit of a hack but progress is based on the current time and we don't want to refactor all of that right now
			m_pDayOffsetSlider->setValue(pEnvMgr->getProgress() * 100);
		}
	}

	const LLVector3 dirCelestialBody = LLVector3::x_axis * ((m_pSkyCelestialBodyGroup->getValue().asInteger() == 0) ? pCurSky->getSunRotation() : pCurSky->getMoonRotation());
	m_pSkyAzimuthSlider->setValue(rlvGetAzimuthFromDirectionVector(dirCelestialBody, true) * RAD_TO_DEG);
	m_pSkyElevationSlider->setValue(rlvGetElevationFromDirectionVector(dirCelestialBody, true) * RAD_TO_DEG);
	m_pSkyGammaSlider->setValue(pCurSky->getGamma());
}

// ====================================================================================
