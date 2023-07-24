// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_WAKES_H__
#define __GAME_WAKES_H__

#include "HardcodedParticleSystem.h"

struct sdWakeParms {

	static const int MAX_POINTS = 4;

	const arcMaterial *centerMat;
	const arcMaterial *edgeMat;
	arcVec2 centerWidths;
	arcVec2 edgeWidths;
	arcVec2 centerScales;
	arcVec2 edgeScales;
	arcVec2 centerTexCoord;
	arcVec2 edgeTexCoord;
	int maxVisDist;

	int numPoints;
	arcVec3 points[MAX_POINTS];
	arcVec3 normals[MAX_POINTS];

	bool noWake;
	void ParseFromDict( const arcDict &spawnArgs );
};

struct sdWakeNode {
	int spawnTime;
	bool breakWake;// Start a new subwake from here
	float alpha;
};

class sdWakeLayer {

public:
	static const int MAX_NODES = 64;

private:
	arcDrawVert *triangleVerts;
	int	firstVert;	//Index of first vertex in the triangle list where this layer can write to

	int numNodes;
	int firstNode;
	sdWakeNode nodes[MAX_NODES];

	// Tweakables
	float	lifeTime;		//Nodes die after this amount of time
	float	posWidth;		//With of "positive side" of the curve normal
	float	negWidth;		//With of "positive side" of the curve normal
	float	posCurScale;	//Scale curvature by this value to modulate the width the positive side
	float	negCurScale;	//Scale curvature by this value to modulate the width the negative side
	float	texNeg;			//Texture coordinate to use on the negative side
	float	texPos;			//Texture coordinate to use on the positive side

	// These update on the go
	float	texOfs;			// How far have we traveled "in curve parameter space"
	float	negScale;
	float	posScale;
	arcVec3	lastOrigin;		// Origin of last "node" call
	arcVec3	lastDeriv;		// First order derivative of last node

	int		AddToBack( sdWakeNode &node );
	void	PopFront();
	int		RemapIndex( int index );
	sdWakeNode &GetNode( int index );

public:
	sdWakeLayer( void );
	void	Init( arcDrawVert *triangleVerts, int firstVertex );
	void	AddNode( const arcVec3 &origin, const arcVec3 &emitLeft, float alpha );
	void	Update( struct srfTriangles_t *triangles );
	void	Break( void );

	void	SetWidths( float negWidth, float posWidth ) { this->posWidth = posWidth; this->negWidth = negWidth; }
	void	SetTexCoords( float texNeg, float texPos ) { this->texNeg = texNeg; this->texPos = texPos; }
	void	SetCurvatureScales( float negCurScale, float posCurScale ) { this->posCurScale = posCurScale; this->negCurScale = negCurScale; }
	int		NumNodes( void ) ;

};

class sdWake : public HardcodedParticleSystem {

	static const int MAX_POINTS = 4;

	arcVec3 minPoint;
	arcVec3 maxPoint;
	int nextNodeTime;

	arcNetList< arcDrawVert > triangleVerts[2];


//	srfTriangles_t *	GetTriSurf( int index ) { return renderEntity.hModel->Surface( index )->geometry; }
//	int					NumSurfaces( void ) { return renderEntity.hModel->NumSurfaces(); }

	sdWakeLayer layer[MAX_POINTS];
	sdWakeLayer layer3;

	int numPoints;
	arcVec3 points[MAX_POINTS];
	arcVec3 normals[MAX_POINTS];
	bool stopped;
	int ticket;

	void AddPoint( const arcVec3 &point );
	void ClearPoints( void );

public:
    sdWake( void );
	void Init(  const sdWakeParms &params, int ticket );
	void Update( const arcVec3 &forward, const arcVec3 &origin, const arcMat3 &axis );
	void Update( void );
	void Break( void );
	bool HasStopped( void );
	int GetTicket() { return ticket; }
};

#define MAX_WAKES 50

class sdWakeManagerLocal {

	int numWakes;
	int ticket;
	sdWake *wakes;

public:
	sdWakeManagerLocal() : wakes(NULL), numWakes(0) {}
	void Init( void );
	void Deinit( void );
	unsigned int AllocateWake( const sdWakeParms &params );
	bool UpdateWake( unsigned int handle, const arcVec3 &forward, const arcVec3 &origin, const arcMat3 &axis );
	void BreakWake( unsigned int wake );
	void Think( void );
};

typedef sdSingleton< sdWakeManagerLocal > sdWakeManager;

#endif //__GAME_WAKES_H__
