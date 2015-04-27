/** 
 *
 * Copyright (c) 2013-2014, Kitty Barnett
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

#include "llbutton.h"
#include "llcombobox.h"
#include "llpanel.h"

// ============================================================================
// Forward declarations
//
class LLSoundDropTarget;

// ============================================================================
// LLSoundPickerCtrl class
//

class LLSoundPickerCtrl : public LLPanel
{
public:
	struct Params :	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<LLComboBox::Params> sound_combo;
		Optional<LLButton::Params> preview_button;
		Optional<LLSoundDropTarget::Params> drop_target;

		Params();
	};

	/*
	 * Constructor
	 */
protected:
	friend class LLUICtrlFactory;
	LLSoundPickerCtrl(const LLSoundPickerCtrl::Params& p);
public:
	virtual ~LLSoundPickerCtrl();

	/*
	 * LLView overrides
	 */
public:
	/*virtual*/ BOOL postBuild();

	/*
	 * Event handlers
	 */
protected:
	void initSoundCombo();
	void onSoundDrop(const LLUUID& idSound);
	void onSoundPreview();
	void onSoundSelect();

	/*
	 * Member variables
	 */
protected:
	LLComboBox* m_pSoundCombo;
	LLButton*   m_pPreviewBtn;
	LLSoundDropTarget* m_pDropTarget;

	static LLSD s_sdSounds;
};

// ============================================================================
