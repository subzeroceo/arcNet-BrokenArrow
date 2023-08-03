#include "..//idlib/Lib.h"
#pragma hdrstop

#include "qe3.h"
#include "Radiant.h"
#include "WaitDlg.h"
#include "DialogTextures.h"
#include "DialogInfo.h"
#include "EditViewDlg.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HTREEITEM FindTreeItem(CTreeCtrl *tree, HTREEITEM root, const char *text, HTREEITEM forceParent);
extern void Select_SetKeyVal(const char *key, const char *val);

const char *CDialogTextures::TypeNames[] = {
	"None",
	"Textures",
	"Materials",
	"Models",
	"Scripts",
	"Sounds",
	"SoundParent",
	"Guis",
	"Particles",
	"Fx"
};

//
// =======================================================================================================================
//    CDialogTextures dialog
// =======================================================================================================================
//
CDialogTextures::CDialogTextures(CWnd *pParent /* =nullptr */ ) :
	CDialog(CDialogTextures::IDD, pParent) {
		setTexture  = true;
		ignoreCollapse = false;
		mode = TEXTURES;
		editMaterial = nullptr;
		editGui = "";
	//{{AFX_DATA_INIT(CDialogTextures)
	//}}AFX_DATA_INIT
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CDialogTextures::DoDataExchange(CDataExchange *pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialogTextures)
	DDX_Control(pDX, IDC_CHECK_HIDEROOT, m_chkHideRoot);
	DDX_Control(pDX, IDC_REFRESH, m_btnRefresh);
	DDX_Control(pDX, IDC_LOAD, m_btnLoad);
	DDX_Control(pDX, IDC_PREVIEW, m_wndPreview);
	DDX_Control(pDX, IDC_TREE_TEXTURES, m_treeTextures);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDialogTextures, CDialog)
//{{AFX_MSG_MAP(CDialogTextures)
	ON_BN_CLICKED(IDC_LOAD, OnLoad)
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	ON_NOTIFY(NM_CLICK, IDC_TREE_TEXTURES, OnClickTreeTextures)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_TEXTURES, OnSelchangedTreeTextures)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_TEXTURES, OnDblclkTreeTextures)
	ON_BN_CLICKED(IDC_PREVIEW, OnPreview)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CHECK_HIDEROOT, OnCheckHideroot)
	ON_COMMAND(ID_MATERIAL_EDIT, OnMaterialEdit)
	ON_COMMAND(ID_MATERIAL_INFO, OnMaterialInfo)
	//}}AFX_MSG_MAP
	ON_WM_SETFOCUS()
	ON_NOTIFY(NM_RCLICK, IDC_TREE_TEXTURES, OnNMRclickTreeTextures)
END_MESSAGE_MAP()
//
// =======================================================================================================================
//    CDialogTextures message handlers
// =======================================================================================================================
//
void CDialogTextures::OnOK() {
	//CDialog::OnOK();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL CDialogTextures::OnInitDialog() {
	CDialog::OnInitDialog();

	m_image.Create(IDB_BITMAP_MATERIAL, 16, 1, RGB(255, 255, 255) );
	m_treeTextures.SetImageList(&m_image, TVSIL_NORMAL);

	// m_wndPreview.SubclassDlgItem(IDC_PREVIEW, this);
	m_wndPreview.setDrawable(&m_testDrawable);
	BuildTree();

	return TRUE;	// return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool CDialogTextures::loadTree( HTREEITEM item, const anString &name, CWaitDlg *dlg ) {
	if ( item == nullptr ) {
		return true;
	}

	if ( m_treeTextures.ItemHasChildren( item ) ) {
		anString childName;
		HTREEITEM nextItem;
		HTREEITEM childItem = m_treeTextures.GetChildItem(item);
		while ( childItem != nullptr ) {
			nextItem = m_treeTextures.GetNextItem( childItem, TVGN_NEXT );
			childName = name + "/" + (const char *)m_treeTextures.GetItemText( childItem );

			if ( m_treeTextures.ItemHasChildren( childItem ) ) {
				if ( !loadTree( childItem, childName, dlg ) ) {
					return false;
				}
			} else {
				DWORD dw = m_treeTextures.GetItemData( childItem );
				if ( dw == TEXTURES || dw == MATERIALS ) {
					if ( dw == TEXTURES ) {
						childName = "textures/" + childName;
					}
					dlg->SetText( childName.c_str() );
					Texture_ForName( childName );
				}
			}
			if ( dlg->CancelPressed() ) {
				return false;
			}

			childItem = nextItem;
		}
	}

	return true;
}

