//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Hud element that indicates the direction of damage taken by the player
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "text_message.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "IEffects.h"
#include "hudelement.h"
#include "vectronic_gamerules.h"
#include "c_baseplayer.h"
#include "idebugoverlaypanel.h"
#include "engine/ivdebugoverlay.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define NUM_DAMAGE_MATERIAL 9

#define START_SLASH_MATERIAL 1
#define END_SLASH_MATERIAL 4

#define NUM_GENERIC_MATERIAL 5
#define NUM_BLAST_MATERIAL 6

#define START_BULLET_MATERIAL 7
#define END_BULLET_MATERIAL 8

class CHudDamageIndicator : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudDamageIndicator, vgui::Panel );
public:
	CHudDamageIndicator( const char *pElementName );
	void Init( void );
	void VidInit( void ) {}
	void Reset( void );
	virtual bool ShouldDraw( void );

	// Handler for our message
	void MsgFunc_Damage( bf_read &msg );

private:
	virtual void OnThink() {}
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	const Vector& GetMarineOrigin();

	// Painting
	void GetDamagePosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation );
	void DrawDamageIndicator( int iMaterial, int x0, int y0, int x1, int y1, float alpha, float flRotation );
	void DrawFriendlyFireIcon( int xpos, int ypos, float alpha );

private:
	// Indication times

	CPanelAnimationVarAliasType( float, m_flMinimumWidth, "MinimumWidth", "800", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flMaximumWidth, "MaximumWidth", "800", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flMinimumHeight, "MinimumHeight", "400", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flMaximumHeight, "MaximumHeight", "400", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flStartRadius, "StartRadius", "220", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flEndRadius, "EndRadius", "200", "proportional_float" );

	CPanelAnimationVarAliasType( int, m_nScreenFlash, "ScreenFlash", "hud/damage/damage_screen01", "textureid" );
	CPanelAnimationVarAliasType( int, m_nFFIcon, "FFIcon", "hud/damage/damage_FF_icon", "textureid" ); //"VGUI/icon_arrow"

	CPanelAnimationVar( float, m_iMaximumDamage, "MaximumDamage", "100" );
	CPanelAnimationVar( float, m_flMinimumTime, "MinimumTime", "1" );
	CPanelAnimationVar( float, m_flMaximumTime, "MaximumTime", "2" );
	CPanelAnimationVar( float, m_flTravelTime, "TravelTime", ".1" );
	CPanelAnimationVar( float, m_flFadeOutPercentage, "FadeOutPercentage", "0.7" );
	CPanelAnimationVar( float, m_flNoise, "Noise", "0.1" );

	// List of damages we've taken
	struct damage_t
	{
		int		iScale;
		float	flLifeTime;
		float	flStartTime;
		Vector	vecDelta;	// Damage origin relative to the player
		int		material;
		int		damagetype;
		bool	bFriendlyFire;
	};
	CUtlVector<damage_t>	m_vecDamages;

	CMaterialReference m_WhiteAdditiveMaterial[NUM_DAMAGE_MATERIAL];
};

ConVar hud_damage_hurt_flash_alpha( "hud_damage_hurt_flash_alpha", "0.5", FCVAR_NONE, "The alpha of the screen flash when the player is at low health (0.0 - 1.0)" );
ConVar hud_damage_scale( "hud_damage_scale", "0.9f", FCVAR_NONE, "Scales the HUD damage indicator" );
ConVar hud_damage_alpha( "hud_damage_alpha", "0.05f", FCVAR_NONE, "Alpha of the HUD damage indicator" );
ConVar hud_damage_slash_scale( "hud_damage_slash_scale", "0.25f", FCVAR_NONE, "Scales the slashes in the HUD damage indicator" );
ConVar hud_damage_slash_radius( "hud_damage_slash_radius", "0.28f", FCVAR_NONE, "Scales the slashes in the HUD damage indicator" );
ConVar hud_damage_slash_alpha( "hud_damage_slash_alpha", "0.85f", FCVAR_NONE, "Scales alpha of the slashes in the HUD damage indicator." );
ConVar hud_damage_bullets_alpha( "hud_damage_bullets_alpha", "0.05f", FCVAR_NONE, "Scales alpha of the bullets in the HUD damage indicator." );

