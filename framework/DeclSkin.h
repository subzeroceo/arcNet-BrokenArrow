#ifndef __DECLSKIN_H__
#define __DECLSKIN_H__

/*
===============================================================================

	anDeclSkin

===============================================================================
*/

typedef struct {
	const anMaterial *		from;			// 0 == any unmatched shader
	const anMaterial *		to;
} skinMapping_t;

class anDeclSkin : public anDecl {
public:
	virtual size_t			Size() const;
	virtual bool			SetDefaultText();
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

	static void				CacheFromDict( const anDict &dict );

	const anMaterial *		RemapShaderBySkin( const anMaterial *shader ) const;

							// model associations are just for the preview dialog in the editor
	const int				GetNumModelAssociations() const;
	const char *			GetAssociatedModel( int index ) const;

private:
	//anList<skinMapping_t, TAG_LIBLIST_DECL>	mappings;
	anList<skinMapping_t>	mappings;
	anStringList				associatedModels;
};

#endif // !__DECLSKIN_H__
