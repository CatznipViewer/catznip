/** 
 *
 * Copyright (c) 2013, Kitty Barnett
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
// LLFloaterChatSounds class
//

LLFloaterChatSounds::LLFloaterChatSounds(const LLSD& sdKey)
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

	mCommitCallbackRegistrar.add("InitSoundsCombo", boost::bind(&LLFloaterChatSounds::onInitSoundsCombo, this, _1, _2));
}

LLFloaterChatSounds::~LLFloaterChatSounds()
{
}

BOOL LLFloaterChatSounds::postBuild(void)
{
	LLComboBox* pIMFriendSound = findChild<LLComboBox>("sound_im_friend");
	findChild<LLSoundDropTarget>("drop_im_friend")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pIMFriendSound));
	findChild<LLButton>("preview_sound_im_friend")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pIMFriendSound));

	LLComboBox* pIMNonFriendSound = findChild<LLComboBox>("sound_im_nonfriend");
	findChild<LLSoundDropTarget>("drop_im_nonfriend")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pIMNonFriendSound));
	findChild<LLButton>("preview_sound_im_nonfriend")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pIMNonFriendSound));

	LLComboBox* pIMConferenceSound = findChild<LLComboBox>("sound_im_conference");
	findChild<LLSoundDropTarget>("drop_im_conference")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pIMConferenceSound));
	findChild<LLButton>("preview_sound_im_conference")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pIMConferenceSound));

	LLComboBox* pIMGroupSound = findChild<LLComboBox>("sound_im_group");
	findChild<LLSoundDropTarget>("drop_im_group")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pIMGroupSound));
	findChild<LLButton>("preview_sound_im_group")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pIMGroupSound));

	return TRUE;
}

LLSD LLFloaterChatSounds::getSelectedSound(LLComboBox* pCombo)
{
	if (pCombo)
	{
		const LLSD& sdValue = pCombo->getSelectedValue();
		if (sdValue.isString())
		{
			// There are three possibilities:
			//   * i|<uuid> => inventory item specified by UUID
			//   * a|<uuid> => sound identified by asset UUID
			//   * <other>  => sound identified by string name
			// (The reason for this hackery is that selectByValue will use LLSD::asString to select by value)
			const std::string strValue = sdValue.asString();
			if ( (strValue.length() > 2) && ('|' == strValue[1]) )
			{
				const LLUUID idSound(strValue.substr(2));
				if ('a' == strValue[0])
					return LLSD().with("type", 0).with("asset_id", idSound);
				else if ('i' == strValue[0])
					return LLSD().with("type", 1).with("item_id", idSound);
			}
			else 
			{
				return sdValue;
			}
		}
	}
	return LLSD();
}

void LLFloaterChatSounds::onDropSound(const LLUUID& idSound, LLComboBox* pCombo)
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

void LLFloaterChatSounds::onInitSoundsCombo(LLUICtrl* pCtrl, const LLSD& sdParam)
{
	LLComboBox* pCombo = dynamic_cast<LLComboBox*>(pCtrl);
	if (!pCombo)
		return;

	//
	// Save the debug setting this combobox controls and add it as the default option
	//
	LLControlVariablePtr pControl = gSavedSettings.getControl(sdParam.asString());
	if (pControl.isNull())
		return;

	m_SettingsMap[pCombo->getName()] = pControl->getName();
	pCombo->add("Default", pControl->getDefault().asString());

	//
	// Add sounds from sounds.xml
	//
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

	//
	// Add the custom option
	//
	pCombo->addSeparator();
	pCombo->add("Custom (Drag and drop sound here)", pControl->getDefault().asString());
}

void LLFloaterChatSounds::onPreviewSound(LLComboBox* pCombo)
{
	if (!pCombo)
		return;
	make_ui_sound(LLViewerChat::getUISoundFromLLSD(getSelectedSound(pCombo)));
}

// ============================================================================
