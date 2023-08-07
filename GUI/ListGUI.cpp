
#include "../idlib/Lib.h"
#pragma hdrstop

#include "ListGUILocal.h"

/*
====================
anListGUILocal::StateChanged
====================
*/
void anListGUILocal::StateChanged() {
	if ( !m_stateUpdates ) {
		return;
	}

	for (  int i = 0; i < Num(); i++ ) {
		m_pGUI->SetStateString( va( "%s_item_%i", m_name.c_str(), i ), (*this)[i].c_str() ); 
	}
	for ( int i = Num() ; i < m_water ; i++ ) {
		m_pGUI->SetStateString( va( "%s_item_%i", m_name.c_str(), i ), "" );
	}
	m_water = Num();
	m_pGUI->StateChanged( com_frameTime );
}

/*
====================
anListGUILocal::GetNumSelections
====================
*/
int anListGUILocal::GetNumSelections() {
	return m_pGUI->State().GetInt( va( "%s_numsel", m_name.c_str() ) );
}

/*
====================
anListGUILocal::GetSelection
====================
*/
int anListGUILocal::GetSelection( char *s, int size, int _sel ) const {
	if ( s ) {		
		s[0] = '\0';
	}
	int sel = m_pGUI->State().GetInt( va( "%s_sel_%i", m_name.c_str(), _sel ), "-1" );
	if ( sel == -1 || sel >= m_ids.Num() ) {
		return -1;
	}
	if ( s ) {
		anStr::snPrintf( s, size, m_pGUI->State().GetString( va( "%s_item_%i", m_name.c_str(), sel ), "" ) );
	}
	// don't let overflow
	if ( sel >= m_ids.Num() ) {
		sel = 0;
	}
	m_pGUI->SetStateInt( va( "%s_selid_0", m_name.c_str() ), m_ids[ sel ] ); 
	return m_ids[ sel ];
}

/*
====================
anListGUILocal::SetSelection
====================
*/
void anListGUILocal::SetSelection( int sel ) {
	m_pGUI->SetStateInt( va( "%s_sel_0", m_name.c_str() ), sel );
	StateChanged();
}

/*
====================
anListGUILocal::Add
====================
*/
void anListGUILocal::Add( int id, const anStr &s ) {
	int i = m_ids.FindIndex( id );
	if ( i == -1 ) {
		Append( s );
		m_ids.Append( id );
	} else {
		(* this)[i] = s;
	}
	StateChanged();
}

/*
====================
anListGUILocal::Push
====================
*/
void anListGUILocal::Push( const anStr &s ) {
	Append( s );
	m_ids.Append( m_ids.Num() );
	StateChanged();
}

/*
====================
anListGUILocal::Del
====================
*/
bool anListGUILocal::Del( int id ) {
	int i = m_ids.FindIndex( id );
	if ( i == -1 ) {
		return false;
	}
	m_ids.RemoveIndex( i );
	this->RemoveIndex( i );
	StateChanged();
	return true;
}

/*
====================
anListGUILocal::Clear
====================
*/
void anListGUILocal::Clear() {
	m_ids.Clear();
	anList<anStr>::Clear();
	if ( m_pGUI ) {
		// will clear all the GUI variables and will set m_water back to 0
		StateChanged();
	}
}

/*
====================
anListGUILocal::IsConfigured
====================
*/
bool anListGUILocal::IsConfigured( void ) const {
	return m_pGUI != nullptr;
}

/*
====================
anListGUILocal::SetStateChanges
====================
*/
void anListGUILocal::SetStateChanges( bool enable ) {
	m_stateUpdates = enable;
	StateChanged();
}

/*
====================
anListGUILocal::Shutdown
====================
*/
void anListGUILocal::Shutdown( void ) {
	m_pGUI = nullptr;
	m_name.Clear();
	Clear();
}
