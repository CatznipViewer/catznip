/**
 *
 * Copyright (c) 2017, Kitty Barnett
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

#include "llfloater.h"

#pragma once

// ============================================================================
// Forward declarations
//

class LLFilterEditor;
class LLInventoryPanel;
class LLSaveFolderState;

// ============================================================================
// LLFloaterInventoryOfferFolderBrowse - floater to browse for an inventory folder
//

class LLFloaterInventoryOfferFolderBrowse : public LLFloater
{
	LOG_CLASS(LLFloaterInventoryOfferFolderBrowse);
public:
	LLFloaterInventoryOfferFolderBrowse();
	~LLFloaterInventoryOfferFolderBrowse() override;

	/*
	 * LLFloater overrides
	 */
public:
	void onCommit() override;
	void onOpen(const LLSD& sdKey) override;
	BOOL postBuild() override;

	/*
	 * Member functions
	 */
protected:
	void onCancel();
	void onCloseAllFolders();
	void onFilterEdit(const std::string& strFilter);
	void onSelect();

	/*
	 * Member variables
	 */
protected:
	LLFilterEditor* m_pFilterEditor = nullptr;
	LLInventoryPanel* m_pInvPanel = nullptr;
	LLSaveFolderState* m_pSavedFolderState = nullptr;
};

// ============================================================================
