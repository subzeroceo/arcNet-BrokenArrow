#ifndef __LANGDICT_H__
#define __LANGDICT_H__

/*
===============================================================================

	Simple dictionary specifically for the localized string tables.

===============================================================================
*/

class idLangKeyValue {
public:
	anString					key;
	anString					value;
};

class arcLangDictionary {
public:
							arcLangDictionary( void );
							~arcLangDictionary( void );

	void					Clear( void );
	bool					Load( const char *fileName, bool clear = true );
	void					Save( const char *fileName );

	const char *			AddString( const char *str );
	const char *			GetString( const char *str ) const;

							// adds the value and key as passed (doesn't generate a "#str_xxxxx" key or ensure the key/value pair is unique)
	void					AddKeyVal( const char *key, const char *val );

	int						GetNumKeyVals( void ) const;
	const idLangKeyValue *	GetKeyVal( int i ) const;

	void					SetBaseID( int id) { baseID = id; };

private:
	anList<idLangKeyValue>	args;
	anHashIndex				hash;

	bool					ExcludeString( const char *str ) const;
	int						GetNextId( void ) const;
	int						GetHashKey( const char *str ) const;

	int						baseID;
};

#endif /* !__LANGDICT_H__ */
