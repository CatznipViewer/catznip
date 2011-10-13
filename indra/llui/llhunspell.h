/** 
 *
 * Copyright (c) 2010, Kitty Barnett
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

#ifndef LL_HUNSPELL_H
#define LL_HUNSPELL_H

#include "llsingleton.h"
#include <boost/signals2.hpp>

class Hunspell;

// ============================================================================

class LLSpellChecker : public LLSingleton<LLSpellChecker>
{
	friend class LLSingleton<LLSpellChecker>;
protected:
	LLSpellChecker();
	~LLSpellChecker();

public:
	void addToCustomDictionary(const std::string& strWord);
	void addToIgnoreList(const std::string& strWord);
	bool checkSpelling(const std::string& strWord) const;
	S32  getSuggestions(const std::string& strWord, std::vector<std::string>& strSuggestionList) const;

	/*
	 * Dictionary related functions
	 */
public:
	const LLSD	getDictionaryData(const std::string& strDictionary) const;
	const LLSD&	getDictionaryMap() const		{ return m_sdDictionaryMap; }
	void		refreshDictionaryMap();
	void		setSecondaryDictionaries(std::vector<std::string> dictList);
protected:
	void		addToDictFile(const std::string& strDictPath, const std::string& strWord);
	void		initHunspell(const std::string& strDictionary);

	/*
	 * Static member functions
	 */
public:
	typedef boost::signals2::signal<void()> settings_change_signal_t;
	static const std::string	getDictionaryAppPath();
	static const std::string	getDictionaryUserPath();
	static boost::signals2::connection setSettingsChangeCallback(const settings_change_signal_t::slot_type& cb);
	static void					setUseSpellCheck(const std::string& strDictionary);
	static bool					useSpellCheck();

	/*
	 * Member variables
	 */
protected:
	Hunspell*					m_pHunspell;
	std::string					m_strDictionaryName;
	std::string					m_strDictionaryFile;
	std::vector<std::string>	m_SecondaryDictionaries;
	LLSD						m_sdDictionaryMap;
	std::vector<std::string>	m_IgnoreList;

	static settings_change_signal_t	s_SettingsChangeSignal;
};

// ============================================================================

#endif // LL_HUNSPELL_H
