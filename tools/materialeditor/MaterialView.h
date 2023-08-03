#pragma once
#include "MaterialDocManager.h"

/**
* MaterialView Interface. Interface to be implemented by classes that want
* notifications of material changes. Classes that implement this interface
* must register themself with the MaterialDocManager class with the
* RegisterView method.
*/
class MaterialView {
public:
	/**
	* Constructor.
	*/
	MaterialView( void ) { materialDocManager = nullptr; };
	// Destructor.
	virtual ~MaterialView( void ) {};

	//////////////////////////////////////////////////////////////////////////
	//Public Interface to be implemented by subclasses
	//////////////////////////////////////////////////////////////////////////

	/**
	* Sets the material document manager for this view instance.
	* @param docManager The material document manager for this view instance.
	*/
	virtual void	SetMaterialDocManager(MaterialDocManager* docManager) { materialDocManager = docManager; };

	/**
	* Called when the selected material has changed.
	* @param pMaterial The newly selected material.
	*/
	virtual void	MV_OnMaterialSelectionChange(MaterialDoc* pMaterial) {};

	/**
	* Called when the material has changed but not applied.
	* @param pMaterial The selected material.
	*/
	virtual void	MV_OnMaterialChange(MaterialDoc* pMaterial) {};

	/**
	* Called when the material changes have been applied.
	* @param pMaterial The selected material.
	*/
	virtual void	MV_OnMaterialApply(MaterialDoc* pMaterial) {};

	/**
	* Called when the material changes have been saved.
	* @param pMaterial The saved material.
	*/
	virtual void	MV_OnMaterialSaved(MaterialDoc* pMaterial) {};

	/**
	* Called when a material file has been saved
	* @param filename path of the file that was saved.
	*/
	virtual void	MV_OnMaterialSaveFile(const char* filename) {};

	/**
	* Called when a material is added
	* @param pMaterial The material that was added.
	*/
	virtual void	MV_OnMaterialAdd(MaterialDoc* pMaterial) {};

	/**
	* Called when a material is deleted
	* @param pMaterial The material that was deleted.
	*/
	virtual void	MV_OnMaterialDelete(MaterialDoc* pMaterial) {};

	/**
	* Called when a stage is added
	* @param pMaterial The material that was affected.
	* @param stageNum The index of the stage that was added
	*/
	virtual void	MV_OnMaterialStageAdd(MaterialDoc* pMaterial, int stageNum) {};

	/**
	* Called when a stage is deleted
	* @param pMaterial The material that was affected.
	* @param stageNum The index of the stage that was deleted
	*/
	virtual void	MV_OnMaterialStageDelete(MaterialDoc* pMaterial, int stageNum) {};

	/**
	* Called when a stage is moved
	* @param pMaterial The material that was deleted.
	* @param from The from index
	* @param to The to index
	*/
	virtual void	MV_OnMaterialStageMove(MaterialDoc* pMaterial, int from, int to) {};

	/**
	* Called when an attribute is changed
	* @param pMaterial The material that was deleted.
	* @param stage The stage that contains the change.
	* @param attribName The attribute that has changed.
	*/
	virtual void	MV_OnMaterialAttributeChanged(MaterialDoc* pMaterial, int stage, const char* attribName) {};


	/**
	* Called when the material name has changed
	* @param pMaterial The material that was deleted.
	* @param oldName The old name of the material.
	*/
	virtual void	MV_OnMaterialNameChanged(MaterialDoc* pMaterial, const char* oldName) {};

	/**
	* Called when a file has been reloaded
	* @param filename The file that was reloaded.
	*/
	virtual void	MV_OnFileReload(const char* filename) {};


protected:
	MaterialDocManager* materialDocManager;
};
