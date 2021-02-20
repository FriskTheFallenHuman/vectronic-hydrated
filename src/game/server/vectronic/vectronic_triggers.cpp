//=========== Copyright © 2013, rHetorical, All rights reserved. =============
//
// Purpose:
//		
//=============================================================================

#include "cbase.h"
#include "vectronic_player.h"
#include "prop_vectronic_projectile.h"
#include "prop_vecbox.h"
#include <convar.h>
#include "saverestore_utlvector.h"
#include "triggers.h"
#include "physics.h"
#include "vphysics_interface.h"
#include "weapon_vecgun.h"

extern ConVar	sv_gravity;

//-----------------------------------------------------------------------------
// noball volume
//-----------------------------------------------------------------------------
class CFuncNoBallVol : public CTriggerMultiple
{
	DECLARE_CLASS( CFuncNoBallVol, CTriggerMultiple );
	DECLARE_DATADESC();

public:
	void Precache( void );
	void Spawn( void );
	void Touch( CBaseEntity *pOther );


private:
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
};


//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( func_noball_volume, CFuncNoBallVol );

BEGIN_DATADESC( CFuncNoBallVol )

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncNoBallVol::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncNoBallVol::Spawn( void )
{
	BaseClass::Spawn();
}

//------------------------------------------------------------------------------
// Inputs
//------------------------------------------------------------------------------
void CFuncNoBallVol::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		InputEnable( inputdata );
	}
	else
	{
		InputDisable( inputdata );
	}
}

void CFuncNoBallVol::InputEnable( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		Enable();
	}
}

void CFuncNoBallVol::InputDisable( inputdata_t &inputdata )
{
	if ( !m_bDisabled )
	{
		Disable();
	}
}

//-----------------------------------------------------------------------------
// Traps the entities
//-----------------------------------------------------------------------------
void CFuncNoBallVol::Touch( CBaseEntity *pEntity )
{
	CBaseAnimating *pAnim = pEntity->GetBaseAnimating();
	if ( !pAnim )
		return;

	CPropParticleBall *pBall = dynamic_cast<CPropParticleBall*>( pEntity );

	if (pBall)
	{
		//if (pBall->GetType() == 0)
		pBall->DoExplosion();
	}
}


//-----------------------------------------------------------------------------
// Cleanser
//-----------------------------------------------------------------------------
class CTriggerCleanser : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerCleanser, CTriggerMultiple );
	DECLARE_DATADESC();

public:

	//Constructor
	CTriggerCleanser()
	{
		m_nFilterType = CLEAR_ALL;
	}

	void Precache( void );
	void Spawn( void );
//	void Think( void );
	void Touch( CBaseEntity *pOther );
//	void EndTouch( CBaseEntity *pOther );


private:

	enum FilterType_t
	{
		CLEAR_ALL				= 0,	// Remove all balls.
		CLEAR_BLUE_ONLY			= 1,	// Only Remove the Blue Ball
		CLEAR_GREEN_ONLY		= 2,	// Only Remove the Green Ball
		CLEAR_YELLOW_ONLY       = 3,
		CLEAR_RED_ONLY			= 4,
		CLEAR_PURPLE_ONLY       = 5,
		CLEAR_ORANGE_ONLY       = 6,

	};

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	int m_nFilterType;
	bool m_bDissolveBox;

	COutputEvent m_OnBoxTouch;
	COutputEvent m_OnPlayerTouch;
	COutputEvent m_OnPlayerBallRemoved;
};


//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( trigger_vecball_cleanser, CTriggerCleanser );

BEGIN_DATADESC( CTriggerCleanser )

	DEFINE_KEYFIELD( m_nFilterType,	FIELD_INTEGER,	"filtertype" ),
	DEFINE_KEYFIELD( m_bDissolveBox, FIELD_BOOLEAN,	"dissolvebox" ),

	// I/O
	DEFINE_OUTPUT( m_OnBoxTouch, "OnBoxTouch" ),
	DEFINE_OUTPUT( m_OnPlayerTouch, "OnPlayerTouch" ),
	DEFINE_OUTPUT( m_OnPlayerBallRemoved, "OnPlayerBallRemoved" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCleanser::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCleanser::Spawn( void )
{
	BaseClass::Spawn();
}

//------------------------------------------------------------------------------
// Inputs
//------------------------------------------------------------------------------
void CTriggerCleanser::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		InputEnable( inputdata );
	}
	else
	{
		InputDisable( inputdata );
	}
}

void CTriggerCleanser::InputEnable( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		Enable();
	}
}

void CTriggerCleanser::InputDisable( inputdata_t &inputdata )
{
	if ( !m_bDisabled )
	{
		Disable();
	}
}

