//=========== Copyright © 2014, rHetorical, All rights reserved. =============
//
// Purpose: Draws the normal TF2 or HL2 HUD.
//
//=============================================================================
#if !defined( CLIENTMODE_VECTRONIC_H )
#define CLIENTMODE_VECTRONIC_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>
#include "ivmodemanager.h"

class CHudViewport;

namespace vgui
{
	typedef unsigned long HScheme;
}

// --------------------------------------------------------------------------------- //
// Purpose: CVectronicModeManager.
// --------------------------------------------------------------------------------- //
class CVectronicModeManager : public IVModeManager
{
public:
				CVectronicModeManager( void ) {}
	virtual		~CVectronicModeManager( void ) {}

	virtual void	Init( void );
	virtual void	SwitchMode( bool commander, bool force ) {}
	virtual void	OverrideView( CViewSetup *pSetup ) {}
	virtual void	CreateMove( float flInputSampleTime, CUserCmd *cmd ) {}
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
};

// --------------------------------------------------------------------------------- //
// Purpose: CVectronicModeManager.
// --------------------------------------------------------------------------------- //
class ClientModeVectronicNormal : public ClientModeShared
{
	DECLARE_CLASS( ClientModeVectronicNormal, ClientModeShared );
public:

			ClientModeVectronicNormal();
	virtual ~ClientModeVectronicNormal();

	virtual void	Init();
	virtual bool	DoPostScreenSpaceEffects( const CViewSetup *pSetup );
};

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void CreateDefaultPanels( void ) { /* don't create any panels yet*/ };
};

extern IClientMode *GetClientModeNormal();
extern ClientModeVectronicNormal* GetClientModeVectronicNormal();

#endif // CLIENTMODE_VECTRONIC_H
