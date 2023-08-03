#include "..//idlib/Lib.h"
#pragma hdrstop

#include "../../game/game.h"
#include "../../sys/win32/win_local.h"
#include "../../sys/win32/rc/common_resource.h"
#include "../../sys/win32/rc/SoundEditor_resource.h"
#include "../comafx/DialogName.h"
#include "../../sys/win32/rc/DeclEditor_resource.h"
#include "../decl/DialogDeclEditor.h"

#include "DialogSound.h"
#include "DialogSoundGroup.h"

#ifdef ID_DEBUG_MEMORY
#undef new
#undef DEBUG_NEW
#define DEBUG_NEW new
#endif

extern HTREEITEM FindTreeItem(CTreeCtrl *tree, HTREEITEM root, const char *text, HTREEITEM forceParent);


/////////////////////////////////////////////////////////////////////////////
// CDialogSound dialog
CDialogSound *g_SoundDialog = nullptr;

CDialogSound::CDialogSound(CWnd* pParent /*=nullptr*/)
	: CDialog(CDialogSound::IDD, pParent) {
	//{{AFX_DATA_INIT(CDialogSound)
	strName = _T( "" );
	fVolume = 0.0f;
	fMax = 0.0f;
	fMin = 0.0f;
	strShader = _T( "" );
	bPlay = TRUE;
	bTriggered = FALSE;
	bOmni = FALSE;
	strGroup = _T( "" );
	bGroupOnly = FALSE;
	bOcclusion = FALSE;
	leadThrough = 0.0f;
	plain = FALSE;
	inUseTree = nullptr;
	random = 0.0f;
	wait = 0.0f;
	shakes = 0.0f;
	looping = TRUE;
	unclamped = FALSE;
	//}}AFX_DATA_INIT
}

void CDialogSound::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialogSound)
	DDX_Control(pDX, IDC_COMBO_SPEAKERS, comboSpeakers);
	DDX_Control(pDX, IDC_COMBO_GROUPS, comboGroups);
	DDX_Control(pDX, IDC_EDIT_VOLUME, editVolume);
	DDX_Control(pDX, IDC_TREE_SOUNDS, treeSounds);
	DDX_Text(pDX, IDC_EDIT_SOUND_NAME, strName);
	DDX_Text(pDX, IDC_EDIT_VOLUME, fVolume);
	DDX_Text(pDX, IDC_EDIT_RANDOM, random);
	DDX_Text(pDX, IDC_EDIT_WAIT, wait);
	DDX_Text(pDX, IDC_EDIT_MAXDIST, fMax);
	DDX_Text(pDX, IDC_EDIT_MINDIST, fMin);
	DDX_Text(pDX, IDC_EDIT_SHADER, strShader);
	DDX_Check(pDX, IDC_CHECK_PLAY, bPlay);
	DDX_Check(pDX, IDC_CHECKP_TRIGGERED, bTriggered);
	DDX_Check(pDX, IDC_CHECK_OMNI, bOmni);
	DDX_Text(pDX, IDC_EDIT_GROUP, strGroup);
	DDX_Check(pDX, IDC_CHECK_GROUPONLY, bGroupOnly);
	DDX_Check(pDX, IDC_CHECK_OCCLUSION, bOcclusion);
	DDX_Text(pDX, IDC_EDIT_LEADTHROUGH, leadThrough);
	DDX_Check(pDX, IDC_CHECK_PLAIN, plain);
	DDX_Check(pDX, IDC_CHECK_LOOPING, looping);
	DDX_Check(pDX, IDC_CHECK_UNCLAMPED, unclamped);
	DDX_Text(pDX, IDC_EDIT_SHAKES, shakes);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDialogSound, CDialog)
	//{{AFX_MSG_MAP(CDialogSound)
	ON_BN_CLICKED(IDC_BTN_SAVEMAP, OnBtnSavemap)
	ON_BN_CLICKED(IDC_BTN_SWITCHTOGAME, OnBtnSwitchtogame)
	ON_BN_CLICKED(IDC_BTN_APPLY_SOUND, OnBtnApply)
	ON_EN_CHANGE(IDC_EDIT_VOLUME, OnChangeEditVolume)
	ON_BN_CLICKED(IDC_BTN_REFRESH, OnBtnRefresh)
	ON_BN_CLICKED(IDC_BTN_PLAYSOUND, OnBtnPlaysound)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_SOUNDS, OnDblclkTreeSounds)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_SOUNDS, OnSelchangedTreeSounds)
	ON_BN_CLICKED(IDC_CHECK_PLAY, OnCheckPlay)
	ON_BN_CLICKED(IDC_BTN_EDIT_SOUND, OnBtnEdit)
	ON_BN_CLICKED(IDC_BTN_DROP, OnBtnDrop)
	ON_BN_CLICKED(IDC_BTN_GROUP, OnBtnGroup)
	ON_BN_CLICKED(IDC_BTN_SAVEMAPAS, OnBtnSavemapas)
	ON_BN_CLICKED(IDC_BTN_YUP, OnBtnYup)
	ON_BN_CLICKED(IDC_BTN_YDN, OnBtnYdn)
	ON_BN_CLICKED(IDC_BTN_XDN, OnBtnXdn)
	ON_BN_CLICKED(IDC_BTN_XUP, OnBtnXup)
	ON_BN_CLICKED(IDC_BTN_ZUP, OnBtnZup)
	ON_BN_CLICKED(IDC_BTN_ZDN, OnBtnZdn)
	ON_BN_CLICKED(IDC_BTN_TRIGGER, OnBtnTrigger)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_CHECK_GROUPONLY, OnCheckGrouponly)
	ON_CBN_SELCHANGE(IDC_COMBO_GROUPS, OnSelchangeComboGroups)
	ON_CBN_SELCHANGE(IDC_COMBO_SPEAKERS, OnSelchangeComboSpeakers)
	ON_BN_CLICKED(IDC_BTN_DOWN, OnBtnDown)
	ON_BN_CLICKED(IDC_BTN_UP, OnBtnUp)
	ON_BN_CLICKED(IDC_BTN_REFRESHSPEAKERS, OnBtnRefreshspeakers)
	ON_BN_CLICKED(IDC_BTN_REFRESHWAVE, OnBtnRefreshwave)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialogSound message handlers

