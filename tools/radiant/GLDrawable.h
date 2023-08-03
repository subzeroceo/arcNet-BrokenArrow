#pragma once
#ifdef None
#undef None
#endif

enum class MouseButton {
	None,
	Left,
	Right,
	Middle
};

class idGLDrawable {
public:
	idGLDrawable();
	~idGLDrawable() {};
	virtual void draw( int x, int y, int w, int h );
	virtual void setMedia( const char *name ){}
	virtual void buttonDown( MouseButton button, float x, float y );
	virtual void buttonUp( MouseButton button, float x, float y );
	virtual void mouseMove( float x, float y );
	virtual int getRealTime() { return realTime; };
	virtual bool ScreenCoords() {
		return true;
	}
	void SetRealTime( int i ) {
		realTime = i;
	}
	virtual void Update() {};
	float getScale() {
		return scale;
	}
	void setScale( float f ) {
		scale = f;
	}
protected:
	float scale;
	float xOffset;
	float yOffset;
	float zOffset;
	float pressX;
	float pressY;
	bool  handleMove;
	MouseButton button;
	int realTime;
};

class idGLDrawableWorld : public idGLDrawable {
public:
	idGLDrawableWorld();
	~idGLDrawableWorld();
	void AddTris( srfTriangles_t *tris, const anMaterial *mat );
	virtual void draw( int x, int y, int w, int h ) override;
	virtual void mouseMove( float x, float y ) override;
	virtual void Update() override { worldDirty = true; };
	virtual void buttonDown( MouseButton button, float x, float y ) override;
	void InitWorld();
protected:

	void InitLight( const anVec3& position );

	anRenderWorld *world;
	anRenderModel *worldModel;
	arcNetHandle_t	worldModelDef;
	arcNetHandle_t	lightDef;
	arcNetHandle_t   modelDef;
	float       light;

	//model
	bool worldDirty;
	anString skinStr;
	anQuats rotation;
	anVec3 lastPress;
	float radius;
	anVec4 rect;
};

class idGLDrawableMaterial : public idGLDrawableWorld {
public:
	idGLDrawableMaterial( const anMaterial *mat ) {
		material = mat;
		scale = 2.0;
		light = 1.0;
		worldDirty = true;
	}

	idGLDrawableMaterial() {
		material = nullptr;
		light = 1.0;
		worldDirty = true;
		realTime = 50;
	}

	virtual void setMedia( const char *name ) override;
	virtual void draw( int x, int y, int w, int h ) override;

	//protected:
	const anMaterial *material;
};

class idGLDrawableModel : public idGLDrawableWorld {
public:
	idGLDrawableModel( const char *name );
	idGLDrawableModel();

	virtual void setMedia( const char *name ) override;

	virtual void draw( int x, int y, int w, int h ) override;
	virtual bool ScreenCoords() override {
		return false;
	}

	void SetSkin( const char *skin );
};