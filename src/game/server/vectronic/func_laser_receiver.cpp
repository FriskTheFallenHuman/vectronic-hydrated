//=========== Copyright © 2012 - 2014, rHetorical, All rights reserved. =============
//
// Purpose: A brush that sends outputs if it is taking damage, or not.
//		
//===================================================================================

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CFuncLReceiver : public CPointEntity
{
public:
	DECLARE_CLASS( CFuncLReceiver, CPointEntity );
	DECLARE_DATADESC();
 
	void Spawn();
	bool CreateVPhysics( void );
	void Think( void );
	void TurnOff( void );
	
private:

 	void InputEnable( inputdata_t &data );
	void InputDisable( inputdata_t &data );

	virtual int OnTakeDamage( const CTakeDamageInfo &info );

	COutputEvent	m_OnDamaged;
	COutputEvent	m_OnOffDamaged;
	bool m_bDisabled;

	bool m_TakingDamage;
	bool m_FiredOutput;

};

LINK_ENTITY_TO_CLASS( func_laser_receiver, CFuncLReceiver );
 
BEGIN_DATADESC( CFuncLReceiver )

	DEFINE_THINKFUNC( TurnOff ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),
	
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),

	// Outputs
	DEFINE_OUTPUT( m_OnDamaged, "OnLaserHit" ),
	DEFINE_OUTPUT( m_OnOffDamaged, "OnLaserLost" ),
 
END_DATADESC()

bool CFuncLReceiver::CreateVPhysics( void )
{
	VPhysicsInitStatic();
	return true;
}

void CFuncLReceiver::Spawn()
{
	BaseClass::Spawn();
 
	// We should collide with physics
	SetSolid( SOLID_BSP );

	// Use our brushmodel
	SetModel( STRING( GetModelName() ) );
 
	// We push things out of our way
	SetMoveType( MOVETYPE_PUSH );
	//AddFlag( FL_WORLDBRUSH );

	m_takedamage = DAMAGE_EVENTS_ONLY;

	RegisterThinkContext( "TurnOff" );
	SetContextThink( &CFuncLReceiver::TurnOff, gpGlobals->curtime, "TurnOff" );

	//SetNextThink(gpGlobals->curtime + .1);

	CreateVPhysics();
}

void CFuncLReceiver::Think( void )
{
	// We are from OnTakeDamage. Tell the entity we are no longer taking damage.
	// If we still are, the bool will reset itself to true. so when we get to TurnOff, nothing will happen!

	//DevMsg("CFuncLReceiver: We are at Think! \n");
	m_TakingDamage = false;
	SetNextThink(gpGlobals->curtime + .1, "TurnOff" );
}

void CFuncLReceiver::TurnOff()
{
	//DevMsg("CFuncLReceiver: We are at turnoff! \n");

	// If the laser is not hitting the brush. We will continue what we are doing.
	if (!m_TakingDamage && m_FiredOutput)
	{
		m_OnOffDamaged.FireOutput( this, this );
		m_FiredOutput = false;
	}
}

void CFuncLReceiver::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
	m_takedamage = DAMAGE_EVENTS_ONLY;
}

void CFuncLReceiver::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
	m_takedamage = DAMAGE_NO;
}

int CFuncLReceiver::OnTakeDamage( const CTakeDamageInfo &info )
{
	int ret = BaseClass::OnTakeDamage( info );
	CBaseEntity *pInflictor = info.GetInflictor();

	// We are the laser, so hit it!
	if ( pInflictor->ClassMatches( "env_laser" ) ) 
	{	
		// We are taking damage.
		m_TakingDamage = true;

		// Fire this output once.
		if( !m_FiredOutput )
		{
			m_OnDamaged.FireOutput( this, this );
			m_FiredOutput = true;
		}

		// Think
		SetNextThink(gpGlobals->curtime + 0.08);

		// Return
		return ret;
	}

	return 0;
}