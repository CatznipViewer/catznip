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
#include "llselectmgr.h"

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
	bool notifyChildren(const LLSD& sdData) override;
	void onOpen(const LLSD& sdKey) override;
	void onVisibilityChange(BOOL new_visibility) override;
	BOOL postBuild() override;

	static LLUUID getFolderFromObject(const LLViewerObject* pObj, const std::string& strName = LLStringUtil::null, bool* pfFound = nullptr);

	/*
	 * Member functions
	 */
public:
	bool         getAcceptIn() const;
	const LLUUID getSelectedFolder() const;
	void         setObjectId(const LLUUID& idObject);
protected:
	void onBrowseFolder();
	void onBrowseFolderCb(const LLSD& sdData);
	void onConfigureFolders();
	void onConfigureFoldersCb();
	void onSelectedFolderChanged();
	void onUpdateSelection();
	void refreshControls();
	void refreshFolders();

	/*
	 * Member variables
	 */
protected:
	bool            m_fHasBeenVisible = false;
	LLCheckBoxCtrl* m_pAcceptInCheck = nullptr;
	LLComboBox*     m_pAcceptInList = nullptr;
	LLButton*       m_pBrowseBtn = nullptr;

	LLUUID          m_idObject;
	bool            m_fShowObjectFolder = true;
	LLUUID          m_idObjectFolder;
	LLObjectSelectionHandle m_ObjectSelectionHandle;
	boost::signals2::connection m_SelectionUpdateConnection;

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
	LLAcceptInFolderOfferBase(LLUUID idBaseFolder, const std::string& strNewFolder) : m_idBaseFolder(idBaseFolder), m_strNewFolder(strNewFolder) {}
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
	std::string m_strNewFolder;
};

// ============================================================================
// LLAcceptInFolderTaskOffer - move an task-to-agent accepted inventory offer to the user-configured folder
//

// [See LLInventoryTransactionObserver which says it's not entirely complete?]
// NOTE: the offer may span mulitple BulkUpdateInventory messages so if we're no longer around then (ie due to "delete this") then
//       we'll miss those; in this specific case we only care about a single object (folder or item and in case of a folder its
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
// LLCreateAcceptInFolder - create the destination folder and then callback
//

class LLCreateAcceptInFolder : public LLAcceptInFolderOfferBase
{
	LOG_CLASS(LLCreateAcceptInFolder);
public:
	typedef boost::signals2::signal<void (const LLUUID& idDestFolder)> folder_created_signal_t;
	LLCreateAcceptInFolder(const LLUUID& idBaseFolder, const folder_created_signal_t::slot_type& cb);
	LLCreateAcceptInFolder(const LLUUID& idBaseFolder, const std::string& strNewFolder, const folder_created_signal_t::slot_type& cb);

	/*
	 * Member functions (+ base class overrides)
	 */
protected:
	void onDestinationCreated(const LLUUID& idFolder) override;

	/*
	 * Member variables
	 */
protected:
	folder_created_signal_t m_FolderCreatedSignal;
};

// ============================================================================
