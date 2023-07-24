#pragma once

#include "MaterialEditor.h"

/**
* View in the Material Editor that functions as a Doom III
* console. It allows users to view console output as well as issue
* console commands to the engine.
*/
class ConsoleView : public CFormView {
public:
	enum{ IDD = IDD_CONSOLE_FORM };

	CEdit			editConsole;
	CEdit			editInput;

	arcNetString		consoleStr;
	arcStringList	consoleHistory;
	arcNetString		currentCommand;
	int				currentHistoryPosition;
	bool			saveCurrentCommand;

public:
	virtual			~ConsoleView();

	//Public Operations
	void			AddText(const char *msg);
	void			SetConsoleText( const arcNetString& text );
	void			ExecuteCommand( const arcNetString& cmd = "" );
protected:
	ConsoleView();
	DECLARE_DYNCREATE(ConsoleView)

	//CFormView Overrides
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);
	virtual void	OnInitialUpdate();

	//Message Handlers
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

	//Protected Operations
	const char*		TranslateString(const char *buf);


};
