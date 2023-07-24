typedef struct {
	const arcMaterial	*material;
	float				color[4];
	int					firstVert;
	int					numVerts;
	int					firstIndex;
	int					numIndexes;
} guiModelSurface_t;

class arcInteractiveGuiModel {
public:
	arcInteractiveGuiModel();

	void	Clear();

	//void	WriteToDemo( ARCDemoFile *demo );
	//void	ReadFromDemo( ARCDemoFile *demo );

	void	EmitToCurrentView( float modelMatrix[16], bool depthHack );
	void	EmitFullScreen();

	// these calls are forwarded from the renderer
	void	SetColor( float r, float g, float b, float a );
	void	DrawStretchPic( const arcDrawVert *verts, const qglIndex_t *indexes, int vertCount, int indexCount, const arcMaterial *hShader,
									bool clip = true, float min_x = 0.0f, float min_y = 0.0f, float max_x = 640.0f, float max_y = 480.0f );
	void	DrawStretchPic( float x, float y, float w, float h,
									float s1, float t1, float s2, float t2, const arcMaterial *hShader);
	void	DrawStretchTri ( arcVec2 p1, arcVec2 p2, arcVec2 p3, arcVec2 t1, arcVec2 t2, arcVec2 t3, const arcMaterial *material );

	//---------------------------
private:
	void	AdvanceSurf();
	void	EmitSurface( guiModelSurface_t *surf, float modelMatrix[16], float modelViewMatrix[16], bool depthHack );

	guiModelSurface_t		*surf;

	arcNetList<guiModelSurface_t>	surfaces;
	arcNetList<qglIndex_t>		indexes;
	arcNetList<arcDrawVert>	verts;
};

