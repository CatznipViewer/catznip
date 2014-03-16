#ifndef LL_PANELEDITORINLINESEARCH_H
#define LL_PANELEDITORINLINESEARCH_H

#include "llpanel.h"

// ============================================================================
// Forward declarations
//

class LLLineEditor;
class LLTextEditor;

// ============================================================================
// LLTextSearchCtrl class
//

class LLTextSearchCtrl : public LLPanel
{
	/*
	 * Constructor
	 */
public:
	LLTextSearchCtrl(LLTextEditor* pEditor);
	virtual ~LLTextSearchCtrl();

	/*
	 * Base class overrides
	 */
public:
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setVisible(BOOL fVisible);

	/*
	 * Event handlers
	 */
protected:
	void onRefreshHighlight();
	void onSearchClicked(bool fSearchDown);

	/*
	 * Member variables
	 */
protected:
	LLLineEditor* m_pSearchEditor;
	boost::signals2::scoped_connection m_CaseInsensitiveConn;
};

// ============================================================================

#endif // LL_PANELEDITORINLINESEARCH_H
