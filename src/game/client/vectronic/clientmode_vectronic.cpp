//=========== Copyright © 2014, rHetorical, All rights reserved. =============
//
// Purpose: Draws the normal TF2 or HL2 HUD.
//
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "clientmode_vectronic.h"
#include "cdll_client_int.h"
#include "iinput.h"
#include "vgui/isurface.h"
#include "vgui/ipanel.h"
#include <vgui_controls/AnimationController.h>
#include "BuyMenu.h"
#include "filesystem.h"
#include "vgui/ivgui.h"
#include "hud_basechat.h"
#include "view_shared.h"
#include "view.h"
#include "ivrenderview.h"
#include "model_types.h"
#include "iefx.h"
#include "dlight.h"
#include <imapoverview.h>
#include "c_playerresource.h"
#include <keyvalues.h>
#include "text_message.h"
#include "panelmetaclassmgr.h"
#include "basevectroniccombatweapon_shared.h"
#include "c_vectronic_player.h"
#include "c_weapon__stubs.h"		//Tony; add stubs
#include "voice_status.h"
#include "voice_gamemgr.h"
#if defined( GLOWS_ENABLE )
#include "clienteffectprecachesystem.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SCREEN_FILE		"scripts/vgui_screens.txt"

class CHudChat;

// The current client mode. Always ClientModeNormal in HL.
IClientMode *g_pClientMode = NULL;

ConVar fov_desired( "fov_desired", "75", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets the base field-of-view.", true, 75.0, true, (float)MAX_FOV );
ConVar default_fov( "default_fov", "90", FCVAR_CHEAT );

#if defined( GLOWS_ENABLE )
CLIENTEFFECT_REGISTER_BEGIN( PrecachePostProcessingEffectsGlow )
	CLIENTEFFECT_MATERIAL( "dev/glow_color" )
	CLIENTEFFECT_MATERIAL( "dev/halo_add_to_screen" )
CLIENTEFFECT_REGISTER_END_CONDITIONAL( engine->GetDXSupportLevel() >= 90 )
#endif

//Tony; add stubs for cycler weapon and cubemap.
STUB_WEAPON_CLASS( cycler_weapon,   WeaponCycler,   C_BaseCombatWeapon );
STUB_WEAPON_CLASS( weapon_cubemap,  WeaponCubemap,  C_BaseCombatWeapon );

//-----------------------------------------------------------------------------
// HACK: the detail sway convars are archive, and default to 0.  Existing CS:S players thus have no detail
// prop sway.  We'll force them to DoD's default values for now.  What we really need in the long run is
// a system to apply changes to archived convars' defaults to existing players.
extern ConVar cl_detail_max_sway;
extern ConVar cl_detail_avoid_radius;
extern ConVar cl_detail_avoid_force;
extern ConVar cl_detail_avoid_recover_speed;

// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
	static ClientModeVectronicNormal g_ClientModeNormal;
	return &g_ClientModeNormal;
}

ClientModeVectronicNormal* GetClientModeVectronicNormal()
{
	Assert( dynamic_cast< ClientModeVectronicNormal* >( GetClientModeNormal() ) );

	return static_cast< ClientModeVectronicNormal* >( GetClientModeNormal() );
}

// --------------------------------------------------------------------------------- //
// CVectronicModeManager implementation.
// --------------------------------------------------------------------------------- //
void CVectronicModeManager::Init( void )
{
	g_pClientMode = GetClientModeNormal();

	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );

	GetClientVoiceMgr()->SetHeadLabelOffset( 40 );
}

void CVectronicModeManager::LevelInit( const char *newmap )
{
	g_pClientMode->LevelInit( newmap );

	// HACK: the detail sway convars are archive, and default to 0.  Existing CS:S players thus have no detail
	// prop sway.  We'll force them to DoD's default values for now.
	if ( !cl_detail_max_sway.GetFloat() &&
		!cl_detail_avoid_radius.GetFloat() &&
		!cl_detail_avoid_force.GetFloat() &&
		!cl_detail_avoid_recover_speed.GetFloat() )
	{
		cl_detail_max_sway.SetValue( "5" );
		cl_detail_avoid_radius.SetValue( "64" );
		cl_detail_avoid_force.SetValue( "0.4" );
		cl_detail_avoid_recover_speed.SetValue( "0.25" );
	}
}

void CVectronicModeManager::LevelShutdown( void )
{
	g_pClientMode->LevelShutdown();
}

static CVectronicModeManager g_VectronicModeManager;
IVModeManager *modemanager = &g_VectronicModeManager;

// --------------------------------------------------------------------------------- //
// CHudViewport implementation.
// --------------------------------------------------------------------------------- //
void CHudViewport::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	gHUD.InitColors( pScheme );
	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// ClientModeVectronicNormal implementation
//-----------------------------------------------------------------------------
ClientModeVectronicNormal::ClientModeVectronicNormal()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

//-----------------------------------------------------------------------------
// Purpose: Use of glow render effect.
//-----------------------------------------------------------------------------
bool ClientModeVectronicNormal::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{
	if ( !BaseClass::DoPostScreenSpaceEffects( pSetup ) )
		return false;

	CMatRenderContextPtr pRenderContext( materials );

#if defined( GLOWS_ENABLE )
	g_GlowObjectManager.RenderGlowEffects( pSetup, 0 );
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeVectronicNormal::~ClientModeVectronicNormal()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeVectronicNormal::Init()
{
	BaseClass::Init();
}

