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
#ifndef LLFLOATERCHATALERTS_H
#define LLFLOATERCHATALERTS_H

#include "llfloater.h"

class LLButton;
class LLCheckBoxCtrl;
class LLColorSwatchCtrl;
class LLLineEditor;
class LLScrollListCtrl;

class LLFloaterChatAlerts : public LLFloater
{
	friend class LLFloaterReg;
private:
	LLFloaterChatAlerts(const LLSD& sdKey);
public:
	/*virtual*/ ~LLFloaterChatAlerts() {}
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

protected:
	void refreshList();
	void refreshEntry();

protected:
	LLScrollListCtrl*  m_pAlertList;
	LLLineEditor*      m_pKeywordEditor;
	LLCheckBoxCtrl*    m_pKeywordCase;
	LLColorSwatchCtrl* m_pColorCtrl;
	LLCheckBoxCtrl*    m_pSoundCheck;
	LLLineEditor*      m_pSoundEditor;
	LLButton*          m_pSoundButton;
	LLCheckBoxCtrl*    m_pTriggerChat;
	LLCheckBoxCtrl*    m_pTriggerIM;
	LLCheckBoxCtrl*    m_pTriggerGroup;
};

#endif  // LLFLOATERCHATALERTS_H
