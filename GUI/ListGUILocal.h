#ifndef __LISTGUI_H__
#define __LISTGUI_H__

/*
===============================================================================

	feed data to a listDef
	each item has an id and a display string

===============================================================================
*/

class anListGUI : protected anList<anString>, public anListGUI {
public:
						anListGUI() { m_pGUI = nullptr; m_water = 0; m_stateUpdates = true; }
						~anListGUI() { }
	// anListGUI interface
	void				Config( anUserInterface *pGUI, const char *name ) { m_pGUI = pGUI; m_name = name; }
	void				Add( int id, const anString& s );
						// use the element count as index for the ids
	void				Push( const anString& s );
	bool				Del( int id );
	void				Clear( void );
	int					Num( void ) { return anList<anString>::Num(); }
	int					GetSelection( char *s, int size, int sel = 0 ) const; // returns the id, not the list index (or -1)
	void				SetSelection( int sel );
	int					GetNumSelections();
	bool				IsConfigured( void ) const;
						// by default, any modification to the list will trigger a full GUI refresh immediately
	void				SetStateChanges( bool enable );
	void				Shutdown( void );

private:
	anUserInterface *	m_pGUI;
	anString				m_name;
	int					m_water;
	anList<int>			m_ids;
	bool				m_stateUpdates;

	void				StateChanged();
};

#endif // !__LISTGUI_H__
