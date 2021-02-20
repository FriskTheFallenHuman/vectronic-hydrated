//======== Copyright © 2013 - 2014, rHetorical, All rights reserved. ==========
//
// Purpose:
//		
//=============================================================================
#include "cbase.h"
#include <convar.h>
#include "gamerules.h"
#include "in_buttons.h"
#include "sprite.h"
#include "particle_parse.h"
#include "weapon_vecgun.h"
#include "rumble_shared.h"

#ifdef CLIENT_DLL
	#include "c_vectronic_player.h"
#else
	#include "vectronic_player.h"
	#include "player.h"
	#include "soundent.h"
	#include "game.h"
#endif

LINK_ENTITY_TO_CLASS( weapon_vecgun, CWeaponVecgun );
PRECACHE_WEAPON_REGISTER( weapon_vecgun );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponVecgun, DT_WeaponVecgun )

BEGIN_NETWORK_TABLE( CWeaponVecgun, DT_WeaponVecgun )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bCanFireBlue) ),
	RecvPropBool( RECVINFO( m_bCanFireGreen ) ),
	RecvPropBool( RECVINFO( m_bCanFireYellow ) ),
	RecvPropInt( RECVINFO( m_nNumShotsFired ) ),
	RecvPropInt( RECVINFO( m_nCurrentSelection ) ),
	RecvPropInt( RECVINFO( m_nDelay ) ),
#else
	SendPropBool( SENDINFO( m_bCanFireBlue ) ),
	SendPropBool( SENDINFO( m_bCanFireGreen ) ),
	SendPropBool( SENDINFO( m_bCanFireYellow ) ),
	SendPropInt ( SENDINFO( m_nNumShotsFired ), 8, SPROP_UNSIGNED ),
	SendPropInt ( SENDINFO( m_nCurrentSelection ), 8, SPROP_UNSIGNED ),
	SendPropInt ( SENDINFO( m_nDelay ), 8, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponVecgun )
END_PREDICTION_DATA()

#ifndef CLIENT_DLL
BEGIN_DATADESC( CWeaponVecgun )
	DEFINE_FIELD( m_flSoonestAttack, FIELD_TIME ),
	DEFINE_FIELD( m_flLastAttackTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flAccuracyPenalty,		FIELD_FLOAT ), //NOTENOTE: This is NOT tracking game time
	DEFINE_FIELD( m_nNumShotsFired,			FIELD_INTEGER ),
	DEFINE_FIELD( m_nCurrentSelection,		FIELD_INTEGER ),
	DEFINE_FIELD( m_nDelay,					FIELD_FLOAT ),

	// What can we fire?
	DEFINE_FIELD( m_bCanFireBlue, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bCanFireGreen, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bCanFireYellow, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_bWasFired, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_bResetting, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_intOnlyBall,		FIELD_INTEGER ),

END_DATADESC()
#endif