void SoundEditorInit( const anDict *spawnArgs ) {

	if ( renderSystem->IsFullScreen() ) {
		common->Printf( "Cannot run the sound editor in fullscreen mode.\n"
					"Set r_fullscreen to 0 and vid_restart.\n" );
		return;
	}

	if ( g_SoundDialog == nullptr ) {
		InitAfx();
		g_SoundDialog = new CDialogSound();
	}

	if ( g_SoundDialog->GetSafeHwnd() == nullptr ) {
		g_SoundDialog->Create(IDD_DIALOG_SOUND);
/*
		// FIXME: restore position
		CRect rct;
		g_SoundDialog->SetWindowPos( nullptr, rct.left, rct.top, 0,0, SWP_NOSIZE );
*/
	}

	idKeyInput::ClearStates();

	g_SoundDialog->ShowWindow( SW_SHOW );
	g_SoundDialog->SetFocus();

	if ( spawnArgs ) {
		const char *name = spawnArgs->GetString( "name" );
		const anDict *dict = engineEdit->MapGetEntityDict( name );
		g_SoundDialog->Set( dict );
	}
}

void SoundEditorRun( void ) {
#if _MSC_VER >= 1300
	MSG *msg = AfxGetCurrentMessage();			// TODO Robert fix me!!
#else
	MSG *msg = &m_msgCur;
#endif

	while( ::PeekMessage(msg, nullptr, nullptr, nullptr, PM_NOREMOVE) ) {
		// pump message
		if ( !AfxGetApp()->PumpMessage() ) {
		}
	}
}

void SoundEditorShutdown( void ) {
	delete g_SoundDialog;
	g_SoundDialog = nullptr;
}

void CDialogSound::OnActivate( UINT nState, CWnd *pWndOther, BOOL bMinimized ) {
	CDialog::OnActivate( nState, pWndOther, bMinimized );
	if ( nState != WA_INACTIVE ) {
	}
}

