/** 
 * @file llgrouplist.cpp
 * @brief List of the groups the agent belongs to.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * Copyright (C) 2010-2016, Kitty Barnett
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llgrouplist.h"

// libs
#include "llbutton.h"
#include "llgroupiconctrl.h"
#include "llmenugl.h"
#include "lltextbox.h"
#include "lltextutil.h"
#include "lltrans.h"

// newview
#include "llagent.h"
#include "llgroupactions.h"
#include "llfloaterreg.h"
#include "llviewercontrol.h"	// for gSavedSettings
#include "llviewermenu.h"		// for gMenuHolder
#include "llvoiceclient.h"
// [SL:KB] - Patch: Agent-DisplayNames | Checked: 2011-11-10 (Catznip-3.2)
#include "llslurl.h"
#include "llwindow.h"
// [/SL:KB]
// [RLVa:KB] - Checked: RLVa-2.0.3
#include "rlvactions.h"
// [/RLVa:KB]

static LLDefaultChildRegistry::Register<LLGroupList> r("group_list");
S32 LLGroupListItem::sIconWidth = 0;

class LLGroupComparator : public LLFlatListView::ItemComparator
{
public:
	LLGroupComparator() {};

	/** Returns true if item1 < item2, false otherwise */
	/*virtual*/ bool compare(const LLPanel* item1, const LLPanel* item2) const
	{
		std::string name1 = static_cast<const LLGroupListItem*>(item1)->getGroupName();
		std::string name2 = static_cast<const LLGroupListItem*>(item2)->getGroupName();

		LLStringUtil::toUpper(name1);
		LLStringUtil::toUpper(name2);

		return name1 < name2;
	}
};

static const LLGroupComparator GROUP_COMPARATOR;


LLGroupList::LLGroupList(const Params& p)
:	LLFlatListViewEx(p)
	, mDirty(true) // to force initial update
{
	// Listen for agent group changes.
	gAgent.addListener(this, "new group");

	mShowIcons = gSavedSettings.getBOOL("GroupListShowIcons");
// [SL:KB] - Patch: UI-GroupListHidden | CHecked: 2014-01-22 (Catznip-3.6)
	mShowHidden = gSavedSettings.getBOOL("GroupListShowHidden");
// [/SL:KB]
	setCommitOnSelectionChange(true);

	// Set default sort order.
	setComparator(&GROUP_COMPARATOR);

	// Set up context menu.
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	registrar.add("People.Groups.Action",			boost::bind(&LLGroupList::onContextMenuItemClick,	this, _2));
	enable_registrar.add("People.Groups.Enable",	boost::bind(&LLGroupList::onContextMenuItemEnable,	this, _2));

	LLToggleableMenu* context_menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_people_groups.xml",
			gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if(context_menu)
		mContextMenuHandle = context_menu->getHandle();
}

LLGroupList::~LLGroupList()
{
	gAgent.removeListener(this);
	if (mContextMenuHandle.get()) mContextMenuHandle.get()->die();
}

// virtual
void LLGroupList::draw()
{
	if (mDirty)
		refresh();

	LLFlatListView::draw();
}

// virtual
BOOL LLGroupList::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLUICtrl::handleRightMouseDown(x, y, mask);

	LLToggleableMenu* context_menu = mContextMenuHandle.get();
	if (context_menu && size() > 0)
	{
		context_menu->buildDrawLabels();
		context_menu->updateParent(LLMenuGL::sMenuContainer);
		LLMenuGL::showPopup(this, context_menu, x, y);
	}

	return handled;
}

//// virtual
//BOOL LLGroupList::handleDoubleClick(S32 x, S32 y, MASK mask)
//{
//	BOOL handled = LLView::handleDoubleClick(x, y, mask);
//	// Handle double click only for the selected item in the list, skip clicks on empty space.
//	if (handled)
//	{
//		if (mDoubleClickSignal)
//		{
//			(*mDoubleClickSignal)(this, x, y, mask);
//		}
//	}
//
//	return handled;
//}

void LLGroupList::setNameFilter(const std::string& filter)
{
	std::string filter_upper = filter;
	LLStringUtil::toUpper(filter_upper);
	if (mNameFilter != filter_upper)
	{
		mNameFilter = filter_upper;

		// set no items message depend on filter state
		updateNoItemsMessage(filter);

		setDirty();
	}
}

static bool findInsensitive(std::string haystack, const std::string& needle_upper)
{
    LLStringUtil::toUpper(haystack);
    return haystack.find(needle_upper) != std::string::npos;
}

