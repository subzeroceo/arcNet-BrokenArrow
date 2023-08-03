
#ifdef 0
class anListGUILocal : protected anList<anString>, public anListGUI {
public:
	virtual				~anListGUI() { }

	// anListGUI interface
	virtual void		Config( anUserInterface *pGUI, const char *name ) = 0;
	virtual void		Add( int id, const anString& s ) = 0;

						// use the element count as index for the ids
	virtual void		Push( const anString& s ) = 0;
	virtual bool		Del( int id ) = 0;
	virtual void		Clear( void ) = 0;
	virtual int			Num( void ) = 0;
	virtual int			GetSelection( char *s, int size, int sel = 0 ) const = 0;
	virtual void		SetSelection( int sel ) = 0;
	virtual int			GetNumSelections() = 0;
	virtual bool		IsConfigured( void ) const = 0;

	virtual void		SetStateChanges( bool enable ) = 0;
	virtual void		Shutdown( void ) = 0;

private:
	anUserInterface *	m_pGUI;
	anString				m_name;
	int					m_water;
	anList<int>			m_ids;
	bool				m_stateUpdates;

	void				StateChanged();
};

#endif
