typedef struct {
	const anMaterial	*material;
	float				color[4];
	int					firstVert;
	int					numVerts;
	int					firstIndex;
	int					numIndexes;
} guiModelSurface_t;

class anInteractiveGuiModel {
public:
	anInteractiveGuiModel();

	void	Clear();

	//void	WriteToDemo( anDemoFile *demo );
	//void	ReadFromDemo( anDemoFile *demo );

	void	EmitToCurrentView( float modelMatrix[16], bool depthHack );
	void	EmitFullScreen();

	// these calls are forwarded from the renderer
	void	SetColor( float r, float g, float b, float a );
	void	DrawStretchPic( const anDrawVertex *verts, const qglIndex_t *indexes, int vertCount, int indexCount, const anMaterial *hShader,
									bool clip = true, float min_x = 0.0f, float min_y = 0.0f, float max_x = 640.0f, float max_y = 480.0f );
	void	DrawStretchPic( float x, float y, float w, float h,
									float s1, float t1, float s2, float t2, const anMaterial *hShader);
	void	DrawStretchTri ( anVec2 p1, anVec2 p2, anVec2 p3, anVec2 t1, anVec2 t2, anVec2 t3, const anMaterial *material );

	//---------------------------
private:
	void	AdvanceSurf();
	void	EmitSurface( guiModelSurface_t *surf, float modelMatrix[16], float modelViewMatrix[16], bool depthHack );

	guiModelSurface_t		*surf;

	anList<guiModelSurface_t>	surfaces;
	anList<qglIndex_t>		indexes;
	anList<anDrawVertex>	verts;
};