void LLGroupList::refresh()
{
	const LLUUID& 		highlight_id	= gAgent.getGroupID();
	S32					count			= gAgent.mGroups.size();
	LLUUID				id;
	bool				have_filter		= !mNameFilter.empty();

	clear();

	for(S32 i = 0; i < count; ++i)
	{
		id = gAgent.mGroups.at(i).mID;
		const LLGroupData& group_data = gAgent.mGroups.at(i);
		if (have_filter && !findInsensitive(group_data.mName, mNameFilter))
			continue;
		addNewItem(id, group_data.mName, group_data.mInsigniaID, ADD_BOTTOM);
	}

	// Sort the list.
	sort();

	// Add "none" to list at top if filter not set (what's the point of filtering "none"?).
	// but only if some real groups exists. EXT-4838
	if (!have_filter && count > 0)
	{
		std::string loc_none = LLTrans::getString("GroupsNone");
		addNewItem(LLUUID::null, loc_none, LLUUID::null, ADD_TOP);
	}

	selectItemByUUID(highlight_id);

	setDirty(false);
	onCommit();
}

void LLGroupList::toggleIcons()
{
	// Save the new value for new items to use.
	mShowIcons = !mShowIcons;
	gSavedSettings.setBOOL("GroupListShowIcons", mShowIcons);

	// Show/hide icons for all existing items.
	std::vector<LLPanel*> items;
	getItems(items);
	for( std::vector<LLPanel*>::const_iterator it = items.begin(); it != items.end(); it++)
	{
		static_cast<LLGroupListItem*>(*it)->setGroupIconVisible(mShowIcons);
	}
}

// [SL:KB] - Patch: UI-GroupListHidden | CHecked: 2014-01-22 (Catznip-3.6)
void LLGroupList::toggleHidden()
{
	// Save the new value for new items to use
	mShowHidden = !mShowHidden;
	gSavedSettings.setBOOL("GroupListShowHidden", mShowHidden);

	// Show/hide hidden status for all existing items
	std::vector<LLPanel*> items;
	getItems(items);
	for (std::vector<LLPanel*>::const_iterator itItem = items.begin(); itItem != items.end(); itItem++)
	{
		static_cast<LLGroupListItem*>(*itItem)->setHighlightHiddenGroup(mShowHidden);
	}
}
// [/SL:KB]

//////////////////////////////////////////////////////////////////////////
// PRIVATE Section
//////////////////////////////////////////////////////////////////////////

void LLGroupList::addNewItem(const LLUUID& id, const std::string& name, const LLUUID& icon_id, EAddPosition pos)
{
	LLGroupListItem* item = new LLGroupListItem();

	item->setGroupID(id);
	item->setName(name, mNameFilter);
	item->setGroupIconID(icon_id);

	item->getChildView("info_btn")->setVisible( false);
	item->getChildView("profile_btn")->setVisible( false);
	item->setGroupIconVisible(mShowIcons);
// [SL:KB] - Patch: UI-GroupListHidden | CHecked: 2014-01-22 (Catznip-3.6)
	item->setHighlightHiddenGroup(mShowHidden);
// [/SL:KB]


// [SL:KB] - Patch: UI-SidepanelPeople | Checked: Catznip-5.2
	item->setDoubleClickCallback(boost::bind(&LLGroupList::onItemDoubleClicked, this, _1, _2, _3, _4));
// [/SL:KB]

	addItem(item, id, pos);

//	setCommentVisible(false);
}

// virtual
bool LLGroupList::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	// Why is "new group" sufficient?
	if (event->desc() == "new group")
	{
		setDirty();
		return true;
	}

	return false;
}

bool LLGroupList::onContextMenuItemClick(const LLSD& userdata)
{
	std::string action = userdata.asString();
	LLUUID selected_group = getSelectedUUID();

	if (action == "view_info")
	{
		LLGroupActions::show(selected_group);
	}
	else if (action == "chat")
	{
		LLGroupActions::startIM(selected_group);
	}
	else if (action == "call")
	{
		LLGroupActions::startCall(selected_group);
	}
	else if (action == "activate")
	{
		LLGroupActions::activate(selected_group);
	}
// [SL:KB] - Patch: Notification-GroupCreateNotice | Checked: 2012-02-16 (Catznip-3.2)
	else if (action == "create_notice")
	{
		LLFloaterReg::showInstance("group_create_notice", LLSD().with("group", selected_group));
	}
	else if (action == "view_chat_history")
	{
		LLGroupActions::viewChatHistory(selected_group);
	}
	else if (action == "view_notices")
	{
		LLGroupActions::showNotices(selected_group);
	}
// [/SL:KB]
	else if (action == "leave")
	{
		LLGroupActions::leave(selected_group);
	}
// [SL:KB] - Patch: Agent-DisplayNames | Checked: 2011-11-10 (Catznip-3.2)
	else if (action == "copy_slurl")
	{
		LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(LLSLURL("group", selected_group, "about").getSLURLString()));
	}
