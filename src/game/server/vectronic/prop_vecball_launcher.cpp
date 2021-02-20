//=========== Copyright © 2014, rHetorical, All rights reserved. =============
//
// Purpose: 
//		
//=============================================================================

#include "cbase.h"
#include "prop_vectronic_projectile.h"
#include "baseanimating.h"
#include "particle_parse.h"
#include "particle_system.h"
#include "soundent.h"
#include "engine/ienginesound.h"
#include "soundenvelope.h"
#include "sprite.h"
#include "game.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ENTITY_MODEL	"models/props/ballshoot.mdl"
#define ENTITY_SOUND	"Weapon_SMG1.Double"

#define BALL0_PARTICLE	"projectile_ball_blue_nodes"
#define BALL0_FIRE		"vecgun_blue_charge_glow"

#define BALL1_PARTICLE	"projectile_ball_green_nodes"
#define BALL1_FIRE		"vecgun_green_charge_glow"

#define BALL2_PARTICLE	"projectile_ball_purple_nodes"
#define BALL2_FIRE		"vecgun_purple_charge_glow"

#define BALL3_PARTICLE	"projectile_ball_red_nodes"
#define BALL3_FIRE		"vecgun_red_charge_glow"

/*
#define BALL4_PARTICLE	"projectile_ball_yellow_nodes"
#define BALL4_FIRE		"vecgun_yellow_charge_glow"

#define BALL5_PARTICLE	"projectile_ball_orange_nodes"
#define BALL5_FIRE		"vecgun_orange_charge_glow"
*/

#define SPRITE			 "sprites/light_glow03.vmt"


//-----------------------------------------------------------------------------
// Purpose: Base
//-----------------------------------------------------------------------------
class CPropVecBallLauncher : public CBaseAnimating
{
	public:
	DECLARE_CLASS( CPropVecBallLauncher, CBaseAnimating );
	DECLARE_DATADESC();

	//Constructor
	CPropVecBallLauncher()
	{
		m_intType = 0;
		m_flLife = 1;
	}

	void Spawn( void );
	void Precache( void );
	void Think( void );

	void SetupParticle();
	void SetupSprite();
	void PreFireBall();
	void FireBall();

	// Input functions.
	void InputLaunchBall( inputdata_t &inputData);

private:

	int m_intType;
	float m_flLife;
	float m_flConeDegrees;
	bool m_bSpawnedSprite;

	CHandle<CParticleSystem> m_hParticleEffect;
	CHandle<CSprite>		m_hSprite;
	COutputEvent	m_OnPostSpawnBall;

};

LINK_ENTITY_TO_CLASS( prop_vecball_launcher, CPropVecBallLauncher );