void CDialogSound::OnMove( int x, int y ) {
	if ( GetSafeHwnd() ) {
		CRect rct;
		GetWindowRect( rct );
		// FIXME: save position
	}
	CDialog::OnMove( x, y );
}

void CDialogSound::OnDestroy() {

	com_editors &= ~EDITOR_SOUND;

	return CDialog::OnDestroy();
}

void CDialogSound::Set( const anDict *source ) {
	if ( source == nullptr ) {
		return;
	}

	fVolume = source->GetFloat( "s_volume", "0" );
	fMin = source->GetFloat( "s_mindistance", "1" );
	fMax = source->GetFloat( "s_maxdistance", "10" );
	leadThrough = source->GetFloat( "s_leadthrough", "0.1" );
	plain = source->GetBool( "s_plain" );
	strShader = source->GetString( "s_shader" );
	strGroup = source->GetString( "soundgroup" );
	bOmni = source->GetInt( "s_omni", "-1" );
	bOcclusion = source->GetBool( "s_occlusion", "0" );
	bTriggered = source->GetInt( "s_waitfortrigger", "-1" );
	random = source->GetFloat( "random" );
	wait = source->GetFloat( "wait" );
	strName = source->GetString( "name" );
	looping = source->GetBool( "s_looping" );
	unclamped = source->GetBool( "s_unclamped" );
	shakes = source->GetFloat( "s_shakes" );
	if (comboSpeakers.SelectString(-1, strName) == CB_ERR) {
		comboSpeakers.SetCurSel(-1 );
	}
	if (comboGroups.SelectString(-1, strGroup) == CB_ERR) {
		comboGroups.SetCurSel(-1 );
	}
	UpdateData(FALSE);
}

void CDialogSound::Get( anDict *source ) {

	if ( source == nullptr ) {
		return;
	}
	UpdateData( TRUE );
	float f = source->GetFloat( "s_volume" );
	source->SetFloat( "s_volume", f );
	source->SetFloat( "s_mindistance", fMin );
	source->SetFloat( "s_maxdistance", fMax );
	source->Set( "s_shader", strShader );
	source->SetInt( "s_omni", bOmni );
	source->SetBool( "s_occlusion", ( bOcclusion != FALSE ) );
	source->SetInt( "s_waitfortrigger", bTriggered );
	source->Set( "soundgroup", strGroup );
	source->Set( "name", strName );
	source->SetFloat( "s_leadthrough", leadThrough );
	source->SetBool( "s_plain", ( plain != FALSE ) );
	source->SetFloat( "wait", wait );
	source->SetFloat( "random", random );
	source->SetBool( "s_looping", looping == TRUE );
	source->SetBool( "s_unclamped", unclamped == TRUE );
	source->SetFloat( "s_shakes", shakes );
}

void CDialogSound::OnBtnSavemap()
{
	OnBtnApply();
	engineEdit->MapSave();
}

void CDialogSound::OnBtnSwitchtogame()
{
	::SetFocus(win32.hWnd);
}

void CDialogSound::SetVolume( float vol ) {
	anList<arcEntity*> list;
	list.SetNum( 128 );
	int count = engineEdit->GetSelectedEntities( list.Ptr(), list.Num() );
	list.SetNum( count );

	if ( count ) {
		// we might be in either the game or the editor
		ARCSoundWorld	*sw = soundSystem->GetPlayingSoundWorld();
		if ( sw ) {
			sw->PlayShaderDirectly( "" );
		}

		for ( int i = 0; i < count; i++ ) {
			const anDict *dict = engineEdit->EntityGetSpawnArgs( list[i] );
			if ( dict == nullptr ) {
				continue;
			}
			const char *name = dict->GetString( "name" );
			const anDict *dict2 = engineEdit->MapGetEntityDict( name );
			if ( dict2 ) {
				engineEdit->MapSetEntityKeyVal( name, "s_volume", va( "%f", vol ) );
				engineEdit->MapSetEntityKeyVal( name, "s_justVolume", "1" );
				engineEdit->EntityUpdateChangeableSpawnArgs( list[i], dict2 );
				fVolume = vol;
				UpdateData( FALSE );
			}
		}
	}
}

