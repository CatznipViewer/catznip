/** 
 *
 * Copyright (c) 2012, Kitty Barnett
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
#include "llviewerprecompiledheaders.h"

#include "message.h"
#include "llagent.h"
#include "llqueryflags.h"
#include "llsearchdirectory.h"

// ============================================================================
// LLSearchDirectory
//

LLSearchDirectory::LLSearchDirectory()
{
}

void LLSearchDirectory::cancelQuery(const LLUUID& idQuery)
{
	places_callback_map_t::iterator itCallback = mPlacesCallbackMap.find(idQuery);
	if (mPlacesCallbackMap.end() != itCallback)
	{
		mPlacesCallbackMap.erase(itCallback);
		return;
	}
}

LLUUID LLSearchDirectory::queryPlaces(const std::string& strQuery, S8 nCategory, U32 nFlags, S32 idxStart, LLSearchDirectory::places_callback_t cb)
{
	LLUUID idQuery = LLUUID::generateNewID();

	nFlags |= DFQ_DWELL_SORT;

	gMessageSystem->newMessage("DirPlacesQuery");
	gMessageSystem->nextBlock("AgentData");
	gMessageSystem->addUUID("AgentID", gAgent.getID());
	gMessageSystem->addUUID("SessionID", gAgent.getSessionID());
	gMessageSystem->nextBlock("QueryData");
	gMessageSystem->addUUID("QueryID", idQuery);
	gMessageSystem->addString("QueryText", strQuery);
	gMessageSystem->addU32("QueryFlags", nFlags);
	gMessageSystem->addS8("Category", nCategory);
	gMessageSystem->addString("SimName", "");
	gMessageSystem->addS32Fast(_PREHASH_QueryStart, idxStart);
	gAgent.sendReliableMessage();

	mPlacesCallbackMap[idQuery] = cb;

	return idQuery;
}

void LLSearchDirectory::processPlacesReply(LLMessageSystem* pMsg, void**)
{
	// See message_template.msg - DirPlacesReply
	LLUUID idAgent, idQuery;
	pMsg->getUUID(_PREHASH_AgentData, _PREHASH_AgentID, idAgent);
	pMsg->getUUID(_PREHASH_QueryData, _PREHASH_QueryID, idQuery );

	places_callback_map_t::iterator itCallback = LLSearchDirectory::instance().mPlacesCallbackMap.find(idQuery);
	if (LLSearchDirectory::instance().mPlacesCallbackMap.end() == itCallback)
		return;

	S32 cntReply = pMsg->getNumberOfBlocks(_PREHASH_QueryReplies); places_results_vec_t placeResults;
	for (S32 idxReply = 0; idxReply < cntReply ; idxReply++)
	{
		LLSearchPlaceResult placeResult;

		pMsg->getUUID(_PREHASH_QueryReplies, _PREHASH_ParcelID, placeResult.mParcelId, idxReply);
		pMsg->getString(_PREHASH_QueryReplies, _PREHASH_Name, placeResult.mParcelName, idxReply);
		pMsg->getBOOL(_PREHASH_QueryReplies, _PREHASH_ForSale, placeResult.mForSale, idxReply);
		pMsg->getBOOL(_PREHASH_QueryReplies, _PREHASH_Auction, placeResult.mIsAuction, idxReply);
		pMsg->getF32(_PREHASH_QueryReplies, _PREHASH_Dwell, placeResult.mDwell, idxReply);

		placeResults.push_back(placeResult);
	}

	U32 nStatus = STATUS_SEARCH_PLACES_NONE;
	if (pMsg->has(_PREHASH_StatusData))
	{
		pMsg->getU32(_PREHASH_StatusData, _PREHASH_Status, nStatus);
	}

	(itCallback->second)(idQuery, nStatus, placeResults);
}

// ============================================================================
