//=========== Copyright © 2013, rHetorical, All rights reserved. =============
//
// Purpose: 
//		
//=============================================================================

#ifndef PROP_VECBOX_H
#define PROP_VECBOX_H

#ifdef _WIN32
#pragma once
#endif

#include "player_pickup.h"
#include "baseglowanimating.h"
#include "sprite.h"

#define VECBOX_MODEL "models/props/puzzlebox.mdl"
#define VECBOX_MODEL_GHOST "models/props/puzzlebox_ghost.mdl"

#define VECBOX_CLEAR_PARTICLE "ball_white_hit_edge" 
#define VECBOX_HIT0_PARTICLE "ball_blue_hit"
#define VECBOX_HIT1_PARTICLE "ball_green_hit" 
#define VECBOX_HIT2_PARTICLE "ball_purple_hit"
#define VECBOX_HIT3_PARTICLE "ball_red_hit" 

/*
#define VECBOX_HIT4_PARTICLE "ball_yellow_hit" 
#define VECBOX_HIT5_PARTICLE "ball_orange_hit"
*/
#define VECBOX_SPRITE "sprites/light_glow03.vmt"

//-----------------------------------------------------------------------------
// Purpose: Base
//-----------------------------------------------------------------------------
class CVecBox : public CBaseGlowAnimating, public CDefaultPlayerPickupVPhysics
{
	DECLARE_CLASS( CVecBox, CBaseGlowAnimating );
public:
	
	DECLARE_DATADESC();
	//DECLARE_SERVERCLASS();
	DECLARE_PREDICTABLE();

	bool CreateVPhysics()
	{
		VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
		return true;
	}

	//Constructor
	CVecBox();

	void Spawn( void );
	void Precache( void );
	void Think( void );
	void Touch( CBaseEntity *pOther );
	void UpdateColor();
	void SetupSprites();

	/*
	// Glows
	void				AddGlowEffect( void );
	void				RemoveGlowEffect( void );
	bool				IsGlowEffectActive( void );
*/
	void EffectBlue();
	void EffectGreen();
	void EffectPurple();
	void EffectRed();

	//
	//void EffectYellow();
	//void EffectOrange();

	bool IsProtected(){return m_bIsProtected;};
	int m_intProtecting;

	void ResetBox();

	void OnDissolve();

	void MakeGhost();
	void MakeGhost2();
	void KillGhost();

	// Use
	void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	void OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t reason );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps();

	//Clear effects
	void Clear();

	unsigned char GetState() const { return m_intLastBallHit;	}

//	void InputEnableActivations ( inputdata_t &inputData );
//	void InputDisableActivations ( inputdata_t &inputData );
	void InputDissolve( inputdata_t &inputData );
	void InputExplode( inputdata_t &inputData );
	void InputKillGhost( inputdata_t &inputData );

	//Plate SkinToggle
	//void InputPlateToggleSkin( inputdata_t &inputData );

/*
protected:
	void SetGlowVector(float r, float g, float b );
	CNetworkVar( bool, m_bGlowEnabled );
	CNetworkVar( float, m_flRedGlowColor );
	CNetworkVar( float, m_flGreenGlowColor );
	CNetworkVar( float, m_flBlueGlowColor );
	*/

private:

	CHandle<CVecBox> m_hBox;

	bool	m_bActivated;
	bool	m_bIsGhost; //Boo! I'm a Ghost!
	bool	m_bHasGhost; // We have a Ghost! AHH!
	bool	m_bIsProtected; // Do we have a sheild?

	bool	m_bGib; //Our Gibs  

	bool m_bIsTouching;
	int m_intLastBallHit;

	bool m_bSpawnedSprites;

enum LastHitBy_t
{
	VECTRONIC_BALL_NONE			= 0,	// We were not hit by any ball last time.
	VECTRONIC_BALL_BLUE			= 1,	// We were hit by the blue ball last time.
	VECTRONIC_BALL_GREEN		= 2,	// We were hit by the green ball last time.
	VECTRONIC_BALL_PURPLE		= 3,	// We were hit by the purple ball last time.
	VECTRONIC_BALL_RED			= 4,	// We were hit by the red ball last time.

//	VECTRONIC_BALL_YELLOW		= 5,	// We were hit by the yellow ball last time.
//	VECTRONIC_BALL_ORANGE		= 6,	// We were hit by the orange ball last time.
};

	int m_intStartActivation;

	float		m_flConeDegrees;
	CHandle<CBasePlayer>	m_hPhysicsAttacker;
	CHandle<CSprite>		m_hSprite[6];

	// 0 = none
	// 1 = 0
	// 2 = 1 ... etc
	Vector MdlTop;

	// Pickup
	COutputEvent m_OnPlayerUse;
	COutputEvent m_OnPlayerPickup;
	COutputEvent m_OnPhysGunPickup;
	COutputEvent m_OnPhysGunDrop;

	// Punt Outputs
	COutputEvent m_OnBlueBall;
	COutputEvent m_OnGreenBall;
	COutputEvent m_OnPurpleBall;	
	COutputEvent m_OnRedBall;

	//COutputEvent m_OnYellowBall;
	//COutputEvent m_OnOrangeBall;

	COutputEvent m_OnReset;

	// Death!
	COutputEvent m_OnFizzled;
	COutputEvent m_OnExplode;

};
#endif // PROP_VECBOX_H
