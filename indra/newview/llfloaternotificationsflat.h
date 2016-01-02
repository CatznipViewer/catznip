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

#ifndef LL_FLOATERNOTIFICATIONSFLAT_H
#define LL_FLOATERNOTIFICATIONSFLAT_H

#include "llfloaternotificationstabbed.h"

// =========================================================================
// Forward declarations
//
class LLComboBox;
class LLFilterEditor;

// =========================================================================
// LLFloaterNotificationsFlat class
//

class LLFloaterNotificationsFlat : public LLFloaterNotifications
{
	/*
	 * Constructor
	 */
public:
    LOG_CLASS(LLFloaterNotificationsFlat);

    LLFloaterNotificationsFlat(const LLSD& key);
    virtual ~LLFloaterNotificationsFlat();

	/*
	 * Base class overrides
	 */
public:
    /*virtual*/ BOOL postBuild();

	/*
	 * Helper functions
	 */
protected:
	enum ENotificationFilter
	{
		NF_NONE = 0,
		NF_SYSTEM,
		NF_GROUP_NOTICE,
		NF_OFFERS,
		NF_TRANSACTION
	};

	bool checkFilter(const LLNotificationListItem* pItem) const;
	void refreshFilter();

	/*
	 * Pure virtual functions
	 */
public:
	/*virtual*/ bool isWindowEmpty() const;
protected:
	/*virtual*/ bool addNotification(LLNotificationListItem* item);
	/*virtual*/ void closeVisibleNotifications();
	/*virtual*/ void collapseVisibleNotifications();
	/*virtual*/ LLPanel* findNotificationByID(const LLUUID& id, const std::string& type);
	/*virtual*/ void getNotifications(std::vector<LLNotificationListItem*>& items);
	/*virtual*/ bool removeNotificationByID(const LLUUID& id, const std::string& type);
	/*virtual*/ void updateNotificationCounters();

	/*
	 * Member variables
	 */
private:
    LLNotificationListView*	m_pMessageList;

	LLComboBox*     m_pFilterType;
	LLFilterEditor* m_pFilterText;

	std::string     m_strFilterText;
	std::string     m_strFilterType;
	std::set<std::string> m_FilterNames;
	std::set<std::string> m_FilterNamesExclude;
};

// =========================================================================

#endif // LL_FLOATERNOTIFICATIONSFLAT_H
