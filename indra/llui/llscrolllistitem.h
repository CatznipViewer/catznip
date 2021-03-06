/** 
 * @file llscrolllistitem.h
 * @brief Scroll lists are composed of rows (items), each of which 
 * contains columns (cells).
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LLSCROLLLISTITEM_H
#define LLSCROLLLISTITEM_H

#include "llpointer.h"		// LLPointer<>
#include "llsd.h"
#include "v4color.h"
#include "llinitparam.h"
#include "llscrolllistcell.h"
#include "llcoord.h"

#include <vector>

class LLCheckBoxCtrl;
class LLResizeBar;
class LLScrollListCtrl;
class LLScrollColumnHeader;
class LLUIImage;

//---------------------------------------------------------------------------
// LLScrollListItem
//---------------------------------------------------------------------------
class LLScrollListItem
{
	friend class LLScrollListCtrl;
public:
// [SL:KB] - Patch: Control-ScrollList | Checked: Catznip-5.2
	typedef boost::function<void(const LLSD& value, LLScrollListCell* cell)> commit_callback_t;
	typedef boost::signals2::signal<void(const LLSD& value, LLScrollListCell* cell)> commit_signal_t;
// [/SL:KB]

	struct Params : public LLInitParam::Block<Params>
	{
		Optional<bool>		enabled;
// [SL:KB] - Patch: Control-ScrollList | Checked: Catznip-5.2
		Optional<commit_callback_t> commit_callback;
// [/SL:KB]
		Optional<void*>		userdata;
		Optional<LLSD>		value;
// [SL:KB] - Patch: Control-ComboItemRemove | Checked: Catznip-3.6
		Optional<bool>		user_removable;
// [/SL:KB]
		
		Ignored				name; // use for localization tools
		Ignored				type; 
		Ignored				length; 

		Multiple<LLScrollListCell::Params> columns;

		Params()
		:	enabled("enabled", true),
			value("value"),
// [SL:KB] - Patch: Control-ComboItemRemove | Checked: Catznip-3.6
			user_removable("user_removable"),
// [/SL:KB]
			name("name"),
			type("type"),
			length("length"),
			columns("columns")
		{
			addSynonym(columns, "column");
			addSynonym(value, "id");
		}
	};

	virtual ~LLScrollListItem();

	void	setSelected( BOOL b )			{ mSelected = b; }
	BOOL	getSelected() const				{ return mSelected; }

	void	setEnabled( BOOL b )			{ mEnabled = b; }
	BOOL	getEnabled() const 				{ return mEnabled; }

	void	setHighlighted( BOOL b )		{ mHighlighted = b; }
	BOOL	getHighlighted() const			{ return mHighlighted; }

	void	setUserdata( void* userdata )	{ mUserdata = userdata; }
	void*	getUserdata() const 			{ return mUserdata; }

// [SL:KB] - Patch: Control-ComboItemRemove | Checked: Catznip-3.6
	void	setUserRemovable(bool b)		{ mUserRemovable = b; }
	bool	getUserRemovable() const 		{ return mUserRemovable; }
// [/SL:KB]

	virtual LLUUID	getUUID() const			{ return mItemValue.asUUID(); }
	LLSD	getValue() const				{ return mItemValue; }
	
	void	setRect(LLRect rect)			{ mRectangle = rect; }
	LLRect	getRect() const					{ return mRectangle; }

	void	addColumn( const LLScrollListCell::Params& p );

	void	setNumColumns(S32 columns);

	void	setColumn( S32 column, LLScrollListCell *cell );
	
	S32		getNumColumns() const;

	LLScrollListCell *getColumn(const S32 i) const;

	std::string getContentsCSV() const;

	virtual void draw(const LLRect& rect, const LLColor4& fg_color, const LLColor4& bg_color, const LLColor4& highlight_color, S32 column_padding);

protected:
	LLScrollListItem( const Params& );

private:
	BOOL	mSelected;
	BOOL	mHighlighted;
	BOOL	mEnabled;
	void*	mUserdata;
// [SL:KB] - Patch: Control-ComboItemRemove | Checked: Catznip-3.6
	bool	mUserRemovable;
// [/SL:KB]

	LLSD	mItemValue;
	std::vector<LLScrollListCell *> mColumns;
	LLRect  mRectangle;
};

#endif
