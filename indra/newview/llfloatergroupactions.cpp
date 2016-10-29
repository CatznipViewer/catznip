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
#include "llfloatergroupactions.h"
#include "llgroupactions.h"
#include "lliconctrl.h"
#include "llinventorydefines.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llpanelgroupnotices.h"
#include "llslurl.h"
#include "lltexteditor.h"
#include "llviewerinventory.h"
#include "llviewermessage.h"

// =========================================================================
// LLFloaterGroupCreateNotice class
//

LLFloaterGroupCreateNotice::LLFloaterGroupCreateNotice(const LLSD& sdKey)
	: LLFloater(sdKey)
	, m_pSubjectCtrl(NULL)
	, m_pMessageCtrl(NULL)
	, m_pAttachIconCtrl(NULL)
	, m_pAttachTextCtrl(NULL)
	, m_pAttachClearBtn(NULL)
{
}

BOOL LLFloaterGroupCreateNotice::postBuild()
{
	findChild<LLUICtrl>("send_btn")->setCommitCallback(boost::bind(&LLFloaterGroupCreateNotice::onClickSend, this));
	findChild<LLUICtrl>("cancel_btn")->setCommitCallback(boost::bind(&LLFloaterGroupCreateNotice::onClickCancel, this));

	m_pSubjectCtrl = findChild<LLLineEditor>("subject_editor");
	m_pMessageCtrl = findChild<LLTextEditor>("message_editor");

	m_pAttachIconCtrl = findChild<LLIconCtrl>("attach_icon");
	m_pAttachTextCtrl = findChild<LLLineEditor>("attach_editor");
	m_pAttachTextCtrl->setTabStop(false);
	m_pAttachTextCtrl->setEnabled(false);
	m_pAttachClearBtn = findChild<LLButton>("attach_clear_btn");
	m_pAttachClearBtn->setCommitCallback(boost::bind(&LLFloaterGroupCreateNotice::onClickClearAttach, this));
	m_pAttachClearBtn->setVisible(false);

	return TRUE;
}

S32 LLFloaterGroupCreateNotice::notifyParent(const LLSD& sdInfo)
{
	if (sdInfo.has("item_id"))
	{
		const LLViewerInventoryItem* pItem = gInventory.getItem(sdInfo["item_id"].asUUID());
		if (pItem)
		{
// [SL:KB] - Patch: Inventory-IconMismatch | Checked: 2011-05-31 (Catznip-2.6)
			std::string strIconName = LLInventoryIcon::getIconName(pItem->getType(), pItem->getInventoryType(), pItem->getFlags());
// [/SL:KB]
//			bool fMulti = pItem->getFlags() & LLInventoryItemFlags::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS;
//			std::string strIconName = LLInventoryIcon::getIconName(pItem->getType(), pItem->getInventoryType(), pItem->getFlags(), fMulti);

			m_idAttachItem = pItem->getUUID();
			m_pAttachIconCtrl->setValue(strIconName);
			m_pAttachIconCtrl->setVisible(true);
			m_pAttachTextCtrl->setValue(pItem->getName());
			m_pAttachClearBtn->setVisible(true);
		}
		return 1;
	}
	return LLFloater::notifyParent(sdInfo);
}

void LLFloaterGroupCreateNotice::onOpen(const LLSD& sdKey)
{
	m_idGroup = sdKey["group"].asUUID();
	if ( (m_idGroup.isNull()) || (!LLGroupActions::hasPowerInGroup(m_idGroup, GP_NOTICES_SEND)) )
	{
		closeFloater();
	}

	findChild<LLUICtrl>("header_text")->setTextArg("[GROUP]", LLSLURL("group", m_idGroup, "inspect").getSLURLString());
	findChild<LLGroupDropTarget>("drop_target")->setGroup(m_idGroup);
}

void LLFloaterGroupCreateNotice::onClickClearAttach()
{
	m_idAttachItem.setNull();
	m_pAttachIconCtrl->setVisible(false);
	m_pAttachTextCtrl->setValue(LLSD());
	m_pAttachClearBtn->setVisible(false);
}

void LLFloaterGroupCreateNotice::onClickSend()
{
	// Must supply a subject
	if (m_pSubjectCtrl->getText().empty())
	{
		LLNotificationsUtil::add("MustSpecifyGroupNoticeSubject");
		return;
	}

	send_group_notice(m_idGroup, m_pSubjectCtrl->getText(), m_pMessageCtrl->getText(), gInventory.getItem(m_idAttachItem));

	closeFloater();
}

void LLFloaterGroupCreateNotice::onClickCancel()
{
	closeFloater();
}

// =========================================================================
