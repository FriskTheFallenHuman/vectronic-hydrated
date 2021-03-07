//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef VectronicPLAYER_H
#define VectronicPLAYER_H
#ifdef _WIN32
#pragma once
#endif

class CVectronicPlayer;

#include "basemultiplayerplayer.h"
#include "baseflex.h"
#include "simtimer.h"
#include "soundenvelope.h"
#include "vectronic_playeranimstate.h"
#include "vectronic_player_shared.h"
#include "vectronic_gamerules.h"
#include "utldict.h"
#include "hintmessage.h"

// How fast can the player walk?
#define PLAYER_WALK_SPEED 150

// Or pickup limits.
#define PLAYER_MAX_LIFT_MASS 85
#define PLAYER_MAX_LIFT_SIZE 128

// Sounds.
#define SOUND_HINT		"Hint.Display"
#define SOUND_WHOOSH	"Player.FallWoosh"
#define SOUND_BURN		"HL2Player.BurnPain"
#define SOUND_FLASHLIGHT_ON	"HL2Player.FlashlightOn"
#define SOUND_FLASHLIGHT_OFF	"HL2Player.FlashlightOff"

//-----------------------------------------------------------------------------
// Purpose: Used to relay outputs/inputs from the player to the world and viceversa
//-----------------------------------------------------------------------------
class CLogicPlayerProxy : public CLogicalEntity
{
	DECLARE_CLASS( CLogicPlayerProxy, CLogicalEntity );

private:

	DECLARE_DATADESC();

public:

	COutputEvent m_OnFlashlightOn;
	COutputEvent m_OnFlashlightOff;
	COutputEvent m_PlayerHasAmmo;
	COutputEvent m_PlayerHasNoAmmo;
	COutputEvent m_PlayerDied;

	COutputInt m_RequestedPlayerHealth;

	void InputRequestPlayerHealth( inputdata_t &inputdata );
	void InputSetFlashlightSlowDrain( inputdata_t &inputdata );
	void InputSetFlashlightNormalDrain( inputdata_t &inputdata );
	void InputSetPlayerHealth( inputdata_t &inputdata );
	void InputRequestAmmoState( inputdata_t &inputdata );
	void InputSuppressCrosshair( inputdata_t &inputdata );

	void Activate ( void );

	bool PassesDamageFilter( const CTakeDamageInfo &info );

	EHANDLE m_hPlayer;
};

//=============================================================================
// >> CVectronicPlayer
//=============================================================================
class CVectronicPlayer : public CBaseMultiplayerPlayer
{
public:
	DECLARE_CLASS( CVectronicPlayer, CBaseMultiplayerPlayer );

	CVectronicPlayer();
	~CVectronicPlayer( void );
	
