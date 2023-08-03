#ifndef __TYPEINFOGEN_H__
#define __TYPEINFOGEN_H__

/*
===================================================================================

	Type Info Generator

	- template classes are commented out (different instantiations are not identified)
	- bit fields are commented out (cannot get the address of bit fields)
	- multiple inheritance is not supported (only tracks a single super type)

===================================================================================
*/

class anConstantInfo {
public:
	anString					name;
	anString					type;
	anString					value;
};

class anEnumValueInfo {
public:
	anString					name;
	int							value;
};

class anEnumTypeInfo {
public:
	anString					typeName;
	anString					scope;
	bool						unnamed;
	bool						isTemplate;
	anList<anEnumValueInfo>		values;
};

class anClassVariableInfo {
public:
	anString					name;
	anString					type;
	int							bits;
};

class anClassTypeInfo {
public:
	anString					typeName;
	anString					superType;
	anString					scope;
	bool						unnamed;
	bool						isTemplate;
	anList<anClassVariableInfo>	variables;
};

class anInfoGen {
public:
								anInfoGen( void );
								~anInfoGen( void );

	void						AddDefine( const char *define );
	void						CreateTypeInfo( const char *path );
	void						WriteTypeInfo( const char *fileName ) const;

private:
	anStringList				defines;

	anList<anConstantInfo *>	constants;
	anList<anEnumTypeInfo *>	enums;
	anList<anClassTypeInfo *>	classes;

	int							numTemplates;
	int							maxInheritance;
	anString					maxInheritanceClass;

	int							GetInheritance( const char *typeName ) const;
	int							EvaluateIntegerString( const anString &string );
	float						EvaluateFloatString( const anString &string );
	anConstantInfo *			FindConstant( const char *name );
	int							GetIntegerConstant( const char *scope, const char *name, anParser &src );
	float						GetFloatConstant( const char *scope, const char *name, anParser &src );
	int							ParseArraySize( const char *scope, anParser &src );
	void						ParseConstantValue( const char *scope, anParser &src, anString &value );
	anEnumTypeInfo *			ParseEnumType( const char *scope, bool isTemplate, bool typeDef, anParser &src );
	anClassTypeInfo *			ParseClassType( const char *scope, const char *templateArgs, bool isTemplate, bool typeDef, anParser &src );
	void						ParseScope( const char *scope, bool isTemplate, anParser &src, anClassTypeInfo *typeInfo );
};

#endif // !__TYPEINFOGEN_H__
