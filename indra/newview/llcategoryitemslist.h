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

#pragma once

#include "lllistcontextmenu.h"
#include "llwearableitemslist.h"

// ============================================================================
// Forward declarations
//

class LLInventoryCategoriesObserver;

// ============================================================================
// LLCategoryItemsList - a flat list view of an inventory folder
//

class LLCategoryItemsList : public LLInventoryItemsList
{
	LOG_CLASS(LLCategoryItemsList);
public:
	struct Params : public LLInitParam::Block<Params, LLInventoryItemsList::Params>
	{
		Optional<bool> link_indication_enabled;
		Optional<bool> worn_indication_enabled;

		Params();
	};

	enum class ESortOrder
	{
		BY_NAME = 0,
		BY_MOST_RECENT = 1
	};

	enum class EIndicatorMask
	{
		SHOW_LINK = 0x01,
		SHOW_WORN = 0x02
	};

	class ContextMenu : public LLWearableItemsList::ContextMenuBase, public LLSingleton<ContextMenu>
	{
		LLSINGLETON(ContextMenu);

	public:
		void show(LLView* pSpawningView, const uuid_vec_t& idSelItems, S32 x, S32 y) override;

	protected:
		const std::string getMenuName() const override { return "menu_category_list_item.xml"; }
	};

	/*
	 * Constructor
	 */
protected:
	friend class LLUICtrlFactory;
	LLCategoryItemsList(const LLCategoryItemsList::Params& p);
public:
	~LLCategoryItemsList() override;

	/*
	 * Parent class overrides
	 */
public:
	BOOL          postBuild() override;
	LLPanel*      createNewItem(LLViewerInventoryItem* pItem) override;
protected:
	void          onItemMouseDoubleClick(item_pair_t* pItemPair, MASK mask) override;

	/*
	 * Member functions
	 */
public:
	ESortOrder    getSortOrder() const { return m_eSortOrder; }
	void          setSortOrder(ESortOrder eSortOrder, bool fSortImmediately = true);
	const LLUUID& getFolderId() const { return m_idFolder; }
	void          setFolderId(const LLUUID& idFolder);
	bool          getCollectFromChildFolders() const { return m_fCollectAll;  }
	void          setCollectFromChildFolders(bool fCollectAll);
protected:
	void          updateList();

	/*
	 * Event handlers
	 */
protected:
	void          onRightClick(S32 x, S32 y);

	/*
	 * Member variables
	 */
protected:
	ESortOrder     m_eSortOrder;
	LLUUID         m_idFolder;
	bool           m_fCollectAll = false;
	unsigned int   m_nIndicatorMask = 0;
	LLInventoryCategoriesObserver* m_pFolderObserver = nullptr;
};

// ============================================================================