//-----------------------------------------------------------------------------
// Traps the entities
//-----------------------------------------------------------------------------
void CTriggerCleanser::Touch( CBaseEntity *pOther )
{
	if ( !PassesTriggerFilters(pOther) )
		return;

	CBaseAnimating *pAnim = pOther->GetBaseAnimating();
	if ( !pAnim )
		return;

	// Player walks though it!
	if ( pOther->IsPlayer())
	{
		CVectronicPlayer *pPlayer = dynamic_cast<CVectronicPlayer*>( pOther );

		if ( pPlayer )
		{
			CWeaponVecgun *pVecgun = dynamic_cast<CWeaponVecgun*>( pPlayer->Weapon_OwnsThisType( "weapon_vecgun" ) );

			if ( pVecgun )
			{
				if (m_nFilterType == CLEAR_ALL )
				{
					pVecgun->ClearGun();
					m_OnPlayerBallRemoved.FireOutput( this, this );
				}
				if (m_nFilterType == CLEAR_BLUE_ONLY && pVecgun->CanFireBlue() )
				{
					pVecgun->ClearBlue();
					m_OnPlayerBallRemoved.FireOutput( this, this );
				}

				if (m_nFilterType == CLEAR_GREEN_ONLY && pVecgun->CanFireGreen() )
				{
					pVecgun->ClearGreen();
					m_OnPlayerBallRemoved.FireOutput( this, this );
				}
				if (m_nFilterType == CLEAR_YELLOW_ONLY && pVecgun->CanFireYellow() )
				{
					pVecgun->ClearYellow();
					m_OnPlayerBallRemoved.FireOutput( this, this );
				}

			}
		}

		m_OnPlayerTouch.FireOutput( this, this );
	}
	else 
	{
		// Ok, now a box comes in.
		if ( pOther->ClassMatches("prop_vecbox") )
		{
			CVecBox *pVecBox = dynamic_cast<CVecBox*>( pOther );

			if ( m_bDissolveBox )
			{
				pVecBox->OnDissolve();
			}
			else
			{
				// If we are a blue filter and the box is not blue, return.
				if (m_nFilterType == CLEAR_BLUE_ONLY && pVecBox->GetState() != 1 )
					return;

				// If we are a green filter and the box is not green, return.
				if (m_nFilterType == CLEAR_GREEN_ONLY && pVecBox->GetState() != 2 )
					return;

				// If we are a yellow filter and the box is not yellow, return.
				if (m_nFilterType == CLEAR_YELLOW_ONLY && pVecBox->GetState() != 3 )
					return;

				// If we are a red filter and the box is not yellow, return.
				if (m_nFilterType == CLEAR_RED_ONLY && pVecBox->GetState() != 4 )
					return;

				// If we are a purple filter and the box is not yellow, return.
				if (m_nFilterType == CLEAR_PURPLE_ONLY && pVecBox->GetState() != 5 )
					return;

				// If we are a orange filter and the box is not yellow, return.
				if (m_nFilterType == CLEAR_ORANGE_ONLY && pVecBox->GetState() != 6 )
					return;

				pVecBox->Clear();
				m_OnBoxTouch.FireOutput( this, this );
			}
		}

		CPropParticleBall *pBall = dynamic_cast<CPropParticleBall*>( pOther );
		if (pBall)
		{
			if (m_nFilterType == CLEAR_BLUE_ONLY && pBall->GetType() != 0)
				return;

			if (m_nFilterType == CLEAR_GREEN_ONLY && pBall->GetType() != 1)
				return;

			if (m_nFilterType == CLEAR_YELLOW_ONLY && pBall->GetType() != 2)
				return;

			pBall->DoExplosion();
		}
	}
}

/*
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCleanser::EndTouch( CBaseEntity *pOther )
{
	CBaseAnimating *pAnim = pOther->GetBaseAnimating();
	if ( !pAnim )
		return;
}
*/

//-----------------------------------------------------------------------------
// Equip Punt
//-----------------------------------------------------------------------------
class CTriggerBallEquip : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerBallEquip, CTriggerMultiple );
	DECLARE_DATADESC();

public:

	//Constructor
	CTriggerBallEquip()
	{

	}

	void Precache( void );
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );

private:
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	float m_flBalltype;

};


//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( trigger_vecball_equip, CTriggerBallEquip );

BEGIN_DATADESC( CTriggerBallEquip )

	DEFINE_KEYFIELD( m_flBalltype, FIELD_FLOAT, "BallType" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerBallEquip::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerBallEquip::Spawn( void )
{
	SetNextThink( gpGlobals->curtime );

	BaseClass::Spawn();
}

//------------------------------------------------------------------------------
// Inputs
//------------------------------------------------------------------------------
void CTriggerBallEquip::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		InputEnable( inputdata );
	}
	else
	{
		InputDisable( inputdata );
	}
}

void CTriggerBallEquip::InputEnable( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		Enable();
	}
}

void CTriggerBallEquip::InputDisable( inputdata_t &inputdata )
{
	if ( !m_bDisabled )
	{
		Disable();
	}
}

//-----------------------------------------------------------------------------
// Traps the entities
//-----------------------------------------------------------------------------
void CTriggerBallEquip::Touch( CBaseEntity *pOther )
{
	if ( pOther->IsPlayer())
	{
		CVectronicPlayer *pPlayer = dynamic_cast<CVectronicPlayer*>( pOther );

		if ( pPlayer )
		{
			CWeaponVecgun *pVecgun = dynamic_cast<CWeaponVecgun*>( pPlayer->Weapon_OwnsThisType( "weapon_vecgun" ) );

			if ( pVecgun )
			{
				if (m_flBalltype == 0 )
				{
					pVecgun->SetCanFireBlue();
				}

				if (m_flBalltype == 1 )
				{
					pVecgun->SetCanFireGreen();
				}

				if (m_flBalltype == 2 )
				{
					pVecgun->SetCanFireYellow();
				}

				if (m_flBalltype >= 3 )
				{
					Msg ("ERROR: Attempted to give player unauthorized ball. Ignoring!\n");
				}

				// Disable when we touch.
				Disable();
			}
			else
			{
				// We don't want to anything if the player touches the trigger without the gun.
				return;
			}
		}
	}
}

void CTriggerBallEquip::EndTouch( CBaseEntity *pOther )
{
	CBaseAnimating *pAnim = pOther->GetBaseAnimating();
	if ( !pAnim )
		return;
}