acttable_t	CWeaponVecgun::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_MP_STAND_PRIMARY,					false },
	{ ACT_MP_RUN,						ACT_MP_RUN_PRIMARY,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_MP_CROUCH_PRIMARY,					false },
	{ ACT_MP_CROUCHWALK,				ACT_MP_CROUCHWALK_PRIMARY,				false },
	{ ACT_MP_JUMP_START,				ACT_MP_JUMP_START_PRIMARY,				false },
	{ ACT_MP_JUMP_FLOAT,				ACT_MP_JUMP_FLOAT_PRIMARY,				false },
	{ ACT_MP_JUMP_LAND,					ACT_MP_JUMP_LAND_PRIMARY,				false },
	{ ACT_MP_AIRWALK,					ACT_MP_AIRWALK_PRIMARY,					false },
};
IMPLEMENT_ACTTABLE( CWeaponVecgun );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponVecgun::CWeaponVecgun( void )
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_fMinRange1		= 24;
	m_fMaxRange1		= 1500;
	m_fMinRange2		= 24;
	m_fMaxRange2		= 200;

	m_bFiresUnderwater	= true;

	//Ok, atleast for now, the puntgun can not fire anything.
	m_bCanFireBlue = false;
	m_bCanFireGreen = false;
	m_bCanFireYellow = false;

	m_bWasFired = false;

	m_nCurrentSelection = VECTRONIC_BALL_BLUE;
	
	//SetNextThink( gpGlobals->curtime );
	m_nDelay = DELAY_TIME;
	AddEffects( EF_NODRAW );

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponVecgun::Precache( void )
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "prop_particle_ball" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponVecgun::Think( void )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponVecgun::DryFire( void )
{
	WeaponSound( EMPTY );

	m_flSoonestAttack			= gpGlobals->curtime + 0.1f;
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack		= gpGlobals->curtime + SequenceDuration();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponVecgun::ChangeBall( void )
{
	if( m_intOnlyBall >= 0 )
		return;

	// If we are at the max slot, go to 0.
	if ( m_nCurrentSelection == MAX_SLOT + 1 )
	{
		if ( CanFireBlue() )
			m_nCurrentSelection = 0;
		else if ( CanFireGreen() )
			m_nCurrentSelection = 1;
		else if ( CanFireYellow() )
			m_nCurrentSelection = 2;
	}
	else
		m_nCurrentSelection++;	// Go to the next slot.

	WeaponSound( WPN_DOUBLE );

#ifndef CLIENT_DLL
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if( pOwner )
	{
		// RUMBLE
		pOwner->RumbleEffect( RUMBLE_PISTOL, 0, RUMBLE_FLAG_RESTART );
	}
#endif

	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponVecgun::TestSlot( void )
{
	// Test what ball is the only ball.
	if ( CanFireBlue() && !CanFireGreen() && !CanFireYellow() )
		m_intOnlyBall = VECTRONIC_BALL_BLUE;
	else if ( !CanFireBlue() && CanFireGreen() && !CanFireYellow() )
		m_intOnlyBall = VECTRONIC_BALL_GREEN;
	else if ( !CanFireBlue() && !CanFireGreen() && CanFireYellow() )
		m_intOnlyBall = VECTRONIC_BALL_YELLOW;
	else
		m_intOnlyBall = -1;

	// Slot test again.
	if ( m_nCurrentSelection == MAX_SLOT + 1 )
	{
		if ( CanFireBlue() )
			m_nCurrentSelection = 0;
		else if ( CanFireGreen() )
			m_nCurrentSelection = 1;
		else if ( CanFireYellow() )
			m_nCurrentSelection = 2;

		return;
	}

	// Here if we are on a slot that we don't have, jump to the next slot.
	if ( m_nCurrentSelection == VECTRONIC_BALL_BLUE && CanFireBlue() == false )
		m_nCurrentSelection++;

	if ( m_nCurrentSelection == VECTRONIC_BALL_GREEN && CanFireGreen() == false )
		m_nCurrentSelection++;

	if ( m_nCurrentSelection == VECTRONIC_BALL_YELLOW && CanFireYellow() == false )
		m_nCurrentSelection++;

	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponVecgun::FireSelectedBall( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;

#ifndef CLIENT_DLL
	// Fire the bullets
	Vector vecSrc	 = pOwner->Weapon_ShootPosition( );
	Vector vecAiming = pOwner->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
	Vector impactPoint = vecSrc + ( vecAiming * MAX_TRACE_LENGTH );

	// Fire the bullets
	Vector vecVelocity = vecAiming * 1000.0f;

	// Fire the combine ball
	CreateParticleBall0( vecSrc, vecVelocity, 0.1f, m_nCurrentSelection, 1, pOwner );
#endif

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

#ifndef CLIENT_DLL
	// RUMBLE
	pOwner->RumbleEffect( RUMBLE_SHOTGUN_SINGLE, 0, RUMBLE_FLAG_RESTART );
#endif

	WeaponSound( SINGLE );

	m_nNumShotsFired++;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponVecgun::PrimaryAttack( void )
{
	if ( ( gpGlobals->curtime - m_flLastAttackTime ) > 0.2f && m_nDelay >= DELAY_TIME)
	{
		// Do we have balls at all?
		if (CanFireBlue() || CanFireGreen() || CanFireYellow() )
		{
			m_nNumShotsFired = 0;
			m_nDelay = 0;
			m_bWasFired = true;
			FireSelectedBall();

		}
		else
		{
			CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
			if ( pOwner == NULL )
				return;

			Vector backward;
			GetVectors( &backward, NULL, NULL );
			pOwner->ViewPunch( QAngle( -6, random->RandomInt( -2, 2 ) ,0 ) );
			pOwner->ApplyAbsVelocityImpulse ( backward * 500.0f * -1 );
	
			//Explosion effect
			//DoEffect( EFFECT_LAUNCH, &tr.endpos );
#ifndef CLIENT_DLL
			color32 white = { 245, 245, 255, 32 };
			UTIL_ScreenFade( pOwner, white, 0.1f, 0.0f, FFADE_IN );
#endif
			//PrimaryFireEffect();
			SendWeaponAnim( ACT_VM_PRIMARYATTACK );
			WeaponSound( SINGLE );
			m_bWasFired = true;
		}
	}
	/*
	else
	{
		m_nNumShotsFired++;
	}
	*/

	m_flLastAttackTime = gpGlobals->curtime;
	m_flSoonestAttack = gpGlobals->curtime + REFIRE_TIME;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if( pOwner )
	{
		pOwner->ViewPunchReset();
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponVecgun::SecondaryAttack( void )
{
	if ( ( gpGlobals->curtime - m_flLastAttackTime ) > 0.2f )
	{
		// Do we have balls at all?
		if (CanFireBlue() || CanFireGreen() || CanFireYellow() )
		{
			m_nNumShotsFired = 0;
			ChangeBall();

		}
		//FireBall1();
	}
	else
	{
		m_nNumShotsFired++;
	}


	m_flLastAttackTime = gpGlobals->curtime;
	m_flSoonestAttack = gpGlobals->curtime + 0.1f;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if( pOwner )
	{
		pOwner->ViewPunchReset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponVecgun::UpdatePenaltyTime( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponVecgun::ItemPreFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponVecgun::ItemBusyFrame( void )
{
	UpdatePenaltyTime();

	// HACK: Set the delay to 100. We need to wait anyway after the player picks up an object.
	m_nDelay = 100;
	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponVecgun::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	CVectronicPlayer *pOwner = To_VectronicPlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Do we have balls at all?
	if (CanFireBlue() || CanFireGreen() || CanFireYellow() )
	{
		TestSlot();
	}

	if ( m_bInReload )
		return;
	
	if(m_bResetting)
	{
		m_nDelay++;

		if (m_nDelay >= DELAY_TIME)
		{
			m_bResetting = false;
		}
	}

	if (m_bWasFired)
	{
		if (m_nDelay < DELAY_TIME)
		{
			m_bResetting = true;
		}
	}

	//Allow a refire as fast as the player can click
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestAttack < gpGlobals->curtime ) || ( ( pOwner->m_nButtons & IN_ATTACK ) == true ) && m_bWasFired )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;

		if (m_nDelay < DELAY_TIME)
		{
			m_bResetting = true;
			m_bWasFired = false;
		}
	}

	//Allow a refire as fast as the player can click
	if ( ( ( pOwner->m_nButtons & IN_ATTACK2 ) == false ) && ( m_flSoonestAttack < gpGlobals->curtime ) )
	{
		m_flNextSecondaryAttack = gpGlobals->curtime - 0.1f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponVecgun::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( 0.25f, 0.5f );
	viewPunch.y = random->RandomFloat( -.6f, .6f );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}

// Our Ball Code

// Granting Punts
void CWeaponVecgun::SetCanFireBlue()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Can we already fire this Punt?
	if (!m_bCanFireBlue)
	{
#ifndef CLIENT_DLL
		// RUMBLE
		pOwner->RumbleEffect( RUMBLE_AR2, 0, RUMBLE_FLAG_RESTART );
#endif
		WeaponSound( RELOAD );
		m_bCanFireBlue = true;

		m_nCurrentSelection = VECTRONIC_BALL_BLUE;
	}
}

void CWeaponVecgun::SetCanFireGreen()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Can we already fire this Punt?
	if (!m_bCanFireGreen)
	{
#ifndef CLIENT_DLL
		// RUMBLE
		pOwner->RumbleEffect( RUMBLE_AR2, 0, RUMBLE_FLAG_RESTART );
#endif
		WeaponSound( RELOAD );
		m_bCanFireGreen = true;

		m_nCurrentSelection = VECTRONIC_BALL_GREEN;
	}
}

void CWeaponVecgun::SetCanFireYellow()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Can we already fire this Punt?
	if (!m_bCanFireYellow)
	{
#ifndef CLIENT_DLL
		// RUMBLE
		pOwner->RumbleEffect( RUMBLE_AR2, 0, RUMBLE_FLAG_RESTART );
#endif
		WeaponSound( RELOAD );
		m_bCanFireYellow = true;

	    m_nCurrentSelection = VECTRONIC_BALL_YELLOW;
	}
}

// Clear the gun
void CWeaponVecgun::ClearGun()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	// Test to see if we have any balls. If so, we play a cute
	// animation and sound.
	if ( m_bCanFireBlue || m_bCanFireGreen || m_bCanFireYellow )
	{
		//Play animation
		SendWeaponAnim( ACT_VM_FIZZLE );

		//Play Sound
		WeaponSound( TAUNT );

#ifndef CLIENT_DLL
		// RUMBLE
		pOwner->RumbleEffect( RUMBLE_SHOTGUN_DOUBLE, 0, RUMBLE_FLAG_ONLYONE );
#endif

		// Remove Punts from gun.
		if (m_bCanFireBlue)
		{
			m_bCanFireBlue = false;
		}

		if (m_bCanFireGreen)
		{
			m_bCanFireGreen = false;
		}

		if (m_bCanFireYellow)
		{
			m_bCanFireYellow = false;
		}
	}
}

void CWeaponVecgun::ClearBlue()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	// Test to see if we have any balls. If so, we play a cute
	// animation and sound.
	if ( m_bCanFireBlue)
	{
		//Play animation
		SendWeaponAnim( ACT_VM_FIZZLE );

#ifndef CLIENT_DLL
		// RUMBLE
		pOwner->RumbleEffect( RUMBLE_SHOTGUN_DOUBLE, 0, RUMBLE_FLAG_ONLYONE );
#endif

		//Play Sound
		WeaponSound( TAUNT );

		m_bCanFireBlue = false;
	}
}

void CWeaponVecgun::ClearGreen()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	// Test to see if we have any balls. If so, we play a cute
	// animation and sound.
	if ( m_bCanFireGreen)
	{
		//Play animation
		SendWeaponAnim( ACT_VM_FIZZLE );

		//Play Sound
		WeaponSound( TAUNT );

#ifndef CLIENT_DLL
		// RUMBLE
		pOwner->RumbleEffect( RUMBLE_SHOTGUN_DOUBLE, 0, RUMBLE_FLAG_ONLYONE );
#endif

		m_bCanFireGreen = false;
	}
}

void CWeaponVecgun::ClearYellow()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	// Test to see if we have any balls. If so, we play a cute
	// animation and sound.
	if ( m_bCanFireYellow)
	{
		//Play animation
		SendWeaponAnim( ACT_VM_FIZZLE );

		//Play Sound
		WeaponSound( TAUNT );

#ifndef CLIENT_DLL
		// RUMBLE
		pOwner->RumbleEffect( RUMBLE_SHOTGUN_DOUBLE, 0, RUMBLE_FLAG_ONLYONE );
#endif

		m_bCanFireYellow = false;
	}
}