	static CVectronicPlayer *CreatePlayer( const char *className, edict_t *ed )
	{
		CVectronicPlayer::s_PlayerEdict = ed;
		return (CVectronicPlayer*)CreateEntityByName( className );
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	// This passes the event to the client's and server's CVectronicPlayerAnimState.
	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	void SetupBones( matrix3x4_t *pBoneToWorld, int boneMask );

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void PostThink( void );
	virtual void PreThink( void );
	virtual bool ClientCommand( const CCommand &args );
	virtual bool BecomeRagdollOnClient( const Vector &force );
	virtual bool WantsLagCompensationOnEntity( const CBaseEntity *pEntity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;
	virtual void FireBullets ( const FireBulletsInfo_t &info );
	virtual void UpdateOnRemove( void );
	virtual void DeathSound( const CTakeDamageInfo &info );
	virtual CBaseEntity* EntSelectSpawnPoint( void );
	virtual void UpdateClientData( void );
	virtual void Splash( void );

	CLogicPlayerProxy	*GetPlayerProxy( void );
	void FirePlayerProxyOutput( const char *pszOutputName, variant_t variant, CBaseEntity *pActivator, CBaseEntity *pCaller );
	EHANDLE m_hPlayerProxy;	// Handle to a player proxy entity for quicker reference
		
	int FlashlightIsOn( void );
	void FlashlightTurnOn( void );
	void FlashlightTurnOff( void );

	virtual Vector GetAutoaimVector( float flDelta );

	void CreateRagdollEntity( void );
	void NoteWeaponFired( void );
	void SetAnimation( PLAYER_ANIM playerAnim );
	void  PickDefaultSpawnTeam( void );

	// Viewmodel + Weapon
	virtual void CreateViewModel( int viewmodelindex = 0 );
	virtual void Weapon_Equip ( CBaseCombatWeapon *pWeapon );
	virtual bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0);
	virtual bool BumpWeapon( CBaseCombatWeapon *pWeapon );
	CNetworkVar( int, m_iShotsFired );	// number of shots fired recently

	float m_flNextMouseoverUpdate;

	// Use + Pickup
	virtual void PlayerUse( void );
	virtual void PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize );
	virtual void ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldingThis );
	virtual void ClearUsePickup();
	virtual bool CanPickupObject( CBaseEntity *pObject, float massLimit, float sizeLimit );
	CNetworkVar( bool, m_bPlayerPickedUpObject );
	bool PlayerHasObject() { return m_bPlayerPickedUpObject; }

	//Walking
	virtual void StartWalking( void );
	virtual void StopWalking( void );
	CNetworkVarForDerived( bool, m_fIsWalking );
	virtual bool IsWalking( void ) { return m_fIsWalking; }
	virtual void  HandleSpeedChanges( void );

	// Damage
	virtual bool PassesDamageFilter( const CTakeDamageInfo &info );
	virtual int OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void OnDamagedByExplosion( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );

	//Regenerate
	float	m_fRegenRemander;
	float	m_fTimeLastHurt;

	// CHEAT
	virtual void CheatImpulseCommands( int iImpulse );
	virtual void GiveAllItems( void );
	virtual void GiveDefaultItems( void );

	//Air Simulation
	CSoundPatch		*m_pAirSound;
	virtual void UpdateWooshSounds( void );
	virtual void CreateSounds( void );
	virtual void StopLoopingSounds( void );

	// Hints
	virtual void HintMessage( const char *pMessage, bool bDisplayIfDead, bool bOverrideClientSettings = false, bool bQuiet = false ); // Displays a hint message to the player
	CHintMessageQueue *m_pHintMessageQueue;
	unsigned int m_iDisplayHistoryBits;
	bool m_bShowHints;

	Vector m_vecTotalBulletForce;	//Accumulator for bullet force in a single frame

	// Tracks our ragdoll entity.
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 

	// Player avoidance
	virtual	bool ShouldCollide( int collisionGroup, int contentsMask ) const;
	void VectronicPushawayThink( void );

	Activity TranslateActivity( Activity ActToTranslate, bool *pRequired = NULL );

#ifdef PLAYER_MOUSEOVER_HINTS
	void UpdateMouseoverHints();
#endif

	Class_T Classify ( void );

public:
	void FireBullet( 
		Vector vecSrc, 
		const QAngle &shootAngles, 
		float vecSpread, 
		int iDamage, 
		int iBulletType,
		CBaseEntity *pevAttacker,
		bool bDoEffects,
		float x,
		float y );
private:
		float m_flTimeUseSuspended;

		Vector m_vecMissPositions[16];
		int m_nNumMissPositions;

private:

	CVectronicPlayerAnimState *m_PlayerAnimState;

	CNetworkQAngle( m_angEyeAngles );

	int m_iLastWeaponFireUsercmd;
	CNetworkVar( int, m_iSpawnInterpCounter );

	CNetworkVar( int, m_cycleLatch ); // Network the cycle to clients periodically
	CountdownTimer m_cycleLatchTimer;
};

inline CVectronicPlayer *To_VectronicPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CVectronicPlayer*>( pEntity );
}

// -------------------------------------------------------------------------------- //
// Ragdoll entities.
// -------------------------------------------------------------------------------- //
class CVectronicRagdoll : public CBaseFlex
{
public:
	DECLARE_CLASS( CVectronicRagdoll, CBaseFlex );
	DECLARE_SERVERCLASS();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState() {	return SetTransmitState( FL_EDICT_ALWAYS );	}

public:
	// In case the client has the player entity, we transmit the player index.
	// In case the client doesn't have it, we transmit the player's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

#endif //VectronicPLAYER_H
