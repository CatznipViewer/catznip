/**
 * @file llfloaterconversationpreview.h
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LLFLOATERCONVERSATIONPREVIEW_H_
#define LLFLOATERCONVERSATIONPREVIEW_H_

#include "llchathistory.h"
#include "llfloater.h"

extern const std::string LL_FCP_COMPLETE_NAME;	//"complete_name"
extern const std::string LL_FCP_ACCOUNT_NAME;		//"user_name"
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
extern const std::string LL_FCP_SESSION_ID;         //"session_id"
extern const std::string LL_FCP_CONVERSATION_PATH;  //"conversation_path"
// [/SL:KB]

// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
class LLComboBox;
// [/SL:KB]
class LLSpinCtrl;

class LLFloaterConversationPreview : public LLFloater
{
public:

	LLFloaterConversationPreview(const LLSD& session_id);
	virtual ~LLFloaterConversationPreview();

	virtual BOOL postBuild();
	void setPages(std::list<LLSD>* messages,const std::string& file_name);

	virtual void draw();
	virtual void onOpen(const LLSD& key);
	virtual void onClose(bool app_quitting);

// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
protected:
	void onMonthFilterChanged();
	void onSearch(const std::string& strNeedle, bool fSearchUp);
	void refreshMonthFilter();
// [/SL:KB]
private:
	void onMoreHistoryBtnClick();
	void showHistory();

	LLMutex			mMutex;
	LLSpinCtrl*		mPageSpinner;
	LLChatHistory*	mChatHistory;
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
	LLComboBox*		mFilterCombo = nullptr;
// [/SL:KB]
	LLUUID			mSessionID;
	int				mCurrentPage;
	int				mPageSize;

	std::list<LLSD>*	mMessages;
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
	typedef std::pair<short, char> monthyear_pair_t;
	typedef std::pair<size_t, size_t> message_filter_t;
	typedef std::map<monthyear_pair_t, message_filter_t> month_lookup_type_t;
	message_filter_t    mMessageFilter = std::make_pair(0, 0);
	month_lookup_type_t mMonthLookup;
// [/SL:KB]
//	std::string		mAccountName;
//	std::string		mCompleteName;
	std::string		mChatHistoryFileName;
	bool			mShowHistory;
	bool			mHistoryThreadsBusy;
	bool			mOpened;
};

#endif /* LLFLOATERCONVERSATIONPREVIEW_H_ */
