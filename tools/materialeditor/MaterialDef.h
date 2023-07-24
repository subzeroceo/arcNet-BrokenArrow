#pragma once

/**
* Represents a single attribute in a material. Represents material, stage
* and special stage attributes. The MaterialDef manager loads these
* definitions from the material definition file as the editor
* is being initialized.
*/
class MaterialDef {

public:
	/**
	* Defines possible attribute types.
	*/
	enum {
		MATERIAL_DEF_TYPE_GROUP,
		MATERIAL_DEF_TYPE_BOOL,
		MATERIAL_DEF_TYPE_STRING,
		MATERIAL_DEF_TYPE_FLOAT,
		MATERIAL_DEF_TYPE_INT
	};

	int					type;
	arcNetString				dictName;
	arcNetString				displayName;
	arcNetString				displayInfo;
	bool				quotes;
	arcHashTable<DWORD>	viewData;

public:

	MaterialDef( void );
	virtual ~MaterialDef( void );

	DWORD	GetViewData(const char* viewName);
	void	SetViewData(const char* viewName, DWORD value);
};

/**
* A list of material attributes. Material, stage, and special stage attributes
* are grouped together during the load process for use by the different view and
* MaterialDoc.
*/
typedef arcNetList<MaterialDef*> MaterialDefList;

/**
* This class contains static utility functions that view and MaterialDoc use
* to access the MaterialDef and MaterialDefList data that is loaded. This class
* is also responsible for loading and destroying the MaterialDef instances.
*/
class MaterialDefManager {

public:

	/**
	* Defines the groupings of material attributes.
	*/
	enum {
		MATERIAL_DEF_MATERIAL = 0,
		MATERIAL_DEF_STAGE,
		MATERIAL_DEF_SPECIAL_STAGE,
		MATERIAL_DEF_NUM
	};

	static void 				InitializeMaterialDefLists();
	static void					InitializeMaterialDefList(arcLexer* src, const char* typeName, MaterialDefList* list);

	static void					DestroyMaterialDefLists();

	static MaterialDefList*		GetMaterialDefs( int type);


protected:
	static MaterialDefList		materialDefs[MATERIAL_DEF_NUM];
};
