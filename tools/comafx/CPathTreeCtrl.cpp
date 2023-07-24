
#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "CPathTreeCtrl.h"


/*
================
CPathTreeCtrl::CPathTreeCtrl
================
*/
CPathTreeCtrl::CPathTreeCtrl() {
}

/*
================
CPathTreeCtrl::~CPathTreeCtrl
================
*/
CPathTreeCtrl::~CPathTreeCtrl() {
}

/*
================
CPathTreeCtrl::PreSubclassWindow
================
*/
void CPathTreeCtrl::PreSubclassWindow() {
	CTreeCtrl::PreSubclassWindow();
	EnableToolTips( TRUE );
}

/*
================
CPathTreeCtrl::FindItem

Find the given path in the tree.
================
*/
HTREEITEM CPathTreeCtrl::FindItem( const arcNetString &pathName ) {
	int lastSlash;
	arcNetString path, tmpPath, itemName;
	HTREEITEM item, parentItem;

	parentItem = NULL;
	item = GetRootItem();

	lastSlash = pathName.Last( '/' );

	while( item && lastSlash > path.Length() ) {
		itemName = GetItemText( item );
		tmpPath = path + itemName;
		if ( pathName.Icmpn( tmpPath, tmpPath.Length() ) == 0 ) {
			parentItem = item;
			item = GetChildItem( item );
			path = tmpPath + "/";
		} else {
			item = GetNextSiblingItem( item );
		}
	}

	for ( item = GetChildItem( parentItem ); item; item = GetNextSiblingItem( item ) ) {
		itemName = GetItemText( item );
		if ( pathName.Icmp( path + itemName ) == 0 ) {
			return item;
		}
	}

	return NULL;
}

/*
================
CPathTreeCtrl::InsertPathIntoTree

Inserts a new item going from the root down the tree only creating paths where necessary.
This is slow and should only be used to insert single items.
================
*/
HTREEITEM CPathTreeCtrl::InsertPathIntoTree( const arcNetString &pathName, const int id ) {
	int lastSlash;
	arcNetString path, tmpPath, itemName;
	HTREEITEM item, parentItem;

	parentItem = NULL;
	item = GetRootItem();

	lastSlash = pathName.Last( '/' );

	while( item && lastSlash > path.Length() ) {
		itemName = GetItemText( item );
		tmpPath = path + itemName;
		if ( pathName.Icmpn( tmpPath, tmpPath.Length() ) == 0 ) {
			parentItem = item;
			item = GetChildItem( item );
			path = tmpPath + "/";
		} else {
			item = GetNextSiblingItem( item );
		}
	}

	while( lastSlash > path.Length() ) {
		pathName.Mid( path.Length(), pathName.Length(), tmpPath );
		tmpPath.Left( tmpPath.Find( '/' ), itemName );
		parentItem = InsertItem( itemName, parentItem );
		path += itemName + "/";
	}

	pathName.Mid( path.Length(), pathName.Length(), itemName );
	item = InsertItem( itemName, parentItem, TVI_SORT );
	SetItemData( item, id );

	return item;
}

/*
================
CPathTreeCtrl::AddPathToTree

Adds a new item to the tree.
Assumes new paths after the current stack path do not yet exist.
================
*/
HTREEITEM CPathTreeCtrl::AddPathToTree( const arcNetString &pathName, const int id, idPathTreeStack &stack ) {
	int lastSlash;
	arcNetString itemName, tmpPath;
	HTREEITEM item;

	lastSlash = pathName.Last( '/' );

	while( stack.Num() > 1 ) {
		if ( pathName.Icmpn( stack.TopName(), stack.TopNameLength() ) == 0 ) {
			break;
		}
		stack.Pop();
	}

	while( lastSlash > stack.TopNameLength() ) {
		pathName.Mid( stack.TopNameLength(), pathName.Length(), tmpPath );
		tmpPath.Left( tmpPath.Find( '/' ), itemName );
		item = InsertItem( itemName, stack.TopItem() );
		stack.Push( item, itemName );
	}

	pathName.Mid( stack.TopNameLength(), pathName.Length(), itemName );
	item = InsertItem( itemName, stack.TopItem() );
	SetItemData( item, id );

	return item;
}

