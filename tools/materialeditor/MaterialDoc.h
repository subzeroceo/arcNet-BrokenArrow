#pragma once

#include "MaterialEditor.h"
#include "MaterialModifier.h"
#include "MaterialDef.h"

/**
* Dictionary representation of a Material Stage.
*/
typedef struct {
	arcDictionary				stageData;
	bool				enabled;
} MEStage_t;

/**
* Dictionary representation of a material.
*/
typedef struct {
	arcDictionary				materialData;
	arcNetList<MEStage_t*>	stages;
} MEMaterial_t;

/**
* Implemented by the edit window that is responsible for modifying the material source text.
*/
class SourceModifyOwner {

public:
	SourceModifyOwner() {};
	virtual ~SourceModifyOwner() {};

	virtual arcNetString GetSourceText() { return ""; };
};

class MaterialDocManager;

/**
* Responsible for managing a single material that is being viewed and/or edited.
*/
class MaterialDoc {

public:
	MaterialDocManager*		manager;
	arcNetString					name;
	arcMaterial*				renderMaterial;
	MEMaterial_t			editMaterial;

	bool					modified;
	bool					applyWaiting;
	bool					deleted;

	bool					sourceModify;
	SourceModifyOwner*		sourceModifyOwner;

public:
	MaterialDoc( void );
	~MaterialDoc( void );

	/**
	* Define the types of stages in a material.
	*/
	enum {
		STAGE_TYPE_NORMAL,
		STAGE_TYPE_SPECIALMAP
	};

	//Initialization Methods
	void			SetRenderMaterial(arcMaterial* material, bool parseMaterial = true, bool parseRenderMatierial = false);

	//Stage Info Methods
	int				GetStageCount();
	int				FindStage( int stageType, const char* name);
	MEStage_t		GetStage( int stage);
	void			EnableStage( int stage, bool enabled);
	void			EnableAllStages(bool enabled);
	bool			IsStageEnabled( int stage);

	//Get Attributes
	const char*		GetAttribute( int stage, const char* attribName, const char* defaultString = "" );
	int				GetAttributeInt( int stage, const char* attribName, const char* defaultString = "0" );
	float			GetAttributeFloat( int stage, const char* attribName, const char* defaultString = "0" );
	bool			GetAttributeBool( int stage, const char* attribName, const char* defaultString = "0" );

	//Set Attribute Methods
	void			SetAttribute( int stage, const char* attribName, const char* value, bool addUndo = true);
	void			SetAttributeInt( int stage, const char* attribName, int value, bool addUndo = true);
	void			SetAttributeFloat( int stage, const char* attribName, float value, bool addUndo = true);
	void			SetAttributeBool( int stage, const char* attribName, bool value, bool addUndo = true);
	void			SetMaterialName(const char* materialName, bool addUndo = true);
	void			SetData( int stage, arcDictionary* data);

	//Source Editing Methods
	void			SourceModify(SourceModifyOwner* owner);
	bool			IsSourceModified();
	void			ApplySourceModify(arcNetString& text);
	const char*		GetEditSourceText();

	//Stage Modification Methods
	void			AddStage( int stageType, const char* stageName, bool addUndo = true);
	void			InsertStage( int stage, int stageType, const char* stageName, bool addUndo = true);
	void			RemoveStage( int stage, bool addUndo = true);
	void			ClearStages();
	void			MoveStage( int from, int to, bool addUndo = true);

	void			ApplyMaterialChanges(bool force = false);
	void			Save();
	void			Delete();

protected:

	//Internal Notifications
	void			OnMaterialChanged();

	//Load Material Methods
	void			ParseMaterialText(const char* source);
	void			ParseMaterial(arcLexer* src);
	void			ParseStage(arcLexer* src);
	void			AddSpecialMapStage(const char* stageName, const char* map);
	bool			ParseMaterialDef(arcNetToken* token, arcLexer* src, int type, arcDictionary* dict);
	void			ClearEditMaterial();

	//Save/Apply Material Methods
	const char*		GenerateSourceText();
	void			ReplaceSourceText();
	void			WriteStage( int stage, aRcFileMemory* file);
	void			WriteSpecialMapStage( int stage, aRcFileMemory* file);
	void			WriteMaterialDef( int stage, aRcFileMemory* file, int type, int indent);
};

