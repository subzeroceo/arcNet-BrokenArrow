#ifndef __RENDERWINDOW_H
#define __RENDERWINDOW_H

class anUserInterface;
class idRenderWindow : public idWindow {
public:
  idRenderWindow(anUserInterface *gui);
  idRenderWindow(idDeviceContext *d, anUserInterface *gui);
  virtual ~idRenderWindow();

  virtual void PostParse();
  virtual void Draw( inttime, float x, float y);
  virtual size_t Allocated() { return idWindow::Allocated(); };
  virtual idWinVar *GetWinVarByName( const char *_name, bool winLookup = false, drawWin_t **owner = nullptr );

private:
  void CommonInit();
  virtual bool ParseInternalVar( const char *name, anParser *src);
  void Render( inttime);
  void PreRender();
  void BuildAnimation( inttime);
  renderView_t refdef;
  anRenderWorld *world;
  renderEntity_t worldEntity;
  renderLight_t rLight;
  const anM8DAnim *modelAnim;

  qhandle_t worldModelDef;
  qhandle_t lightDef;
  qhandle_t modelDef;
  idWinStr modelName;
  idWinStr animName;
  anString animClass;
  idWinVec4 lightOrigin;
  idWinVec4 lightColor;
  idWinVec4 modelOrigin;
  idWinVec4 modelRotate;
  idWinVec4 viewOffset;
  idWinBool needsRender;
  int animLength;
  int animEndTime;
  bool updateAnimation;
};

#endif // __RENDERWINDOW_H
