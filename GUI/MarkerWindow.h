#ifndef __MARKERWINDOW_H
#define __MARKERWINDOW_H

class anUserInterface;

typedef struct {
	int time;
	const anMaterial *mat;
	idRectangle rect;
} markerData_t;

class idMarkerWindow : public idWindow {
public:
	idMarkerWindow(anUserInterface *gui);
	idMarkerWindow(idDeviceContext *d, anUserInterface *gui);
	virtual ~idMarkerWindow();
	virtual size_t Allocated(){return idWindow::Allocated();};
	virtual idWinVar *GetWinVarByName(const char *_name, bool winLookup = false);

	virtual const char *HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void PostParse();
	virtual void Draw( inttime, float x, float y);
	virtual const char *RouteMouseCoords(float xd, float yd);
	virtual void		Activate(bool activate, anString &act);
	virtual void MouseExit();
	virtual void MouseEnter();

	
private:
	virtual bool ParseInternalVar(const char *name, anParser *src);
	void CommonInit();
	void Line( intx1, int y1, int x2, int y2, dword* out, dword color);
	void Point( intx, int y, dword *out, dword color);
	logStats_t loggedStats[MAX_LOGGED_STATS];
	anList<markerData_t> markerTimes;
	anString statData;
	int numStats;
	dword *imageBuff;
	const anMaterial *markerMat;
	const anMaterial *markerStop;
	anVec4 markerColor;
	int currentMarker;
	int currentTime;
	int stopTime;
};

#endif // __MARKERWINDOW_H