HTREEITEM CDialogTextures::findItem(const char *name, HTREEITEM item, HTREEITEM *foundItem) {
	if (*foundItem || item == nullptr ) {
		return *foundItem;
	}
	if (m_treeTextures.ItemHasChildren(item) ) {
		HTREEITEM nextItem;
		HTREEITEM childItem = m_treeTextures.GetChildItem(item);
		while (childItem != nullptr && *foundItem == nullptr ) {
			nextItem = childItem;
			if (m_treeTextures.ItemHasChildren(nextItem) ) {
				findItem(name, nextItem, foundItem);
			} else {
				DWORD dw = m_treeTextures.GetItemData(nextItem);
				if (dw == TEXTURES) {
					const char *matName = buildItemName(nextItem, TypeNames[TEXTURES] );
					if ( !anString::Icmpn( name, "textures/", 9 ) && stricmp(name + 9, matName) == 0 ) {
						*foundItem = nextItem;
						return *foundItem;
					}
				} else if (dw == MATERIALS) {
					const char *matName = buildItemName(nextItem, TypeNames[MATERIALS] );
					if (stricmp(name, matName) == 0 ) {
						*foundItem = nextItem;
						return *foundItem;
					}
				} else if (dw == SOUNDS) {
					if (stricmp(name, m_treeTextures.GetItemText(nextItem) ) == 0 ) {
						*foundItem = nextItem;
						return *foundItem;
					}
				}
			}
			childItem = m_treeTextures.GetNextItem(childItem, TVGN_NEXT);
			//childItem = nextItem;
	   }
	}
	return *foundItem;
}

void CDialogTextures::CollapseChildren(HTREEITEM parent) {
	HTREEITEM nextItem;
	HTREEITEM childItem = m_treeTextures.GetChildItem(parent);
	while (childItem) {
		nextItem = m_treeTextures.GetNextItem(childItem, TVGN_NEXT);
		if (m_treeTextures.ItemHasChildren(childItem) ) {
			CollapseChildren(childItem);
			m_treeTextures.Expand(childItem, TVE_COLLAPSE);
		}
		childItem = nextItem;
	}
}

void CDialogTextures::SelectCurrentItem(bool collapse, const char *name, int id) {
	HTREEITEM root = m_treeTextures.GetRootItem();
	anString qt;
	if ((id == TEXTURES) || (id == MATERIALS) ) {
		HTREEITEM matItem = nullptr;
		HTREEITEM *matPtr = &matItem;
		// FIXME: This is a hack.  How should this really work?
		if (id == MATERIALS && !anString::Icmpn( name, "textures/", 9 ) ) {
			// Texture_SetTexture calls SelectCurrentItem with id == MATERIALS
			id = TEXTURES;
		}
		setTexture = false;
		if (root) {
			if (collapse && !ignoreCollapse) {
				CollapseChildren(root);
			}

			HTREEITEM *check = nullptr;
			qt = TypeNames[id];
			qt += "/";
			if (id == TEXTURES && !anString::Icmpn( name, "textures/", 9 ) ) {
				// strip off "textures/"
				qt += name + 9;
			} else {
				qt += name;
			}
			if (quickTree.Get(qt, &check) ) {
				matItem = *check;
			}
			if (matItem == nullptr ) {
				matItem = findItem(name, root, matPtr);
			}
			if (matItem) {
				m_treeTextures.SelectItem(matItem);
			}
		}
		setTexture = true;
	} else if (id == SOUNDS) {
		if (root) {
			if (collapse && !ignoreCollapse) {
				CollapseChildren(root);
			}
			HTREEITEM sel = FindTreeItem(&m_treeTextures, root, name, nullptr );
			if (sel) {
				m_treeTextures.SelectItem(sel);
			}
		}
	}
}

