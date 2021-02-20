//=========== Copyright © 2014, rHetorical, All rights reserved. =============
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "con_nprint.h"
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/Panel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CHudBuildInfo : public Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudBuildInfo, Panel );

public:
	CHudBuildInfo( const char *pElementName );

	virtual void Paint( void );

};

DECLARE_HUDELEMENT( CHudBuildInfo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudBuildInfo::CHudBuildInfo( const char *pElementName ) : BaseClass( NULL, "BuildInfo" ), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetPaintBackgroundType( 0 );
	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBuildInfo::Paint( void )
{
	con_nprint_s nxPrn;
    nxPrn.time_to_live = gpGlobals->curtime;
    nxPrn.index = 0;
    nxPrn.fixed_width_font = true;
    nxPrn.color[0] = 0.118;
    nxPrn.color[1] = 0.341;
    nxPrn.color[2] = 0.612;
    engine->Con_NXPrintf( &nxPrn, "Vectronic Hydrated" );
    nxPrn.index = 1;
    nxPrn.color[0] = 0;
    nxPrn.color[1] = 0.596;
    nxPrn.color[2] = 0.051;
    engine->Con_NXPrintf( &nxPrn, "Build Date / Time %s, at %s", __DATE__, __TIME__ );
    nxPrn.index = 2;
    nxPrn.color[0] = 0.5;
    nxPrn.color[1] = 1;
    nxPrn.color[2] = 0.5;
    engine->Con_NXPrintf( &nxPrn, "Version: %s", "Pre-Alpha" );
    nxPrn.index = 3;
    nxPrn.color[0] = 1;
    nxPrn.color[1] = 1;
    nxPrn.color[2] = 1;
    engine->Con_NXPrintf( &nxPrn, "Release Date: TBD" );
    nxPrn.index = 4;
    engine->Con_NXPrintf( &nxPrn, "Codename: %s", "Water" );
    nxPrn.index = 6;
	engine->Con_NXPrintf( &nxPrn, "Permissions: Developers Only" );
	nxPrn.index = 7;
	engine->Con_NXPrintf( &nxPrn, "Copyright EX Team, Not all rights reserved" );
}