/*
================
CPathTreeCtrl::SearchTree

Search the three using the search string.
Adds the matched tree items to the result tree.
Returns the number of items added to the result tree.
================
*/
int CPathTreeCtrl::SearchTree( treeItemCompare_t compare, void *data, CPathTreeCtrl &result ) {
	idPathTreeStack stack, searchStack;
	HTREEITEM item, child;
	arcNetString name;
	int id, numItems;

	numItems = 0;
	result.DeleteAllItems();
	stack.PushRoot( NULL );

	item = GetRootItem();
	searchStack.PushRoot( item );
	id = 0;

	while( searchStack.Num() > 0 ) {

		for ( child = GetChildItem( item ); child; child = GetChildItem( child ) ) {
			searchStack.Push( item, GetItemText( item ) );
			item = child;
		}

		name = searchStack.TopName();
		name += GetItemText( item );
		id = GetItemData( item );

		if ( compare( data, item, name ) ) {
			result.AddPathToTree( name, id, stack );
			numItems++;
		}

		for ( item = GetNextSiblingItem( item ); item == NULL;  ) {
			item = GetNextSiblingItem( searchStack.TopItem() );
			searchStack.Pop();
			if ( searchStack.Num() <= 0 ) {
				return numItems;
			}
		}
	}

	return numItems;
}

BEGIN_MESSAGE_MAP(CPathTreeCtrl,CTreeCtrl)
	//{{AFX_MSG_MAP(CPathTreeCtrl)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
================
CPathTreeCtrl::OnToolHitTest
================
*/
int CPathTreeCtrl::OnToolHitTest( CPoint point, TOOLINFO * pTI ) const {
	RECT rect;

	UINT nFlags;
	HTREEITEM hitem = HitTest( point, &nFlags );
	if ( nFlags & TVHT_ONITEM ) {
		GetItemRect( hitem, &rect, TRUE );
		pTI->hwnd = m_hWnd;
		pTI->uId = (UINT)hitem;
		pTI->lpszText = LPSTR_TEXTCALLBACK;
		pTI->rect = rect;
		return pTI->uId;
	}
	return -1;
}

/*
================
CPathTreeCtrl::OnToolTipText
================
*/
BOOL CPathTreeCtrl::OnToolTipText( UINT id, NMHDR * pNMHDR, LRESULT * pResult ) {
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

	UINT nID = pNMHDR->idFrom;

	*pResult = 0;

	// Do not process the message from built in tooltip
	if ( nID == (UINT)m_hWnd &&
			( ( pNMHDR->code == TTN_NEEDTEXTA && pTTTA->uFlags & TTF_IDISHWND ) ||
			( pNMHDR->code == TTN_NEEDTEXTW && pTTTW->uFlags & TTF_IDISHWND ) ) ) {
		return FALSE;
	}

	CString toolTip = "?";

	// Get the mouse position
	const MSG* pMessage;
	CPoint pt;
	pMessage = GetCurrentMessage();
	ASSERT ( pMessage );
	pt = pMessage->pt;
	ScreenToClient( &pt );

	// get the tree item
	UINT nFlags;
	HTREEITEM hitem = HitTest( pt, &nFlags );

	if ( nFlags & TVHT_ONITEM ) {
		// relay message to parent
		pTTTA->hdr.hwndFrom = GetSafeHwnd();
		pTTTA->hdr.idFrom = (UINT) hitem;
		if ( GetParent()->SendMessage( WM_NOTIFY, ( TTN_NEEDTEXT << 16 ) | GetDlgCtrlID(), (LPARAM)pTTTA ) == FALSE ) {
			return FALSE;
		}
	}

	return TRUE;    // message was handled
}