// Start of our data description for the class
BEGIN_DATADESC( CPropVecBallLauncher )
 
	//Save/load
	DEFINE_FIELD( m_hParticleEffect, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_intType, FIELD_INTEGER, "skin"),
	DEFINE_KEYFIELD(m_flLife, FIELD_FLOAT, "Life"),

	DEFINE_INPUTFUNC( FIELD_VOID, "LaunchBall", InputLaunchBall ),

	DEFINE_OUTPUT( m_OnPostSpawnBall, "OnPostSpawnProjectile" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVecBallLauncher::Precache( void )
{
	PrecacheModel( ENTITY_MODEL );
	PrecacheScriptSound( ENTITY_SOUND );
	PrecacheModel( SPRITE );
	//PrecacheParticleSystem( "projectile_ball_warp" );

	PrecacheParticleSystem( BALL0_PARTICLE );
	PrecacheParticleSystem( BALL0_FIRE);

	PrecacheParticleSystem( BALL1_PARTICLE );
	PrecacheParticleSystem( BALL1_FIRE);

	PrecacheParticleSystem( BALL2_PARTICLE );
	PrecacheParticleSystem( BALL2_FIRE);

	PrecacheParticleSystem( BALL3_PARTICLE );
	PrecacheParticleSystem( BALL3_FIRE);
/*
	PrecacheParticleSystem( BALL4_PARTICLE );
	PrecacheParticleSystem( BALL4_FIRE);

	PrecacheParticleSystem( BALL5_PARTICLE );
	PrecacheParticleSystem( BALL5_FIRE);
*/ 
	UTIL_PrecacheOther( "prop_particle_ball" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the entity's initial state
//-----------------------------------------------------------------------------
void CPropVecBallLauncher::Spawn( void )
{
	Precache();
	SetModel( ENTITY_MODEL );
	SetSolid( SOLID_VPHYSICS );

//	SetCollisionGroup(COLLISION_GROUP_VPHYSICS);

	// Make it soild to other entities.
	VPhysicsInitStatic();

	SetSequence( LookupSequence("idle") );
	SetupParticle();
	m_nSkin = m_intType;

	// 09/16/14 - New animation frame makes the shadows.... ECH!
	AddEffects(EF_NOSHADOW);

	if(!m_bSpawnedSprite)
	{
		SetupSprite();
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVecBallLauncher::SetupSprite()
{
	if (m_bSpawnedSprite)
		return;

	m_hSprite = CSprite::SpriteCreate( SPRITE, GetAbsOrigin(), false );
	
	m_hSprite->SetAttachment( this, LookupAttachment( "muzzle2" ) );

	m_hSprite->SetTransparency( kRenderWorldGlow, 255, 255, 255, 255, kRenderFxNone );
	m_hSprite->SetBrightness( 255, 0.2f );
	m_hSprite->SetScale( 0.30f ); //10

	// 8-17-14: Messy messy. ASW does not like 4 values for rendercolor. X_X
	if(m_intType == 0)
	{
		m_hSprite->SetRenderColor(BALL0_COLOR);
	}
	else if (m_intType == 1)
	{
		m_hSprite->SetRenderColor(BALL1_COLOR);
	}
	else if (m_intType == 2)
	{
		m_hSprite->SetRenderColor(BALL2_COLOR);
	}
	else if (m_intType == 3)
	{
		m_hSprite->SetRenderColor(BALL3_COLOR);
	}

	/*
	else if (m_intType == 4)
	{
		m_hSprite->SetRenderColor(BALL4_COLOR);
	}
	else if (m_intType == 5)
	{
		m_hSprite->SetRenderColor(BALL5_COLOR);
	}
	*/

	m_hSprite->TurnOn();

	m_bSpawnedSprite = true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVecBallLauncher::SetupParticle()
{
	m_hParticleEffect = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
	if ( m_hParticleEffect != NULL )
	{
		Vector vecAbsOrigin = GetAbsOrigin();
		QAngle angles = GetAbsAngles();
			
		int		attachment;
		attachment = LookupAttachment( "muzzle2" );

		GetAttachment( attachment, vecAbsOrigin );
		m_hParticleEffect->SetAbsOrigin( vecAbsOrigin );
		m_hParticleEffect->SetAbsAngles( angles );
		m_hParticleEffect->SetParent(this);
		m_hParticleEffect->KeyValue( "start_active", "1" );

		if(m_intType == 0)
		{
			m_hParticleEffect->KeyValue( "effect_name", BALL0_PARTICLE );
		}
		else if (m_intType == 1)
		{
			m_hParticleEffect->KeyValue( "effect_name", BALL1_PARTICLE );
		}
		else if (m_intType == 2)
		{
			m_hParticleEffect->KeyValue( "effect_name", BALL2_PARTICLE );
		}
		else if (m_intType == 3)
		{
			m_hParticleEffect->KeyValue( "effect_name", BALL3_PARTICLE );
		}
		/*
		else if (m_intType == 4)
		{
			m_hParticleEffect->KeyValue( "effect_name", BALL4_PARTICLE );
		}
		else if (m_intType == 5)
		{
			m_hParticleEffect->KeyValue( "effect_name", BALL5_PARTICLE );
		}
		*/
		DispatchSpawn( m_hParticleEffect );
		m_hParticleEffect->Activate();

	}
}

//-----------------------------------------------------------------------------
// Purpose: "Then I flip a switch..."
//-----------------------------------------------------------------------------
void CPropVecBallLauncher::InputLaunchBall( inputdata_t &inputData)
{
	PreFireBall();
	m_OnPostSpawnBall.FireOutput( inputData.pActivator, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVecBallLauncher::PreFireBall()
{
	// 8-17-14: We have an issue here when we port this to ASW.
	// Prop does not animate correctly. 

#define NEW_ANIMATION

#ifdef NEW_ANIMATION
	int nSequence = LookupSequence( "in" );
	ResetSequence( nSequence );
	SetPlaybackRate( 1.0f );
	UseClientSideAnimation();
#else
	int nSequence = LookupSequence( "fire" );
	ResetSequence( nSequence );
	SetPlaybackRate( 1.0f );
	UseClientSideAnimation();
	ResetClientsideFrame();
#endif
	// Wait a tad bit for actual firing.
	SetNextThink( gpGlobals->curtime + 0.18 );
}

//-----------------------------------------------------------------------------
// Purpose: Precache assets used by the entity
//-----------------------------------------------------------------------------
void CPropVecBallLauncher::Think( void )
{
	// Play Fire sound.
	EmitSound( ENTITY_SOUND );
	//m_hParticleEffect->StopParticleSystem();

#ifdef NEW_ANIMATION
	int nSequence = LookupSequence( "idle" );
	ResetSequence( nSequence );
	SetPlaybackRate( 1.0f );
	UseClientSideAnimation();
#endif

	FireBall();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVecBallLauncher::FireBall()
{
	CPropParticleBall *pBall = static_cast<CPropParticleBall*>( CreateEntityByName( "prop_particle_ball" ) );

	if ( pBall == NULL )
		 return;

	Vector vecAbsOrigin = GetAbsOrigin();
	QAngle angles = GetAbsAngles();
			
	int		attachment;
	attachment = LookupAttachment( "muzzle" );

	GetAttachment( attachment, vecAbsOrigin );
	Vector zaxis;

	//GetAttachment( LookupAttachment( "muzzle" ), vecAbsOrigin, GetAbsAngles() );
	//DispatchParticleEffect( "projectile_ball_warp", PATTACH_POINT_FOLLOW, this, attachment );

	if(m_intType == 0)
	{
		DispatchParticleEffect( BALL0_FIRE, PATTACH_POINT_FOLLOW, this, attachment );
	}
	else if (m_intType == 1)
	{
		DispatchParticleEffect( BALL1_FIRE, PATTACH_POINT_FOLLOW, this, attachment );
	}
	else if (m_intType == 2)
	{
		DispatchParticleEffect( BALL2_FIRE, PATTACH_POINT_FOLLOW, this, attachment );
	}
	else if (m_intType == 3)
	{
		DispatchParticleEffect( BALL3_FIRE, PATTACH_POINT_FOLLOW, this, attachment );
	}
	/*
	else if (m_intType == 4)
	{
		DispatchParticleEffect( BALL4_FIRE, PATTACH_POINT_FOLLOW, this, attachment );
	}
	else if (m_intType == 5)
	{
		DispatchParticleEffect( BALL5_FIRE, PATTACH_POINT_FOLLOW, this, attachment );
	}
	*/
	pBall->SetAbsOrigin( vecAbsOrigin );

	//	pBall->SetRadius( radius );
	pBall->SetRadius( 8 );

	pBall->SetAbsOrigin( vecAbsOrigin );
	pBall->SetOwnerEntity( this );

	Vector vDirection;
	QAngle qAngle = GetAbsAngles();

	qAngle = qAngle + QAngle ( random->RandomFloat( -m_flConeDegrees, m_flConeDegrees ), random->RandomFloat( -m_flConeDegrees, m_flConeDegrees ), 0 );

	AngleVectors( qAngle, &vDirection, NULL, NULL );

	vDirection *= 1000.0f;
	pBall->SetAbsVelocity( vDirection );

//	Vector vecVelocity = 500.0f;
//	pBall->SetAbsVelocity( vecVelocity );
	pBall->Spawn();

	pBall->SetState( CPropParticleBall::STATE_THROWN );
	pBall->SetSpeed( vDirection.Length() );

	pBall->EmitSound( "ParticleBall.Launch" );

	PhysSetGameFlags( pBall->VPhysicsGetObject(), FVPHYSICS_WAS_THROWN );

//	pBall->StartWhizSoundThink();

	pBall->SetMass( 0.1f );

	pBall->StartLifetime( m_flLife );

	pBall->SetWeaponLaunched( true );

	pBall->SetType ( m_intType );

//	return pBall;

}