void CDialogSound::ApplyChanges( bool volumeOnly, bool updateInUseTree ) {
	anList<arcEntity*> list;
	float vol;

	vol = fVolume;

	list.SetNum( 128 );
	int count = engineEdit->GetSelectedEntities( list.Ptr(), list.Num() );
	list.SetNum( count );

	if ( count ) {
		// we might be in either the game or the editor
		ARCSoundWorld	*sw = soundSystem->GetPlayingSoundWorld();
		if ( sw ) {
			sw->PlayShaderDirectly( "" );
		}

		for ( int i = 0; i < count; i++ ) {
			const anDict *dict = engineEdit->EntityGetSpawnArgs( list[i] );
			if ( dict == nullptr ) {
				continue;
			}
			const char *name = dict->GetString( "name" );
			const anDict *dict2 = engineEdit->MapGetEntityDict( name );
			if ( dict2 ) {
				if ( volumeOnly ) {
					float f = dict2->GetFloat( "s_volume" );
					f += vol;
					engineEdit->MapSetEntityKeyVal( name, "s_volume", va( "%f", f ) );
					engineEdit->MapSetEntityKeyVal( name, "s_justVolume", "1" );
					engineEdit->EntityUpdateChangeableSpawnArgs( list[i], dict2 );
					fVolume = f;
					UpdateData( FALSE );
				} else {
					anDict src;
					src.SetFloat( "s_volume", dict2->GetFloat( "s_volume" ) );
					Get( &src );
					src.SetBool( "s_justVolume", true );
					engineEdit->MapCopyDictToEntity( name, &src );
					engineEdit->EntityUpdateChangeableSpawnArgs( list[i], dict2 );
					Set( dict2 );
				}
			}
		}
	}

	AddGroups();
	AddSpeakers();
	if ( updateInUseTree ) {
		AddInUseSounds();
	}
}

void CDialogSound::OnBtnApply() {
	ApplyChanges();
}

void CDialogSound::OnChangeEditVolume() {
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO: Add your control notification handler code here

}

HTREEITEM CDialogSound::AddStrList(const char *root, const anStringList &list, int id) {
	anString		out;

	HTREEITEM	base = treeSounds.InsertItem(root);
	HTREEITEM	item = base;
	HTREEITEM	add;

	int count = list.Num();

	anString	last, path, path2;
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
			add = treeSounds.InsertItem(out, item);
			quickTree.Set(name, add);
			treeSounds.SetItemData(add, id);
			treeSounds.SetItemImage(add, 2, 2);
			continue;
		} else {
			last.Empty();
		}

		index = 0;
		item = base;
		path = "";
		path2 = "";
		while (index >= 0 ) {
			index = name.Find('/');
			if (index >= 0 ) {
				HTREEITEM newItem = nullptr;
				HTREEITEM *check = nullptr;
				name.Left( index, out );
				path += out;
				if (quickTree.Get(path, &check) ) {
					newItem = *check;
				}

				//HTREEITEM newItem = FindTreeItem(&treeSounds, item, name.Left(index, out), item);
				if (newItem == nullptr ) {
					newItem = treeSounds.InsertItem(out, item);
					quickTree.Set(path, newItem);
					treeSounds.SetItemData(newItem, WAVEDIR);
					treeSounds.SetItemImage(newItem, 0, 1 );
				}

				assert(newItem);
				item = newItem;
				name.Right( name.Length() - index - 1, out );
				name = out;
				path += "/";
			} else {
				add = treeSounds.InsertItem(name, item);
				treeSounds.SetItemData(add, id);
				treeSounds.SetItemImage(add, 2, 2);
				path = "";
			}
		}
	}
	return base;
}