#ifndef CLIENT_DLL
void CC_JumpToBlue( void )
{
	CVectronicPlayer *pPlayer = To_VectronicPlayer( UTIL_GetCommandClient() );

	CWeaponVecgun *pVecGun = static_cast<CWeaponVecgun*>( pPlayer->Weapon_OwnsThisType( "weapon_vecgun" ) );
	if ( pVecGun && pVecGun->CanFireBlue() )
	{
		pVecGun->WeaponSound( WPN_DOUBLE );
		pVecGun->SetJumpToSelection(0);
	}
}

static ConCommand jumpto_blue("jumpto_blue", CC_JumpToBlue, "Makes the guns current selection be blue.\n\tArguments:   	none ");

void CC_JumpToGreen( void )
{
	CVectronicPlayer *pPlayer = To_VectronicPlayer( UTIL_GetCommandClient() );

	CWeaponVecgun *pVecGun = static_cast<CWeaponVecgun*>( pPlayer->Weapon_OwnsThisType( "weapon_vecgun" ) );
	if ( pVecGun && pVecGun->CanFireGreen() )
	{
		pVecGun->WeaponSound( WPN_DOUBLE );
		pVecGun->SetJumpToSelection(1);
	}
}

static ConCommand jumpto_green("jumpto_green", CC_JumpToGreen, "Makes the guns current selection be green.\n\tArguments:   	none ");

