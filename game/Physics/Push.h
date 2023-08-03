
#ifndef __PUSH_H__
#define __PUSH_H__

/*
===============================================================================

  Allows physics objects to be pushed geometrically.

===============================================================================
*/

#define PUSHFL_ONLYMOVEABLE			1		// only push moveable entities
#define PUSHFL_NOGROUNDENTITIES		2		// don't push entities the clip model rests upon
#define PUSHFL_CLIP					4		// also clip against all non-moveable entities
#define PUSHFL_CRUSH				8		// kill blocking entities
#define PUSHFL_APPLYIMPULSE			16		// apply impulse to pushed entities

//#define NEW_PUSH

class idPush {
public:
					// Try to push other entities by moving the given entity.
					// If results.fraction < 1.0 the move was blocked by results.c.entityNum
					// Returns total mass of all pushed entities.
	float			ClipTranslationalPush( trace_t &results, anEntity *pusher, const int flags,
											const anVec3 &newOrigin, const anVec3 &move );

	float			ClipRotationalPush( trace_t &results, anEntity *pusher, const int flags,
											const anMat3 &newAxis, const anRotation &rotation );

	float			ClipPush( trace_t &results, anEntity *pusher, const int flags,
											const anVec3 &oldOrigin, const anMat3 &oldAxis,
												anVec3 &newOrigin, anMat3 &newAxis );

					// initialize saving the positions of entities being pushed
	void			InitSavingPushedEntityPositions( void );
					// move all pushed entities back to their previous position
	void			RestorePushedEntityPositions( void );
					// returns the number of pushed entities
	int				GetNumPushedEntities( void ) const { return numPushed; }
					// get the ith pushed entity
	anEntity *		GetPushedEntity( int i ) const { assert( i >= 0 && i < numPushed ); return pushed[i].ent; }

private:
	struct pushed_s {
		anEntity *	ent;					// pushed entity
		anAngles	deltaViewAngles;		// actor delta view angles
	}				pushed[MAX_GENTITIES];	// pushed entities
	int				numPushed;				// number of pushed entities

	struct pushedGroup_s {
		anEntity *	ent;
		float		fraction;
		bool		groundContact;
		bool		test;
	}				pushedGroup[MAX_GENTITIES];
	int				pushedGroupSize;

private:
	void			SaveEntityPosition( anEntity *ent );
	bool			RotateEntityToAxial( anEntity *ent, anVec3 rotationPoint );
#ifdef NEW_PUSH
	bool			CanPushEntity( anEntity *ent, anEntity *pusher, anEntity *initialPusher, const int flags );
	void			AddEntityToPushedGroup( anEntity *ent, float fraction, bool groundContact );
	bool			IsFullyPushed( anEntity *ent );
	bool			ClipTranslationAgainstPusher( trace_t &results, anEntity *ent, anEntity *pusher, const anVec3 &translation );
	int				GetPushableEntitiesForTranslation( anEntity *pusher, anEntity *initialPusher, const int flags,
											const anVec3 &translation, anEntity *entityList[], int maxEntities );
	bool			ClipRotationAgainstPusher( trace_t &results, anEntity *ent, anEntity *pusher, const anRotation &rotation );
	int				GetPushableEntitiesForRotation( anEntity *pusher, anEntity *initialPusher, const int flags,
											const anRotation &rotation, anEntity *entityList[], int maxEntities );
#else
	void			ClipEntityRotation( trace_t &trace, const anEntity *ent, const anClipModel *clipModel,
										anClipModel *skip, const anRotation &rotation );
	void			ClipEntityTranslation( trace_t &trace, const anEntity *ent, const anClipModel *clipModel,
										anClipModel *skip, const anVec3 &translation );
	int				TryTranslatePushEntity( trace_t &results, anEntity *check, anClipModel *clipModel, const int flags,
												const anVec3 &newOrigin, const anVec3 &move );
	int				TryRotatePushEntity( trace_t &results, anEntity *check, anClipModel *clipModel, const int flags,
												const anMat3 &newAxis, const anRotation &rotation );
	int				DiscardEntities( anEntity *entityList[], int numEntities, int flags, anEntity *pusher );
#endif
};

#endif /* !__PUSH_H__ */