void CDialogSound::AddSounds(bool rootItems) {
	int i, j;
	anStringList list(1024);
	anStringList list2(1024);
	HTREEITEM base = treeSounds.InsertItem( "Sound Shaders" );

	for ( i = 0; i < declManager->GetNumDecls( DECL_SOUND ); i++ ) {
		const anSoundShader *poo = declManager->SoundByIndex( i, false);
		list.AddUnique( poo->GetFileName() );
	}
	list.Sort();

	for ( i = 0; i < list.Num(); i++ ) {
		HTREEITEM child = treeSounds.InsertItem(list[i], base);
		treeSounds.SetItemData(child, SOUNDPARENT);
		treeSounds.SetItemImage(child, 0, 1 );
		list2.Clear();
		for ( j = 0; j < declManager->GetNumDecls( DECL_SOUND ); j++ ) {
			const anSoundShader *poo = declManager->SoundByIndex(j, false);
			if ( anString::Icmp( list[i], poo->GetFileName() ) == 0 ) {
				list2.Append( poo->GetName() );
			}
		}
		list2.Sort();
		for ( j = 0; j < list2.Num(); j++ ) {
			HTREEITEM child2 = treeSounds.InsertItem( list2[j], child );
			treeSounds.SetItemData(child2, SOUNDS);
			treeSounds.SetItemImage(child2, 2, 2);
		}
	}

	anFileList *files;
	files = fileSystem->ListFilesTree( "sound", ".wav|.ogg", true );
    AddStrList( "Wave files", files->GetList(), WAVES );
	fileSystem->FreeFileList( files );
}

void CDialogSound::AddGroups() {
	comboGroups.ResetContent();
	anString work;
	CWaitCursor cursor;

	anList<const char*> list;
	list.SetNum( 1024 );
	int count = engineEdit->MapGetUniqueMatchingKeyVals( "soundgroup", list.Ptr(), list.Num() );
	for ( int i = 0; i < count; i++ ) {
		comboGroups.AddString( list[i] );
	}
}

void CDialogSound::AddInUseSounds() {
	if ( inUseTree ) {
		treeSounds.DeleteItem( inUseTree );
		inUseTree = nullptr;
	}
	inUseTree = treeSounds.InsertItem( "Sounds in use" );
	anList< const char *> list;
	list.SetNum( 512 );
	int i, count = engineEdit->MapGetEntitiesMatchingClassWithString( "speaker", "", list.Ptr(), list.Num() );
	anStringList list2;
	for ( i = 0; i < count; i++ ) {
		const anDict *dict = engineEdit->MapGetEntityDict( list[i] );
		if ( dict ) {
			const char *p = dict->GetString( "s_shader" );
			if ( p && *p ) {
				list2.AddUnique( p );
			}
		}
	}
	list2.Sort();
	count = list2.Num();
	for ( i = 0; i < count; i++ ) {
		HTREEITEM child = treeSounds.InsertItem( list2[i], inUseTree );
		treeSounds.SetItemData( child, INUSESOUNDS );
		treeSounds.SetItemImage( child, 2, 2 );
	}
}

void CDialogSound::AddSpeakers() {
	UpdateData( TRUE );
	comboSpeakers.ResetContent();

	CWaitCursor cursor;
	anList< const char *> list;
	list.SetNum( 512 );

	CString group( "" );
	if (bGroupOnly && comboGroups.GetCurSel() >= 0 ) {
		comboGroups.GetLBText( comboGroups.GetCurSel(), group );
	}
	int count = engineEdit->MapGetEntitiesMatchingClassWithString( "speaker", group, list.Ptr(), list.Num() );

	for ( int i = 0; i < count; i++ ) {
		comboSpeakers.AddString(list[i] );
	}
}

