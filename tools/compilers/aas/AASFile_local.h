/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __AASFILELOCAL_H__
#define __AASFILELOCAL_H__

/*
===============================================================================

	AAS File Local

===============================================================================
*/

class anSEASFileLocal : public anSEASFile {
	friend class anSEASBuild;
	friend class anSEASReach;
	friend class anSEASCluster;
public:
								anSEASFileLocal( void );
	virtual 					~anSEASFileLocal( void );

public:
	virtual anVec3				EdgeCenter( int edgeNum ) const;
	virtual anVec3				FaceCenter( int faceNum ) const;
	virtual anVec3				AreaCenter( int areaNum ) const;

	virtual anBounds			EdgeBounds( int edgeNum ) const;
	virtual anBounds			FaceBounds( int faceNum ) const;
	virtual anBounds			AreaBounds( int areaNum ) const;

	virtual int					PointAreaNum( const anVec3 &origin ) const;
	virtual int					PointReachableAreaNum( const anVec3 &origin, const anBounds &searchBounds, const int areaFlags, const int excludeTravelFlags ) const;
	virtual int					BoundsReachableAreaNum( const anBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const;
	virtual void				PushPointIntoAreaNum( int areaNum, anVec3 &point ) const;
	virtual bool				Trace( seasTrace_t &trace, const anVec3 &start, const anVec3 &end ) const;
	virtual void				PrintInfo( void ) const;

public:
	bool						Load( const anString &fileName, unsigned int mapFileCRC );
	bool						Write( const anString &fileName, unsigned int mapFileCRC );

	int							MemorySize( void ) const;
	void						ReportRoutingEfficiency( void ) const;
	void						Optimize( void );
	void						LinkReversedReachability( void );
	void						FinishAreas( void );

	void						Clear( void );
	void						DeleteReachabilities( void );
	void						DeleteClusters( void );

private:
	bool						ParseIndex( anLexer &src, anList<seasIndex_t> &indexes );
	bool						ParsePlanes( anLexer &src );
	bool						ParseVertices( anLexer &src );
	bool						ParseEdges( anLexer &src );
	bool						ParseFaces( anLexer &src );
	bool						ParseReachabilities( anLexer &src, int areaNum );
	bool						ParseAreas( anLexer &src );
	bool						ParseNodes( anLexer &src );
	bool						ParsePortals( anLexer &src );
	bool						ParseClusters( anLexer &src );

private:
	int							BoundsReachableAreaNum_r( int nodeNum, const anBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const;
	void						MaxTreeDepth_r( int nodeNum, int &depth, int &maxDepth ) const;
	int							MaxTreeDepth( void ) const;
	int							AreaContentsTravelFlags( int areaNum ) const;
	anVec3						AreaReachableGoal( int areaNum ) const;
	int							NumReachabilities( void ) const;
};

#endif /* !__AASFILELOCAL_H__ */