// [/SL:KB]

	return true;
}

bool LLGroupList::onContextMenuItemEnable(const LLSD& userdata)
{
	LLUUID selected_group_id = getSelectedUUID();
	bool real_group_selected = selected_group_id.notNull(); // a "real" (not "none") group is selected

	// each group including "none" can be activated
// [RLVa:KB] - Checked: RLVa-1.3.0
	if (userdata.asString() == "activate")
		return (gAgent.getGroupID() != selected_group_id) && (RlvActions::canChangeActiveGroup());
	else if (userdata.asString() == "leave")
		return (real_group_selected) && ((gAgent.getGroupID() != selected_group_id) || (RlvActions::canChangeActiveGroup()));
// [/RLVa:KB]
//	if (userdata.asString() == "activate")
//		return gAgent.getGroupID() != selected_group_id;

	if (userdata.asString() == "call")
	  return real_group_selected && LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking();

// [SL:KB] - Patch: Notification-GroupCreateNotice | Checked: 2012-02-16 (Catznip-3.2)
	if (userdata.asStringRef() == "create_notice")
	{
		return real_group_selected && LLGroupActions::hasPowerInGroup(selected_group_id, GP_NOTICES_SEND);
	}
	else if (userdata.asStringRef() == "view_chat_history")
	{
		return LLGroupActions::hasChatHistory(selected_group_id);
	}
// [/SL:KB]

	return real_group_selected;
}

// [SL:KB] - Patch: UI-SidepanelPeople | Checked: Catznip-5.2
boost::signals2::connection LLGroupList::setItemDoubleClickCallback(const mouse_signal_t::slot_type& cb)
{
	return mItemDoubleClickSignal.connect(cb);
}

void LLGroupList::onItemDoubleClicked(LLUICtrl* ctrl, S32 x, S32 y, MASK mask)
{
	mItemDoubleClickSignal(ctrl, x, y, mask);
}
// [/SL:KB]

/************************************************************************/
/*          LLGroupListItem implementation                              */
/************************************************************************/

LLGroupListItem::LLGroupListItem()
:	LLPanel(),
mGroupIcon(NULL),
mGroupNameBox(NULL),
mInfoBtn(NULL),
// [SL:KB] - Patch: UI-GroupListHidden | CHecked: 2014-01-22 (Catznip-3.6)
mShowHidden(true),
// [/SL:KB]
mGroupID(LLUUID::null)
{
	buildFromFile( "panel_group_list_item.xml");

	// Remember group icon width including its padding from the name text box,
	// so that we can hide and show the icon again later.
	if (!sIconWidth)
	{
		sIconWidth = mGroupNameBox->getRect().mLeft - mGroupIcon->getRect().mLeft;
	}
}

LLGroupListItem::~LLGroupListItem()
{
	LLGroupMgr::getInstance()->removeObserver(this);
}

//virtual
BOOL  LLGroupListItem::postBuild()
{
	mGroupIcon = getChild<LLGroupIconCtrl>("group_icon");
	mGroupNameBox = getChild<LLTextBox>("group_name");

	mInfoBtn = getChild<LLButton>("info_btn");
	mInfoBtn->setClickedCallback(boost::bind(&LLGroupListItem::onInfoBtnClick, this));

	childSetAction("profile_btn", boost::bind(&LLGroupListItem::onProfileBtnClick, this));

	return TRUE;
}

//virtual
void LLGroupListItem::setValue( const LLSD& value )
{
	if (!value.isMap()) return;
	if (!value.has("selected")) return;
	getChildView("selected_icon")->setVisible( value["selected"]);
}

void LLGroupListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible( true);
	if (mGroupID.notNull()) // don't show the info button for the "none" group
	{
		mInfoBtn->setVisible(true);
		getChildView("profile_btn")->setVisible( true);
	}

	LLPanel::onMouseEnter(x, y, mask);
}

void LLGroupListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible( false);
	mInfoBtn->setVisible(false);
	getChildView("profile_btn")->setVisible( false);

	LLPanel::onMouseLeave(x, y, mask);
}