BOOL CDialogSound::OnInitDialog() {
	CDialog::OnInitDialog();

	// Indicate the sound dialog is opened
	com_editors |= EDITOR_SOUND;

	inUseTree = nullptr;
	AddSounds(true);
	AddGroups();
	AddSpeakers();
	AddInUseSounds();
	SetWaveSize();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDialogSound::OnBtnRefresh() {
	CWaitCursor cursor;
	treeSounds.DeleteAllItems();
	quickTree.Clear();
	declManager->Reload( false );
	AddSounds(true);
}

void CDialogSound::OnBtnPlaysound() {
	if (playSound.GetLength() ) {
		// we might be in either the game or the editor
		ARCSoundWorld	*sw = soundSystem->GetPlayingSoundWorld();
		if ( sw ) {
			sw->PlayShaderDirectly(playSound);
		}
	}

}

void CDialogSound::OnDblclkTreeSounds(NMHDR* pNMHDR, LRESULT* pResult) {
	*pResult = 0;
	CPoint pt;
	GetCursorPos( &pt );
	treeSounds.ScreenToClient( &pt );
	HTREEITEM item = treeSounds.HitTest( pt );

	if (item) {
		DWORD dw = treeSounds.GetItemData( item );
		if ( dw == SOUNDS || dw == INUSESOUNDS ) {
			if ( !treeSounds.ItemHasChildren( item ) ) {
				strShader = treeSounds.GetItemText( item );
				UpdateData( FALSE );
				ApplyChanges( false, ( dw == SOUNDS ) );
			}
		} else if ( dw == WAVES ) {
			strShader = RebuildItemName( "Wave Files", item );
			UpdateData( FALSE );
			OnBtnApply();
		}
	}
	*pResult = 0;
}

void CDialogSound::SetWaveSize( const char *p ) {
	CWnd *wnd = GetDlgItem( IDC_STATIC_WAVESIZE );
	if ( wnd ) {
		wnd->SetWindowText( ( p && *p ) ? p : "unknown" );
	}
}

void CDialogSound::OnSelchangedTreeSounds(NMHDR* pNMHDR, LRESULT* pResult) {
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM	item = treeSounds.GetSelectedItem();
	SetWaveSize();
	if (item) {
		DWORD	dw = treeSounds.GetItemData(item);
		if ( dw == SOUNDS || dw == INUSESOUNDS ) {
			playSound = treeSounds.GetItemText(item);
			if (bPlay){
				OnBtnPlaysound();
			}
		} else if (dw == WAVES) {
			playSound = RebuildItemName( "Wave Files", item);
			float size = fileSystem->ReadFile( playSound, nullptr );
			SetWaveSize( va( "%0.2f mb", size / ( 1024 * 1024)  ) );
			if (bPlay){
				OnBtnPlaysound();
			}
		}
	}

	*pResult = 0;
}

void CDialogSound::OnCheckPlay() {
	UpdateData(TRUE);
}

void CDialogSound::OnBtnEdit() {
	const anDecl *decl = declManager->FindDeclWithoutParsing( DECL_SOUND, strShader, false );

	if ( decl ) {
		DialogDeclEditor *declEditor;

		declEditor = new DialogDeclEditor;
		declEditor->Create( IDD_DIALOG_DECLEDITOR, GetParent() );
		declEditor->LoadDecl( const_cast<anDecl *>( decl ) );
		declEditor->ShowWindow( SW_SHOW );
		declEditor->SetFocus();
	}
}

void CDialogSound::OnBtnDrop() {
	anString		classname;
	anString		key;
	anString		value;
	anVec3		org;
	anDict		args;
	anAngles	viewAngles;


	engineEdit->PlayerGetViewAngles( viewAngles );
	engineEdit->PlayerGetEyePosition( org );
	org += anAngles( 0, viewAngles.yaw, 0 ).ToForward() * 80 + anVec3( 0, 0, 1 );
	args.Set( "origin", org.ToString() );
	args.Set( "classname", "speaker" );
	args.Set( "angle", va( "%f", viewAngles.yaw + 180 ) );
	args.Set( "s_shader", strShader);
	args.Set( "s_looping", "1" );
	args.Set( "s_shakes", "0" );


	anString name = engineEdit->GetUniqueEntityName( "speaker" );
	bool nameValid = false;
	while ( !nameValid) {
		DialogName dlg( "Name Speaker", this);
		dlg.m_strName = name;
		if (dlg.DoModal() == IDOK) {
			arcEntity *gameEnt = engineEdit->FindEntity(dlg.m_strName);
			if (gameEnt) {
				if (MessageBox( "Please choose another name", "Duplicate Entity Name!", MB_OKCANCEL) == IDCANCEL) {
					return;
				}
			} else {
				nameValid = true;
				name = dlg.m_strName;
			}
		}
	}

	args.Set( "name", name.c_str() );

	arcEntity *ent = nullptr;
	engineEdit->SpawnEntityDef( args, &ent );
	if (ent) {
		engineEdit->EntityUpdateChangeableSpawnArgs( ent, nullptr );
		engineEdit->ClearEntitySelection();
		engineEdit->AddSelectedEntity( ent );
	}

	engineEdit->MapAddEntity( &args );
	const anDict *dict = engineEdit->MapGetEntityDict( args.GetString( "name" ) );
	Set( dict );
	AddGroups();
	AddSpeakers();
}

void CDialogSound::OnBtnGroup()
{
	anList<arcEntity*> list;

	list.SetNum( 128 );
	int count = engineEdit->GetSelectedEntities( list.Ptr(), list.Num() );
	list.SetNum( count );

	bool removed = false;
	if (count) {
		for ( int i = 0; i < count; i++ ) {
			const anDict *dict = engineEdit->EntityGetSpawnArgs( list[i] );
			if ( dict == nullptr ) {
				continue;
			}
			const char *name = dict->GetString( "name" );
			dict = engineEdit->MapGetEntityDict( name );
			if ( dict ) {
				if (MessageBox( "Are you Sure?", "Delete Selected Speakers", MB_YESNO) == IDYES) {
					engineEdit->MapRemoveEntity( name );
					arcEntity *gameEnt = engineEdit->FindEntity( name );
					if ( gameEnt ) {
						engineEdit->EntityStopSound( gameEnt );
						engineEdit->EntityDelete( gameEnt );
						removed = true;
					}
				}
			}
		}
	}

	if (removed) {
		AddGroups();
		AddSpeakers();
	}

}

void CDialogSound::OnBtnSavemapas() {
	CFileDialog dlgSave(FALSE,"map",nullptr,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,"Map Files (*.map)|*.map||",AfxGetMainWnd() );
	if (dlgSave.DoModal() == IDOK) {
		OnBtnApply();
		anString work;
		work = fileSystem->OSPathToRelativePath( dlgSave.m_ofn.lpstrFile );
		engineEdit->MapSave( work );
	}
}

anString CDialogSound::RebuildItemName(const char *root, HTREEITEM item) {
	// have to build the name back up
	anString strParent;
	HTREEITEM parent = treeSounds.GetParentItem(item);
	anString name = treeSounds.GetItemText(item);
	while (true && parent) {
		anString test = treeSounds.GetItemText(parent);
		if ( anString::Icmp(test, root) == 0 ) {
			break;
		}
		strParent = test;
		strParent += "/";
		strParent += name;
		name = strParent;
		parent = treeSounds.GetParentItem(parent);
		if (parent == nullptr ) {
			break;
		}
	}
 	return strParent;
}


void CDialogSound::UpdateSelectedOrigin( float x, float y, float z ) {
	anList<arcEntity*> list;
	anVec3 origin;
	anVec3 vec( x, y, z );

	list.SetNum( 128 );
	int count = engineEdit->GetSelectedEntities( list.Ptr(), list.Num() );
	list.SetNum( count );

	if ( count ) {
		for ( int i = 0; i < count; i++ ) {
			const anDict *dict = engineEdit->EntityGetSpawnArgs( list[i] );
			if ( dict == nullptr ) {
				continue;
			}
			const char *name = dict->GetString( "name" );
			engineEdit->EntityTranslate( list[i], vec );
			engineEdit->EntityUpdateVisuals( list[i] );
			engineEdit->MapEntityTranslate( name, vec );
		}
	}
}

void CDialogSound::OnBtnYup() {
	UpdateSelectedOrigin(0, 8, 0 );
}

void CDialogSound::OnBtnYdn() {
	UpdateSelectedOrigin(0, -8, 0 );
}

void CDialogSound::OnBtnXdn() {
	UpdateSelectedOrigin(-8, 0, 0 );
}

void CDialogSound::OnBtnXup() {
	UpdateSelectedOrigin(8, 0, 0 );
}

void CDialogSound::OnBtnZup() {
	UpdateSelectedOrigin(0, 0, 8);
}

void CDialogSound::OnBtnZdn() {
	UpdateSelectedOrigin(0, 0, -8);
}

void CDialogSound::OnBtnTrigger() {
	engineEdit->TriggerSelected();
}

void CDialogSound::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) {
}


