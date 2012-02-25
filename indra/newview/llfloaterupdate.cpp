/** 
 *
 * Copyright (c) 2011, Kitty Barnett
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
#include "llevents.h"
#include "llfloaterupdate.h"
#include "llmediactrl.h"

LLFloaterUpdate::LLFloaterUpdate(const LLSD& sdData)
	: LLModalDialog(sdData["update_version"])
	, m_strReplyPumpName(sdData["reply_pump"].asString())
	, m_eType((EType)sdData["type"].asInteger())
	, m_fUpdateRequired(sdData["update_required"].asBoolean())
	, m_strUpdateUrl(sdData["update_url"].asString())
	, m_strUpdateVersion(sdData["update_version"].asString())
{
}

BOOL LLFloaterUpdate::postBuild()
{
	LLButton* pBtnOk = getChild<LLButton>("btnOk");
	pBtnOk->setCommitCallback(boost::bind(&LLFloaterUpdate::onOkOrCancel, this, true));
	std::string strBtnOk = getString((TYPE_INSTALL == m_eType) ? "button_ok_install" : "button_ok_download");
	pBtnOk->setLabelSelected(strBtnOk);
	pBtnOk->setLabelUnselected(strBtnOk);

	LLButton* pBtnCancel = getChild<LLButton>("btnCancel");
	pBtnCancel->setCommitCallback(boost::bind(&LLFloaterUpdate::onOkOrCancel, this, false));
	pBtnCancel->setEnabled(!m_fUpdateRequired);

	LLCheckBoxCtrl* pChkSkip = getChild<LLCheckBoxCtrl>("chkSkip");
	pChkSkip->setEnabled(!m_fUpdateRequired);

	LLStringUtil::format_map_t args;
	args["VERSION"] = m_strUpdateVersion;
	childSetValue("txtUpdateMessage", getString((m_fUpdateRequired) ? "update_string_required" : "update_string_optional", args));

	LLMediaCtrl* pWebBrowser = getChild<LLMediaCtrl>("webUpdate");
	if ( (pWebBrowser) && (!m_strUpdateUrl.empty()) )
		pWebBrowser->navigateTo(m_strUpdateUrl);

	return TRUE;
}

void LLFloaterUpdate::onOkOrCancel(bool fCommit)
{
	LLSD sdData;
	sdData["commit"] = fCommit;
	sdData["skip"] = findChild<LLCheckBoxCtrl>("chkSkip")->getValue();
	sdData["version"] = m_strUpdateVersion;

	LLEventPumps::instance().obtain(m_strReplyPumpName).post(sdData);

	closeFloater();
}
