/**
 * @file llfloaterconversationpreview.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llavatarnamecache.h"
#include "llconversationlog.h"
#include "llfloaterconversationpreview.h"
#include "llimview.h"
#include "lllineeditor.h"
#include "llfloaterimnearbychat.h"
#include "llspinctrl.h"
#include "lltrans.h"
#include "llnotificationsutil.h"
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
#include <boost/lexical_cast.hpp>
// [/SL:KB]

const std::string LL_FCP_COMPLETE_NAME("complete_name");
const std::string LL_FCP_ACCOUNT_NAME("user_name");
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
const std::string LL_FCP_SESSION_ID("session_id");
const std::string LL_FCP_CONVERSATION_PATH("conversation_path");
// [/SL:KB]

LLFloaterConversationPreview::LLFloaterConversationPreview(const LLSD& session_id)
:	LLFloater(session_id),
	mChatHistory(NULL),
//	mSessionID(session_id.asUUID()),
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
	mSessionID( (!session_id.isMap()) ? session_id.asUUID() : session_id[LL_FCP_SESSION_ID].asUUID() ),
// [/SL:KB]
	mCurrentPage(0),
	mPageSize(gSavedSettings.getS32("ConversationHistoryPageSize")),
//	mAccountName(session_id[LL_FCP_ACCOUNT_NAME]),
//	mCompleteName(session_id[LL_FCP_COMPLETE_NAME]),
	mMutex(),
	mShowHistory(false),
	mMessages(NULL),
	mHistoryThreadsBusy(false),
	mIsGroup(false),
	mOpened(false)
{
}

LLFloaterConversationPreview::~LLFloaterConversationPreview()
{
}

BOOL LLFloaterConversationPreview::postBuild()
{
	mChatHistory = getChild<LLChatHistory>("chat_history");
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
	mChatHistory->getEditor()->setTrackEnd(false);
	mFilterCombo = findChild<LLComboBox>("filter_combo");
	mFilterCombo->setCommitCallback(boost::bind(&LLFloaterConversationPreview::onMonthFilterChanged, this));
	mSearchEditor = findChild<LLLineEditor>("search_editor");
	mSearchEditor->setCommitOnFocusLost(false);
	mSearchEditor->setCommitCallback(boost::bind(&LLFloaterConversationPreview::onSearch, this, ESearchDirection::DOWN));
	findChild<LLButton>("search_prev_btn")->setCommitCallback(boost::bind(&LLFloaterConversationPreview::onSearch, this, ESearchDirection::UP));
	findChild<LLButton>("search_next_btn")->setCommitCallback(boost::bind(&LLFloaterConversationPreview::onSearch, this, ESearchDirection::DOWN));
	findChild<LLButton>("open_editor_btn")->setCommitCallback(boost::bind(&LLFloaterConversationPreview::onOpenEditor, this));
// [/SL:KB]

	const LLConversation* conv = LLConversationLog::instance().getConversation(mSessionID);
	std::string name;
	std::string file;

//	if (mAccountName != "")
//	{
//		name = mCompleteName;
//		file = mAccountName;
//	}
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
	const LLSD& sdKey = getKey();
	if (sdKey.isMap())
	{
		name = sdKey[LL_FCP_COMPLETE_NAME].asString();
		file = sdKey[LL_FCP_CONVERSATION_PATH].asString();
	}
// [/SL:KB]
	else if (mSessionID != LLUUID::null && conv)
	{
		name = conv->getConversationName();
		file = conv->getHistoryFileName();
		mIsGroup = (LLIMModel::LLIMSession::GROUP_SESSION == conv->getConversationType());
	}
	else
	{
		name = LLTrans::getString("NearbyChatTitle");
		file = "chat";
	}
	mChatHistoryFileName = file;
	if (mIsGroup)
	{
		mChatHistoryFileName += GROUP_CHAT_SUFFIX;
	}
	LLStringUtil::format_map_t args;
	args["[NAME]"] = name;
	std::string title = getString("Title", args);
	setTitle(title);

	return LLFloater::postBuild();
}

void LLFloaterConversationPreview::setPages(std::list<LLSD>* messages, const std::string& file_name)
{
	// [SL:KB] - NOTE: This runs in the context of the LLLoadHistoryThread and *not* the main thread
	if(file_name == mChatHistoryFileName && messages)
	{
		// additional protection to avoid changes of mMessages in setPages()
		LLMutexLock lock(&mMutex);
		if (mMessages)
		{
			delete mMessages; // Clean up temporary message list with "Loading..." text
		}
		mMessages = messages;
//		mCurrentPage = (mMessages->size() ? (mMessages->size() - 1) / mPageSize : 0);

		mPageSpinner->setEnabled(true);
//		mPageSpinner->setMaxValue(mCurrentPage+1);
//		mPageSpinner->set(mCurrentPage+1);
//
//		std::string total_page_num = llformat("/ %d", mCurrentPage+1);
//		getChild<LLTextBox>("page_num_label")->setValue(total_page_num);
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
		mRefreshMonthFilter = true;
// [/SL:KB]
		mShowHistory = true;
	}
	LLLoadHistoryThread* loadThread = LLLogChat::getInstance()->getLoadHistoryThread(mSessionID);
	if (loadThread)
	{
		loadThread->removeLoadEndSignal(boost::bind(&LLFloaterConversationPreview::setPages, this, _1, _2));
	}
}

// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
void LLFloaterConversationPreview::onMonthFilterChanged()
{
	const LLSD& sdValue = mFilterCombo->getSelectedValue();

	const month_lookup_type_t::iterator monthLookup = mMonthLookup.find(std::make_pair(sdValue["year"].asInteger(), sdValue["month"].asInteger()));
	if (mMonthLookup.end() != monthLookup)
	{
		mMessageFilter = monthLookup->second;

		mCurrentPage = 0;
		mPageSpinner->setMaxValue((mMessageFilter.second - mMessageFilter.first) / mPageSize + 1);
		mPageSpinner->forceSetValue(mCurrentPage + 1);
	}
	else
	{
		mMessageFilter = std::make_pair(0, 0);

		mCurrentPage = (mMessages->size() ? (mMessages->size() - 1) / mPageSize : 0);
		mPageSpinner->setMaxValue(mCurrentPage + 1);
		mPageSpinner->forceSetValue(mCurrentPage + 1);
	}

	getChild<LLTextBox>("page_num_label")->setValue(llformat("/ %d", (int)mPageSpinner->getMaxValue()));
	mShowHistory = true;
}

void LLFloaterConversationPreview::onOpenEditor()
{
	std::string strFilePath = LLLogChat::makeLogFileName(mChatHistoryFileName);
	if ( (!strFilePath.empty()) && (!LLFile::isfile(strFilePath)) )
	{
		strFilePath = LLLogChat::oldLogFileName(mChatHistoryFileName);
		if ( (!strFilePath.empty()) && (!LLFile::isfile(strFilePath)) )
			return;
	}

	LLView::getWindow()->openFile(strFilePath);
}

void LLFloaterConversationPreview::onSearch(ESearchDirection eDirection)
{
	if (mSearchEditor->getText().empty())
	{
		return;
	}

	if (LLTextEditor* pChatEditor = mChatHistory->getEditor())
	{
		// If the user manually scrolled the chat hisotry then start searching at the first visible line rather than the top
		if ( (!pChatEditor->scrolledToStart()) && (pChatEditor->getCursorPos() == 0) )
		{
			pChatEditor->setCursorPos(pChatEditor->getFirstVisibleLine());
		}

		bool fContinue = true;
		do
		{
			fContinue = !pChatEditor->selectNext(mSearchEditor->getText(), true, false, ESearchDirection::UP == eDirection, true);
			if (fContinue)
			{
				int newPage = (ESearchDirection::DOWN == eDirection) ? mPageSpinner->get() + 1 : mPageSpinner->get() - 1;
				if ( (newPage < mPageSpinner->getMinValue()) || (newPage > mPageSpinner->getMaxValue()) )
				{
					fContinue = false;
					LLNotificationsUtil::add("GenericAlert", LLSD().with("MESSAGE", getString((newPage < mPageSpinner->getMinValue()) ? "NotFoundStart" : "NotFoundEnd")));
					newPage = llclamp(newPage, (int)mPageSpinner->getMinValue(), (int)mPageSpinner->getMaxValue());
					if (mPageSpinner->getValueF32() == newPage)
						break;
				}

				mPageSpinner->forceSetValue(newPage);
				mPageSpinner->onCommit();
				showHistory();
				mShowHistory = false;

				if (ESearchDirection::DOWN == eDirection)
					pChatEditor->startOfDoc();
				else
					pChatEditor->endOfDoc();
			}
		} while (fContinue);
	}
}

void LLFloaterConversationPreview::refreshMonthFilter()
{
	// Timestamps, if present, will conform to TIMESTAMP_AND_STUFF so we can extract by character index (see lllogchat.cpp)
	// (although as it turns out we can't trust any log that was imported from a different viewer)
	int idxMessage = 0; month_lookup_type_t::iterator monthLookup = mMonthLookup.end(), yearLookup = mMonthLookup.end();
	for (std::list<LLSD>::const_iterator itMessage = mMessages->cbegin(), endMessage = mMessages->cend(); itMessage != endMessage; ++itMessage, ++idxMessage)
	{
		const LLSD& sdMessage = *itMessage;
		if (!sdMessage.has(LL_IM_TIME))
			continue;

		std::string strTime = sdMessage[LL_IM_TIME].asString();
		if (strTime.length() < 9)
			continue;

		try
		{
			int nYear = boost::lexical_cast<int>(strTime.data(), 4);
			int nMonth = boost::lexical_cast<int>(strTime.data() + 5, 2);

			if ( (monthLookup != mMonthLookup.end()) || (monthLookup->second.first != nYear) || (monthLookup->second.second != nMonth) )
			{
				yearLookup = mMonthLookup.find(std::make_pair(nYear, 0));
				monthLookup = mMonthLookup.find(std::make_pair(nYear, nMonth));
			}

			if (mMonthLookup.end() != monthLookup)
			{
				monthLookup->second.second = idxMessage;
			}
			else
			{
				if ( (yearLookup != mMonthLookup.end()) || (yearLookup->second.first != nYear) )
					yearLookup = mMonthLookup.insert(std::make_pair(std::make_pair(nYear, 0), std::make_pair(idxMessage, idxMessage))).first;
				monthLookup = mMonthLookup.insert(std::make_pair(std::make_pair(nYear, nMonth), std::make_pair(idxMessage, idxMessage))).first;
			}
			yearLookup->second.second = idxMessage;
		}
		catch (const boost::bad_lexical_cast&)
		{
			continue;
		}
	}

	if (LLStringOps::sMonthList.empty())
		LLStringOps::setupMonthNames(LLTrans::getString("dateTimeMonthNames"));

	mFilterCombo->clear();
	mFilterCombo->add(getString("DefaultFilter"));
	if (!mMonthLookup.empty())
	{
		mFilterCombo->addSeparator();
		for (const auto& kvYearMonth : mMonthLookup)
		{
			std::string strItem;
			if (kvYearMonth.first.second > 0)
				strItem = llformat("%s %d", LLStringOps::sMonthList[kvYearMonth.first.second - 1].c_str(), kvYearMonth.first.first);
			else
				strItem = llformat("-- %d --", kvYearMonth.first.first);
			mFilterCombo->add(strItem, LLSD().with("year", kvYearMonth.first.first).with("month", kvYearMonth.first.second));
		}
	}
	mFilterCombo->selectFirstItem();

	mRefreshMonthFilter = false;
}
// [/SL:KB]

void LLFloaterConversationPreview::draw()
{
	if(mShowHistory)
	{
		showHistory();
		mShowHistory = false;
	}
	LLFloater::draw();
}

void LLFloaterConversationPreview::onOpen(const LLSD& key)
{
	if (mOpened)
	{
		return;
	}
	mOpened = true;
	if (!LLLogChat::getInstance()->historyThreadsFinished(mSessionID))
	{
		LLNotificationsUtil::add("ChatHistoryIsBusyAlert");
		mHistoryThreadsBusy = true;
		closeFloater();
		return;
	}
	LLSD load_params;
	load_params["load_all_history"] = true;
	load_params["cut_off_todays_date"] = false;
	load_params["is_group"] = mIsGroup;

	// The temporary message list with "Loading..." text
	// Will be deleted upon loading completion in setPages() method
	mMessages = new std::list<LLSD>();


	LLSD loading;
	loading[LL_IM_TEXT] = LLTrans::getString("loading_chat_logs");
	mMessages->push_back(loading);
	mPageSpinner = getChild<LLSpinCtrl>("history_page_spin");
	mPageSpinner->setCommitCallback(boost::bind(&LLFloaterConversationPreview::onMoreHistoryBtnClick, this));
	mPageSpinner->setMinValue(1);
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
	mPageSpinner->forceSetValue(1);
// [/SL:KB]
//	mPageSpinner->set(1);
	mPageSpinner->setEnabled(false);

	// The actual message list to load from file
	// Will be deleted in a separate thread LLDeleteHistoryThread not to freeze UI
	// LLDeleteHistoryThread is started in destructor
	std::list<LLSD>* messages = new std::list<LLSD>();

	LLLogChat *log_chat_inst = LLLogChat::getInstance();
	log_chat_inst->cleanupHistoryThreads();
	
	LLLoadHistoryThread* loadThread = new LLLoadHistoryThread(mChatHistoryFileName, messages, load_params);
	loadThread->setLoadEndSignal(boost::bind(&LLFloaterConversationPreview::setPages, this, _1, _2));
	loadThread->start();
	log_chat_inst->addLoadHistoryThread(mSessionID, loadThread);

	LLDeleteHistoryThread* deleteThread = new LLDeleteHistoryThread(messages, loadThread);
	log_chat_inst->addDeleteHistoryThread(mSessionID, deleteThread);

	mShowHistory = true;
}

void LLFloaterConversationPreview::onClose(bool app_quitting)
{
	mOpened = false;
	if (!mHistoryThreadsBusy)
	{
		LLDeleteHistoryThread* deleteThread = LLLogChat::getInstance()->getDeleteHistoryThread(mSessionID);
		if (deleteThread)
		{
			deleteThread->start();
		}
	}
}

void LLFloaterConversationPreview::showHistory()
{
	// additional protection to avoid changes of mMessages in setPages
	LLMutexLock lock(&mMutex);

// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
	if (mRefreshMonthFilter)
	{
		refreshMonthFilter();
		onMonthFilterChanged();
	}
// [/SL:KB]

	if(mMessages == NULL || !mMessages->size() || mCurrentPage * mPageSize >= mMessages->size())
	{
		return;
	}

	mChatHistory->clear();
	std::ostringstream message;
	std::list<LLSD>::const_iterator iter = mMessages->begin();
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
	std::list<LLSD>::const_iterator iter_end = mMessages->end();
	if (mMessageFilter.first > 0)
	{
		int idxStart = mMessageFilter.first + mCurrentPage * mPageSize;
		std::advance(iter, idxStart);
		iter_end = iter;
		std::advance(iter_end, mMessageFilter.second - idxStart + 1);
	}
	else
	{
		std::advance(iter, mCurrentPage * mPageSize);
	}
// [/SL:KB]
//	std::advance(iter, mCurrentPage * mPageSize);

//	for (int msg_num = 0; iter != mMessages->end() && msg_num < mPageSize; ++iter, ++msg_num)
// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
	for (int msg_num = 0; iter != iter_end && msg_num < mPageSize; ++iter, ++msg_num)
// [/SL:KB]
	{
		LLSD msg = *iter;

		LLUUID from_id 		= LLUUID::null;
		std::string time	= msg["time"].asString();
		std::string from	= msg["from"].asString();
		std::string message	= msg["message"].asString();

		if (msg["from_id"].isDefined())
		{
			from_id = msg["from_id"].asUUID();
		}
		else
 		{
			std::string legacy_name = gCacheName->buildLegacyName(from);
			from_id = LLAvatarNameCache::getInstance()->findIdByName(legacy_name);
 		}

		LLChat chat;
		chat.mFromID = from_id;
		chat.mSessionID = mSessionID;
		chat.mFromName = from;
		chat.mTimeStr = time;
		chat.mChatStyle = CHAT_STYLE_HISTORY;
		chat.mText = message;

		if (from_id.isNull() && SYSTEM_FROM == from)
		{
			chat.mSourceType = CHAT_SOURCE_SYSTEM;

		}
		else if (from_id.isNull())
		{
			chat.mSourceType = LLFloaterIMNearbyChat::isWordsName(from) ? CHAT_SOURCE_UNKNOWN : CHAT_SOURCE_OBJECT;
		}

		LLSD chat_args;
		chat_args["use_plain_text_chat_history"] =
						gSavedSettings.getBOOL("PlainTextChatHistory");
		chat_args["show_time"] = gSavedSettings.getBOOL("IMShowTime");
		chat_args["show_names_for_p2p_conv"] = gSavedSettings.getBOOL("IMShowNamesForP2PConv");

		mChatHistory->appendMessage(chat,chat_args);
	}

// [SL:KB] - Patch: Chat-Logs | Checked: Catznip-5.2
	mChatHistory->getEditor()->deselect();
	if (mCurrentPage + 1 < mPageSpinner->getMaxValue())
		mChatHistory->getEditor()->startOfDoc();
	else
		mChatHistory->getEditor()->endOfDoc();
// [/SL:KB]
}

void LLFloaterConversationPreview::onMoreHistoryBtnClick()
{
	mCurrentPage = (int)(mPageSpinner->getValueF32());
	if (!mCurrentPage)
	{
		return;
	}

	mCurrentPage--;
	mShowHistory = true;
}
