#ifndef __LISTWINDOW_H
#define __LISTWINDOW_H

class idSliderWindow;

enum {
	TAB_TYPE_TEXT = 0,
	TAB_TYPE_ICON = 1
};

struct idTabRect {
	int x;
	int w;
	int align;
	int valign;
	int	type;
	anVec2 iconSize;
	float iconVOffset;
};

class anListWindow : public idWindow {
public:
	anListWindow(anUserInterface *gui);
	anListWindow(idDeviceContext *d, anUserInterface *gui);

	virtual const char*	HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void		PostParse();
	virtual void		Draw( inttime, float x, float y);
	virtual void		Activate(bool activate, anStr &act);
	virtual void		HandleBuddyUpdate(idWindow *buddy);
	virtual void		StateChanged( bool redraw = false );
	virtual size_t		Allocated(){return idWindow::Allocated();};
	virtual idWinVar*	GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t** owner = nullptr );

	void				UpdateList();
	
private:
	virtual bool		ParseInternalVar(const char *name, anParser *src);
	void				CommonInit();
	void				InitScroller( bool horizontal );
	void				SetCurrentSel( int sel );
	void				AddCurrentSel( int sel );
	int					GetCurrentSel();
	bool				IsSelected( int index );
	void				ClearSelection( int sel );

	anList<idTabRect>	tabInfo;
	int					top;
	float				sizeBias;
	bool				horizontal;
	anStr				tabStopStr;
	anStr				tabAlignStr;
	anStr				tabVAlignStr;
	anStr				tabTypeStr;
	anStr				tabIconSizeStr;
	anStr				tabIconVOffsetStr;
	idHashTable<const anMaterial*> iconMaterials;						
	bool				multipleSel;

	anStringList			listItems;
	idSliderWindow*		scroller;
	anList<int>			currentSel;
	anStr				listName;

	int					clickTime;

	int					typedTime;
	anStr				typed;
};

#endif // __LISTWINDOW_H
