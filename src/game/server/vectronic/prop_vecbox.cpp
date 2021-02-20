//======== Copyright © 2013 - 2014, rHetorical, All rights reserved. ==========
//
// Purpose: 
//		
//=============================================================================

#include "cbase.h"
#include "engine/IEngineSound.h"
#include "vectronic_player.h"
#include "prop_vectronic_projectile.h"
#include "prop_vecbox.h"
#include "filters.h"
#include "physics.h"
#include "vphysics_interface.h"
#include "entityoutput.h"
#include "studio.h"
#include "explode.h"
#include <convar.h>
#include "particle_parse.h"
#include "props.h"
#include "physics_collisionevent.h"
#include "player_pickup.h"
#include "player_pickup_controller.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar player_throwforce;

void CC_GiveVecBox( void )
{
	engine->ServerCommand("ent_create prop_vecbox\n");
}

static ConCommand makebox("ent_create_vecbox", CC_GiveVecBox, "Give the player a Vectronic Box\n", FCVAR_CHEAT );

LINK_ENTITY_TO_CLASS( prop_vecbox, CVecBox );
PRECACHE_REGISTER(prop_vecbox);

/*
IMPLEMENT_SERVERCLASS_ST(CVecBox, DT_VecBox)
	SendPropBool( SENDINFO( m_bGlowEnabled ) ),
	SendPropFloat( SENDINFO( m_flRedGlowColor ) ),
	SendPropFloat( SENDINFO( m_flGreenGlowColor ) ),
	SendPropFloat( SENDINFO( m_flBlueGlowColor ) ),
END_SEND_TABLE()
*/

BEGIN_DATADESC( CVecBox )

	//Save/load
	DEFINE_USEFUNC( Use ),
	DEFINE_KEYFIELD( m_intStartActivation, FIELD_INTEGER, "startstate" ),

	DEFINE_FIELD( m_intLastBallHit, FIELD_INTEGER ),
	DEFINE_FIELD( m_intProtecting, FIELD_INTEGER ),
	/*
	DEFINE_FIELD( m_bGlowEnabled, FIELD_BOOLEAN ),
	*/
	DEFINE_FIELD( m_bHasGhost, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsGhost, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bActivated, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsProtected, FIELD_BOOLEAN ),
