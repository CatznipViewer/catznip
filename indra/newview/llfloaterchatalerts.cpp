/** 
 *
 * Copyright (c) 2012, Kitty Barnett
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

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llfloaterchatalerts.h"
#include "lllineeditor.h"
#include "llscrolllistctrl.h"
#include "lltextparser.h"

// ============================================================================
// LLFloaterChatAlerts
//

LLFloaterChatAlerts::LLFloaterChatAlerts(const LLSD& sdKey)
	: LLFloater(sdKey)
	, m_pAlertList(NULL)
	, m_pKeywordEditor(NULL)
	, m_pKeywordCase(NULL)
	, m_pColorCtrl(NULL)
	, m_pSoundCheck(NULL)
	, m_pSoundEditor(NULL)
	, m_pSoundButton(NULL)
	, m_pTriggerChat(NULL)
	, m_pTriggerIM(NULL)
	, m_pTriggerGroup(NULL)
{
}

BOOL LLFloaterChatAlerts::postBuild(void)
{
	m_pAlertList = findChild<LLScrollListCtrl>("alerts_list");
	m_pAlertList->setCommitOnKeyboardMovement(TRUE);
	m_pAlertList->setCommitOnSelectionChange(TRUE);
	m_pAlertList->setCommitCallback(boost::bind(&LLFloaterChatAlerts::refreshEntry, this));

	m_pKeywordEditor = findChild<LLLineEditor>("alerts_keyword");
	m_pKeywordCase = findChild<LLCheckBoxCtrl>("alerts_keyword_case");
	m_pColorCtrl = findChild<LLColorSwatchCtrl>("alerts_highlight_color");
	m_pSoundCheck = findChild<LLCheckBoxCtrl>("alerts_sound_check");
	m_pSoundEditor = findChild<LLLineEditor>("alerts_sound_name");
	m_pSoundButton = findChild<LLButton>("alerts_sound_browse");
	m_pTriggerChat = findChild<LLCheckBoxCtrl>("alerts_trigger_chat");
	m_pTriggerIM = findChild<LLCheckBoxCtrl>("alerts_trigger_im");
	m_pTriggerGroup = findChild<LLCheckBoxCtrl>("alerts_trigger_group");

	return TRUE;
}

void LLFloaterChatAlerts::onOpen(const LLSD& key)
{
	refreshList();
}

void LLFloaterChatAlerts::refreshList()
{
	m_pAlertList->clearRows();

	// Set-up a row we can just reuse
	LLSD sdRow;
	LLSD& sdColumns = sdRow["columns"];
	sdColumns[0]["column"] = "alert_keyword";
	sdColumns[0]["type"] = "text";

	const LLTextParser::highlight_list_t& highlights = LLTextParser::instance().getHighlights();
	for (LLTextParser::highlight_list_t::const_iterator itHighlight = highlights.begin(); itHighlight != highlights.end(); ++itHighlight)
	{
		const LLHighlightEntry& entry = *itHighlight;

		sdColumns[0]["value"] = entry.mPattern;
		sdRow["value"] = entry.getId();

		m_pAlertList->addElement(sdRow, ADD_BOTTOM);
	}

	refreshEntry();
}

void LLFloaterChatAlerts::refreshEntry()
{
	const LLUUID idEntry = m_pAlertList->getSelectedValue().asUUID();
	const LLHighlightEntry* pEntry = (idEntry.notNull()) ? LLTextParser::instance().getHighlightById(idEntry) : NULL;
	bool fEnable = (NULL != pEntry);

	findChild<LLUICtrl>("alerts_keyword_label")->setEnabled(fEnable);
	m_pKeywordEditor->setEnabled(fEnable);
	m_pKeywordEditor->setText( (pEntry) ? pEntry->mPattern : "" );
	m_pKeywordCase->setEnabled(fEnable);
	m_pKeywordCase->set( (pEntry) ? pEntry->mCaseSensitive : false );
	findChild<LLUICtrl>("alerts_color_label")->setEnabled(fEnable);
	m_pColorCtrl->setEnabled(fEnable);
	m_pColorCtrl->set( (pEntry) ? pEntry->mColor : LLColor4::white );

	bool fEnableSound = (pEntry) ? pEntry->mSoundAsset.notNull() : false;
	m_pSoundCheck->setEnabled(fEnable);
	m_pSoundCheck->set( (pEntry) ? fEnableSound : false );
	m_pSoundEditor->setEnabled(fEnable && fEnableSound);
	m_pSoundEditor->setText( (pEntry) ? pEntry->mSoundAsset.asString() : "" );
	m_pSoundButton->setEnabled(fEnable && fEnableSound);

	findChild<LLUICtrl>("alerts_trigger_label")->setEnabled(fEnable);
	m_pTriggerChat->setEnabled(fEnable);
	m_pTriggerChat->set( (pEntry) ? pEntry->mCategoryMask & LLHighlightEntry::CAT_NEARBYCHAT: false );
	m_pTriggerIM->setEnabled(fEnable);
	m_pTriggerIM->set( (pEntry) ? pEntry->mCategoryMask & LLHighlightEntry::CAT_IM : false );
	m_pTriggerGroup->setEnabled(fEnable);
	m_pTriggerGroup->set( (pEntry) ? pEntry->mCategoryMask & LLHighlightEntry::CAT_GROUP : false );
}

// ============================================================================
