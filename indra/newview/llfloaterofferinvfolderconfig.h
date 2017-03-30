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
#include "lluuid.h"

#pragma once

// ============================================================================
// Forward declarations
//

class LLLineEditor;
class LLScrollListCtrl;

// ============================================================================
// LLAcceptInFolder - helper data structure
//

struct LLAcceptInFolder
{
	LLAcceptInFolder() {}
	LLAcceptInFolder(const LLSD& sdData) : mData(sdData) {}
	LLAcceptInFolder(const LLUUID& idFolder, const std::string& strName, const std::string& strSubFolder) : mData(LLSD().with("uuid", idFolder).with("name", strName).with("subfolder", strSubFolder)) {}

	LLUUID      getId() const { return mData["uuid"].asUUID(); }
	std::string getName() const { return mData["name"].asString(); }
	std::string getPath() const { return getPath(getId()); }
	bool        hasSubFolder() const { return mData.has("subfolder"); }
	std::string getSubFolder() const { return mData["subfolder"].asString(); }
	const LLSD& toLLSD() const { return mData; }

	static std::string getPath(const LLUUID& idFolder);

protected:
	LLSD mData;
};

// ============================================================================
// LLFloaterInventoryOfferFolderConfig - configuration floater for 'Accept in' folders
//

class LLFloaterInventoryOfferFolderConfig : public LLFloater
{
	LOG_CLASS(LLFloaterInventoryOfferFolderConfig);
public:
	LLFloaterInventoryOfferFolderConfig(const LLSD& sdKey);
	~LLFloaterInventoryOfferFolderConfig() override;

	/*
	 * LLFloater overrides
	 */
public:
	LLSD getValue() const { return m_sdFolderItems; }
	void onCommit() override;
	void onOpen(const LLSD& sdKey) override;
	BOOL postBuild() override;

	/*
	 * Member functions
	 */
protected:
	void onAddFolder();
	void onRemoveFolder();
	void onSelectFolder();
	void onBrowseFolder();
	void onBrowseFolderCb(const LLSD& sdData);
	void onSaveFolder();
	void onOk();
	void onCancel();

	void addItem(const LLAcceptInFolder& folderInfo);
	void clearControls();
	void refreshControls();
	void refreshItems();

	/*
	 * Member variables
	 */
protected:
	LLSD m_sdFolderItems;

	LLScrollListCtrl* m_pFolderList = nullptr;
	LLButton*         m_pFolderAddBtn = nullptr;
	LLButton*         m_pFolderRemoveBtn = nullptr;
	LLButton*         m_pFolderSaveBtn = nullptr;

	LLSD*         m_pEditFolderItem = nullptr;
	LLUUID        m_idEditFolder;
	LLLineEditor* m_pEditFolderName = nullptr;
	LLLineEditor* m_pEditFolderPath = nullptr;
	LLButton*     m_pEditFolderBrowse = nullptr;
	LLLineEditor* m_pEditFolderSubfolder = nullptr;

	LLHandle<LLFloater> m_BrowseFloaterHandle;
};

// ============================================================================
