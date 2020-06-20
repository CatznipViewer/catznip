/**
 *
 * Copyright (c) 2020, Kitty Barnett
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

#pragma once

#include "lliconctrl.h"
#include "llpanel.h"
#include "lltextbox.h"

// ============================================================================
// LLFlatButton class - WIP (Do not use)
//

class LLFlatButton : public LLPanel
{
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<LLIconCtrl::Params> bg_hover;
		Optional<LLIconCtrl::Params> bg_selected;
		Optional<LLIconCtrl::Params> image;
		Optional<LLIconCtrl::Params> image_selected;
		Optional<LLTextBox::Params> label;
		Optional<CommitCallbackParam> commit_callback;

		Params();
	};

protected:
	friend class LLUICtrlFactory;
	LLFlatButton(const Params&);

	/*
	 * LLView overrides
	 */
public:
	BOOL postBuild() override;
	void setValue(const LLSD& sdValue) override;

	/*
	 * Event handlers
	 */
protected:
	void onClicked();

	/*
	 * Member variables
	 */
protected:
	LLIconCtrl* m_pBgHoverIcon = nullptr;
	LLIconCtrl* m_pBgSelectIcon = nullptr;
	LLIconCtrl* m_pImage = nullptr;
	LLIconCtrl* m_pImageSelected = nullptr;
	LLTextBox* m_pLabel = nullptr;
};

// Build time optimization, generate once in .cpp file
#ifndef LLFLATBUTTON_CPP
extern template class LLFlatButton* LLView::getChild<class LLFlatButton>(const std::string& name, BOOL recurse) const;
#endif

// ============================================================================
