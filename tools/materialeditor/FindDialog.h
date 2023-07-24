#pragma once

#include "MaterialEditor.h"
#include "../common/registryoptions.h"

class MEMainFrame;

/**
* Dialog that provides an input box and several checkboxes to define
* the parameters of a search. These parameters include: text string, search
* scope and search only name flag.
*/
class FindDialog : public CDialog
{

public:
	enum { IDD = IDD_FIND };

public:
	FindDialog(CWnd* pParent = NULL);
	virtual ~FindDialog();

	BOOL					Create();

protected:
	DECLARE_DYNAMIC(FindDialog)

	//Overrides
	virtual void			DoDataExchange(CDataExchange* pDX);
	virtual BOOL			OnInitDialog();

	//Messages
	afx_msg void			OnBnClickedFindNext();
	virtual void			OnCancel();
	DECLARE_MESSAGE_MAP()

	//Protected Operations
	void					LoadFindSettings();
	void					SaveFindSettings();

protected:
	MEMainFrame*			parent;
	MaterialSearchData_t	searchData;
	rvRegistryOptions		registry;
};
