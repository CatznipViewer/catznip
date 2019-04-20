/**
 *
 * Copyright (c) 2011-2016, Kitty Barnett
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

#ifndef LL_LLFLOATERUPDATE_H
#define LL_LLFLOATERUPDATE_H

#include "llmodaldialog.h"

// ====================================================================================
// Forward declarations
//

class LLButton;
class LLProgressBar;
class LLTextBox;

// ====================================================================================
// LLFloaterUpdate
//

class LLFloaterUpdate : public LLModalDialog
{
	friend class LLFloaterReg;
private:
	LLFloaterUpdate(const LLSD& sdData);
	~LLFloaterUpdate() override {}

	/*
	 * LLView overrides
	 */
public:
	BOOL postBuild() override;

	/*
	 * Event handlers
	 */
protected:
	void onAcceptOrCancel(bool fCommit);

	/*
	 * Member variables
	 */
private:
	enum EType { TYPE_DOWNLOAD = 0, TYPE_INSTALL = 1 };

	EType       m_eType;
	bool        m_fRequired;
	std::string m_strVersion;
	std::string m_strInformation;
	std::string m_strInformationUrl;
};

// ====================================================================================
// LLFloaterUpdateProgress
//

class LLFloaterUpdateProgress : public LLModalDialog
{
	friend class LLFloaterReg;
private:
	LLFloaterUpdateProgress(const LLSD& sdKey, bool fModal);
	~LLFloaterUpdateProgress() override;

	/*
	 * LLView overrides
	 */
public:
	BOOL postBuild() override;
	void onOpen(const LLSD& sdKey) override;

	/*
	 * Event handlers
	 */
public:
	void onDownloadProgress(const LLSD& sdData);
	void onDownloadCompleted();
	void onInstallBtn();

	/*
	 * Member variables
	 */
protected:
	bool           m_fRequired = false;
	LLProgressBar* m_pProgressBar = nullptr;
	LLTextBox*     m_pProgressText = nullptr;
	LLButton*      m_pInstallBtn = nullptr;
};

// ====================================================================================

#endif // LL_LLFLOATERUPDATE_H
