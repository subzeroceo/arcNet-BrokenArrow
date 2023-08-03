#ifndef __GAMEWINDOW_H__
#define __GAMEWINDOW_H__

class idGameWindowProxy : public idWindow {
public:
				idGameWindowProxy( idDeviceContext *d, anUserInterface *gui );
	void		Draw( int time, float x, float y );
};

#endif
