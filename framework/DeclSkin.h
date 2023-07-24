#ifndef __DECLSKIN_H__
#define __DECLSKIN_H__

/*
===============================================================================

	arcDeclSkin

===============================================================================
*/

typedef struct {
	const arcMaterial *		from;			// 0 == any unmatched shader
	const arcMaterial *		to;
} skinMapping_t;

class arcDeclSkin : public arcDecleration {
public:
	virtual size_t			Size() const;
	virtual bool			SetDefaultText();
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

	const arcMaterial *		RemapShaderBySkin( const arcMaterial *shader ) const;

							// model associations are just for the preview dialog in the editor
	const int				GetNumModelAssociations() const;
	const char *			GetAssociatedModel( int index ) const;

private:
	arcNetList<skinMapping_t, TAG_LIBLIST_DECL>	mappings;
	arcStringList				associatedModels;
};

#endif /* !__DECLSKIN_H__ */