DECLARE_HUDELEMENT( CHudDamageIndicator );
DECLARE_HUD_MESSAGE( CHudDamageIndicator, Damage );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudDamageIndicator::CHudDamageIndicator( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDamageIndicator" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH );

	m_WhiteAdditiveMaterial[0].Init( "hud/damage/damagecensor03", TEXTURE_GROUP_VGUI ); 

	m_WhiteAdditiveMaterial[1].Init( "hud/damage/damage_slash01", TEXTURE_GROUP_VGUI ); 
	m_WhiteAdditiveMaterial[2].Init( "hud/damage/damage_slash02", TEXTURE_GROUP_VGUI ); 
	m_WhiteAdditiveMaterial[3].Init( "hud/damage/damage_slash03", TEXTURE_GROUP_VGUI ); 
	m_WhiteAdditiveMaterial[4].Init( "hud/damage/damage_slash04", TEXTURE_GROUP_VGUI ); 
	m_WhiteAdditiveMaterial[5].Init( "hud/damage/damage_generic01", TEXTURE_GROUP_VGUI ); 
	m_WhiteAdditiveMaterial[6].Init( "hud/damage/damage_blast01", TEXTURE_GROUP_VGUI ); 
	m_WhiteAdditiveMaterial[7].Init( "hud/damage/damage_bullet01", TEXTURE_GROUP_VGUI ); 
	m_WhiteAdditiveMaterial[8].Init( "hud/damage/damage_bullet02", TEXTURE_GROUP_VGUI ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDamageIndicator::Init( void )
{
	HOOK_HUD_MESSAGE( CHudDamageIndicator, Damage );
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDamageIndicator::Reset( void )
{
	m_vecDamages.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudDamageIndicator::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	float percentDead = 1.0 - clamp( (float)((float)pPlayer->GetHealth() / (float)pPlayer->GetMaxHealth()), 0.0f, 1.0f);
	if ( percentDead >= 0.66f )
		return true;

	// Don't draw if we don't have any damage to indicate
	if ( !m_vecDamages.Count() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Convert a damage position in world units to the screen's units
//-----------------------------------------------------------------------------
void CHudDamageIndicator::GetDamagePosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation )
{
	// Player Data
	Vector playerPosition = MainViewOrigin();
	QAngle playerAngles = MainViewAngles();

	Vector forward, right, up(0,0,1);
	AngleVectors (playerAngles, &forward, NULL, NULL );
	forward.z = 0;
	VectorNormalize(forward);
	CrossProduct( up, forward, right );
	float front = DotProduct(vecDelta, forward);
	float side = DotProduct(vecDelta, right);
	*xpos = flRadius * -side;
	*ypos = flRadius * -front;

	// Get the rotation (yaw)
	*flRotation = atan2(*xpos,*ypos) + M_PI;
	*flRotation *= 180 / M_PI;

	float yawRadians = -(*flRotation) * M_PI / 180.0f;
	float ca = cos( yawRadians );
	float sa = sin( yawRadians );
				 
	// Rotate it around the circle
	*xpos = (int)((ScreenWidth() / 2) + (flRadius * sa));
	*ypos = (int)((ScreenHeight() / 2) - (flRadius * ca));
}

//-----------------------------------------------------------------------------
// Purpose: Draw a single damage indicator
//-----------------------------------------------------------------------------
void CHudDamageIndicator::DrawDamageIndicator(int iMaterial, int x0, int y0, int x1, int y1, float alpha, float flRotation )
{
	CMatRenderContextPtr pRenderContext( materials );
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_WhiteAdditiveMaterial[iMaterial] );

	// Get the corners, since they're being rotated
	int wide = x1 - x0;
	int tall = y1 - y0;
	Vector2D vecCorners[4];
	Vector2D center( x0 + (wide * 0.5f), y0 + (tall * 0.5f) );
	float yawRadians = -flRotation * M_PI / 180.0f;
	Vector2D axis[2];
	axis[0].x = cos(yawRadians);
	axis[0].y = sin(yawRadians);
	axis[1].x = -axis[0].y;
	axis[1].y = axis[0].x;
	Vector2DMA( center, -0.5f * wide, axis[0], vecCorners[0] );
	Vector2DMA( vecCorners[0], -0.5f * tall, axis[1], vecCorners[0] );
	Vector2DMA( vecCorners[0], wide, axis[0], vecCorners[1] );
	Vector2DMA( vecCorners[1], tall, axis[1], vecCorners[2] );
	Vector2DMA( vecCorners[0], tall, axis[1], vecCorners[3] );

//	float zpos = 0;

	// Draw the sucker
	CMeshBuilder meshBuilder;
	meshBuilder.Begin(pMesh, MATERIAL_QUADS, 1);

	int iAlpha = alpha * 255;
	meshBuilder.Color4ub(255, 255, 255, iAlpha);
	meshBuilder.TexCoord2f(0, 0, 0);
	meshBuilder.Position3f(vecCorners[0].x, vecCorners[0].y, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub(255, 255, 255, iAlpha);
	meshBuilder.TexCoord2f(0, 1, 0);
	meshBuilder.Position3f(vecCorners[1].x, vecCorners[1].y, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub(255, 255, 255, iAlpha);
	meshBuilder.TexCoord2f(0, 1, 1);
	meshBuilder.Position3f(vecCorners[2].x, vecCorners[2].y, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub(255, 255, 255, iAlpha);
	meshBuilder.TexCoord2f(0, 0, 1);
	meshBuilder.Position3f(vecCorners[3].x, vecCorners[3].y, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

void CHudDamageIndicator::DrawFriendlyFireIcon( int xpos, int ypos, float alpha )
{
	int size = 46;
	int iXPos = xpos - (size/2);
	int iYPos = ypos - (size/2);

	vgui::surface()->DrawSetColor(Color( 255, 255, 255, alpha ));
	vgui::surface()->DrawSetTexture( m_nFFIcon );
	vgui::Vertex_t icon[4] = 
	{ 
		vgui::Vertex_t( Vector2D(iXPos,			iYPos),				Vector2D(0, 0) ), 
		vgui::Vertex_t( Vector2D(iXPos+size,	iYPos),				Vector2D(1, 0) ),
		vgui::Vertex_t( Vector2D(iXPos+size,	iYPos+size),		Vector2D(1, 1) ), 
		vgui::Vertex_t( Vector2D(iXPos,			iYPos+size),		Vector2D(0, 1) )
	}; 
	vgui::surface()->DrawTexturedPolygon( 4, icon );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDamageIndicator::Paint()
{
	// Iterate backwards, because we might remove them as we go
	int iSize = m_vecDamages.Count();
	for (int i = iSize-1; i >= 0; i--)
	{
		// Scale size to the damage
		int clampedDamage = clamp( m_vecDamages[i].iScale, 0, m_iMaximumDamage );

		float fMinWidth = m_flMinimumWidth * hud_damage_scale.GetFloat();
		float fMaxWidth = m_flMaximumWidth * hud_damage_scale.GetFloat();
		float fMinHeight = m_flMinimumHeight * hud_damage_scale.GetFloat();
		float fMaxHeight = m_flMaximumHeight * hud_damage_scale.GetFloat();

		// scale slashes
		if ( m_vecDamages[i].material >= START_SLASH_MATERIAL && m_vecDamages[i].material <= NUM_GENERIC_MATERIAL )
		{
			fMinWidth *= hud_damage_slash_scale.GetFloat() * 0.2f;
			fMaxWidth *= hud_damage_slash_scale.GetFloat() * 0.2f;
			fMinHeight *= hud_damage_slash_scale.GetFloat();
			fMaxHeight *= hud_damage_slash_scale.GetFloat();
		}
		else if ( m_vecDamages[i].material == NUM_BLAST_MATERIAL )
		{
			fMinWidth *= hud_damage_slash_scale.GetFloat() * 0.8f;
			fMaxWidth *= hud_damage_slash_scale.GetFloat() * 0.8f;
			fMinHeight *= hud_damage_slash_scale.GetFloat();
			fMaxHeight *= hud_damage_slash_scale.GetFloat();
		}

		int iWidth = RemapVal(clampedDamage, 0, m_iMaximumDamage, fMinWidth, fMaxWidth ) * 0.5;
		int iHeight = RemapVal(clampedDamage, 0, m_iMaximumDamage, fMinHeight, fMaxHeight ) * 0.5;

		// Find the place to draw it
		float xpos, ypos;
		float flRotation;
		float flTimeSinceStart = ( gpGlobals->curtime - m_vecDamages[i].flStartTime );
		float flRadius = RemapVal( MIN( flTimeSinceStart, m_flTravelTime ), 0, m_flTravelTime, m_flStartRadius, m_flEndRadius );


		if ( m_vecDamages[i].damagetype & DMG_BULLET )
		{
			flRadius = m_flEndRadius;
			iWidth = iHeight;
		}

		flRadius *= hud_damage_scale.GetFloat();
		// scale slashes
		if ( m_vecDamages[i].material > 0 )
		{
			flRadius *= hud_damage_slash_radius.GetFloat();
		}

		GetDamagePosition( m_vecDamages[i].vecDelta, flRadius, &xpos, &ypos, &flRotation );

		float flAnimWidth = iWidth;

		// Calculate life left
		float flLifeLeft = ( m_vecDamages[i].flLifeTime - gpGlobals->curtime );
		if ( flLifeLeft > 0 )
		{
			float flPercent = flTimeSinceStart / (m_vecDamages[i].flLifeTime - m_vecDamages[i].flStartTime);
			float alpha;
			if ( flPercent <= m_flFadeOutPercentage )
			{
				alpha = 1.0;
			}
			else
			{
				alpha = 1.0 - RemapVal( flPercent, m_flFadeOutPercentage, 1.0, 0.0, 1.0 );

				if ( m_vecDamages[i].material >= START_SLASH_MATERIAL && m_vecDamages[i].material <= NUM_GENERIC_MATERIAL )
					flAnimWidth = iWidth * alpha;
			}

			if ( m_vecDamages[i].damagetype & DMG_BULLET )
			{
				alpha *= hud_damage_bullets_alpha.GetFloat();
			}
			else if ( m_vecDamages[i].material > 0 )
			{
				alpha *= hud_damage_slash_alpha.GetFloat();
			}
			else
			{
				alpha *= hud_damage_alpha.GetFloat();
			}
			DrawDamageIndicator( m_vecDamages[i].material, xpos-flAnimWidth, ypos-iHeight, xpos+flAnimWidth, ypos+iHeight, alpha, flRotation );
			
			if ( m_vecDamages[i].material > 0 && m_vecDamages[i].bFriendlyFire )
				DrawFriendlyFireIcon( xpos, ypos, alpha );
		}
		else
		{
			m_vecDamages.Remove(i);
		}
	}

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	float percentDead = 1.0 - clamp( (float)((float)pPlayer->GetHealth() / (float)pPlayer->GetMaxHealth()), 0.0f, 1.0f);
	float bgalpha = ((245 * 1) * percentDead) * hud_damage_hurt_flash_alpha.GetFloat();

	int w = ScreenWidth();
	int t = ScreenHeight();

	// draw the BG first
	vgui::surface()->DrawSetColor(Color( 255, 255, 255, bgalpha ));
	vgui::surface()->DrawSetTexture( m_nScreenFlash );
	vgui::Vertex_t screen[4] = 
	{ 
		vgui::Vertex_t( Vector2D(0,		0),			Vector2D(0, 0) ), 
		vgui::Vertex_t( Vector2D(w,		0),			Vector2D(1, 0) ),
		vgui::Vertex_t( Vector2D(w,		t),			Vector2D(1, 1) ), 
		vgui::Vertex_t( Vector2D(0,		t),			Vector2D(0, 1) )
	}; 
	vgui::surface()->DrawTexturedPolygon( 4, screen );
}

const Vector& CHudDamageIndicator::GetMarineOrigin()
{
	return MainViewOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for Damage message
//-----------------------------------------------------------------------------
void CHudDamageIndicator::MsgFunc_Damage( bf_read &msg )
{
	damage_t damage;
	damage.iScale = msg.ReadShort();
	damage.damagetype = msg.ReadLong();

	if ( damage.iScale > m_iMaximumDamage )
		damage.iScale = m_iMaximumDamage;

	Vector vecOrigin;
	msg.ReadBitVec3Coord( vecOrigin );
	damage.bFriendlyFire = msg.ReadOneBit() ? true : false;

	damage.flStartTime = gpGlobals->curtime;
	damage.flLifeTime = gpGlobals->curtime + RemapVal(damage.iScale, 0, m_iMaximumDamage, m_flMinimumTime, m_flMaximumTime);

	if ( vecOrigin == vec3_origin )
		vecOrigin = GetMarineOrigin();

	damage.vecDelta = (vecOrigin - GetMarineOrigin());
	VectorNormalize( damage.vecDelta );

	// Add some noise
	damage.vecDelta[0] += random->RandomFloat( -m_flNoise, m_flNoise );
	damage.vecDelta[1] += random->RandomFloat( -m_flNoise, m_flNoise );
	damage.vecDelta[2] += random->RandomFloat( -m_flNoise, m_flNoise );

	// do 2D
	damage.vecDelta[2] = 0;

	VectorNormalize( damage.vecDelta );

	damage.material = 0;	// curve for the first

	m_vecDamages.AddToTail( damage );

	if ( damage.damagetype & DMG_SLASH )
	{
		// add a slash for drones
		damage.material = RandomInt(START_SLASH_MATERIAL, END_SLASH_MATERIAL );
		damage.vecDelta[0] += random->RandomFloat( -m_flNoise * 5, m_flNoise * 5 );
		damage.vecDelta[1] += random->RandomFloat( -m_flNoise * 5, m_flNoise * 5 );

		m_vecDamages.AddToTail( damage );
	}
	else if ( damage.damagetype & DMG_BLAST )
	{
		// add a slash for drones
		damage.material = NUM_BLAST_MATERIAL;
		damage.vecDelta[0] += random->RandomFloat( -m_flNoise * 5, m_flNoise * 5 );
		damage.vecDelta[1] += random->RandomFloat( -m_flNoise * 5, m_flNoise * 5 );

		m_vecDamages.AddToTail( damage );
	}
	else if ( damage.damagetype & DMG_BULLET )
	{
		// add a bullet for for drones
		damage.material = RandomInt(START_BULLET_MATERIAL, END_BULLET_MATERIAL );
		damage.vecDelta[0] += random->RandomFloat( -m_flNoise * 5, m_flNoise * 5 );
		damage.vecDelta[1] += random->RandomFloat( -m_flNoise * 5, m_flNoise * 5 );

		m_vecDamages.AddToTail( damage );
	}
	else
	{
		// add a slash for drones
		damage.material = NUM_GENERIC_MATERIAL;
		damage.vecDelta[0] += random->RandomFloat( -m_flNoise * 5, m_flNoise * 5 );
		damage.vecDelta[1] += random->RandomFloat( -m_flNoise * 5, m_flNoise * 5 );

		m_vecDamages.AddToTail( damage );
	}
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudDamageIndicator::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	// set our size
	int screenWide, screenTall;
	int x, y;
	GetPos(x, y);
	GetHudSize(screenWide, screenTall);
	SetBounds(0, y, screenWide, screenTall - y);
}