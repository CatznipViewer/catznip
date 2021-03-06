/** 
 * @file llsearcheditor.h
 * @brief Text editor widget that represents a search operation
 *
 * Features: 
 *		Text entry of a single line (text, delete, left and right arrow, insert, return).
 *		Callbacks either on every keystroke or just on the return key.
 *		Focus (allow multiple text entry widgets)
 *		Clipboard (cut, copy, and paste)
 *		Horizontal scrolling to allow strings longer than widget size allows 
 *		Pre-validation (limit which keys can be used)
 *		Optional line history so previous entries can be recalled by CTRL UP/DOWN
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_SEARCHEDITOR_H
#define LL_SEARCHEDITOR_H

#include "lllineeditor.h"
#include "llbutton.h"

class LLSearchEditor : public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLLineEditor::Params>
	{
		Optional<LLButton::Params>	search_button, 
									clear_button;
		Optional<bool>				search_button_visible, 
									clear_button_visible;
		Optional<commit_callback_t> keystroke_callback;

		Params()
		:	search_button("search_button"),
			search_button_visible("search_button_visible"),
			clear_button("clear_button"), 
			clear_button_visible("clear_button_visible")
		{}
	};

	void setCommitOnFocusLost(BOOL b)	{ if (mSearchEditor) mSearchEditor->setCommitOnFocusLost(b); }

protected:
	LLSearchEditor(const Params&);
	friend class LLUICtrlFactory;

public:
	virtual ~LLSearchEditor() {}

	/*virtual*/ void	draw();

	void setText(const LLStringExplicit &new_text) { mSearchEditor->setText(new_text); }
	const std::string& getText() const		{ return mSearchEditor->getText(); }
// [SL:KB] - Patch: Control-SearchEditor
	void setTextColor(const LLColor4& c) { mSearchEditor->setFgColor(c); }
	const LLColor4& getTextColor() const { return mSearchEditor->getFgColor(); }
// [/SL:KB]

	// LLUICtrl interface
	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;
	virtual BOOL	setTextArg( const std::string& key, const LLStringExplicit& text );
	virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );
	virtual void	setLabel( const LLStringExplicit &new_label );
	virtual void	clear();
	virtual void	setFocus( BOOL b );

// [SL:KB] - Patch: UI-SidepanelPlacesSearch | Checked: 2012-08-31 (Catznip-3.3)
	/*virtual*/ BOOL isDirty() const;
	/*virtual*/ void resetDirty();
// [/SL:KB]

	void			setKeystrokeCallback( commit_callback_t cb ) { mKeystrokeCallback = cb; }
	void			setTextChangedCallback( commit_callback_t cb ) { mTextChangedCallback = cb; }

protected:
// [SL:KB] - Patch: Control-AvatarEditor | Checked: Catznip-5.2
	virtual void onClearButtonClick(const LLSD& data);
	virtual void onSearchButtonClick();
// [/SL:KB]
//	void onClearButtonClick(const LLSD& data);
	virtual void handleKeystroke();

	commit_callback_t mKeystrokeCallback;
	commit_callback_t mTextChangedCallback;
	LLLineEditor* mSearchEditor;
	LLButton* mSearchButton;
	LLButton* mClearButton;
};

#endif  // LL_SEARCHEDITOR_H
