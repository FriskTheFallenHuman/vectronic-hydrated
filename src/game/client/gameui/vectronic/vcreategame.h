//=====================================================================================//
//
// Purpose:
//
//=====================================================================================//

#ifndef __VCREATEGAME_H__
#define __VCREATEGAME_H__
#ifdef _WIN32
#pragma once
#endif

#include "basemodui.h"
#include <vgui_controls/PropertyDialog.h>

class CreateGameServerPage;
class CreateGameGameplayPage;

namespace BaseModUI 
{
	//-----------------------------------------------------------------------------
	// Purpose: dialog for the window
	//-----------------------------------------------------------------------------
	class CreateGameProperty : public vgui::PropertyDialog
	{
		DECLARE_CLASS_SIMPLE( CreateGameProperty,  vgui::PropertyDialog );

	public:
		CreateGameProperty( vgui::Panel *parent );
		~CreateGameProperty();

		virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
		virtual bool	OnOK( bool applyOnly );
		virtual void	OnCommand( const char *command );
		virtual void	OnKeyCodeTyped( vgui::KeyCode code );

	private:
		CreateGameServerPage *m_pServerPage;
		CreateGameGameplayPage *m_pGameplayPage;

		// for loading/saving game config
		KeyValues *m_pSavedData;
	};

	//-----------------------------------------------------------------------------
	// Purpose: window for launching a server
	//-----------------------------------------------------------------------------
	class CreateGame : public CBaseModFrame
	{
		DECLARE_CLASS_SIMPLE( CreateGame,  CBaseModFrame );

	public:
		CreateGame( vgui::Panel *parent, const char *panelName );
		~CreateGame();

		virtual void	Activate( void );
		virtual void	PerformLayout( void );
		virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
		virtual void	PaintBackground() {}

		virtual void	OnCommand( const char *command );
		virtual void	OnKeyCodeTyped( vgui::KeyCode code );
	private:

		CreateGameProperty *m_pProperty;
	};
}
#endif // __VCREATEGAME_H__
