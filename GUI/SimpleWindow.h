#ifndef __SIMPLEWIN_H__
#define __SIMPLEWIN_H__

class anUserInterface;
class idDeviceContext;
class idSimpleWindow;

typedef struct {
	idWindow *win;
	idSimpleWindow *simp;
} drawWin_t;

class idSimpleWindow {
	friend class idWindow;
public:
					idSimpleWindow(idWindow* win);
	virtual			~idSimpleWindow();
	void			Redraw(float x, float y);
	void			StateChanged( bool redraw );

	anStr			name;

	idWinVar *		GetWinVarByName(const char *_name);
	int				GetWinVarOffset( idWinVar *wv, drawWin_t* owner);
	size_t			Size();

	idWindow*		GetParent ( void ) { return mParent; }

	virtual void	WriteToSaveGame( anFile *savefile );
	virtual void	ReadFromSaveGame( anFile *savefile );

protected:
	void 			CalcClientRect(float xofs, float yofs);
	void 			SetupTransforms(float x, float y);
	void 			DrawBackground(const idRectangle &drawRect);
	void 			DrawBorderAndCaption(const idRectangle &drawRect);

	anUserInterface *gui;
	idDeviceContext *dc;
	int 			flags;
	idRectangle 	drawRect;			// overall rect
	idRectangle 	clientRect;			// client area
	idRectangle 	textRect;
	anVec2			origin;
	int 			fontNum;
	float 			matScalex;
	float 			matScaley;
	float 			borderSize;
	int 			textAlign;
	float 			textAlignx;
	float 			textAligny;
	int				textShadow;

	idWinStr		text;
	idWinBool		visible;
	idWinRectangle	rect;				// overall rect
	idWinVec4		backColor;
	idWinVec4		matColor;
	idWinVec4		foreColor;
	idWinVec4		borderColor;
	idWinFloat		textScale;
	idWinFloat		rotate;
	idWinVec2		shear;
	idWinBackground	backGroundName;

	const anMaterial* background;
	
	idWindow *		mParent;

	idWinBool	hideCursor;
};

#endif // !__SIMPLEWIN_H__
