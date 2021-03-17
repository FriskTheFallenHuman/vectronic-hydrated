//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef __VCREATEGAME_H__
#define __VCREATEGAME_H__

#include "basemodui.h"

class CPanelListPanel;
class CDescription;
class mpcontrol_t;

namespace BaseModUI 
{
	//-----------------------------------------------------------------------------
	// Purpose: window for launching a server
	//-----------------------------------------------------------------------------
	class CreateGame : public CBaseModFrame
	{
		DECLARE_CLASS_SIMPLE( CreateGame, CBaseModFrame );

	public:
		CreateGame( vgui::Panel *parent, const char *panelName );
		~CreateGame();

		virtual void	Activate( void );
		virtual void	PerformLayout( void );
		virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );

		virtual void	OnCommand( const char *command );
		virtual void	OnKeyCodeTyped( vgui::KeyCode code );
		virtual void	PaintBackground();

		// returns currently entered information about the server
		virtual void	SetMap( const char *name );
		virtual bool	IsRandomMapSelected();
		virtual const char	*GetMapName();

		// returns currently entered information about the server
		virtual	int GetMaxPlayers();
		virtual const char *GetPassword();
		virtual const char *GetHostName();

	protected:
		virtual void	OnApplyChanges();

	private:
		virtual void	LoadMapList();
		virtual void	LoadMaps( const char *pszPathID );
		virtual const char *GetValue( const char *cvarName, const char *defaultValue );
		virtual void LoadGameOptionsList();
		virtual void GatherCurrentValues();

		CDescription *m_pDescription;
		mpcontrol_t *m_pList;
		CPanelListPanel *m_pOptionsList;

		vgui::ComboBox *m_pMapList;

		enum { DATA_STR_LENGTH = 64 };
		char m_szMapName[DATA_STR_LENGTH];

		// for loading/saving game config
		KeyValues *m_pSavedData;
	};

};

#endif // __VCREATEGAME_H__