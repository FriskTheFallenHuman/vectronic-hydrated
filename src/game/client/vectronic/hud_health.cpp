//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
#include "iclientmode.h"
#include "c_vectronic_player.h"
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define HEALTH_INIT 80 

//-----------------------------------------------------------------------------
// Purpose: Health panel
//-----------------------------------------------------------------------------
class CHudHealth : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE( CHudHealth, CHudNumericDisplay );

public:
	CHudHealth( const char *pElementName );

	virtual void ApplySchemeSettings( IScheme *scheme );
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void Reset( void );
	virtual void OnThink();
			void MsgFunc_Damage( bf_read &msg );

private:
	CHudTexture *m_pHealthIcon;

	CPanelAnimationVarAliasType( float, icon_xpos, "icon_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon_ypos, "icon_ypos", "2", "proportional_float" );
	CPanelAnimationVar( int, m_iHealthDisabledAlpha, "BarDisabledAlpha", "50");
	CPanelAnimationVarAliasType( float, m_flBarInsetX, "BarInsetX", "26", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarInsetY, "BarInsetY", "3", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarWidth, "BarWidth", "84", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarHeight, "BarHeight", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarChunkWidth, "BarChunkWidth", "2", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarChunkGap, "BarChunkGap", "1", "proportional_float" );

	// old variables
	int		m_iHealth;
	Color	 m_HealthColor;
	int		m_bitsDamage;

	float m_flHealth;
	int m_nHealthLow;
	float icon_tall;
	float icon_wide;

protected:
	virtual void Paint();
};	

DECLARE_HUDELEMENT( CHudHealth );
DECLARE_HUD_MESSAGE( CHudHealth, Damage );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudHealth::CHudHealth( const char *pElementName ) : CHudElement( pElementName ), CHudNumericDisplay( NULL, "HudHealth" )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );
	SetPaintBackgroundEnabled( false );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::Init()
{
	HOOK_HUD_MESSAGE( CHudHealth, Damage );
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::Reset()
{
	m_flHealth = HEALTH_INIT;

	m_nHealthLow = -1;
	m_HealthColor = GetFgColor();

	if ( !m_pHealthIcon )
		m_pHealthIcon = gHUD.GetIcon( "item_healthkit" );

	if ( m_pHealthIcon )
	{

		icon_tall = GetTall() - YRES(2);
		float scale = icon_tall / (float)m_pHealthIcon->Height();
		icon_wide = (scale)* (float)m_pHealthIcon->Width();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::VidInit()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::OnThink()
{
	float newHealth = 0;

	C_VectronicPlayer *local = C_VectronicPlayer::GetLocalVectronicPlayer();
	if ( !local )
		return;

	// Never below zero 
	newHealth = max(local->GetHealth(), 0);

	// Only update the fade if we've changed health
	if ( newHealth == m_flHealth )
		return;

	m_flHealth = newHealth;

	if ( newHealth >= 20)
		surface()->DrawSetColor( m_HealthColor );
	else if ( newHealth > 0 )
		surface()->DrawSetColor( 255, 0, 0, 255 );
}

//------------------------------------------------------------------------
// Purpose: draws the power bar
//------------------------------------------------------------------------
void CHudHealth::Paint()
{
	// Get bar chunks
	int chunkCount = m_flBarWidth / (m_flBarChunkWidth + m_flBarChunkGap);
	int enabledChunks = (int)((float)chunkCount * (m_flHealth / 100.0f) + 0.5f);

	// Draw the suit power bar
	surface()->DrawSetColor( m_HealthColor );

	int xpos = m_flBarInsetX, ypos = m_flBarInsetY;

	for ( int i = 0; i < enabledChunks; i++ )
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += ( m_flBarChunkWidth + m_flBarChunkGap );
	}

	// Draw the exhausted portion of the bar.
	surface()->DrawSetColor( Color( m_HealthColor[0], m_HealthColor[1], m_HealthColor[2], m_iHealthDisabledAlpha ) );

	for ( int i = enabledChunks; i < chunkCount; i++ )
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += ( m_flBarChunkWidth + m_flBarChunkGap );
	}

	if ( m_pHealthIcon )
		m_pHealthIcon->DrawSelf( icon_xpos, icon_ypos, icon_wide, icon_tall, GetFgColor() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::MsgFunc_Damage( bf_read &msg )
{

	int armor = msg.ReadByte();	// armor
	int damageTaken = msg.ReadByte();	// health
	long bitsDamage = msg.ReadLong(); // damage bits
	bitsDamage; // variable still sent but not used

	Vector vecFrom;

	vecFrom.x = msg.ReadBitCoord();
	vecFrom.y = msg.ReadBitCoord();
	vecFrom.z = msg.ReadBitCoord();

	// Actually took damage?
	if ( damageTaken > 0 || armor > 0 )
	{
		if ( damageTaken > 0 )
		{
			// start the animation
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthDamageTaken" );
		}
	}
}