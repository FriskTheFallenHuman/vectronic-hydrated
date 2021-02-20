//=========== Copyright © 2014, rHetorical, All rights reserved. =============
//
// Purpose: 
//		
//=============================================================================

#include "cbase.h"
#include "prop_vecball_dispenser.h"
#include "vectronic_player.h"
#include "weapon_vecgun.h"
#include "vectronic_shareddefs.h"
#include "soundenvelope.h"

#define ENTITY_MODEL "models/props/balldispenser.mdl"

//----------------------------------------------------------
// Particle attachment spawning. Read InputTestEntity for info. 
//----------------------------------------------------------
LINK_ENTITY_TO_CLASS( prop_vecball_dispenser, CVecBallDispenser );

BEGIN_DATADESC( CVecBallDispenser )

	// Save/load
	DEFINE_INPUTFUNC( FIELD_VOID, "RespawnBall", InputRespawn ),

	DEFINE_SOUNDPATCH( m_pLoopSound ),
	DEFINE_FIELD( m_hParticleEffect, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hBaseParticleEffect, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bDispatched, FIELD_BOOLEAN ),
	//DEFINE_FIELD( m_hSprite, FIELD_EHANDLE ),
	//DEFINE_FIELD( m_bSpawnedSprite, FIELD_BOOLEAN ),
	//DEFINE_FIELD( m_hVecgun, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_intType, FIELD_INTEGER, "skin"),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "startdisabled"),

	DEFINE_OUTPUT( m_OnBallEquipped, "OnBallEquipped" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Precache assets used by the entity
//-----------------------------------------------------------------------------
void CVecBallDispenser::Precache( void )
{
	PrecacheModel( ENTITY_MODEL );
	PrecacheParticleSystem( "projectile_ball_blue_3rdperson" );
	PrecacheScriptSound ( "Vectronic.Dispenser_Start" );
	PrecacheScriptSound ( "Vectronic.Dispenser_loop" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the entity's initial state
//-----------------------------------------------------------------------------
void CVecBallDispenser::Spawn( void )
{
	Precache();
	SetModel( ENTITY_MODEL );

	SetSolid( SOLID_VPHYSICS );

	// Make it soild to other entities.
	VPhysicsInitStatic();

	m_nSkin = m_intType;

	if ( m_intType == 0 )
	{
		EnableBall0();
	}
	else if ( m_intType == 1 )
	{
		EnableBall1();
	}
	else if ( m_intType == 2 )
	{
		EnableBall2();
	}
	else
	{
		DevMsg("CVecBallDispenser: Unsupported ball type called.\n");
	}

//	SetupSprite();

	if (!m_bDisabled)
	{
		TurnOn();
	}
}

/*
//-----------------------------------------------------------------------------
// Purpose: Sets up the entity's sprite
//-----------------------------------------------------------------------------
void CVecBallDispenser::SetupSprite()
{
	if (m_bSpawnedSprite)
		return;

	m_hSprite = CSprite::SpriteCreate( "sprites/glow_test03.vmt", GetAbsOrigin(), false );
	
	Vector vecAbsOrigin = GetAbsOrigin();
	m_hSprite->SetAbsOrigin( vecAbsOrigin );
	//m_hSprite->SetAttachment( this, LookupAttachment( "attach_spotlight" ) );

	if ( m_intType == 0 )
	{
		m_hSprite->SetTransparency(kRenderTransAdd, BALL0_COLOR, kRenderFxSpotlight );
	}
	else if ( m_intType == 1 )
	{
		m_hSprite->SetTransparency( kRenderTransAdd, BALL1_COLOR, kRenderFxSpotlight );
	}
	else if ( m_intType == 2 )
	{
		m_hSprite->SetTransparency( kRenderTransAdd, BALL2_COLOR, kRenderFxSpotlight );
	}

	

	m_hSprite->SetBrightness( 255, 0.2f );
	m_hSprite->SetScale( 0.50f );
	m_hSprite->TurnOff();

	m_bSpawnedSprite = true;
}
*/
//-----------------------------------------------------------------------------
// Purpose: Sets up the entity's initial state
//-----------------------------------------------------------------------------
void CVecBallDispenser::TurnOn( void )
{
	// If we are starting enabled.
	if(!m_bDispatched)
	{
		DispatchSpawn( m_hParticleEffect );
		DispatchSpawn( m_hBaseParticleEffect );

		m_hBaseParticleEffect->Activate();
		m_hParticleEffect->Activate();
		m_bDispatched = true;
	}

	//DispatchSpawn( m_hSprite );
	m_hParticleEffect->StartParticleSystem();
	m_hBaseParticleEffect->StartParticleSystem();
	CreateSounds();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBallDispenser::EnableBall0()
{
	m_hParticleEffect = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
	if ( m_hParticleEffect != NULL )
	{
		Vector vecAbsOrigin = GetAbsOrigin();
		QAngle angles = GetAbsAngles();
			
		int		attachment;
		attachment = LookupAttachment( "attach_particle" );

		GetAttachment( attachment, vecAbsOrigin );
		m_hParticleEffect->SetAbsOrigin( vecAbsOrigin );
		m_hParticleEffect->SetAbsAngles( angles );
		m_hParticleEffect->SetParent(this);
		m_hParticleEffect->KeyValue( "start_active", "0" );
		m_hParticleEffect->KeyValue( "effect_name", "projectile_ball_blue_3rdperson" );
		//DispatchSpawn( m_hParticleEffect );
		//m_hParticleEffect->Activate();

	}

	m_hBaseParticleEffect = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
	if ( m_hBaseParticleEffect != NULL )
	{
		Vector vecAbsOrigin = GetAbsOrigin();
		QAngle angles = GetAbsAngles();
			
		int		attachment;
		attachment = LookupAttachment( "attach_spotlight" );

		GetAttachment( attachment, vecAbsOrigin );
		m_hBaseParticleEffect->SetAbsOrigin( vecAbsOrigin );
		m_hBaseParticleEffect->SetAbsAngles( angles );
		m_hBaseParticleEffect->SetParent(this);
		m_hBaseParticleEffect->KeyValue( "start_active", "0" );
		m_hBaseParticleEffect->KeyValue( "effect_name", "dispenser_core_blue" );
		//DispatchSpawn( m_hBaseParticleEffect );
		//m_hBaseParticleEffect->Activate();

	}

	if(m_bDispatched)
	{
		m_bDisabled = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBallDispenser::EnableBall1()
{
	m_hParticleEffect = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
	if ( m_hParticleEffect != NULL )
	{
		Vector vecAbsOrigin = GetAbsOrigin();
		QAngle angles = GetAbsAngles();
			
		int		attachment;
		attachment = LookupAttachment( "attach_particle" );

		GetAttachment( attachment, vecAbsOrigin );
		m_hParticleEffect->SetAbsOrigin( vecAbsOrigin );
		m_hParticleEffect->SetAbsAngles( angles );
		m_hParticleEffect->SetParent(this);
		m_hParticleEffect->KeyValue( "start_active", "0" );
		m_hParticleEffect->KeyValue( "effect_name", "projectile_ball_green_3rdperson" );
		//DispatchSpawn( m_hParticleEffect );
		//m_hParticleEffect->Activate();
	}

	m_hBaseParticleEffect = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
	if ( m_hBaseParticleEffect != NULL )
	{
		Vector vecAbsOrigin = GetAbsOrigin();
		QAngle angles = GetAbsAngles();
			
		int		attachment;
		attachment = LookupAttachment( "attach_spotlight" );

		GetAttachment( attachment, vecAbsOrigin );
		m_hBaseParticleEffect->SetAbsOrigin( vecAbsOrigin );
		m_hBaseParticleEffect->SetAbsAngles( angles );
		m_hBaseParticleEffect->SetParent(this);
		m_hBaseParticleEffect->KeyValue( "start_active", "0" );
		m_hBaseParticleEffect->KeyValue( "effect_name", "dispenser_core_green" );
		//DispatchSpawn( m_hBaseParticleEffect );
		//m_hBaseParticleEffect->Activate();

	}

	if(m_bDispatched)
	{
		m_bDisabled = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBallDispenser::EnableBall2()
{
	m_hParticleEffect = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
	if ( m_hParticleEffect != NULL )
	{
		Vector vecAbsOrigin = GetAbsOrigin();
		QAngle angles = GetAbsAngles();
			
		int		attachment;
		attachment = LookupAttachment( "attach_particle" );

		GetAttachment( attachment, vecAbsOrigin );
		m_hParticleEffect->SetAbsOrigin( vecAbsOrigin );
		m_hParticleEffect->SetAbsAngles( angles );
		m_hParticleEffect->SetParent(this);
		m_hParticleEffect->KeyValue( "start_active", "0" );
		m_hParticleEffect->KeyValue( "effect_name", "projectile_ball_purple_3rdperson" );
		//DispatchSpawn( m_hParticleEffect );
		//m_hParticleEffect->Activate();
	}

	m_hBaseParticleEffect = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
	if ( m_hBaseParticleEffect != NULL )
	{
		Vector vecAbsOrigin = GetAbsOrigin();
		QAngle angles = GetAbsAngles();
			
		int		attachment;
		attachment = LookupAttachment( "attach_spotlight" );

		GetAttachment( attachment, vecAbsOrigin );
		m_hBaseParticleEffect->SetAbsOrigin( vecAbsOrigin );
		m_hBaseParticleEffect->SetAbsAngles( angles );
		m_hBaseParticleEffect->SetParent(this);
		m_hBaseParticleEffect->KeyValue( "start_active", "0" );
		m_hBaseParticleEffect->KeyValue( "effect_name", "dispenser_core_purple" );
		//DispatchSpawn( m_hBaseParticleEffect );
		//m_hBaseParticleEffect->Activate();

	}

	if(m_bDispatched)
	{
		m_bDisabled = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBallDispenser::Touch( CBaseEntity *pOther )
{
	if (m_bDisabled)
		return;

	if ( pOther->IsPlayer())
	{
		CVectronicPlayer *pPlayer = To_VectronicPlayer (pOther );
		if ( pPlayer )
		{
			CWeaponVecgun *pVecgun = dynamic_cast<CWeaponVecgun*>( pPlayer->Weapon_OwnsThisType( "weapon_vecgun" ) );

			if ( pVecgun )
			{
				if(!m_bDispatched)
					return;

				if (m_intType == 0)
				{
					if (pVecgun->CanFireBlue())
						return;

					pVecgun->SetCanFireBlue();
				}

				if (m_intType == 1 )
				{
					if (pVecgun->CanFireGreen())
						return;

					pVecgun->SetCanFireGreen();
				}

				if (m_intType == 2 )
				{
					if (pVecgun->CanFireYellow())
						return;

					pVecgun->SetCanFireYellow();
				}

				// Disable when we touch.
				m_bDisabled = true;
				m_OnBallEquipped.FireOutput(this,this);
				m_hParticleEffect->StopParticleSystem();
				m_hBaseParticleEffect->StopParticleSystem();
				StopLoopingSounds();
			}
		}
		else
		{
			// We don't want to anything if the player touches the trigger without the gun.
			return;
		}

	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBallDispenser::InputRespawn ( inputdata_t &inputData )
{
	//m_hParticleEffect->StartParticleSystem();
	//m_hBaseParticleEffect->StartParticleSystem();
	TurnOn();
	EmitSound("Vectronic.Dispenser_Start");
	CreateSounds();

	if(m_bDispatched)
	{
		m_bDisabled = false;
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBallDispenser::CreateSounds()
{
	if ( !m_pLoopSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		CReliableBroadcastRecipientFilter filter;

		m_pLoopSound = controller.SoundCreate( filter, entindex(), "Vectronic.Dispenser_loop" );
		controller.Play( m_pLoopSound, 0.78, 100 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBallDispenser::StopLoopingSounds()
{
	if ( m_pLoopSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		controller.SoundDestroy( m_pLoopSound );
		m_pLoopSound = NULL;
	}

	BaseClass::StopLoopingSounds();
}