void CDialogSound::OnCheckGrouponly() {
	AddSpeakers();
}

void CDialogSound::OnSelchangeComboGroups() {
	CWaitCursor cursor;
	engineEdit->ClearEntitySelection();
	if ( comboGroups.GetCurSel() >= 0 ) {
		CString group;
		comboGroups.GetLBText( comboGroups.GetCurSel(), group );

		anList< const char *> list;
		list.SetNum( 512 );
		int count = engineEdit->MapGetEntitiesMatchingClassWithString( "speaker", group, list.Ptr(), list.Num() );
		for ( int i = 0; i < count; i++ ) {
			arcEntity *gameEnt = engineEdit->FindEntity( list[i] );
			if (gameEnt) {
				engineEdit->AddSelectedEntity( gameEnt );
				Set( engineEdit->EntityGetSpawnArgs( gameEnt ) );
			}
		}
	}
	AddSpeakers();
}

void CDialogSound::OnSelchangeComboSpeakers() {
	CWaitCursor cursor;
	engineEdit->ClearEntitySelection();
	if ( comboSpeakers.GetCurSel() >= 0 ) {
		CString speaker;
		comboSpeakers.GetLBText( comboSpeakers.GetCurSel(), speaker );
		anList< const char *> list;
		list.SetNum( 512 );
		int count = engineEdit->MapGetEntitiesMatchingClassWithString( "speaker", speaker, list.Ptr(), list.Num() );
		for ( int i = 0; i < count; i++ ) {
			arcEntity *gameEnt = engineEdit->FindEntity( list[i] );
			if (gameEnt) {
				engineEdit->AddSelectedEntity( gameEnt );
				Set( engineEdit->EntityGetSpawnArgs( gameEnt ) );
			}
		}
	}
}

