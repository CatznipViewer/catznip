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
// LLAcceptInFolderOfferBase - base class (based on RLVa)
//

class LLAcceptInFolderOfferBase
{
protected:
	LLAcceptInFolderOfferBase(LLUUID idBaseFolder) : m_idBaseFolder(idBaseFolder) {}
	virtual ~LLAcceptInFolderOfferBase() {}
protected:
	bool         createDestinationFolder();
	virtual void onDestinationCreated(const LLUUID& idFolder) = 0;
private:
	static void  onCategoryCreateCallback(LLUUID idFolder, LLAcceptInFolderOfferBase* pInstance);

protected:
	LLUUID m_idBaseFolder;
private:
	std::list<std::string> m_DestPath;
};

// ============================================================================
// LLAcceptInFolderTaskOffer - move an task-to-agent accepted inventory offer to the user-configured folder
//

// [See LLInventoryTransactionObserver which says it's not entirely complete?]
// NOTE: the offer may span mulitple BulkUpdateInventory messages so if we're no longer around then (ie due to "delete this") then
//       we'll miss those; in this specific case we only care about a single object (folder or itemà and in case of a folder its
//       data is present in the very first message
class LLAcceptInFolderTaskOffer : public LLAcceptInFolderOfferBase, public LLInventoryObserver
{
public:
	LLAcceptInFolderTaskOffer(const std::string& strDescription, const LLUUID& idTransaction, const LLUUID& idBaseFolder);

	/*
	 * Member functions (+ base class overrides)
	 */
public:
	void changed(U32 mask) override;
protected:
	void done();
	void doneIdle();
	void onDestinationCreated(const LLUUID& idFolder) override;

	/*
	 * Member variables
	 */
protected:
	uuid_vec_t  m_Folders;
	uuid_vec_t  m_Items;
	LLUUID      m_idTransaction;
	std::string m_strDescription;
};

// ============================================================================
// LLAcceptInFolderAgentOffer - move an agent-to-agent accepted inventory offer to the user-configured folder
//

class LLAcceptInFolderAgentOffer : public LLAcceptInFolderOfferBase, public LLInventoryFetchItemsObserver
{
	LOG_CLASS(LLAcceptInFolderAgentOffer);
public:
	LLAcceptInFolderAgentOffer(const LLUUID& idInvObject, const LLUUID& idBaseFolder);

	/*
	 * Member functions (+ base class overrides)
	 */
public:
	void done() override;
protected:
	void doneIdle();
	void onDestinationCreated(const LLUUID& idFolder) override;

	/*
	 * Member variables
	 */
protected:
	LLUUID m_InvObjectId;
};

// ============================================================================
