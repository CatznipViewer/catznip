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

#ifndef LLFLOATERGROUPACTIONS_H
#define LLFLOATERGROUPACTIONS_H

#include "llfloater.h"
#include "lllineeditor.h"
#include "lltexteditor.h"

// =========================================================================

class LLFloaterGroupCreateNotice : public LLFloater
{
public:
	LLFloaterGroupCreateNotice(const LLSD& sdKey);

public:
	/*virtual*/ BOOL postBuild();
	/*virtual*/ S32  notifyParent(const LLSD& sdInfo);
	/*virtual*/ void onOpen(const LLSD& sdKey);

	/*
	 * Event callbacks
	 */
protected:
	void onClickClearAttach();
	void onClickSend();
	void onClickCancel();

	/*
	 * Member variables
	 */
protected:
	LLUUID			m_idGroup;
	LLLineEditor*	m_pSubjectCtrl;
	LLTextEditor*	m_pMessageCtrl;
	LLUUID			m_idAttachItem;
	LLIconCtrl*		m_pAttachIconCtrl;
	LLLineEditor*	m_pAttachTextCtrl;
	LLButton*		m_pAttachClearBtn;
};

// =========================================================================

#endif // LLFLOATERGROUPACTIONS_H
