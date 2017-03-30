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

#include "llinventoryobserver.h"
#include "llpanel.h"

#pragma once

// ============================================================================
// Forward declarations
//

class LLCheckBoxCtrl;
class LLComboBox;

// ============================================================================
// LLPanelInventoryOfferFolder - destination folder picker for inventory offer toasts
//

class LLPanelInventoryOfferFolder : public LLPanel
{
	LOG_CLASS(LLPanelInventoryOfferFolder);
public:
	LLPanelInventoryOfferFolder();
	~LLPanelInventoryOfferFolder() override;

	/*
	 * LLPanel overrides
	 */
public:
	BOOL postBuild() override;

	/*
	 * Member functions
	 */
protected:
	void onBrowseFolder();
	void onBrowseFolderCb(const LLSD& sdData);
	void onConfigureFolders();
	void onConfigureFoldersCb();
	void refreshControls();
	void refreshFolders();

	/*
	 * Member variables
	 */
protected:
	LLCheckBoxCtrl* m_pAcceptInCheck = nullptr;
	LLComboBox* m_pAcceptInList = nullptr;
	LLButton* m_pBrowseBtn = nullptr;

	LLHandle<LLFloater> m_BrowseFloaterHandle;
	LLHandle<LLFloater> m_ConfigureFloaterHandle;
};

// ============================================================================
// LLAcceptInFolderAgentOffer - move an agent-to-agent accepted inventory offer to the specified folder
//

class LLAcceptInFolderAgentOffer : public LLInventoryFetchItemsObserver
{
	LOG_CLASS(LLAcceptInFolderAgentOffer);
public:
	LLAcceptInFolderAgentOffer(const LLUUID& idInvObject, const LLUUID& idDestFolder);

	/*
	 * LLInventoryFetchItemsObserver overrides
	 */
public:
	void done() override;
protected:
	void onDone();

	/*
	 * Member variables
	 */
protected:
	LLUUID m_InvObjectId;
	LLUUID m_DestFolderId;
};

// ============================================================================
