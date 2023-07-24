#ifndef __DECLTABLE_H__
#define __DECLTABLE_H__

/*
===============================================================================

	tables are used to map a floating point input value to a floating point
	output value, with optional wrap / clamp and interpolation

===============================================================================
*/

class arcDeclTable : public arcDecleration {
public:
	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

	float					TableLookup( float index ) const;

private:
	bool					clamp;
	bool					snap;
	arcNetList<float, TAG_LIBLIST_DECL>			values;
};

#endif