//=========== Copyright © 2014, rHetorical, All rights reserved. =============
//
// Purpose: 
//		
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_numericdisplay.h"

#include "c_vectronic_player.h"
#include "weapon_vecgun.h"

#include "ConVar.h"

//-----------------------------------------------------------------------------
// Purpose: Health panel
//-----------------------------------------------------------------------------
class CHudBallIcons : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudBallIcons, vgui::Panel );

public:
	CHudBallIcons( const char *pElementName );
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void Reset( void );
//	virtual void OnThink (void);
	bool ShouldDraw( void );

	virtual void Paint( void );
	virtual void ApplySchemeSettings( IScheme *scheme );

private:

	CHudTexture *m_pBall0Icon;
	CHudTexture *m_pBall1Icon;
	CHudTexture *m_pBall2Icon;

	CPanelAnimationVarAliasType( float, icon0_xpos, "icon0_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon0_ypos, "icon0_ypos", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, icon1_xpos, "icon1_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon1_ypos, "icon1_ypos", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, icon2_xpos, "icon2_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon2_ypos, "icon2_ypos", "0", "proportional_float" );

	float icon_tall;
	float icon_wide;

	bool m_bDraw;

};	

DECLARE_HUDELEMENT( CHudBallIcons );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudBallIcons::CHudBallIcons( const char *pElementName ) : CHudElement (pElementName), BaseClass (NULL, "HudBallIcons")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent (pParent);

	SetHiddenBits( HIDEHUD_PLAYERDEAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBallIcons::Init()
{
}

void CHudBallIcons::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	//m_cColor = scheme->GetColor( "Normal", Color( 255, 208, 64, 255 ) );

	SetPaintBackgroundEnabled( false );

	if( !m_pBall0Icon || !m_pBall1Icon || !m_pBall2Icon )
	{
		m_pBall0Icon = gHUD.GetIcon( "BallIcon" );
		m_pBall1Icon = gHUD.GetIcon( "BallIcon" );
		m_pBall2Icon = gHUD.GetIcon( "BallIcon" );
	}	

	if( m_pBall0Icon )
	{
		icon_tall = GetTall() - YRES(2);
		float scale = icon_tall / (float)m_pBall0Icon->Height();
		icon_wide = ( scale ) * (float)m_pBall0Icon->Width() + 2;
	}

	if( m_pBall1Icon )
	{
		icon_tall = GetTall() - YRES(2);
		float scale = icon_tall / (float)m_pBall1Icon->Height();
		icon_wide = ( scale ) * (float)m_pBall1Icon->Width() + 2;
	}

	if( m_pBall2Icon )
	{
		icon_tall = GetTall() - YRES(2);
		float scale = icon_tall / (float)m_pBall2Icon->Height();
		icon_wide = ( scale ) * (float)m_pBall2Icon->Width() + 2;
	}

	//SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: reset health to normal color at round restart
//-----------------------------------------------------------------------------
void CHudBallIcons::Reset()
{
//	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthRestored");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBallIcons::VidInit()
{
}
/*
//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudBallIcons:: OnThink (void)
{

//	ShouldDraw();
}
*/
bool CHudBallIcons::ShouldDraw( void )
{
	C_VectronicPlayer *local = C_VectronicPlayer::GetLocalVectronicPlayer();

	if (!local)
		return false;

	if(!local->IsAlive() || local->IsObserver())
		return false;

	if (local->PlayerHasObject())
		return false;

	return true;
}
void CHudBallIcons::Paint( void )
{
	C_VectronicPlayer *pPlayer = C_VectronicPlayer::GetLocalVectronicPlayer();
	if( !pPlayer )
		return;

	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon == NULL )
		return;

	C_WeaponVecgun *pVecgun = To_Vecgun(pWeapon);
	if ( pVecgun == NULL )
		return;

	Color WhiteColor(255,255,255,96);

	// ~FIXFIX: Faded colors are too... faded.
	Color BlueColor(BALL0_COLOR);
	Color FadedBlueColor(134,199,197,150);

	Color GreenColor(BALL1_COLOR);
	Color FadedGreenColor(112,176,109,150);

	Color PurpleColor(BALL2_COLOR);
	Color FadedPurpleColor(136,123,193,150);

	if( !m_pBall0Icon || !m_pBall1Icon || !m_pBall2Icon )
		return;

	int m_intCurrentBall;
	m_intCurrentBall = pVecgun->GetCurrentSelection();

	if (pVecgun->CanFireBlue())
	{
		if ( m_intCurrentBall == 0 || m_intCurrentBall > 2)
		{
			m_pBall0Icon->DrawSelf( icon0_xpos, icon0_ypos, icon_wide, icon_tall, BlueColor );
		}
		else
		{
			m_pBall0Icon->DrawSelf( icon0_xpos, icon0_ypos, icon_wide, icon_tall, FadedBlueColor );
		}
	}
	else
	{
		m_pBall0Icon->DrawSelf( icon0_xpos, icon0_ypos, icon_wide, icon_tall, WhiteColor );
	}

	if (pVecgun->CanFireGreen())
	{
		if ( m_intCurrentBall == 1 )
		{
			m_pBall1Icon->DrawSelf( icon1_xpos, icon1_ypos, icon_wide, icon_tall, GreenColor );
		}
		else
		{
			m_pBall1Icon->DrawSelf( icon1_xpos, icon1_ypos, icon_wide, icon_tall, FadedGreenColor );
		}
	}
	else
	{
		m_pBall1Icon->DrawSelf( icon1_xpos, icon1_ypos, icon_wide, icon_tall, WhiteColor );
	}

	if (pVecgun->CanFireYellow())
	{
		if ( m_intCurrentBall == 2 )
		{
			m_pBall1Icon->DrawSelf( icon2_xpos, icon2_ypos, icon_wide, icon_tall, PurpleColor );
		}
		else
		{
			m_pBall1Icon->DrawSelf( icon2_xpos, icon2_ypos, icon_wide, icon_tall, FadedPurpleColor );
		}
	}
	else
	{
		m_pBall1Icon->DrawSelf( icon2_xpos, icon2_ypos, icon_wide, icon_tall, WhiteColor );
	}

	BaseClass::Paint();
}