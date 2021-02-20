//=========== Copyright © 2013, rHetorical, All rights reserved. =============
//
// Purpose: 
//		
//=============================================================================
#ifndef WEAPON_VECGUN_H
#define WEAPON_VECGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "basevectroniccombatweapon_shared.h"
#include "sprite.h"

#ifndef CLIENT_DLL
	#include "prop_vectronic_projectile.h"
	#include "basecombatcharacter.h"
#endif

#ifdef CLIENT_DLL
	#define CWeaponVecgun C_WeaponVecgun
#endif

#define MAX_SLOT 2
#define	REFIRE_TIME		0.1f
#define DELAY_TIME		50

//-----------------------------------------------------------------------------
// CWeaponVecgun
//-----------------------------------------------------------------------------
class CWeaponVecgun : public CBaseVectronicCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponVecgun, CBaseVectronicCombatWeapon );

	CWeaponVecgun( void );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

	void	Precache( void );
	void	Think( void );
	void	ItemPostFrame( void );
	void	ItemPreFrame( void );
	void	ItemBusyFrame( void );
	void	AddViewKick( void );
	void	DryFire( void );
	void	Shake();
	void	UpdatePenaltyTime( void );

	void	PrimaryAttack( void ); // Fire the selected ball.
	void	SecondaryAttack( void ); // Change the selection

	void	FireSelectedBall( void ); // Fire the selected ball.
	void	ChangeBall( void );
	void	TestSlot( void );

	//Granting Punts!
	void SetCanFireBlue();
	void SetCanFireGreen();
	void SetCanFireYellow();

	//Clear!
	void ClearGun();
	void ClearBlue();
	void ClearGreen();
	void ClearYellow();

	virtual int	GetMinBurst() { return 1; }
	virtual int	GetMaxBurst() { return 3; }
	virtual float GetFireRate( void ) {	return 0.5f; }

	int	GetNumShots() {	return m_nNumShotsFired; };
	float GetDelay() { return m_nDelay; };

	CNetworkVar( bool,	m_bCanFireBlue );
	CNetworkVar( bool,	m_bCanFireGreen );
	CNetworkVar( bool,	m_bCanFireYellow );

	bool	CanFireBlue(){return m_bCanFireBlue;};
	bool	CanFireGreen(){return m_bCanFireGreen;};
	bool	CanFireYellow(){return m_bCanFireYellow;};

	CNetworkVar( float,	m_nDelay );
	CNetworkVar( int,	m_nCurrentSelection );
	CNetworkVar( int,	m_nNumShotsFired );

	int	GetCurrentSelection() { return m_nCurrentSelection;	};
	void SetJumpToSelection( int intselect )	{ m_nCurrentSelection = intselect; };

private:

	float	m_flSoonestAttack;
	float	m_flLastAttackTime;
	float	m_flAccuracyPenalty;

	int		m_intOnlyBall;

	bool	m_bResetting;

	bool	m_bWasFired;

	//int		m_nCurrentSelection;

	enum BallSelection
	{
		VECTRONIC_BALL_BLUE			= 0,	// We were hit by the blue ball last time.
		VECTRONIC_BALL_GREEN		= 1,	// We were hit by the green ball last time.
		VECTRONIC_BALL_YELLOW		= 2,	// We were hit by the yellow ball last time.
	};
};

inline CWeaponVecgun* To_Vecgun( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;
	Assert( dynamic_cast< CWeaponVecgun* >( pEntity ) != NULL );
	return static_cast< CWeaponVecgun* >( pEntity );
}

#endif // WEAPON_VECGUN_H