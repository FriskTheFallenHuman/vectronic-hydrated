//=========== Copyright © 2014, rHetorical, All rights reserved. =============
//
// Purpose: 
//
//=============================================================================

#ifndef PROP_VECBALL_DISPENSER_H
#define PROP_VECBALL_DISPENSER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "prop_vectronic_projectile.h"
#include "baseanimating.h"
#include "sprite.h"
#include "particle_parse.h"
#include "particle_system.h"

//----------------------------------------------------------
// Particle attachment spawning. Read InputTestEntity for info. 
//----------------------------------------------------------
class CVecBallDispenser : public CBaseAnimating
{
public:
	DECLARE_CLASS( CVecBallDispenser, CBaseAnimating );
	DECLARE_DATADESC();
 
	CVecBallDispenser()
	{
		m_bDisabled = false;
	}

	void Spawn( void );
	void Precache( void );
	void Touch( CBaseEntity *pOther );

	// Sound
	void CreateSounds();
	void StopLoopingSounds();

	// Start enabled?
//	bool m_bEnabled;
	bool m_bDispatched;

	void TurnOn();

//	void SetupSprite();
//	bool m_bSpawnedSprite;

	// Functions to call for the right ball dispensers.
	void EnableBall0();
	void EnableBall1();
	void EnableBall2();

	void InputRespawn ( inputdata_t &inputData );

private:
 
	int	 m_intType; // 0 = Blue, 1 = Green
	bool m_bDisabled;

	CSoundPatch		*m_pLoopSound;

	CHandle<CParticleSystem> m_hParticleEffect;
	CHandle<CParticleSystem> m_hBaseParticleEffect;
	//CHandle<CSprite> m_hSprite;
	//CHandle<CWeaponVecgun> m_hVecgun;

	COutputEvent m_OnBallEquipped;
};

#endif // PROP_VECBALL_DISPENSER_H