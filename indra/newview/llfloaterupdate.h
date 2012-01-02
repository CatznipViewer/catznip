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

#ifndef LL_LLUPDATE_H
#define LL_LLUPDATE_H

#include "llmodaldialog.h"

class LLFloaterUpdate : public LLModalDialog
{
public:
	LLFloaterUpdate(const LLSD& sdData);
	/*virtual*/ ~LLFloaterUpdate() {}

	BOOL postBuild();

	enum EType { TYPE_DOWNLOAD = 0, TYPE_INSTALL = 1 };
protected:
	void onOkOrCancel(bool fCommit);

private:
	std::string	m_strReplyPumpName;
	EType		m_eType;
	bool		m_fUpdateRequired;
	std::string	m_strUpdateUrl;
	std::string	m_strUpdateVersion;
};

#endif // LL_LLUPDATE_H
