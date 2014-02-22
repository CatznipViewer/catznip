/** 
 *
 * Copyright (c) 2013-2014, Kitty Barnett
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

#include "llcombobox.h"
#include "llfloaterchatalerts.h"
#include "llfloaterchatsounds.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llsdserialize.h"
#include "llviewerchat.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"

// ============================================================================
// LLFloaterUISounds class
//

LLFloaterUISounds::LLFloaterUISounds(const LLSD& sdKey)
	: LLFloater(sdKey)
{
	std::vector<std::string> strFiles = gDirUtilp->findSkinnedFilenames(LLDir::XUI, "sounds.xml", LLDir::ALL_SKINS);
	if (!strFiles.empty())
	{
		llifstream fileStream(strFiles.front(), std::ios::binary);
		if (fileStream.is_open())
		{
			LLSDSerialize::fromXMLDocument(m_sdSounds, fileStream);
			fileStream.close();
		}
	}

	mCommitCallbackRegistrar.add("InitSoundsCombo", boost::bind(&LLFloaterUISounds::onInitSoundsCombo, this, _1, _2));
}

LLFloaterUISounds::~LLFloaterUISounds()
{
}

BOOL LLFloaterUISounds::postBuild(void)
{
	const char* pstrComboNames[] =
	{
		"sound_friend_conv", "sound_friend_im",
		"sound_nonfriend_conv", "sound_nonfriend_im",
		"sound_conference_conv", "sound_conference_im",
		"sound_group_conv", "sound_group_im"
	};

	for (int idxComboName = 0, cntComboName = sizeof(pstrComboNames) / sizeof(char*); idxComboName < cntComboName; idxComboName++)
		initComboCallbacks(findChild<LLComboBox>(pstrComboNames[idxComboName]));
	return TRUE;
}

void LLFloaterUISounds::initComboCallbacks(LLComboBox* pCombo)
{
	pCombo->setCommitCallback(boost::bind(&LLFloaterUISounds::onSelectSound, this, _1));
	pCombo->getListControl()->setCommitOnSelectionChange(true);
	findChild<LLButton>(pCombo->getName() + "_preview")->setCommitCallback(boost::bind(&LLFloaterUISounds::onPreviewSound, this, pCombo));
	findChild<LLSoundDropTarget>(pCombo->getName() + "_drop")->setDropCallback(boost::bind(&LLFloaterUISounds::onDropSound, this, _1, pCombo));

	// Select the current setting
	setttings_map_t::const_iterator itSetting = m_SettingsMap.find(pCombo->getName());
	if (m_SettingsMap.end() != itSetting)
	{
		const std::string strSetting = gSavedSettings.getString(itSetting->second);
		if ( (strSetting.length() > 2) && ('i' == strSetting[0]) )
			onDropSound(LLUUID(strSetting.substr(2)), pCombo);
		else
			pCombo->selectByValue(strSetting);
	}
}

void LLFloaterUISounds::onDropSound(const LLUUID& idSound, LLComboBox* pCombo)
{
	const LLInventoryItem* pItem = gInventory.getItem(idSound);
	if ( (pItem) && (pCombo) && (pCombo->getEnabled()) )
	{
		const std::string& strValue = "i|" + pItem->getUUID().asString();
		if (!pCombo->selectByValue(strValue))
		{
			LLScrollListItem* pNewItem = pCombo->add(pItem->getName(), strValue);
			if (pNewItem)
			{
				LLScrollListText* pCell = dynamic_cast<LLScrollListText*>(pNewItem->getColumn(0));
				if (pCell)
					pCell->setIcon(LLInventoryIcon::getIconName(LLInventoryType::ICONNAME_SOUND));
			}
			pCombo->selectByValue(strValue);
		}
	}
}

void LLFloaterUISounds::onInitSoundsCombo(LLUICtrl* pCtrl, const LLSD& sdParam)
{
	LLComboBox* pCombo = dynamic_cast<LLComboBox*>(pCtrl);
	if (!pCombo)
		return;

	// Save the debug setting this combobox controls and add it as the default option
	LLControlVariablePtr pControl = gSavedSettings.getControl(sdParam.asString());
	if (pControl.isNull())
		return;

	m_SettingsMap[pCombo->getName()] = pControl->getName();
	pCombo->add("Default", pControl->getDefault().asString());

	// Add sounds from sounds.xml
	if ( (m_sdSounds.isMap()) && (m_sdSounds.size() > 0) )
	{
		pCombo->addSeparator();
		for (LLSD::map_const_iterator itSound = m_sdSounds.beginMap(); itSound != m_sdSounds.endMap(); ++itSound)
		{
			const LLSD& sdSound = itSound->second;
			if (!sdSound.has("asset_uuid"))
				continue;
			pCombo->add(itSound->first, "a|" + sdSound["asset_uuid"].asString());
		}
	}

	// Add the custom option
	pCombo->addSeparator();
	pCombo->add("Custom (Drop sound here)", pControl->getDefault().asString());
}

void LLFloaterUISounds::onPreviewSound(LLComboBox* pCombo)
{
	if (!pCombo)
		return;
	make_ui_sound(LLViewerChat::getUISoundFromSettingsString(pCombo->getValue().asString()));
}

void LLFloaterUISounds::onSelectSound(const LLUICtrl* pCtrl)
{
	setttings_map_t::const_iterator itSetting = (pCtrl) ? m_SettingsMap.find(pCtrl->getName()) : m_SettingsMap.end();
	if (m_SettingsMap.end() != itSetting)
	{
		gSavedSettings.setString(itSetting->second, pCtrl->getValue().asString());
	}
}

// ============================================================================
