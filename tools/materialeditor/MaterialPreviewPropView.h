#pragma once
#include "../common/PropTree/PropTreeView.h"
#include "MaterialPreviewView.h"

// MaterialPreviewPropView view

class MaterialPreviewPropView : public CPropTreeView {
	DECLARE_DYNCREATE(MaterialPreviewPropView)

protected:
	MaterialPreviewPropView();           // protected constructor used by dynamic creation
	virtual ~MaterialPreviewPropView();

public:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view

	afx_msg void OnPropertyChangeNotification( NMHDR *nmhdr, LRESULT *lresult );
	afx_msg void OnPropertyButtonClick( NMHDR *nmhdr, LRESULT *lresult );

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void AddLight( void );
	void InitializePropTree( void );

	void RegisterPreviewView( MaterialPreviewView *view );

protected:

	int		numLights;

	MaterialPreviewView	*materialPreview;

	DECLARE_MESSAGE_MAP()
};
