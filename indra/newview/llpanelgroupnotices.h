/** 
 * @file llpanelgroupnotices.h
 * @brief A panel to display group notices.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_LLPANELGROUPNOTICES_H
#define LL_LLPANELGROUPNOTICES_H

#include "llpanelgroup.h"
#include "llpointer.h"
#include "llinventory.h"

class LLLineEditor;
class LLTextEditor;
class LLButton;
class LLIconCtrl;
class LLCheckBoxCtrl;
class LLScrollListCtrl;

// [SL:KB] - Patch: Notification-GroupCreateNotice | Checked: 2012-02-16 (Catznip-3.2.2) | Added: Catznip-3.2.2
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLGroupDropTarget : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		// *NOTE: These parameters logically Mandatory, but are not
		// specified in XML files, hence Optional
		Optional<LLUUID>	group_id;
		Params()
		:	group_id("group_id")
		{
			changeDefault(mouse_opaque, false);
			changeDefault(follows.flags, FOLLOWS_ALL);
		}
	};
	LLGroupDropTarget(const Params&);
	~LLGroupDropTarget() {};

	void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	//
	// LLView functionality
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
	void setGroup (LLUUID group) {mGroupID = group;};

protected:
	LLUUID	mGroupID;
};
// [/SL:KB]

class LLPanelGroupNotices : public LLPanelGroupTab
{
public:
	LLPanelGroupNotices();
	virtual ~LLPanelGroupNotices();

	// LLPanelGroupTab
	virtual void activate();
	//virtual bool needsApply(std::string& mesg);
	//virtual bool apply(std::string& mesg);
	//virtual void update();
	
	virtual BOOL postBuild();
	virtual BOOL isVisibleByAgent(LLAgent* agentp);

// [SL:KB] - Patch: Notification-GroupCreateNotice | Checked: 2012-02-16 (Catznip-3.2.2) | Added: Catznip-3.2.2
//	/*virtual*/ S32 notifyParent(const LLSD& sdInfo);
// [/SL:KB]

//	void setItem(LLPointer<LLInventoryItem> inv_item);

	static void processGroupNoticesListReply(LLMessageSystem* msg, void** data);

	void showNotice(const std::string& subject,
					const std::string& message,
					const bool& has_inventory,
					const std::string& inventory_name,
					LLOfferInfo* inventory_offer);

	void refreshNotices();

	virtual void setGroupID(const LLUUID& id);

private:
//	static void onClickRemoveAttachment(void* data);
	static void onClickOpenAttachment(void* data);
//	static void onClickSendMessage(void* data);
	static void onClickNewMessage(void* data);
	static void onClickRefreshNotices(void* data);

	void processNotices(LLMessageSystem* msg);
	static void onSelectNotice(LLUICtrl* ctrl, void* data);

//	enum ENoticeView
//	{
//		VIEW_PAST_NOTICE,
//		CREATE_NEW_NOTICE
//	};
//
//	void arrangeNoticeView(ENoticeView view_type);

	LLPointer<LLInventoryItem>	mInventoryItem;
	
//	LLLineEditor		*mCreateSubject;
//    LLLineEditor		*mCreateInventoryName;
//	LLTextEditor		*mCreateMessage;

	LLLineEditor		*mViewSubject;
    LLLineEditor		*mViewInventoryName;
	LLTextEditor		*mViewMessage;
	
//	LLButton			*mBtnSendMessage;
	LLButton			*mBtnNewMessage;
//	LLButton			*mBtnRemoveAttachment;
	LLButton			*mBtnOpenAttachment;
	LLButton			*mBtnGetPastNotices;

//	LLPanel				*mPanelCreateNotice;
	LLPanel				*mPanelViewNotice;

//	LLIconCtrl		 *mCreateInventoryIcon;
	LLIconCtrl		 *mViewInventoryIcon;
	
	LLScrollListCtrl *mNoticesList;

	std::string		mNoNoticesStr;

	LLOfferInfo* mInventoryOffer;

	static std::map<LLUUID,LLPanelGroupNotices*>	sInstances;
};

#endif