/*
	DEFINE_FIELD( m_flRedGlowColor, FIELD_FLOAT ),
	DEFINE_FIELD( m_flGreenGlowColor, FIELD_FLOAT ),
	DEFINE_FIELD( m_flBlueGlowColor, FIELD_FLOAT ),
*/
	DEFINE_FIELD( m_hBox, FIELD_EHANDLE ),
	DEFINE_AUTO_ARRAY( m_hSprite, FIELD_EHANDLE ),

	// I/O
	DEFINE_INPUTFUNC( FIELD_VOID, "Dissolve", InputDissolve ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Explode", InputExplode ),

	DEFINE_INPUTFUNC( FIELD_VOID, "KillGhost", InputKillGhost ),

	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),
	DEFINE_OUTPUT( m_OnPlayerPickup, "OnPlayerPickup" ),
	DEFINE_OUTPUT( m_OnPhysGunPickup, "OnPhysGunPickup" ),
	DEFINE_OUTPUT( m_OnPhysGunDrop, "OnPhysGunDrop" ),

	DEFINE_OUTPUT( m_OnBlueBall, "OnBlueBall" ),
	DEFINE_OUTPUT( m_OnGreenBall, "OnGreenBall" ),
	DEFINE_OUTPUT( m_OnPurpleBall, "OnPurpleBall" ),
	DEFINE_OUTPUT( m_OnReset, "OnReset" ),

	DEFINE_OUTPUT( m_OnExplode, "OnExplode" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CVecBox::CVecBox( void )
{
/*
	m_bGlowEnabled.Set( false );

	m_flRedGlowColor = 0.76f;
	m_flGreenGlowColor = 0.76f;
	m_flBlueGlowColor = 0.76f;

	*/
	m_bIsGhost = false;
	m_bActivated = false;
	m_bIsProtected = false;
	m_intProtecting = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::Precache( void )
{
	// Our Box Model
	PrecacheModel( VECBOX_MODEL );
	PrecacheModel( VECBOX_MODEL_GHOST );
	PrecacheModel( VECBOX_SPRITE );

	// Particles
	PrecacheParticleSystem( VECBOX_CLEAR_PARTICLE );
	PrecacheParticleSystem( VECBOX_HIT0_PARTICLE );
	PrecacheParticleSystem(	VECBOX_HIT1_PARTICLE );
	PrecacheParticleSystem( VECBOX_HIT2_PARTICLE );
	PrecacheParticleSystem( VECBOX_HIT3_PARTICLE );

	//PrecacheParticleSystem( VECBOX_HIT4_PARTICLE );
	//PrecacheParticleSystem( VECBOX_HIT5_PARTICLE );

	// Sounds
	PrecacheScriptSound( "VecBox.Activate" );
	PrecacheScriptSound( "VecBox.Deactivate" );
	PrecacheScriptSound( "VecBox.ClearShield" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::Spawn( void )
{
	SetModel( VECBOX_MODEL );
	SetSolid( SOLID_VPHYSICS );
	CreateVPhysics();
	SetUse( &CVecBox::Use );

	// For Start up Activations.
	if (m_intStartActivation == VECTRONIC_BALL_BLUE )
	{
		m_intLastBallHit = VECTRONIC_BALL_BLUE;
		VPhysicsGetObject()->EnableGravity( false );
	}

	if (m_intStartActivation == VECTRONIC_BALL_GREEN )
	{
		MakeGhost();
		m_intLastBallHit = VECTRONIC_BALL_GREEN;
	}

	// Quick Think!
	SetNextThink( gpGlobals->curtime + .1 );

	SetupSprites();

	BaseClass::Spawn();

}
/*
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::AddGlowEffect( void )
{
	SetTransmitState( FL_EDICT_ALWAYS );
	m_bGlowEnabled.Set( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::RemoveGlowEffect( void )
{
	m_bGlowEnabled.Set( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CVecBox::IsGlowEffectActive( void )
{
	return m_bGlowEnabled;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::SetGlowVector(float r, float g, float b )
{
	m_flRedGlowColor = r;
	m_flGreenGlowColor = g;
	m_flBlueGlowColor = b;
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::SetupSprites( void )
{
	if (m_bIsGhost)
		return;

	if(!m_bSpawnedSprites)
	{
		int i;
		for ( i = 0; i < 6; i++ )
		{
			if (m_hSprite[i] == NULL )
			{
				const char *attachNames[] = 
				{
					"sprite_attach1",
					"sprite_attach2",
					"sprite_attach3",
					"sprite_attach4",
					"sprite_attach5",
					"sprite_attach6"
				};

				m_hSprite[i] = CSprite::SpriteCreate( VECBOX_SPRITE, GetAbsOrigin(), false );
				//m_hSprite[i]->SetAsTemporary();
				m_hSprite[i]->SetAttachment( this, LookupAttachment( attachNames[i] ) );
				m_hSprite[i]->SetTransparency( kRenderWorldGlow, 255, 255, 255, 255, kRenderFxNone );
				m_hSprite[i]->SetColor(255,255,255);
				m_hSprite[i]->SetBrightness( 255, 0.2f );
				m_hSprite[i]->SetScale( 0.30f ); //10
				m_hSprite[i]->TurnOn();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::Think( void )
{
	if (m_bIsGhost)
		return;

	BaseClass::Think();
	m_bIsTouching = false;

	switch ( m_intLastBallHit )
	{
		/*
		case VECTRONIC_BALL_NONE:
			//m_nSkin = 0;
			break;

		case VECTRONIC_BALL_BLUE:
			//m_nSkin = 1;
			break;
		*/

		case VECTRONIC_BALL_GREEN:
			if (!m_bHasGhost)
			{
				if (m_bIsProtected)
				{
					if (m_intProtecting == VECTRONIC_BALL_BLUE)
					{
						MakeGhost();
					}
					m_bIsProtected = false;

				}
				else
				{
					MakeGhost();
				}
			}

			//m_nSkin = 2;
			break;
		/*
		case VECTRONIC_BALL_YELLOW:
			//m_nSkin = 3;
			break;
		*/
	}

	DevMsg("CVecBox: Done Thinking\n");
}

//Physcannon shit

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	m_hPhysicsAttacker = pPhysGunUser;

	if ( reason == PICKED_UP_BY_CANNON || reason == PICKED_UP_BY_PLAYER )
	{
		m_OnPlayerPickup.FireOutput( pPhysGunUser, this );
	}

	if ( reason == PICKED_UP_BY_CANNON )
	{
		m_OnPhysGunPickup.FireOutput( pPhysGunUser, this );
	}
}
void CVecBox::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t reason )
{
	m_OnPhysGunDrop.FireOutput( pPhysGunUser, this );
}

