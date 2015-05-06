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
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llsdserialize.h"
#include "llsoundpicker.h"
#include "llviewerchat.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"

// ============================================================================
// LLSoundPickerCtrl class
//

static LLDefaultChildRegistry::Register<LLSoundPickerCtrl> r("sound_picker");

LLSD LLSoundPickerCtrl::s_sdSounds;

LLSoundPickerCtrl::Params::Params()
	: sound_combo("sound_combo")
	, preview_button("preview_button")
	, drop_target("drop_target")
{
}

LLSoundPickerCtrl::LLSoundPickerCtrl(const LLSoundPickerCtrl::Params& p)
	: LLPanel(p)
	, m_pSoundCombo(NULL)
	, m_pPreviewBtn(NULL)
	, m_pDropTarget(NULL)
{
	//
	// Set up the child controls
	//
	LLRect rct(p.rect);

	// Preview button
	LLButton::Params preview_button = p.preview_button;
	S32 padCenter = (rct.getHeight() - preview_button.rect.height) / 2;
	preview_button.rect = LLRect(rct.getWidth() - preview_button.rect.width(), padCenter + preview_button.rect.height, rct.getWidth(), padCenter);
	m_pPreviewBtn = LLUICtrlFactory::create<LLButton>(preview_button);
	addChild(m_pPreviewBtn);

	// Sound combo
	LLComboBox::Params sound_combo = p.sound_combo;
	padCenter = (rct.getHeight() - sound_combo.rect.height) / 2;
	sound_combo.rect = LLRect(0, padCenter + sound_combo.rect.height, preview_button.rect.left, padCenter);
	m_pSoundCombo = LLUICtrlFactory::create<LLComboBox>(sound_combo);
	addChild(m_pSoundCombo);

	// Drop target
	LLSoundDropTarget::Params drop_target = p.drop_target;
	drop_target.rect = LLRect(0, drop_target.rect.height, rct.getWidth(), 0);
	m_pDropTarget = LLUICtrlFactory::create<LLSoundDropTarget>(drop_target);
	addChild(m_pDropTarget);

	//
	// Load the default packaged sound options
	//
	if (s_sdSounds.isUndefined())
	{
		s_sdSounds = LLSD::emptyArray();

		std::vector<std::string> strFiles = gDirUtilp->findSkinnedFilenames(LLDir::XUI, "sounds.xml", LLDir::ALL_SKINS);
		if (!strFiles.empty())
		{
			llifstream fileStream(strFiles.front(), std::ios::binary);
			if (fileStream.is_open())
			{
				LLSDSerialize::fromXMLDocument(s_sdSounds, fileStream);
				fileStream.close();
			}
		}
	}
}

LLSoundPickerCtrl::~LLSoundPickerCtrl()
{
}

BOOL LLSoundPickerCtrl::postBuild()
{
	initSoundCombo();

	m_pSoundCombo->setCommitCallback(boost::bind(&LLSoundPickerCtrl::onSoundSelect, this));
	m_pSoundCombo->getListControl()->setCommitOnSelectionChange(true);
	m_pPreviewBtn->setCommitCallback(boost::bind(&LLSoundPickerCtrl::onSoundPreview, this));
	m_pDropTarget->setDropCallback(boost::bind(&LLSoundPickerCtrl::onSoundDrop, this, _1));

	return TRUE;
}

void LLSoundPickerCtrl::initSoundCombo()
{
	// Set up the sound options for the combobox
	LLControlVariable* pControl = getControlVariable();
	if (pControl)
	{
		const std::string strDefaultSound = pControl->getDefault().asString();
		if (!strDefaultSound.empty())
		{
			m_pSoundCombo->add("Default", strDefaultSound);
			m_pSoundCombo->add("No sound", LLStringUtil::null);
		}
		else
		{
			m_pSoundCombo->add("Default (no sound)", LLStringUtil::null);
		}

		// Add sounds from sounds.xml
		if ( (s_sdSounds.isMap()) && (s_sdSounds.size() > 0) )
		{
			m_pSoundCombo->addSeparator();
			for (LLSD::map_const_iterator itSound = s_sdSounds.beginMap(); itSound != s_sdSounds.endMap(); ++itSound)
			{
				const LLSD& sdSound = itSound->second;
				if (!sdSound.has("asset_uuid"))
					continue;
				m_pSoundCombo->add(itSound->first, "a|" + sdSound["asset_uuid"].asString());
			}
		}

		// Add the custom option
		m_pSoundCombo->addSeparator();
		m_pSoundCombo->add("Custom (Drop sound here)", pControl->getDefault().asString());

		// Select the current setting
		const std::string strSetting = pControl->getValue().asString();
		if ( (strSetting.length() > 2) && ('i' == strSetting[0]) )
			onSoundDrop(LLUUID(strSetting.substr(2)));
		else
			m_pSoundCombo->selectByValue(strSetting);
	}
}

void LLSoundPickerCtrl::onSoundDrop(const LLUUID& idSound)
{
	const LLInventoryItem* pItem = gInventory.getItem(idSound);
	if ( (pItem) && (m_pSoundCombo) && (m_pSoundCombo->getEnabled()) )
	{
		const std::string& strValue = "i|" + pItem->getUUID().asString();
		if (!m_pSoundCombo->selectByValue(strValue))
		{
			LLScrollListItem* pNewItem = m_pSoundCombo->add(pItem->getName(), strValue);
			if (pNewItem)
			{
				LLScrollListText* pCell = dynamic_cast<LLScrollListText*>(pNewItem->getColumn(0));
				if (pCell)
					pCell->setIcon(LLInventoryIcon::getIconName(LLInventoryType::ICONNAME_SOUND));
			}
			m_pSoundCombo->selectByValue(strValue);
		}
	}
}

void LLSoundPickerCtrl::onSoundPreview()
{
	if (m_pSoundCombo)
	{
		make_ui_sound(LLViewerChat::getUISoundFromSettingsString(m_pSoundCombo->getValue().asString()));
	}
}

void LLSoundPickerCtrl::onSoundSelect()
{
	LLControlVariable* pControl = getControlVariable();
	if ( (m_pSoundCombo) && (pControl) )
	{
		pControl->setValue(m_pSoundCombo->getValue().asString());
	}
}

// ============================================================================