// [SL:KB] - Patch: UI-GroupListHidden | CHecked: 2014-01-22 (Catznip-3.6)
void LLGroupListItem::setName(const std::string& name, const std::string& highlight)
{
	mGroupName = name;
	mHighlight = highlight;

	LLTextUtil::textboxSetHighlightedVal(mGroupNameBox, mGroupNameStyle, name, highlight);
	if (!mGroupNameSuffix.empty())
	{
		mGroupNameBox->appendText(" ", false, mGroupNameStyle);

		LLStyle::Params styleSuffix = mGroupNameStyle;
		styleSuffix.font.style = "ITALIC";
		mGroupNameBox->appendText(mGroupNameSuffix, false, styleSuffix);
	}

	mGroupNameBox->setToolTip(name);
}
// [/SL:KB]
//void LLGroupListItem::setName(const std::string& name, const std::string& highlight)
//{
//	mGroupName = name;
//	LLTextUtil::textboxSetHighlightedVal(mGroupNameBox, mGroupNameStyle, name, highlight);
//	mGroupNameBox->setToolTip(name);
//}

void LLGroupListItem::setGroupID(const LLUUID& group_id)
{
	LLGroupMgr::getInstance()->removeObserver(this);
	
	mID = group_id;
	mGroupID = group_id;
	setActive(group_id == gAgent.getGroupID());

// [SL:KB] - Patch: UI-GroupListHidden | CHecked: 2014-01-22 (Catznip-3.6)
	LLGroupData dataGroup;
	if (gAgent.getGroupData(group_id, dataGroup))
	{
		setHidden(!dataGroup.mListInProfile);
	}
// [/SL:KB]

	LLGroupMgr::getInstance()->addObserver(this);
}

void LLGroupListItem::setGroupIconID(const LLUUID& group_icon_id)
{
	mGroupIcon->setIconId(group_icon_id);
}

void LLGroupListItem::setGroupIconVisible(bool visible)
{
	// Already done? Then do nothing.
	if (mGroupIcon->getVisible() == (BOOL)visible)
		return;

	// Show/hide the group icon.
	mGroupIcon->setVisible(visible);

	// Move the group name horizontally by icon size + its distance from the group name.
	LLRect name_rect = mGroupNameBox->getRect();
	name_rect.mLeft += visible ? sIconWidth : -sIconWidth;
	mGroupNameBox->setRect(name_rect);
}

// [SL:KB] - Patch: UI-GroupListHidden | CHecked: 2014-01-22 (Catznip-3.6)
void LLGroupListItem::setHighlightHiddenGroup(bool highlight)
{
	mShowHidden = highlight;

	LLGroupData dataGroup;
	if ( (mGroupID.notNull()) && (gAgent.getGroupData(mGroupID, dataGroup)) )
	{
		setHidden(!dataGroup.mListInProfile) ;
	}
}

void LLGroupListItem::setHidden(bool hidden)
{
	mGroupNameSuffix = (mShowHidden) && (hidden) ? "(hidden)" : "";
	setName(mGroupName, mHighlight);
}
// [/SL:KB]

//////////////////////////////////////////////////////////////////////////
// Private Section
//////////////////////////////////////////////////////////////////////////
void LLGroupListItem::setActive(bool active)
{
//	// *BUG: setName() overrides the style params.

	// Active group should be bold.
	LLFontDescriptor new_desc(mGroupNameBox->getFont()->getFontDesc());

	// *NOTE dzaporozhan
	// On Windows LLFontGL::NORMAL will not remove LLFontGL::BOLD if font 
	// is predefined as bold (SansSerifSmallBold, for example)
	new_desc.setStyle(active ? LLFontGL::BOLD : LLFontGL::NORMAL);
	LLFontGL* new_font = LLFontGL::getFont(new_desc);
	mGroupNameStyle.font = new_font;

// [SL:KB] - Patch: UI-GroupListHidden | CHecked: 2014-01-22 (Catznip-3.6)
	setName(mGroupName, mHighlight);
// [/SL:KB]
//	// *NOTE: You cannot set the style on a text box anymore, you must
//	// rebuild the text.  This will cause problems if the text contains
//	// hyperlinks, as their styles will be wrong.
//	mGroupNameBox->setText(mGroupName, mGroupNameStyle);
}

void LLGroupListItem::onInfoBtnClick()
{
	LLFloaterReg::showInstance("inspect_group", LLSD().with("group_id", mGroupID));
}

void LLGroupListItem::onProfileBtnClick()
{
	LLGroupActions::show(mGroupID);
}

void LLGroupListItem::changed(LLGroupChange gc)
{
	LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(mID);
	if(group_data)
		setGroupIconID(group_data->mInsigniaID);
}

//EOF
