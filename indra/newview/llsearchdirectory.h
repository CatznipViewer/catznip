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
#ifndef LL_LLPLACESSEARCH_H
#define LL_LLPLACESSEARCH_H

#include "llsingleton.h"

// ============================================================================
// LLSearchPlaceResult helper struct
//

struct LLSearchPlaceResult
{
	LLSearchPlaceResult() : mForSale(false), mIsAuction(false), mDwell(0.0f) {}

	LLSearchPlaceResult(const LLUUID& idParcel, const std::string& strName, bool fForSale, bool fIsAuction, F32 nDwell)
		: mParcelId(idParcel), mParcelName(strName), mForSale(fForSale), mIsAuction(fIsAuction), mDwell(nDwell)
	{
	}

	LLUUID      mParcelId;
	std::string mParcelName;
	BOOL        mForSale;
	BOOL        mIsAuction;
	F32         mDwell;
};

// ============================================================================
// LLSearchDirectory class
//

class LLSearchDirectory : public LLSingleton<LLSearchDirectory>
{
	friend class LLSingleton<LLSearchDirectory>;
private:
	LLSearchDirectory();
	/*virtual*/ ~LLSearchDirectory();

	/*
	 * Member functions
	 */
public:
	void cancelQuery(const LLUUID& idQuery);

	typedef std::vector<LLSearchPlaceResult> places_results_vec_t;
	typedef boost::function<void(const LLUUID&, U32, const places_results_vec_t&)> places_callback_t;
	LLUUID queryPlaces(std::string strQuery, S8 nCategory, U32 nFlags, S32 idxStart, places_callback_t cb);
	static void processPlacesReply(LLMessageSystem* pMsg, void**);

	/*
	 * Member variables
	 */
public:
	static U32 MIN_QUERY_LENGTH_PLACES;
	static U32 NUM_RESULTS_PAGE_PLACES;
protected:
	typedef std::map<LLUUID, places_callback_t> places_callback_map_t;
	places_callback_map_t mPlacesCallbackMap;
};

// ============================================================================

#endif //LL_LLPLACESSEARCH_H