void CC_JumpToPurple( void )
{
	CVectronicPlayer *pPlayer = To_VectronicPlayer( UTIL_GetCommandClient() );

	CWeaponVecgun *pVecGun = static_cast<CWeaponVecgun*>( pPlayer->Weapon_OwnsThisType( "weapon_vecgun" ) );
	if ( pVecGun && pVecGun->CanFireYellow() )
	{
		pVecGun->WeaponSound( WPN_DOUBLE );
		pVecGun->SetJumpToSelection(2);
	}
}

static ConCommand jumpto_purple("jumpto_purple", CC_JumpToPurple, "Makes the guns current selection be purple.\n\tArguments:   	none ");

void CC_JumpToNext( void )
{
	CVectronicPlayer *pPlayer = To_VectronicPlayer( UTIL_GetCommandClient() );

	CWeaponVecgun *pVecGun = static_cast<CWeaponVecgun*>( pPlayer->Weapon_OwnsThisType( "weapon_vecgun" ) );
	if ( pVecGun && pVecGun->CanFireYellow() )
	{
		pVecGun->WeaponSound( WPN_DOUBLE );
		pVecGun->m_nCurrentSelection++;
	}
}

static ConCommand jumpto_nextball("jumpto_nextball", CC_JumpToNext, "Makes the gun jump to the next ball slot.\n\tArguments:   	none ");
#endif