void CDialogTextures::OnLoad() {
	CWaitCursor cursor;
	CWaitDlg dlg;
	dlg.AllowCancel( true );
	dlg.SetWindowText( "Loading textures..." );
	Texture_HideAll();
	HTREEITEM item = m_treeTextures.GetSelectedItem();
	anString name = buildItemName( item, TypeNames[TEXTURES] );
	if ( !name.Cmpn( TypeNames[MATERIALS], strlen( TypeNames[MATERIALS] ) ) ) {
		name = buildItemName( item, TypeNames[MATERIALS] );
	}
	loadTree( item, name, &dlg );
}

const char *CDialogTextures::buildItemName(HTREEITEM item, const char *rootName) {
	itemName = m_treeTextures.GetItemText(item);

	// have to build the name back up
	HTREEITEM parent = m_treeTextures.GetParentItem(item);
	while (true) {
		anString strParent = m_treeTextures.GetItemText(parent);
		if ( anString::Icmp(strParent, rootName) == 0 ) {
			break;
		}
		strParent += "/";
		strParent += itemName;
		itemName = strParent;
		parent = m_treeTextures.GetParentItem(parent);
		if (parent == nullptr ) {
			break;
		}
	}
	return itemName;
}
/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CDialogTextures::OnRefresh() {
	quickTree.Clear();

	addModels( true );

	if (mode == TEXTURES) {
		anStringList textures(1024);
		int count = declManager->GetNumDecls( DECL_MATERIAL );
		int i;
		const anMaterial *mat;
		for ( i = 0; i < count; i++ ) {
			mat = declManager->MaterialByIndex( i, false);
			if ( mat->IsValid() && mat->TestMaterialFlag(MF_EDITOR_VISIBLE) && !anString::Icmpn( mat->GetName(), "textures/", 9 ) ) {
				textures.Append(mat->GetName() );
			}
		}

		declManager->Reload( false );

		BuildTree();
		count = textures.Num();
		for ( i = 0; i < count; i++ ) {
			mat = declManager->FindMaterial(textures[i].c_str() );
			if ( mat ) {
				mat->SetMaterialFlag(MF_EDITOR_VISIBLE);
			}
		}
		SelectCurrentItem(false, g_qeglobals.d_texturewin.texdef.name, CDialogTextures::TEXTURES);
	} else if (mode == MATERIALS) {
		anStringList textures(1024);
		int count = declManager->GetNumDecls( DECL_MATERIAL );
		int i;
		const anMaterial *mat;

		for ( i = 0; i < count; i++ ) {
			mat = declManager->MaterialByIndex( i, false);
			if ( mat->IsValid() && mat->TestMaterialFlag(MF_EDITOR_VISIBLE) && anString::Icmpn( mat->GetName(), "textures/", 9 ) ) {
				textures.Append(mat->GetName() );
			}
		}

		declManager->Reload( false );

		BuildTree();
		count = textures.Num();
		for ( i = 0; i < count; i++ ) {
			mat = declManager->FindMaterial(textures[i].c_str() );
			if ( mat ) {
				mat->SetMaterialFlag(MF_EDITOR_VISIBLE);
			}
		}
		SelectCurrentItem(false, g_qeglobals.d_texturewin.texdef.name, CDialogTextures::MATERIALS);
	} else if (mode == SOUNDS || mode == SOUNDPARENT) {
		HTREEITEM root = m_treeTextures.GetRootItem();
		HTREEITEM sib = m_treeTextures.GetNextItem(root, TVGN_ROOT);
		while (sib) {
			anString str = m_treeTextures.GetItemText(sib);
			if (str.Icmp(TypeNames[SOUNDS] ) == 0 ) {
				CWaitCursor cursor;
				m_treeTextures.DeleteItem(sib);

				declManager->Reload( false );
				bool rootItems = m_chkHideRoot.GetCheck() == 0;
				addSounds(rootItems);
				return;
			}
			sib = m_treeTextures.GetNextSiblingItem(sib);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
HTREEITEM FindTreeItem(CTreeCtrl *tree, HTREEITEM root, const char *text, HTREEITEM forceParent) {
	HTREEITEM	theItem = nullptr;
	if (root) {
		if ((theItem = tree->GetNextSiblingItem(root) ) != nullptr ) {
			theItem = FindTreeItem(tree, theItem, text, nullptr );
			if (theItem) {
				if (forceParent) {
					if (tree->GetParentItem(theItem) == forceParent) {
						return theItem;
					}
				} else {
					return theItem;
				}
			}
		}
	}

	if ((theItem = tree->GetChildItem(root) ) != nullptr ) {
		theItem = FindTreeItem(tree, theItem, text, nullptr );
		if (theItem) {
			if (forceParent) {
				if (tree->GetParentItem(theItem) == forceParent) {
					return theItem;
				}
			} else {
				return theItem;
			}
		}
	}

	if (text && anString::Icmp(tree->GetItemText(root), text) == 0 ) {
		return root;
	}

	if (theItem && forceParent) {
		if (tree->GetParentItem(theItem) != forceParent) {
			theItem = nullptr;
		}
	}
	return theItem;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CDialogTextures::BuildTree() {
	CWaitCursor cursor;
	m_treeTextures.DeleteAllItems();
	bool rootItems = m_chkHideRoot.GetCheck() == 0;

	anTimer timer;

	timer.Start();

	addMaterials( rootItems );
	// _D3XP removed
	//addModels( rootItems );
	addScripts( rootItems );
	addSounds( rootItems );
	addGuis( rootItems );
	addParticles( rootItems );

	timer.Stop();

	common->Printf( "CDialogTextures::BuildTree() took %.0f milliseconds\n", timer.Milliseconds() );
}

void CDialogTextures::OnClickTreeTextures(NMHDR *pNMHDR, LRESULT *pResult) {
	*pResult = 0;

	CPoint pt;
	GetCursorPos(&pt);
	m_treeTextures.ScreenToClient(&pt);
	HTREEITEM item = m_treeTextures.HitTest(pt);

	if (item) {
		DWORD	dw = m_treeTextures.GetItemData(item);
		mode = dw;
		if (mode == SOUNDS) {
			anString loadName;
 			if ( !m_treeTextures.ItemHasChildren(item) ) {
				loadName = m_treeTextures.GetItemText(item);
				anString actionName = m_treeTextures.GetItemText(item);
				soundSystem->SetMute( false );
				g_qeglobals.sw->PlayShaderDirectly(actionName);
			} else {
				loadName = m_treeTextures.GetItemText(item);
			}

		} else {
			g_qeglobals.sw->StopAllSounds();
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CDialogTextures::OnSelchangedTreeTextures(NMHDR *pNMHDR, LRESULT *pResult) {
	NM_TREEVIEW *pNMTreeView = (NM_TREEVIEW *) pNMHDR;
	*pResult = 0;

	editMaterial = nullptr;
	editGui = "";
	mediaName = "";
	currentFile.Empty();
	m_wndPreview.setDrawable(&m_testDrawable);
	HTREEITEM	item = m_treeTextures.GetSelectedItem();
	if (item) {
		DWORD	dw = m_treeTextures.GetItemData(item);
		mode = dw;
		if ((dw == TEXTURES) || (dw == MATERIALS) ) {
			anString matName = m_treeTextures.GetItemText(item);

			// have to build the name back up
			HTREEITEM parent = m_treeTextures.GetParentItem(item);
			while (true) {
				anString strParent = m_treeTextures.GetItemText(parent);
				if ( anString::Icmp(strParent, TypeNames[dw] ) == 0 ) {
					break;
				}
				strParent += "/";
				strParent += matName;
				matName = strParent;
				parent = m_treeTextures.GetParentItem(parent);
				if (parent == nullptr ) {
					break;
				}
			}
			if ( dw == TEXTURES ) {
				matName = "textures/" + matName;
			}

			const anMaterial *mat = Texture_ForName(matName);
			editMaterial = mat;
			m_drawMaterial.setMedia(matName);
			m_wndPreview.setDrawable(&m_drawMaterial);
			m_wndPreview.RedrawWindow();

			ignoreCollapse = true;
			Select_SetDefaultTexture(mat, false, setTexture);
			ignoreCollapse = false;
		} else if (dw == MODELS) {
			anString strParent;
			anString modelName = m_treeTextures.GetItemText(item);
			// have to build the name back up
			HTREEITEM parent = m_treeTextures.GetParentItem(item);
			while (true) {
				strParent = m_treeTextures.GetItemText(parent);
				if ( anString::Icmp(strParent, TypeNames[MODELS] ) == 0 ) {
					break;
				}
				strParent += "/";
				strParent += modelName;
				modelName = strParent;
				parent = m_treeTextures.GetParentItem(parent);
				if (parent == nullptr ) {
					break;
				}
			}
			strParent = "models/";
			strParent += modelName;
			m_drawModel.setMedia(strParent);
			mediaName = strParent;
			m_wndPreview.setDrawable(&m_drawModel);
			m_drawModel.SetRealTime(0 );
			m_wndPreview.RedrawWindow();
		} else if (dw == SCRIPTS) {
		} else if (dw == SOUNDS) {
		} else if (dw == PARTICLES) {
			anString strParent;
			anString modelName = m_treeTextures.GetItemText(item);
			// have to build the name back up
			HTREEITEM parent = m_treeTextures.GetParentItem(item);
			while (true) {
				strParent = m_treeTextures.GetItemText(parent);
				if ( anString::Icmp(strParent, TypeNames[PARTICLES] ) == 0 ) {
					break;
				}
				strParent += "/";
				strParent += modelName;
				modelName = strParent;
				parent = m_treeTextures.GetParentItem(parent);
				if (parent == nullptr ) {
					break;
				}
			}
			strParent = modelName;
			mediaName = strParent;
			mediaName += ".prt";
			m_drawModel.setMedia(mediaName);
			m_drawModel.SetRealTime(50);
			m_wndPreview.setDrawable(&m_drawModel);
			m_wndPreview.RedrawWindow();
		} else if (dw == GUIS) {
			anString strParent;
			anString modelName = m_treeTextures.GetItemText(item);
			// have to build the name back up
			HTREEITEM parent = m_treeTextures.GetParentItem(item);
			while (true) {
				strParent = m_treeTextures.GetItemText(parent);
				if ( anString::Icmp(strParent, TypeNames[GUIS] ) == 0 ) {
					break;
				}
				strParent += "/";
				strParent += modelName;
				modelName = strParent;
				parent = m_treeTextures.GetParentItem(parent);
				if (parent == nullptr ) {
					break;
				}
			}
			strParent = "guis/";
			strParent += modelName;
			const anMaterial *mat = declManager->FindMaterial( "guiSurfaces/guipreview" );
			mat->SetGui(strParent);
			editGui = strParent;
			m_drawMaterial.setMedia( "guiSurfaces/guipreview" );
			m_drawMaterial.setScale(4.4f);
			m_wndPreview.setDrawable(&m_drawMaterial);
			m_wndPreview.RedrawWindow();
		}
	}

}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CDialogTextures::addMaterials( bool rootItems ) {
	anStringList textures(1024);
	anStringList materials(1024);

	textures.SetGranularity( 1024 );
	materials.SetGranularity( 1024 );

	int count = declManager->GetNumDecls( DECL_MATERIAL );
	if ( count > 0 ) {
		for ( int i = 0; i < count; i++ ) {
			const anMaterial *mat = declManager->MaterialByIndex( i, false );
			if ( !rootItems ) {
				if ( strchr( mat->GetName(), '/' ) == nullptr && strchr( mat->GetName(), '\\' ) == nullptr ) {
					continue;
				}
			}
			// add everything except the textures/ materials
			if ( anString::Icmpn( mat->GetName(), "textures/", 9 ) == 0 ) {
				textures.Append( mat->GetName() );
			} else {
				materials.Append( mat->GetName() );
			}
		}
		anStringListSortPath( textures );
		addStrList( TypeNames[TEXTURES], textures, TEXTURES );
		anStringListSortPath( materials );
		addStrList( TypeNames[MATERIALS], materials, MATERIALS );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CDialogTextures::addParticles( bool rootItems ) {
	anStringList list(1024);
	int count = declManager->GetNumDecls( DECL_PARTICLE );
	if (count > 0 ) {
		for ( int i = 0; i < count; i++ ) {
			const anDecl *ips = declManager->DeclByIndex( DECL_PARTICLE, i, false );
			if ( !rootItems) {
				if (strchr(ips->GetName(), '/') == nullptr && strchr(ips->GetName(), '\\') == nullptr ) {
					continue;
				}
			}
			list.Append(ips->GetName() );
		}
		anStringListSortPath( list );
		addStrList( TypeNames[PARTICLES], list, PARTICLES );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CDialogTextures::addSounds(bool rootItems) {
	int i, j;
	anStringList list(1024);
	anStringList list2(1024);
	HTREEITEM base = m_treeTextures.InsertItem(TypeNames[SOUNDS] );

	for ( i = 0; i < declManager->GetNumDecls( DECL_SOUND ); i++ ) {
		const anSoundShader *poo = declManager->SoundByIndex( i, false);
		list.AddUnique( poo->GetFileName() );
	}
	anStringListSortPath( list );

	for ( i = 0; i < list.Num(); i++ ) {
		HTREEITEM child = m_treeTextures.InsertItem(list[i], base);
		m_treeTextures.SetItemData(child, SOUNDPARENT);
		m_treeTextures.SetItemImage(child, 0, 1 );
		list2.Clear();
		for ( j = 0; j < declManager->GetNumDecls( DECL_SOUND ); j++ ) {
			const anSoundShader *poo = declManager->SoundByIndex(j, false);
			if ( anString::Icmp( list[i], poo->GetFileName() ) == 0 ) {
				list2.Append( poo->GetName() );
			}
		}
		anStringListSortPath( list2 );
		for ( j = 0; j < list2.Num(); j++ ) {
			HTREEITEM child2 = m_treeTextures.InsertItem( list2[j], child );
			m_treeTextures.SetItemData(child2, SOUNDS);
			m_treeTextures.SetItemImage(child2, 2, 2);
		}
	}

}

void CDialogTextures::addStrList( const char *root, const anStringList &list, int id ) {
	anString		out, path;

	HTREEITEM base = m_treeTextures.GetRootItem();
	while (base) {
		out = m_treeTextures.GetItemText(base);
		if (stricmp(root, out) == 0 ) {
			break;
		}
		base = m_treeTextures.GetNextSiblingItem(base);
	}

	if (base == nullptr ) {
		base = m_treeTextures.InsertItem(root);
	}

	HTREEITEM	item = base;
	HTREEITEM	add;

	int		count = list.Num();

	anString	last, qt;
	for ( int i = 0; i < count; i++ ) {
		anString name = list[i];

		// now break the name down convert to slashes
		name.BackSlashesToSlashes();
		name.Strip(' ');

		int index;
		int len = last.Length();
		if (len == 0 ) {
			index = name.Last('/');
			if (index >= 0 ) {
				name.Left(index, last);
			}
		} else if (anString::Icmpn(last, name, len) == 0 && name.Last('/') <= len) {
			name.Right(name.Length() - len - 1, out);
			add = m_treeTextures.InsertItem(out, item);
			qt = root;
			qt += "/";
			qt += name;
			quickTree.Set(qt, add);
			m_treeTextures.SetItemData(add, id);
			m_treeTextures.SetItemImage(add, 2, 2);
			continue;
		} else {
			last.Empty();
		}

		index = 0;
		item = base;
		path = "";
		while (index >= 0 ) {
			index = name.Find('/');
			if (index >= 0 ) {
				HTREEITEM newItem = nullptr;
				HTREEITEM *check = nullptr;
				name.Left( index, out );
				path += out;
				qt = root;
				qt += "/";
				qt += path;
				if (quickTree.Get(qt, &check) ) {
					newItem = *check;
				}
				//HTREEITEM	newItem = FindTreeItem(&m_treeTextures, item, name.Left(index, out), item);
				if (newItem == nullptr ) {
					newItem = m_treeTextures.InsertItem(out, item);
					qt = root;
					qt += "/";
					qt += path;
					quickTree.Set(qt, newItem);
					m_treeTextures.SetItemImage(newItem, 0, 1 );
				}

				assert(newItem);
				item = newItem;
				name.Right( name.Length() - index - 1, out );
				name = out;
				path += "/";
			} else {
				add = m_treeTextures.InsertItem(name, item);
				qt = root;
				qt += "/";
				qt += path;
				qt += name;
				quickTree.Set(qt, add);
				m_treeTextures.SetItemData(add, id);
				m_treeTextures.SetItemImage(add, 2, 2);
				path = "";
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CDialogTextures::addModels(bool rootItems) {
	anFileList *files = fileSystem->ListFilesTree( "models", ".ase|.lwo|.ma", true );

	if ( files->GetNumFiles() ) {
		addStrList( TypeNames[MODELS], files->GetList(), MODELS );
	}

	fileSystem->FreeFileList( files );
}

void CDialogTextures::addGuis( bool rootItems ) {
	anFileList *files = fileSystem->ListFilesTree( "guis", ".gui", true );

	if ( files->GetNumFiles() ) {
		addStrList( TypeNames[GUIS], files->GetList(), GUIS );
	}

	fileSystem->FreeFileList( files );
}

void CDialogTextures::addScripts(bool rootItems) {
/*	anFileList *files = fileSystem->ListFilesExt( "def", ".script" );

	if ( files->GetNumFiles() ) {
		addStrList( "Scripts", files->GetList(), 3);
	}*/
}

void CDialogTextures::OnDblclkTreeTextures(NMHDR *pNMHDR, LRESULT *pResult) {
	CPoint pt;
	GetCursorPos(&pt);
	m_treeTextures.ScreenToClient(&pt);
	HTREEITEM item = m_treeTextures.HitTest(pt);
	if (item) {
		DWORD dw = m_treeTextures.GetItemData(item);
		mode = dw;
		if (mode == SOUNDS) {
 			if ( !m_treeTextures.ItemHasChildren(item) ) {
				anString shaderName = m_treeTextures.GetItemText(item);
				Select_SetKeyVal( "s_shader", shaderName);
				entity_t *ent = selected_brushes.next->owner;
				if (ent) {
					g_Inspectors->UpdateEntitySel(ent->eclass);
					MessageBeep(MB_OK);
				}
			}
		} else if (mode == MODELS || mode == PARTICLES ) {
			if (mediaName.Length() ) {
				g_Inspectors->entityDlg.UpdateKeyVal( "model", mediaName);
			}
		} else if (mode <= MATERIALS) {
			OnLoad();
		}
	}

	*pResult = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CDialogTextures::OnPreview() {
	// TODO: Add your control notification handler code here
}

//void CDialogTextures::OnSave()
//{
/*
	CString str;
	m_wndEditShader.GetWindowText(str);
	if (currentFile.length() && str.GetLength() ) {
		fileSystem->WriteFile(currentFile, str.GetBuffer(0 ), str.GetLength() );
	}
*/
//}

int CDialogTextures::OnCreate(LPCREATESTRUCT lpCreateStruct) {
	if (CDialog::OnCreate(lpCreateStruct) == -1 )
		return -1;

	// TODO: Add your specialized creation code here

	return 0;
}

void CDialogTextures::OnSize(UINT nType, int cx, int cy) {
	CDialog::OnSize(nType, cx, cy);

	if (m_btnLoad.GetSafeHwnd() == nullptr ) {
		return;
	}

	CRect rect, rect2, rect3;
	GetClientRect(rect);
	m_btnLoad.GetWindowRect(rect2);

	m_btnLoad.SetWindowPos(nullptr, rect.left + 4, rect.top + 4, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	m_btnRefresh.SetWindowPos(nullptr, rect.left + rect2.Width() + 4, rect.top + 4, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);


	int right = rect.right - 4 - rect3.Width() - 4;


	right = rect3.right - 4 - rect3.Width() - 4;

	m_chkHideRoot.GetWindowRect(rect3);
	m_chkHideRoot.SetWindowPos(nullptr, right - rect3.Width() * 2, rect.top + 4, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	m_chkHideRoot.ShowWindow(SW_HIDE);

	int verticalSpace = (rect.Height() - rect2.Height() - 12) / 2;

	m_treeTextures.SetWindowPos(nullptr, rect.left + 4, rect.top + 8 + rect2.Height(), (rect.Width() - 8), verticalSpace, SWP_SHOWWINDOW);
	m_wndPreview.SetWindowPos(nullptr, rect.left + 4, rect.top + 12 + rect2.Height() + verticalSpace, (rect.Width() - 8), verticalSpace, SWP_SHOWWINDOW);

	RedrawWindow();
}

BOOL CDialogTextures::PreCreateWindow(CREATESTRUCT& cs) {
	return CDialog::PreCreateWindow(cs);
}

void CDialogTextures::OnCheckHideroot() {
	BuildTree();
}

void CDialogTextures::CollapseEditor() {
	if (g_qeglobals.d_savedinfo.editorExpanded) {
	}
}


void CDialogTextures::OnCancel() {
}


BOOL CDialogTextures::PreTranslateMessage(MSG* pMsg) {
	if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_RETURN) ) {
		if (pMsg->wParam == VK_ESCAPE) {
			g_pParentWnd->GetCamera()->SetFocus();
			Select_Deselect();
		}
		return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CDialogTextures::OnSetFocus(CWnd* pOldWnd) {
	CDialog::OnSetFocus(pOldWnd);
	RedrawWindow();
}

void CDialogTextures::OnNMRclickTreeTextures(NMHDR *pNMHDR, LRESULT *pResult) {
	*pResult = 0;

	CPoint pt;
	GetCursorPos(&pt);
	m_treeTextures.ScreenToClient(&pt);
	HTREEITEM item = m_treeTextures.HitTest(pt);

	if (item) {
		DWORD	dw = m_treeTextures.GetItemData(item);
		mode = dw;
		if (mode == TEXTURES || mode == MATERIALS || mode == GUIS) {
			m_treeTextures.SelectItem(item);
			HandlePopup(this, IDR_POPUP_MATERIAL);
		}
	}
}

void CDialogTextures::OnMaterialEdit() {
	CEditViewDlg dlg;
	if ((mode == TEXTURES || mode == MATERIALS) && editMaterial) {
		dlg.SetMode(CEditViewDlg::MATERIALS);
		dlg.SetMaterialInfo(editMaterial->GetName(), editMaterial->GetFileName(), editMaterial->GetLineNum() );
		dlg.DoModal();
	} else if (mode == GUIS && editGui.Length() ) {
		dlg.SetMode(CEditViewDlg::GUIS);
		dlg.SetGuiInfo(editGui);
		dlg.DoModal();
	}
}

void CDialogTextures::OnMaterialInfo() {
/*	anString str;
	if (editMaterial) {
		str = "File: ";
		str += editMaterial->getFileName();
		str += "\r\nName: ";
		str = editMaterial->getName();
		ShowInfoDialog(str);
	} else if (editGui.Length() ) {
		str = "File: ";
		str += editGui;
		ShowInfoDialog(str);
	}*/
}
