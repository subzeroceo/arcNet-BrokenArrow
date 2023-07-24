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

class arcConstantInfo {
public:
	arcNetString						name;
	arcNetString						type;
	arcNetString						value;
};

class arcEnumValueInfo {
public:
	arcNetString						name;
	int							value;
};

class arcEnumTypeInfo {
public:
	arcNetString						typeName;
	arcNetString						scope;
	bool						unnamed;
	bool						isTemplate;
	arcNetList<arcEnumValueInfo>		values;
};

class arcClassVariableInfo {
public:
	arcNetString						name;
	arcNetString						type;
	int							bits;
};

class arcClassTypeInfo {
public:
	arcNetString						typeName;
	arcNetString						superType;
	arcNetString						scope;
	bool						unnamed;
	bool						isTemplate;
	arcNetList<arcClassVariableInfo>	variables;
};

class ARCInfoGen {
public:
								ARCInfoGen( void );
								~ARCInfoGen( void );

	void						AddDefine( const char *define );
	void						CreateTypeInfo( const char *path );
	void						WriteTypeInfo( const char *fileName ) const;

private:
	arcStringList				defines;

	arcNetList<arcConstantInfo *>	constants;
	arcNetList<arcEnumTypeInfo *>	enums;
	arcNetList<arcClassTypeInfo *>	classes;

	int							numTemplates;
	int							maxInheritance;
	arcNetString					maxInheritanceClass;

	int							GetInheritance( const char *typeName ) const;
	int							EvaluateIntegerString( const arcNetString &string );
	float						EvaluateFloatString( const arcNetString &string );
	arcConstantInfo *			FindConstant( const char *name );
	int							GetIntegerConstant( const char *scope, const char *name, ARCParser &src );
	float						GetFloatConstant( const char *scope, const char *name, ARCParser &src );
	int							ParseArraySize( const char *scope, ARCParser &src );
	void						ParseConstantValue( const char *scope, ARCParser &src, arcNetString &value );
	arcEnumTypeInfo *			ParseEnumType( const char *scope, bool isTemplate, bool typeDef, ARCParser &src );
	arcClassTypeInfo *			ParseClassType( const char *scope, const char *templateArgs, bool isTemplate, bool typeDef, ARCParser &src );
	void						ParseScope( const char *scope, bool isTemplate, ARCParser &src, arcClassTypeInfo *typeInfo );
};

#endif // !__TYPEINFOGEN_H__
