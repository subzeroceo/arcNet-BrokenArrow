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
	anStr					name;
	anStr					type;
	anStr					value;
};

class anEnumValueInfo {
public:
	anStr					name;
	int							value;
};

class anEnumTypeInfo {
public:
	anStr					typeName;
	anStr					scope;
	bool						unnamed;
	bool						isTemplate;
	anList<anEnumValueInfo>		values;
};

class anClassVariableInfo {
public:
	anStr					name;
	anStr					type;
	int							bits;
};

class anClassTypeInfo {
public:
	anStr					typeName;
	anStr					superType;
	anStr					scope;
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
	anStr					maxInheritanceClass;

	int							GetInheritance( const char *typeName ) const;
	int							EvaluateIntegerString( const anStr &string );
	float						EvaluateFloatString( const anStr &string );
	anConstantInfo *			FindConstant( const char *name );
	int							GetIntegerConstant( const char *scope, const char *name, anParser &src );
	float						GetFloatConstant( const char *scope, const char *name, anParser &src );
	int							ParseArraySize( const char *scope, anParser &src );
	void						ParseConstantValue( const char *scope, anParser &src, anStr &value );
	anEnumTypeInfo *			ParseEnumType( const char *scope, bool isTemplate, bool typeDef, anParser &src );
	anClassTypeInfo *			ParseClassType( const char *scope, const char *templateArgs, bool isTemplate, bool typeDef, anParser &src );
	void						ParseScope( const char *scope, bool isTemplate, anParser &src, anClassTypeInfo *typeInfo );
};

#endif // !__TYPEINFOGEN_H__
