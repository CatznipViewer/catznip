/** 
 *
 * Copyright (c) 2016, Kitty Barnett
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

#include "llviewerprecompiledheaders.h" // must be first include

#include "llavatarnamecache.h"
#include "llcombobox.h"
#include "llfiltereditor.h"
#include "llfloaternotificationsflat.h"

#include <boost/algorithm/string.hpp>

extern LLNotificationDateComparator NOTIF_DATE_COMPARATOR;

// =========================================================================
// LLFloaterNotificationsFlat class
//

LLFloaterNotificationsFlat::LLFloaterNotificationsFlat(const LLSD& key)
	: LLFloaterNotifications(key),
	  m_pMessageList(nullptr),
	  m_pFilterType(nullptr),
	  m_pFilterText(nullptr)
{
}

LLFloaterNotificationsFlat::~LLFloaterNotificationsFlat()
{
}

BOOL LLFloaterNotificationsFlat::postBuild()
{
	m_pMessageList = getChild<LLNotificationListView>("notification_list");

	m_pFilterType = getChild<LLComboBox>("filter_type");
	m_pFilterType->setCommitCallback(boost::bind(&LLFloaterNotificationsFlat::refreshFilter, this));
	m_pFilterText = getChild<LLFilterEditor>("filter_text");
	m_pFilterText->setCommitCallback(boost::bind(&LLFloaterNotificationsFlat::refreshFilter, this));

	m_pMessageList->setComparator(&NOTIF_DATE_COMPARATOR);

	return LLFloaterNotifications::postBuild();
}

bool LLFloaterNotificationsFlat::checkFilter(const LLNotificationListItem* pItem) const
{
	bool fVisible = true;

	LLNotificationPtr pNotification = (pItem) ? LLNotifications::getInstance()->find(pItem->getID()) : nullptr;
	if (pNotification)
	{
		// Filter by type
		if ( (fVisible) && (!m_strFilterType.empty()))
		{
			fVisible = (m_strFilterType == pNotification->getType());
		}

		// Filter by text
		if ( (fVisible) && (!m_strFilterText.empty()) )
		{
			if ("groupnotify" == pNotification->getType())
			{
				const LLSD& sdPayload = pNotification->getPayload();

				// Check the notice's subject or body
				fVisible = 
					(!boost::ifind_first(sdPayload["subject"].asString(), m_strFilterText).empty()) || 
					(!boost::ifind_first(sdPayload["message"].asString(), m_strFilterText).empty());
				// Check the group's name
				if (!fVisible)
				{
					const LLUUID idGroup = sdPayload["group_id"]; std::string strGroupName;
					if ( (idGroup.notNull()) && (gCacheName->getGroupName(idGroup, strGroupName)) )
						fVisible = !boost::ifind_first(strGroupName, m_strFilterText).empty();
				}
				// Check the sender's name
				if (!fVisible)
				{
					const LLUUID idSender = sdPayload["sender_id"]; LLAvatarName avSender;
					if ( (idSender.notNull()) && (LLAvatarNameCache::get(idSender, &avSender)) )
					{
						fVisible =
							(!boost::ifind_first(avSender.getCompleteName(), m_strFilterText).empty()) ||
							( (!avSender.isDisplayNameDefault()) && (!boost::ifind_first(avSender.getLegacyName(), m_strFilterText).empty()) );
					}
				}
			}
			else
			{
				fVisible = !boost::ifind_first(pItem->getTitle(), m_strFilterText).empty();
			}
		}

		// Filter by notification name (inclusive)
		if ( (fVisible) && (!m_FilterNames.empty()) )
		{
			fVisible = (std::find(m_FilterNames.begin(), m_FilterNames.end(), pNotification->getName()) != m_FilterNames.end());
		}

		// Filter by notification name (exclusive)
		if ( (fVisible) && (!m_FilterNamesExclude.empty()) )
		{
			fVisible = (std::find(m_FilterNamesExclude.begin(), m_FilterNamesExclude.end(), pNotification->getName()) == m_FilterNamesExclude.end());
		}
	}

	return fVisible;
}

void LLFloaterNotificationsFlat::refreshFilter()
{
	std::string strNewFilterText = m_pFilterText->getText(), strNewFilterType;
	std::set<std::string> NewFilterNames, NewFilterNamesExclude;
	switch ((ENotificationFilter)m_pFilterType->getSelectedValue().asInteger())
	{
		case NF_SYSTEM:
			strNewFilterType = "notify";
			NewFilterNamesExclude = LLNotificationListItem::getTransactionTypes();
			break;
		case NF_GROUP_NOTICE:
			strNewFilterType = "groupnotify";
			break;
		case NF_OFFERS:
			strNewFilterType = "offer";
			break;
		case NF_TRANSACTION:
			strNewFilterType = "notify";
			NewFilterNames = LLNotificationListItem::getTransactionTypes();
			break;
	}

	const bool fHasFilter = (!strNewFilterType.empty()) || (!strNewFilterText.empty()) || (NewFilterNames.empty()) || (NewFilterNamesExclude.empty());

	const bool fFilterSubset =
		(fHasFilter) && 
		(strNewFilterType == m_strFilterType) && 
		(!m_strFilterText.empty()) && (!boost::ifind_first(strNewFilterText, m_strFilterText).empty()) &&
		(NewFilterNames == m_FilterNames) &&
		(NewFilterNamesExclude == m_FilterNamesExclude);

	m_strFilterType = strNewFilterType;
	m_strFilterText = strNewFilterText;
	m_FilterNames = NewFilterNames;
	m_FilterNamesExclude = NewFilterNamesExclude;

	std::vector<LLPanel*> itemList;
	m_pMessageList->getItems(itemList);
	for (LLPanel* pItemPanel : itemList)
	{
		LLNotificationListItem* pWellItem = (fHasFilter) ? dynamic_cast<LLNotificationListItem*>(pItemPanel) : nullptr;
		if (!pWellItem)
		{
			// Unknown item (visible when not filtering, otherwise invisible) or not filtering
			pItemPanel->setVisible(!fHasFilter);
			continue;
		}

		// If the new filtered set will be a subset of the previous pass then we can only check the currently visible items
		if ( (!fFilterSubset) || (pWellItem->getVisible()) )
			pWellItem->setVisible(checkFilter(pWellItem));
	}

	m_pMessageList->notify(LLSD().with("rearrange", LLSD()));
}

bool LLFloaterNotificationsFlat::isWindowEmpty() const
{
	// NOTE: consider all items, not just the visible ones
	return (!m_pMessageList) || (m_pMessageList->size(false) == 0);
}

bool LLFloaterNotificationsFlat::addNotification(LLNotificationListItem* pItem)
{
	if (m_pMessageList->addNotification(pItem, false))
	{
		pItem->setVisible(checkFilter(pItem));
		m_pMessageList->sort();
		return true;
	}
	return false;
}

void LLFloaterNotificationsFlat::closeVisibleNotifications()
{
	// Need to clear notification channel, to add storable toasts into the list.
	clearScreenChannels();

	std::vector<LLPanel*> itemList;
	m_pMessageList->getItems(itemList);
	for (LLPanel* pItemPanel : itemList)
	{
		if (!pItemPanel->getVisible())
			continue;

		LLNotificationListItem* pItemNotify = dynamic_cast<LLNotificationListItem*>(pItemPanel);
		if (pItemNotify)
			onItemClose(pItemNotify);
	}
}

void LLFloaterNotificationsFlat::collapseVisibleNotifications()
{
	std::vector<LLPanel*> itemList;
	m_pMessageList->getItems(itemList);
	for (LLPanel* pItemPanel : itemList)
	{
		if (!pItemPanel->getVisible())
			continue;

		LLNotificationListItem* pItemNotify = dynamic_cast<LLNotificationListItem*>(pItemPanel);
		if (pItemNotify)
			pItemNotify->setExpanded(FALSE);
	}
}

LLPanel* LLFloaterNotificationsFlat::findNotificationByID(const LLUUID& id, const std::string& /*type*/)
{
	return m_pMessageList->getItemByValue(id);
}

void LLFloaterNotificationsFlat::getNotifications(std::vector<LLNotificationListItem*>& items)
{
	items.clear();
	
	std::vector<LLPanel*> itemList;
	m_pMessageList->getItems(itemList);

	for (LLPanel* pItemPanel : itemList)
	{
		LLNotificationListItem* pItemNotify = dynamic_cast<LLNotificationListItem*>(pItemPanel);
		if (pItemNotify)
			items.push_back(pItemNotify);
	}
}

bool LLFloaterNotificationsFlat::removeNotificationByID(const LLUUID& id, const std::string& /*type*/)
{
	return m_pMessageList->removeItemByValue(id);
}

void LLFloaterNotificationsFlat::updateNotificationCounters()
{
}

// =========================================================================
