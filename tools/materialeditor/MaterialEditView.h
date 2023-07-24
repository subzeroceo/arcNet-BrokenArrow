#pragma once

#include "MaterialEditor.h"
#include "MaterialPropTreeView.h"
#include "StageView.h"

#include "../comafx/CSyntaxRichEditCtrl.h"

/**
* View that contains the material edit controls. These controls include
* the stage view, the properties view and the source view.
*/
class MaterialEditView : public CFormView, public MaterialView, SourceModifyOwner {

public:
	enum{ IDD = IDD_MATERIALEDIT_FORM };

	CEdit						m_nameEdit;
	CSplitterWnd				m_editSplitter;

	StageView*					m_stageView;
	MaterialPropTreeView*		m_materialPropertyView;
	CTabCtrl					m_tabs;
	CSyntaxRichEditCtrl			m_textView;

public:
	virtual			~MaterialEditView();

	//MaterialView Interface
	virtual void	MV_OnMaterialSelectionChange(MaterialDoc* pMaterial);
	virtual void	MV_OnMaterialNameChanged(MaterialDoc* pMaterial, const char* oldName);

	//SourceModifyOwner Interface
	virtual arcNetString GetSourceText();

protected:
	MaterialEditView();
	DECLARE_DYNCREATE(MaterialEditView)

	void			GetMaterialSource();
	void			ApplyMaterialSource();

	//CFormView Overrides
	virtual void	DoDataExchange(CDataExchange* pDX);
	virtual void	OnInitialUpdate();

	//Message Handlers
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg void 	OnTcnSelChange(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnEnChangeEdit( NMHDR *pNMHDR, LRESULT *pResult );
	DECLARE_MESSAGE_MAP()

protected:
	bool initHack;
	bool sourceInit;

	bool	sourceChanged;
	arcNetString	currentMaterialName;
};
