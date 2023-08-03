#ifndef __EDITFIELD_H__
#define __EDITFIELD_H__

/*
===============================================================================

	Edit field

===============================================================================
*/

const int MAX_EDIT_LINE = 256;

typedef struct autoComplete_s {
	bool			valid;
	int				length;
	char			completionString[MAX_EDIT_LINE];
	char			currentMatch[MAX_EDIT_LINE];
	int				matchCount;
	int				matchIndex;
	int				findMatchIndex;
} autoComplete_t;

class arcEditField {
public:
					arcEditField();
					~arcEditField();

	void			Clear();
	void			SetWidthInChars( int w );
	void			SetCursor( int c );
	int				GetCursor() const;
	void			ClearAutoComplete();
	int				GetAutoCompleteLength() const;
	void			AutoComplete();
	void			CharEvent( int c );
	void			KeyDownEvent( int key );
	void			Paste();
	char *			GetBuffer();
	void			Draw( int x, int y, int width, bool showCursor );
	void			SetBuffer( const char *buffer );

private:
	int				cursor;
	int				scroll;
	int				widthInChars;
	char			buffer[MAX_EDIT_LINE];
	autoComplete_t	autoComplete;
};

class arcTextDimension {
public:
					arcTextDimension( void );
					~arcTextDimension( void );

	void			Init( const wchar_t *text, const int textLength, const an2DBounds &rect, unsigned int flags, const qhandle_t font, const int pointSize, anList<int> *lineBreaks = nullptr );

	int				GetAdvance( const int index ) const;
	float			GetWidth( const int startIndex, const int endIndex ) const;

	int				GetTextWidth() const { return width; }
	int				GetTextHeight() const { return height; }
	int				GetLineHeight() const { return lineHeight; }

	int				ToVirtualScreenSize( const int size ) const;
	float			ToVirtualScreenSizeFloat( const int size ) const;

private:
	static const int	BASE_BUFFER = 256;
	float				scale;
	int					width;
	int					height;
	int					lineHeight;
	int					*advances;
	int					advancesBase[ BASE_BUFFER ];
	int					textLength;
};

/*
============
arcTextDimension::arcTextDimension
============
*/
ARC_INLINE arcTextDimension::arcTextDimension( void ) :
	scale( 1.0f ),
	width( 0 ),
	height( 0 ),
	lineHeight( 0 ),
	advances( &advancesBase[0] ),
	textLength( 0 ) {
}

/*
============
arcTextDimension::~arcTextDimension
============
*/
ARC_INLINE arcTextDimension::~arcTextDimension( void ) {
	if ( advances != &advancesBase[0] ) {
		Mem_Free( advances );
	}
}

/*
============
arcTextDimension::Init
============
*/
ARC_INLINE void arcTextDimension::Init( const wchar_t *text, const int textLength, const an2DBounds &rect, unsigned int flags, const qhandle_t font, const int pointSize, anList<int> *lineBreaks ) {
	if ( lineBreaks != nullptr ) {
		lineBreaks->SetNum( 0, false );
	}

	if ( textLength == 0 ) {
		memset( advances, 0, this->textLength * sizeof( int ) );
		this->textLength = 0;
		return;
	}

	if ( advances != nullptr && textLength > this->textLength && advances != &advancesBase[0] ) {
		Mem_Free( advances );
		advances = &advancesBase[0];
	}

	if ( textLength > BASE_BUFFER ) {
		advances = static_cast<int *>( Mem_Alloc( textLength * sizeof( int ) ) );
	}

	deviceContext->GetTextDimensions( text, rect, flags, font, pointSize, width, height, &scale, &advances, lineBreaks );

	int numLines = ( lineBreaks != nullptr ) ? lineBreaks->Num() : 0;

	// a trailing empty line isn't included in the total drawn height
	if ( text[ textLength - 1 ] == L'\n' ) {
		numLines--;
	}

	lineHeight = anMath::Ftoi( anMath::Ceil( static_cast<float>( height ) / ( numLines + 1 ) ) );
	this->textLength = textLength;
}

/*
============
arcTextDimension::GetAdvance
============
*/
ARC_INLINE int arcTextDimension::GetAdvance( const int index ) const {
	if ( advances == nullptr ) {
		return 0;
	}

	return advances[index];
}

/*
============
arcTextDimensionHelper::GetWidth
============
*/
ARC_INLINE float arcTextDimension::GetWidth( const int startIndex, const int endIndex ) const {
	if ( advances == nullptr || textLength == 0 ) {
		return 0.0f;
	}

	int width = 0;
	for ( int i = startIndex; i <= endIndex && i < textLength ; i++ ) {
		width += advances[i];
	}
	return ToVirtualScreenSize( width );
}

/*
============
arcTextDimension::ToVirtualScreenSize
============
*/
ARC_INLINE int arcTextDimension::ToVirtualScreenSize( const int size ) const {
	return anMath::Ftoi( anMath::Ceil( ( size >> 6 ) / scale ) );
}

/*
============
arcTextDimension::ToVirtualScreenSizeFloat
============
*/
ARC_INLINE float arcTextDimension::ToVirtualScreenSizeFloat( const int size ) const {
	return ( size >> 6 ) / scale;
}

#endif // __EDITFIELD_H__