void CDialogSound::OnBtnDown() {
	fVolume	= -1.0;
	UpdateData( FALSE );
	ApplyChanges( true, false );
}

void CDialogSound::OnBtnUp() {
	fVolume	= 1.0;
	UpdateData( FALSE );
	ApplyChanges( true, false );
}

void CDialogSound::OnBtnRefreshspeakers() {
	AddGroups();
	AddSpeakers();
}

void CDialogSound::OnBtnRefreshwave() {
	HTREEITEM	item = treeSounds.GetSelectedItem();
	if (item && treeSounds.GetItemData( item ) == WAVEDIR) {
		anString path = "sound/";
		path += RebuildItemName( "sound", item);
		anFileList *files;
		files = fileSystem->ListFilesTree( path, ".wav" );
		HTREEITEM child = treeSounds.GetChildItem(item);
		while (child) {
			HTREEITEM next = treeSounds.GetNextSiblingItem(child);
			if (treeSounds.GetItemData(child) == WAVES) {
				treeSounds.DeleteItem(child);
			}
			child = next;
		}
		int c = files->GetNumFiles();
		for ( int i = 0; i < c; i++ ) {
			anString work = files->GetFile( i );
			work.StripPath();
			child = treeSounds.InsertItem(work, item);
			treeSounds.SetItemData( child, WAVES );
			treeSounds.SetItemImage( child, 2, 2 );
		}
		fileSystem->FreeFileList( files );
	}
}

BOOL CDialogSound::PreTranslateMessage(MSG* pMsg) {
	CWnd *wnd = GetDlgItem( IDC_EDIT_VOLUME );
	if ( wnd && pMsg->hwnd == wnd->GetSafeHwnd() ) {
		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN ) {
			CString str;
			wnd->GetWindowText( str );
			SetVolume( atof( str ) );
			return TRUE;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}
