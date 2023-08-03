#ifndef __DECLTABLE_H__
#define __DECLTABLE_H__

/*
===============================================================================

	tables are used to map a floating point input value to a floating point
	output value, with optional wrap / clamp and interpolation

===============================================================================
*/

class anDeclTable : public anDecl {
public:
	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

			float			GetMaxValue( void ) const { return( maxValue ); }
			float			GetMinValue( void ) const { return( minValue ); }

	float					TableLookup( float index ) const;
	int						NumValues( void ) const { return values.Num(); }
	float					GetValue( int index ) const { return values[ index ]; }

private:
	bool					clamp;
	bool					snap;
	bool					discontinuous;
	bool					isLinear;
	float					minValue;
	float					maxValue;
	anList<float, TAG_LIBLIST_DECL>			values;
};

#endif