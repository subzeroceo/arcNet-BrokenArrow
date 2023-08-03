#ifndef __SEASFILELOCAL_H__
#define __SEASFILELOCAL_H__

/*
===============================================================================

	SEAS File
 [S]ituational Awareness System:
 [E]nvironment Awareness System:
			Awareness
 [S]patial Awareness System:
===============================================================================
*/

class anSEASFileLocal : public anSEASFile {
	friend class anSEASBuild;
	friend class anSEASReach;
	friend class anSEASCluster;
public:
								anSEASFileLocal();
	virtual 					~anSEASFileLocal();

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
	virtual void				PrintInfo() const;

public:
	bool						Load( const anString &fileName, unsigned int mapFileCRC );
	bool						Write( const anString &fileName, unsigned int mapFileCRC );

	int							MemorySize() const;
	void						ReportRoutingEfficiency() const;
	void						Optimize();
	void						LinkReversedReachability();
	void						FinishAreas();

	void						Clear();
	void						DeleteReachabilities();
	void						DeleteClusters();

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
	int							MaxTreeDepth() const;
	int							AreaContentsTravelFlags( int areaNum ) const;
	anVec3						AreaReachableGoal( int areaNum ) const;
	int							NumReachabilities() const;
};

#endif // !__SEASFILELOCAL_H__