//End of Physcannon shit

//-----------------------------------------------------------------------------
// Purpose: Pick me up!
//-----------------------------------------------------------------------------
int CVecBox::ObjectCaps()
{ 
	int caps = BaseClass::ObjectCaps();

	if ( !m_bIsGhost )
	{
		caps |= FCAP_IMPULSE_USE;
	}

	return caps;
}
void CVecBox::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//DevMsg ("+USE WAS PRESSED ON ME\n");
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( pPlayer )
	{
		if ( HasSpawnFlags( SF_PHYSPROP_ENABLE_PICKUP_OUTPUT ) )
		{
			m_OnPlayerUse.FireOutput( this, this );
		}
		pPlayer->PickupObject( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::InputDissolve( inputdata_t &inputData )
{
	// We may have to think for this...
	m_OnFizzled.FireOutput( this, this );
	OnDissolve();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::InputKillGhost( inputdata_t &inputData )
{
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::InputExplode( inputdata_t &inputData )
{
	Vector vecUp;
	GetVectors( NULL, NULL, &vecUp );
	Vector vecOrigin = WorldSpaceCenter() + ( vecUp * 12.0f );

	if (m_bHasGhost)
	{
		KillGhost();
	}

	ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), this, 100.0, 75.0, true);
	m_OnExplode.FireOutput( this, this );
	m_bGib = true;

	CPVSFilter filter( vecOrigin );
	for ( int i = 0; i < 4; i++ )
	{
		Vector gibVelocity = RandomVector(-100,100);
		int iModelIndex = modelinfo->GetModelIndex( g_PropDataSystem.GetRandomChunkModel( "MetalChunks" ) );	
		te->BreakModel( filter, 0.0, vecOrigin, GetAbsAngles(), Vector(40,40,40), gibVelocity, iModelIndex, 150, 4, 2.5, BREAK_METAL );
	}
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity* ConvertToSimpleBox ( CBaseEntity* pEnt )
{
	CBaseEntity *pRetVal = NULL;
	pRetVal = CreateEntityByName( "simple_physics_prop" );

	pRetVal->KeyValue( "model", STRING(pEnt->GetModelName()) );
	pRetVal->SetAbsOrigin( pEnt->GetAbsOrigin() );
	pRetVal->SetAbsAngles( pEnt->GetAbsAngles() );
	pRetVal->Spawn();
	pRetVal->VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
	
	return pRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::OnDissolve()
{
	// First test if we have a child ghost. 
	// NOT PUTTING THIS HERRE WILL RESRORT TO CRASH!!
	if (m_bHasGhost)
	{
		KillGhost();
	}

	// Now, Fizzle like the shit in Portal!
	CBaseAnimating *pBaseAnimating = dynamic_cast<CBaseAnimating*>( this );

	if ( pBaseAnimating && !pBaseAnimating->IsDissolving() )
	{
		Vector vOldVel;
		AngularImpulse vOldAng;
		pBaseAnimating->GetVelocity( &vOldVel, &vOldAng );

		IPhysicsObject* pOldPhys = pBaseAnimating->VPhysicsGetObject();

		if ( pOldPhys && ( pOldPhys->GetGameFlags() & FVPHYSICS_PLAYER_HELD ) )
		{
			CVectronicPlayer  *pPlayer = (CVectronicPlayer *)GetPlayerHoldingEntity( pBaseAnimating );
			if( pPlayer )
			{
				// Modify the velocity for held objects so it gets away from the player
				pPlayer->ForceDropOfCarriedPhysObjects( pBaseAnimating );

				pPlayer->GetAbsVelocity();
				vOldVel = pPlayer->GetAbsVelocity() + Vector( pPlayer->EyeDirection2D().x * 4.0f, pPlayer->EyeDirection2D().y * 4.0f, -32.0f );
			}
		}

		// Swap object with an disolving physics model to avoid touch logic
		CBaseEntity *pDisolvingObj = ConvertToSimpleBox( pBaseAnimating );
		if ( pDisolvingObj )
		{
			// Remove old prop, transfer name and children to the new simple prop
			pDisolvingObj->SetName( pBaseAnimating->GetEntityName() );
			UTIL_TransferPoseParameters( pBaseAnimating, pDisolvingObj );
			TransferChildren( pBaseAnimating, pDisolvingObj );
			pDisolvingObj->SetCollisionGroup( COLLISION_GROUP_INTERACTIVE_DEBRIS );
			pBaseAnimating->AddSolidFlags( FSOLID_NOT_SOLID );
			pBaseAnimating->AddEffects( EF_NODRAW );

			IPhysicsObject* pPhys = pDisolvingObj->VPhysicsGetObject();
			if ( pPhys )
			{
				pPhys->EnableGravity( false );

				Vector vVel = vOldVel;
				AngularImpulse vAng = vOldAng;

				// Disolving hurts, damp and blur the motion a little
				vVel *= 0.5f;
				vAng.z += 20.0f;

				pPhys->SetVelocity( &vVel, &vAng );
			}

			pBaseAnimating->AddFlag( FL_DISSOLVING );
			UTIL_Remove( pBaseAnimating );
		}
		
		CBaseAnimating *pDisolvingAnimating = dynamic_cast<CBaseAnimating*>( pDisolvingObj );
		if ( pDisolvingAnimating ) 
		{
			pDisolvingAnimating->Dissolve( "", gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::Touch( CBaseEntity *pOther )
{
	CPropParticleBall *pBall = dynamic_cast<CPropParticleBall*>( pOther );

	if (!pBall)
		return;

	if (!m_bIsTouching)
	{
		if (pBall->GetType() == 0)
		{
			if (m_bIsGhost)
			{
				DevMsg ("CVecBox: Hit by a Ball But we are a ghost.\n");
				return;
			}
			else
			{
				EffectBlue();
				m_bActivated = true;
			}
		}

		if (pBall->GetType() == 1)
		{
			if (m_bIsGhost)
			{
				DevMsg ("CVecBox: Hit by a Ball But we are a ghost.\n");
				return;
			}
			else
			{
				EffectGreen();
				m_bActivated = true;
			}
		}

		if (pBall->GetType() == 2)
		{
			if (m_bIsGhost)
			{
				DevMsg ("CVecBox: Hit by a Ball But we are a ghost.\n");
				return;
			}
			else if( m_intLastBallHit != VECTRONIC_BALL_NONE )
			{
				EffectPurple();
				m_bActivated = true;
			}
		}

		if (pBall->GetType() == 3)
		{
			if (m_bIsGhost)
			{
				DevMsg ("CVecBox: Hit by a Ball But we are a ghost.\n");
				return;
			}
			else
			{
				EffectRed();
				m_bActivated = true;
			}
		}
		/*
		if (pBall->GetType() == 4)
		{
			if (m_bIsGhost)
			{
				DevMsg ("CVecBox: Hit by a Ball But we are a ghost.\n");
				return;
			}
			else
			{
				EffectPurple();
				m_bActivated = true;
			}
		}

		if (pBall->GetType() == 5)
		{
			if (m_bIsGhost)
			{
				DevMsg ("CVecBox: Hit by a Ball But we are a ghost.\n");
				return;
			}
			else
			{
				EffectOrange();
				m_bActivated = true;
			}
		}
		*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::EffectBlue()
{
	if ( m_bIsGhost)
		return;

	m_bIsTouching = true;

	// If we get hit again by the same ball, reset!
	if ( m_intLastBallHit == VECTRONIC_BALL_BLUE )
	{
		ResetBox();
		m_bActivated = false;
	}
	else //If we are hit with the blue ball, and we have not been before.
	{
		DevMsg ("CVecBox: Hit by Ball 0\n");
		m_intLastBallHit = VECTRONIC_BALL_BLUE;
		VPhysicsGetObject()->EnableGravity( false );

		// Only do this when we got hit, and we were not protected.
		if (!m_bIsProtected)
		{
			m_OnBlueBall.FireOutput( this, this );
			EmitSound( "VecBox.Activate" );
		}
		else
		{
			EmitSound("VecBox.ClearShield");
		}

		// No matter what, kill our ghost if we have one!
		if (m_bHasGhost)
		{
			KillGhost();
		}

		// We are no longer protected
		m_bIsProtected = false;

		// Update our color!
		UpdateColor();
	}

	// Think sets skin and sprite.
	SetNextThink( gpGlobals->curtime + .1 );

	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::EffectGreen()
{
	if ( m_bIsGhost)
		return;

	m_bIsTouching = true;

	// If we get hit again by the same ball, reset!
	if ( m_intLastBallHit == VECTRONIC_BALL_GREEN )
	{
		ResetBox();
		m_bActivated = false;
	}
	else //If we are hit with the green ball, and we have not been before.
	{
		DevMsg ("CVecBox: Hit by Ball 1\n");
		m_intLastBallHit = VECTRONIC_BALL_GREEN;
		VPhysicsGetObject()->EnableGravity( true );

		// Only do this when we got hit, and we were not protected.
		if (!m_bIsProtected)
		{
			m_OnGreenBall.FireOutput( this, this );
			EmitSound( "VecBox.Activate" );
		}
		else
		{
			EmitSound("VecBox.ClearShield");
		}

		// No matter what, kill our ghost if we have one!
		if (m_bHasGhost && !m_bIsProtected)
		{
			KillGhost();
		}

		// We are no longer protected
		//m_bIsProtected = false;

		// Update our color!
		UpdateColor();
	}

	// Think function will make a new ghost for us.
	SetNextThink( gpGlobals->curtime + .1 );

	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::EffectPurple()
{
	if ( m_bIsGhost)
		return;

	m_bIsTouching = true;

	// OK! Yellow ball is diffrent. Instead of clearing out the
	// previous activation and replacing it with another one,
	// The yellow ball adds a sheild to the previous activaton. 

	// Don't protect null boxes!
	if ( m_intLastBallHit == VECTRONIC_BALL_NONE)
		return;

	// If we are yellow and we are hit again by yellow, Restore our previous!
	if ( m_intLastBallHit == VECTRONIC_BALL_PURPLE)
	{
		if ( m_intProtecting == VECTRONIC_BALL_BLUE )
		{
			EffectBlue();
		}
		else if ( m_intProtecting == VECTRONIC_BALL_GREEN )
		{
			EffectGreen();
		}
		/*
		else if ( m_intProtecting == VECTRONIC_BALL_RED )
		{
			EffectRed();
		}
		else if ( m_intProtecting == VECTRONIC_BALL_YELLOW )
		{
			EffectYellow();
		}
		else if ( m_intProtecting == VECTRONIC_BALL_ORANGE )
		{
			EffectOrange();
		}
		*/
		return;
	}

	// Save our previous activations in another int.
	if ( m_intLastBallHit == VECTRONIC_BALL_BLUE )
	{
		SetGlowVector(0.5254901960784314f, 0.7803921568627451f, 0.7725490196078432f );
		AddGlowEffect();

		m_intProtecting = VECTRONIC_BALL_BLUE;
	}
	else if ( m_intLastBallHit == VECTRONIC_BALL_GREEN )
	{
		SetGlowVector(0.4392156862745098f, 0.6901960784313725f, 0.42745098039215684f );
		AddGlowEffect();

		m_intProtecting = VECTRONIC_BALL_GREEN;
	}
	else if ( m_intLastBallHit == VECTRONIC_BALL_RED )
	{

		SetGlowVector(0.7568627450980392f, 0.48627450980392156f, 0.4823529411764706f );
		AddGlowEffect();

		m_intProtecting = VECTRONIC_BALL_RED;
	}
		/*
	else if ( m_intLastBallHit == VECTRONIC_BALL_PURPLE )
	{
		SetGlowVector(0.60f, 0.0f, 0.60f );
		AddGlowEffect();

		m_intProtecting = VECTRONIC_BALL_PURPLE;
	}
	else if ( m_intLastBallHit == VECTRONIC_BALL_ORANGE )
	{
		SetGlowVector(0.60f, 0.50f, 0.0f );
		AddGlowEffect();

		m_intProtecting = VECTRONIC_BALL_ORANGE;
	}
	*/

	// This bool is used to tell the box, and other entites that we are protected!
	if (m_intProtecting > 0)
	{
		DevMsg ("CVecBox: Saved previous activaton!\n");
		m_bIsProtected = true;
	}

	// Do the usual bullshit.
	DevMsg ("CVecBox: Hit by Ball 2\n");
	m_OnPurpleBall.FireOutput( this, this );
	EmitSound( "VecBox.Activate" );

	m_intLastBallHit = VECTRONIC_BALL_PURPLE;
	UpdateColor();

	SetNextThink( gpGlobals->curtime + .1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::EffectRed()
{
	if ( m_bIsGhost)
		return;

	m_bIsTouching = true;
	VPhysicsGetObject()->EnableGravity( true );
	// If we get hit again by the same ball, reset!
	if ( m_intLastBallHit == VECTRONIC_BALL_RED )
	{
		ResetBox();
		m_bActivated = false;
	}
	else //If we are hit with the red ball, and we have not been before.
	{
		DevMsg ("CVecBox: Hit by Ball 3\n");
		m_intLastBallHit = VECTRONIC_BALL_RED;

		// Only do this when we got hit, and we were not protected.
		if (!m_bIsProtected)
		{
			m_OnRedBall.FireOutput( this, this );
			EmitSound( "VecBox.Activate" );
		}
		else
		{
			EmitSound("VecBox.ClearShield");
		}

		// No matter what, kill our ghost if we have one!
		if (m_bHasGhost)
		{
			KillGhost();
		}

		// We are no longer protected
		m_bIsProtected = false;

		// Update our color!
		UpdateColor();
	}

	// Think sets skin and sprite.
	SetNextThink( gpGlobals->curtime + .1 );

	return;
}

/*
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::EffectPurple()
{
	if ( m_bIsGhost)
		return;

	m_bIsTouching = true;
	VPhysicsGetObject()->EnableGravity( true );
	// If we get hit again by the same ball, reset!
	if ( m_intLastBallHit == VECTRONIC_BALL_PURPLE )
	{
		ResetBox();
		m_bActivated = false;
	}
	else //If we are hit with the blue ball, and we have not been before.
	{
		DevMsg ("CVecBox: Hit by Ball 4\n");
		m_intLastBallHit = VECTRONIC_BALL_PURPLE;

		// Only do this when we got hit, and we were not protected.
		if (!m_bIsProtected)
		{
			m_OnPurpleBall.FireOutput( this, this );
			EmitSound( "VecBox.Activate" );
		}
		else
		{
			EmitSound("VecBox.ClearShield");
		}

		// No matter what, kill our ghost if we have one!
		if (m_bHasGhost)
		{
			KillGhost();
		}

		// We are no longer protected
		m_bIsProtected = false;

		// Update our color!
		UpdateColor();
	}

	// Think sets skin and sprite.
	SetNextThink( gpGlobals->curtime + .1 );

	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::EffectOrange()
{
	if ( m_bIsGhost)
		return;

	m_bIsTouching = true;
	VPhysicsGetObject()->EnableGravity( true );
	// If we get hit again by the same ball, reset!
	if ( m_intLastBallHit == VECTRONIC_BALL_ORANGE )
	{
		ResetBox();
		m_bActivated = false;
	}
	else //If we are hit with the blue ball, and we have not been before.
	{
		DevMsg ("CVecBox: Hit by Ball 5\n");
		m_intLastBallHit = VECTRONIC_BALL_ORANGE;

		// Only do this when we got hit, and we were not protected.
		if (!m_bIsProtected)
		{
			m_OnOrangeBall.FireOutput( this, this );
			EmitSound( "VecBox.Activate" );
		}
		else
		{
			EmitSound("VecBox.ClearShield");
		}

		// No matter what, kill our ghost if we have one!
		if (m_bHasGhost)
		{
			KillGhost();
		}

		// We are no longer protected
		m_bIsProtected = false;

		// Update our color!
		UpdateColor();
	}

	// Think sets skin and sprite.
	SetNextThink( gpGlobals->curtime + .1 );

	return;
}

*/
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVecBox::Clear( void )
{
	if (!m_bActivated )
		return;

	// Hold up! If we are saved by a purple ball, clear only the purple ball and return to the previous activation!
	if (m_bIsProtected)
	{
		if ( m_intProtecting == VECTRONIC_BALL_BLUE )
		{
			DevMsg ("CVecBox: Going back to previous activation!\n");
			EffectBlue();
		}
		else if ( m_intProtecting == VECTRONIC_BALL_GREEN )
		{
			DevMsg ("CVecBox: Going back to previous activation!\n");

			// Instead of sending it back to EffectGreen, we do the same thing
			// only this time we don't kill and remake our ghost!
			m_intLastBallHit = VECTRONIC_BALL_GREEN;
			VPhysicsGetObject()->EnableGravity( true );
			EmitSound("VecBox.ClearShield");

			// We are no longer protected
			m_bIsProtected = false;

			// Update our color!
			UpdateColor();
		}
		else if ( m_intProtecting == VECTRONIC_BALL_RED )
		{
			DevMsg ("CVecBox: Going back to previous activation!\n");
			EffectRed();
		}

		m_intProtecting = 0;
		m_bIsProtected = false;
		return;
	}

	//Debug msg
	DevMsg ("CVecBox: Ok, we are activated. CLEAN ME!!\n");

	//Reset Vphysics
	VPhysicsGetObject()->Wake();
	VPhysicsGetObject()->EnableGravity( true );

	ResetBox();
}

void CVecBox::ResetBox()
{
	DevMsg ("CVecBox: A Ball of this type already hit this. Resetting!\n");
	m_OnReset.FireOutput( this, this );

	// If we are not 0, play the sound and reset it to 0. Because 0 means no Punts.
	if (m_intLastBallHit != VECTRONIC_BALL_NONE)
	{
		//Play Sound Zzzzz
		EmitSound( "VecBox.Deactivate" );

		//Set int to 0
		m_intLastBallHit = VECTRONIC_BALL_NONE;
	}
	
	// Ok We are off.
	m_bActivated = false;

	// We are no longer protected
	m_bIsProtected = false;

	// No matter what, kill our ghost if we have one!
	if (m_bHasGhost)
	{
		KillGhost();
	}

	//Reset Vphysics
	VPhysicsGetObject()->EnableGravity( true );

	RemoveGlowEffect();

	// Update our color back to white.
	UpdateColor();
}

void CVecBox::UpdateColor()
{
	if ( m_intLastBallHit == VECTRONIC_BALL_NONE)
	{
		m_nSkin = 0;

		//#ifdef GE_GLOWS_ENABLE
		//SetGlow(false);
		//#endif

		DispatchParticleEffect (VECBOX_CLEAR_PARTICLE, PATTACH_ABSORIGIN, this );
	}

	if ( m_intLastBallHit == VECTRONIC_BALL_BLUE)
	{
		m_nSkin = 1;

		//AddGlowEffect();

		//#ifdef GE_GLOWS_ENABLE
		//color32 outline0clr = { BALL0_COLOR };
		//SetGlow(true, Color(outline0clr.r, outline0clr.g, outline0clr.b, outline0clr.a));
		//#endif

		RemoveGlowEffect();

		DispatchParticleEffect (VECBOX_HIT0_PARTICLE, PATTACH_ABSORIGIN, this );
		
	}

	if ( m_intLastBallHit == VECTRONIC_BALL_GREEN)
	{
		m_nSkin = 2;

		//AddGlowEffect();

		//#ifdef GE_GLOWS_ENABLE
		//color32 outline1clr = { BALL1_COLOR };
		//SetGlow(true, Color(outline1clr.r, outline1clr.g, outline1clr.b, outline1clr.a));
		//#endif

		RemoveGlowEffect();

		DispatchParticleEffect (VECBOX_HIT1_PARTICLE, PATTACH_ABSORIGIN, this );
		
	}

	if ( m_intLastBallHit == VECTRONIC_BALL_PURPLE)
	{
		m_nSkin = 3;

		//#ifdef GE_GLOWS_ENABLE
		//color32 outline1clr = { BALL2_COLOR };
		//SetGlow(true, Color(outline1clr.r, outline1clr.g, outline1clr.b, outline1clr.a));
		//#endif

		DispatchParticleEffect (VECBOX_HIT2_PARTICLE, PATTACH_ABSORIGIN, this );

		
	}

	if ( m_intLastBallHit == VECTRONIC_BALL_RED)
	{
		m_nSkin = 4;

		//#ifdef GE_GLOWS_ENABLE
		//color32 outline1clr = { BALL2_COLOR };
		//SetGlow(true, Color(outline1clr.r, outline1clr.g, outline1clr.b, outline1clr.a));
		//#endif

		DispatchParticleEffect (VECBOX_HIT3_PARTICLE, PATTACH_ABSORIGIN, this );

		RemoveGlowEffect();
	}

	/*
	if ( m_intLastBallHit == VECTRONIC_BALL_PURPLE)
	{
		m_nSkin = 5;

		//#ifdef GE_GLOWS_ENABLE
		//color32 outline1clr = { BALL2_COLOR };
		//SetGlow(true, Color(outline1clr.r, outline1clr.g, outline1clr.b, outline1clr.a));
		//#endif

		DispatchParticleEffect (VECBOX_HIT4_PARTICLE, PATTACH_ABSORIGIN, this );

		RemoveGlowEffect();
	}

	if ( m_intLastBallHit == VECTRONIC_BALL_ORANGE)
	{
		m_nSkin = 6;

		//#ifdef GE_GLOWS_ENABLE
		//color32 outline1clr = { BALL2_COLOR };
		//SetGlow(true, Color(outline1clr.r, outline1clr.g, outline1clr.b, outline1clr.a));
		//#endif

		DispatchParticleEffect (VECBOX_HIT5_PARTICLE, PATTACH_ABSORIGIN, this );

		RemoveGlowEffect();

	}
	*/
	//if(m_bSpawnedSprites && !m_bIsGhost )
	//{
		int i;
		for ( i = 0; i < 6; i++ )
		{
			if (m_hSprite[i] != NULL )
			{
				if ( m_intLastBallHit == VECTRONIC_BALL_NONE)
				{
					m_hSprite[i]->SetRenderColor(255,255,255);
				}
				if ( m_intLastBallHit == VECTRONIC_BALL_BLUE)
				{
					m_hSprite[i]->SetRenderColor(BALL0_COLOR);
				}
				if ( m_intLastBallHit == VECTRONIC_BALL_GREEN)
				{
					m_hSprite[i]->SetRenderColor(BALL1_COLOR);
				}
				if ( m_intLastBallHit == VECTRONIC_BALL_PURPLE)
				{
					m_hSprite[i]->SetRenderColor(BALL2_COLOR);
				}
				if ( m_intLastBallHit == VECTRONIC_BALL_RED)
				{
					m_hSprite[i]->SetRenderColor(BALL3_COLOR);
				}
				/*
				if ( m_intLastBallHit == VECTRONIC_BALL_PURPLE)
				{
					m_hSprite[i]->SetRenderColor(BALL4_COLOR);
					//m_hSprite[i]->SetColor(112, 176, 109);
				}
				if ( m_intLastBallHit == VECTRONIC_BALL_ORANGE)
				{
					m_hSprite[i]->SetRenderColor(BALL5_COLOR);
					//m_hSprite[i]->SetColor(112, 176, 109);
				}
				*/
			}
		}
	//}

}
void CVecBox::MakeGhost()
{
	m_bHasGhost = true;
	//CVecBox *pBox = static_cast<CVecBox*>( CreateEntityByName( "prop_vecbox" ) );
	m_hBox = (CVecBox *) CreateEntityByName( "prop_vecbox" );

	if ( m_hBox == NULL )
		return;

	Vector vecAbsOrigin = GetAbsOrigin();
	Vector zaxis;

	m_hBox->SetAbsOrigin( vecAbsOrigin );

	Vector vDirection;

	vDirection *= 0.0f;

	// Woof Woof, The ghost should get the boxes angles and then spawn.
	QAngle qAngles = this->GetAbsAngles();
	SetOwnerEntity( m_hBox );
	GetOwnerEntity()->SetAbsAngles(qAngles);

	m_hBox->m_bIsGhost = true;
	m_hBox->Spawn();
	m_hBox->MakeGhost2();
}

void CVecBox::MakeGhost2()
{
	// If we make it here, we are a ghost!
	DevMsg ("CVecBox: We have made a ghost!\n");
	SetModel(VECBOX_MODEL_GHOST);

	SetRenderColor(230,230,230);

	SetGlowVector( 0.76f, 0.76f, 0.76f);
	//SetGlowVector( 0.6078431372549019f, 0.6862745098039216f, 0.6039215686274509f);
	AddGlowEffect();

	AddEffects(EF_NOSHADOW);
	m_nSkin = 0;
	VPhysicsGetObject()->EnableMotion ( false );
	SetSolid( SOLID_VPHYSICS );

}

void CVecBox::KillGhost()
{
	variant_t emptyVariant;
	GetOwnerEntity()->AcceptInput( "KillGhost", this, this, emptyVariant, 0 );
	DevMsg ("CVecBox: Ghost is dead\n");
	m_bIsGhost = false;
	m_bHasGhost = false;
}

// ###################################################################
//	> FilterClass
// ###################################################################
class CFilterBoxType : public CBaseFilter
{
	DECLARE_CLASS( CFilterBoxType, CBaseFilter );
	DECLARE_DATADESC();

public:
	int m_iBoxType;

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		CVecBox *pBox = dynamic_cast<CVecBox*>(pEntity );

		if ( pBox )
		{
			return pBox->GetState() == m_iBoxType;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS( filter_vecbox_activation, CFilterBoxType );

BEGIN_DATADESC( CFilterBoxType )
	// Keyfields
	DEFINE_KEYFIELD( m_iBoxType,	FIELD_INTEGER,	"boxtype" ),
END_DATADESC()