//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL2.
//
//=============================================================================//

#include "cbase.h"
#include "basevectroniccombatweapon_shared.h"
#include "vectronic_player.h"
#include "globalstate.h"
#include "game.h"
#include "gamerules.h"
#include "vectronic_player_shared.h"
#include "predicted_viewmodel.h"
#include "in_buttons.h"
#include "vectronic_gamerules.h"
#include "KeyValues.h"
#include "team.h"
#include "eventqueue.h"
#include "gamestats.h"
#include "tier0/vprof.h"
#include "bone_setup.h"
#include "datacache/imdlcache.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "obstacle_pushaway.h"
#include "ilagcompensationmanager.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h" 
#include "simtimer.h"
#include "player_pickup.h"
#include "trains.h"
#include "ai_basenpc.h"
#include "vphysics/player_controller.h"
#include "soundenvelope.h"
#include "ai_speech.h"		
#include "sceneentity.h"
#include "hintmessage.h"
#include "items.h"
#include "weapon_vecgun.h"
#include "filters.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CBaseEntity *g_pLastSpawn;
extern int	gEvilImpulse101;
extern ConVar flashlight;

// Show annotations?
ConVar hud_show_annotations( "hud_show_annotations", "1" );

ConVar sv_regeneration ( "sv_regeneration", "1", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sv_regeneration_wait_time ( "sv_regeneration_wait_time", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sv_regeneration_rate ( "sv_regeneration_rate", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sv_regen_interval( "sv_regen_interval", "20", FCVAR_REPLICATED | FCVAR_CHEAT, "Set what interval of health to regen to.\n    i.e. if this is set to the default value (20), if you are damaged to 75 health, you'll regenerate to 80 health.\n    Set this to 0 to disable this mechanic.");
ConVar sv_regen_limit( "sv_regen_limit", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Set the limit as to how much health you can regen to.\n    i.e. if this is set at 50, you can only regen to 50 health. If you are hurt and you are above 50 health, you will not regen.\n    Set this to 0 to disable this mechanic.");

#define PUSHAWAY_THINK_CONTEXT	"VectronicPushawayThink"
#define CYCLELATCH_UPDATE_INTERVAL	0.2f
#define PLAYER_PHYSDAMAGE_SCALE 4.0f

// ***************** CTEPlayerAnimEvent **********************

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //
class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name ) {}

	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nData ), 32 )
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
	CPVSFilter filter( (const Vector&)pPlayer->EyePosition() );

	//Tony; use prediction rules.
	filter.UsePredictionRules();
	
	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

// ***************** CVectronicPlayer **********************

LINK_ENTITY_TO_CLASS( player, CVectronicPlayer );
PRECACHE_REGISTER ( player );

extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

//Tony; this should ideally be added to dt_send.cpp
void* SendProxy_SendNonLocalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	pRecipients->SetAllRecipients();
	pRecipients->ClearRecipient( objectID - 1 );
	return ( void * )pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendNonLocalDataTable );


BEGIN_SEND_TABLE_NOBASE( CVectronicPlayer, DT_VectronicLocalPlayerExclusive )
	// send a hi-res origin to the local player for use in prediction
	SendPropVector( SENDINFO( m_vecOrigin ), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat( SENDINFO_VECTORELEM( m_angEyeAngles, 0 ), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CVectronicPlayer, DT_VectronicNonLocalPlayerExclusive )
	// send a lo-res origin to other players
	SendPropVector( SENDINFO( m_vecOrigin ), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat( SENDINFO_VECTORELEM( m_angEyeAngles, 0 ), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM( m_angEyeAngles, 1 ), 10, SPROP_CHANGES_OFTEN ),
	// Only need to latch cycle for other players
	// If you increase the number of bits networked, make sure to also modify the code below and in the client class.
	SendPropInt( SENDINFO( m_cycleLatch ), 4, SPROP_UNSIGNED ),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST( CVectronicPlayer, DT_VectronicPlayer )
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),

	// playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	SendPropExclude( "DT_BaseFlex", "m_flexWeight" ),
	SendPropExclude( "DT_BaseFlex", "m_blinktoggle" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

	// Data that only gets sent to the local player.
	SendPropDataTable( "vectronic_localdata", 0, &REFERENCE_SEND_TABLE(DT_VectronicLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
	// Data that gets sent to all other players
	SendPropDataTable( "vectronic_nonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_VectronicNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),

	SendPropEHandle( SENDINFO( m_hRagdoll ) ),
	SendPropInt( SENDINFO( m_iSpawnInterpCounter ), 4 ),
	SendPropBool( SENDINFO( m_bPlayerPickedUpObject) ),
	SendPropInt( SENDINFO( m_iShotsFired ), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()

BEGIN_DATADESC( CVectronicPlayer )
	DEFINE_FIELD( m_bPlayerPickedUpObject, FIELD_BOOLEAN ),
	DEFINE_AUTO_ARRAY( m_vecMissPositions, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_nNumMissPositions, FIELD_INTEGER ),
	DEFINE_SOUNDPATCH( m_pAirSound ),
	DEFINE_FIELD( m_fTimeLastHurt, FIELD_TIME )
END_DATADESC()

// ***************** CVectronicRagdoll **********************

LINK_ENTITY_TO_CLASS( vectronic_ragdoll, CVectronicRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CVectronicRagdoll, DT_VectronicRagdoll )
	SendPropVector( SENDINFO( m_vecRagdollOrigin ), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt( SENDINFO( m_nForceBone ), 8, 0 ),
	SendPropVector( SENDINFO( m_vecForce ), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) )
END_SEND_TABLE()

#pragma warning( disable : 4355 )

CVectronicPlayer::CVectronicPlayer()
{
	//Tony; create our player animation state.
	m_PlayerAnimState = CreatePlayerAnimState( this );
	UseClientSideAnimation();

	m_angEyeAngles.Init();

	m_iLastWeaponFireUsercmd = 0;

	m_iSpawnInterpCounter = 0;

	m_cycleLatch = 0;
	m_cycleLatchTimer.Invalidate();

	m_nNumMissPositions = 0;

	// Set up the hints.
	m_pHintMessageQueue = new CHintMessageQueue(this);
	m_iDisplayHistoryBits = 0;
	m_bShowHints = true;

	if ( m_pHintMessageQueue )
		m_pHintMessageQueue->Reset();
	
	// We did not fire any shots.
	m_iShotsFired = 0;

	m_fRegenRemander = 0;
}

CVectronicPlayer::~CVectronicPlayer( void )
{
	m_PlayerAnimState->Release();

	delete m_pHintMessageQueue;
	m_pHintMessageQueue = NULL;

	m_flNextMouseoverUpdate = gpGlobals->curtime;
}

void CVectronicPlayer::UpdateOnRemove( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	BaseClass::UpdateOnRemove();
}

void CVectronicPlayer::Precache( void )
{
	BaseClass::Precache();

	// Sounds
	PrecacheScriptSound( SOUND_HINT );
	PrecacheScriptSound( SOUND_WHOOSH );
	PrecacheScriptSound( SOUND_BURN );
	PrecacheScriptSound( SOUND_FLASHLIGHT_ON );
	PrecacheScriptSound( SOUND_FLASHLIGHT_OFF );

	// Last, precache the player model or else the game will crash when the player dies.
	PrecacheModel ( "models/player/pipdummy_female_pm.mdl" );
}

void CVectronicPlayer::GiveAllItems( void )
{
	EquipSuit();

	// If you want the player to always start with something, give it
	// to them here.
	GiveNamedItem( "weapon_vecgun" );

	CWeaponVecgun *pVecGun = static_cast<CWeaponVecgun*>( Weapon_OwnsThisType( "weapon_vecgun" ) );
	if ( pVecGun )
	{
		pVecGun->SetCanFireBlue();
		pVecGun->SetCanFireGreen();
		pVecGun->SetCanFireYellow();
	}
}

void CVectronicPlayer::GiveDefaultItems( void )
{
}

void CVectronicPlayer::PickDefaultSpawnTeam( void )
{
	if ( GetTeamNumber() == 0 )
	{
		if ( rHGameRules()->IsDeathmatch() == false )
		{
			if ( GetModelPtr() == NULL )
				ChangeTeam( TEAM_UNASSIGNED );
		}
	}
}

void CVectronicPlayer::VectronicPushawayThink( void )
{
	// Push physics props out of our way.
	PerformObstaclePushaway( this );
	SetNextThink( gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, PUSHAWAY_THINK_CONTEXT );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVectronicPlayer::Spawn( void )
{
	PickDefaultSpawnTeam();

	// Dying without a player model crashes the client
	SetModel( "models/player/pipdummy_female_pm.mdl" );
	SetMaxSpeed( PLAYER_WALK_SPEED );

	BaseClass::Spawn();

	StartWalking();	
	
	if ( !IsObserver() )
	{
		pl.deadflag = false;
		RemoveEffects( EF_NODRAW );
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		
		GiveDefaultItems();
	}

#ifdef PLAYER_MOUSEOVER_HINTS
	m_iDisplayHistoryBits &= ~DHM_ROUND_CLEAR;
	SetLastSeenEntity ( NULL );
#endif

	// We did not fire any shots.
	m_iShotsFired = 0;

	GetPlayerProxy();

	RemoveEffects( EF_NOINTERP );

	m_nRenderFX = kRenderNormal;

	m_Local.m_iHideHUD = 0;
	
	AddFlag( FL_ONGROUND ); // set the player on the ground at the start of the round.

	m_impactEnergyScale = PLAYER_PHYSDAMAGE_SCALE;

	m_iSpawnInterpCounter = (m_iSpawnInterpCounter + 1) % 8;

	m_Local.m_bDucked = false;

	SetPlayerUnderwater( false );

	m_cycleLatchTimer.Start( CYCLELATCH_UPDATE_INTERVAL );

	//Tony; do the spawn animevent
	DoAnimationEvent( PLAYERANIMEVENT_SPAWN );

	SetContextThink( &CVectronicPlayer::VectronicPushawayThink, gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, PUSHAWAY_THINK_CONTEXT );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
Class_T  CVectronicPlayer::Classify ( void )
{
	return CLASS_PLAYER;
}

bool CVectronicPlayer::CanPickupObject( CBaseEntity *pObject, float massLimit, float sizeLimit )
{
	// reep: Ported from the base since in the base this is HL2 exclusive. Yeah, don't you LOVE base code? 

	//Must be valid
	if ( pObject == NULL )
		return false;

	//Must move with physics
	if ( pObject->GetMoveType() != MOVETYPE_VPHYSICS )
		return false;

	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = pObject->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );

	//Must have a physics object
	if (!count)
		return false;

	float objectMass = 0;
	bool checkEnable = false;
	for ( int i = 0; i < count; i++ )
	{
		objectMass += pList[i]->GetMass();
		if ( !pList[i]->IsMoveable() )
		{
			checkEnable = true;
		}
		if ( pList[i]->GetGameFlags() & FVPHYSICS_NO_PLAYER_PICKUP )
			return false;
		if ( pList[i]->IsHinged() )
			return false;
	}

	//Must be under our threshold weight
	if ( massLimit > 0 && objectMass > massLimit )
		return false;

	if ( sizeLimit > 0 )
	{
		const Vector &size = pObject->CollisionProp()->OBBSize();
		if ( size.x > sizeLimit || size.y > sizeLimit || size.z > sizeLimit )
			return false;
	}

	return true;
}

void CVectronicPlayer::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
	// can't pick up what you're standing on
	if ( GetGroundEntity() == pObject )
	{
		HintMessage( "#Player_CantPickUp", false );
		return;
	}

	if ( bLimitMassAndSize == true )
	{
		if ( CanPickupObject( pObject, PLAYER_MAX_LIFT_MASS, PLAYER_MAX_LIFT_SIZE ) == false )
		{
			HintMessage( "#Player_ObjectTooHeavy", false );
			return;
		}
	}

	// Can't be picked up if NPCs are on me
	if ( pObject->HasNPCsOnIt() )
		return;

	// Bool is to tell the client that we have an object. This is incase you want to change the crosshair 
	// or something for your project.
	m_bPlayerPickedUpObject = true;

	PlayerPickupObject( this, pObject );

}

void CVectronicPlayer::ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldingThis )
{
	m_bPlayerPickedUpObject = false;
	BaseClass::ForceDropOfCarriedPhysObjects( pOnlyIfHoldingThis );
}

void CVectronicPlayer::PlayerUse ( void )
{
	// Was use pressed or released?
	if ( ! ((m_nButtons | m_afButtonPressed | m_afButtonReleased) & IN_USE) )
		return;

	if ( m_afButtonPressed & IN_USE )
	{
		// Currently using a latched entity?
		if ( ClearUseEntity() )
		{
			if (m_bPlayerPickedUpObject)
			{
				m_bPlayerPickedUpObject = false;
			}
			return;
		}
		else
		{
			if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
			{
				m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				return;
			}
		}

		// Tracker 3926:  We can't +USE something if we're climbing a ladder
		if ( GetMoveType() == MOVETYPE_LADDER )
		{
			return;
		}
	}

	if( m_flTimeUseSuspended > gpGlobals->curtime )
	{
		// Something has temporarily stopped us being able to USE things.
		// Obviously, this should be used very carefully.(sjb)
		return;
	}

	CBaseEntity *pUseEntity = FindUseEntity();

	bool usedSomething = false;

	// Found an object
	if ( pUseEntity )
	{
		//!!!UNDONE: traceline here to prevent +USEing buttons through walls			
		int caps = pUseEntity->ObjectCaps();
		variant_t emptyVariant;

		if ( ( (m_nButtons & IN_USE) && (caps & FCAP_CONTINUOUS_USE) ) ||
			 ( (m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE|FCAP_ONOFF_USE)) ) )
		{
			if ( caps & FCAP_CONTINUOUS_USE )
				m_afPhysicsFlags |= PFLAG_USING;

			pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_TOGGLE );

			usedSomething = true;
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ( (m_afButtonReleased & IN_USE) && (pUseEntity->ObjectCaps() & FCAP_ONOFF_USE) )	// BUGBUG This is an "off" use
		{
			pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_TOGGLE );

			usedSomething = true;
		}
	}

	// Debounce the use key
	if ( usedSomething && pUseEntity )
	{
		m_Local.m_nOldButtons |= IN_USE;
		m_afButtonPressed &= ~IN_USE;
	}
}

void CVectronicPlayer::ClearUsePickup()
{
	m_bPlayerPickedUpObject = false;
}

bool CVectronicPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	bool bRet = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
	return bRet;
}

void CVectronicPlayer::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );
}

void CVectronicPlayer::PreThink( void )
{
	QAngle vOldAngles = GetLocalAngles();
	QAngle vTempAngles = GetLocalAngles();

	vTempAngles = EyeAngles();
	if ( vTempAngles[PITCH] > 180.0f )
		vTempAngles[PITCH] -= 360.0f;

	SetLocalAngles( vTempAngles );

	BaseClass::PreThink();

	if ( m_pHintMessageQueue )
		m_pHintMessageQueue->Update();

	//Reset bullet force accumulator, only lasts one frame
	m_vecTotalBulletForce = vec3_origin;

	SetLocalAngles( vOldAngles );
}

void CVectronicPlayer::PostThink( void )
{
	BaseClass::PostThink();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );

	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();
	m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	//Play Woosh!
	CreateSounds();
	UpdateWooshSounds();

	if ( m_flNextMouseoverUpdate < gpGlobals->curtime )
	{
		m_flNextMouseoverUpdate = gpGlobals->curtime + 0.2f;
		if ( m_bShowHints )
		{
#ifdef PLAYER_MOUSEOVER_HINTS
			UpdateMouseoverHints();
#endif
		}
	}

	// Regenerate heath
	if ( IsAlive() && GetHealth() < GetMaxHealth() && (sv_regeneration.GetInt() == 1) )
	{
		// Color to overlay on the screen while the player is taking damage
		color32 hurtScreenOverlay = { 80, 0, 0, 64 };

		if ( gpGlobals->curtime > m_fTimeLastHurt + sv_regeneration_wait_time.GetFloat() )
		{
			// Regenerate based on rate, and scale it by the frametime
			m_fRegenRemander += sv_regeneration_rate.GetFloat() * gpGlobals->frametime;
		
			if( m_fRegenRemander >= 1 )
			{
				//If the regen interval is set, and the health is evenly divisible by that interval, don't regen.
				if ( sv_regen_interval.GetFloat() > 0 && floor(m_iHealth / sv_regen_interval.GetFloat()) == m_iHealth / sv_regen_interval.GetFloat() )
					m_fRegenRemander = 0;
				//If the regen limit is set, and the health is equal to or above the limit, don't regen.
				else if ( sv_regen_limit.GetFloat() > 0 && m_iHealth >= sv_regen_limit.GetFloat() )
					m_fRegenRemander = 0;
				else 
				{
					TakeHealth( m_fRegenRemander, DMG_GENERIC );
					m_fRegenRemander = 0;
				}
			}
		}
		else
		{
			UTIL_ScreenFade( this, hurtScreenOverlay, 1.0f, 0.1f, FFADE_IN|FFADE_PURGE );
		}	
	}

	if ( IsAlive() && m_cycleLatchTimer.IsElapsed() )
	{
		m_cycleLatchTimer.Start( CYCLELATCH_UPDATE_INTERVAL );

		// Compress the cycle into 4 bits. Can represent 0.0625 in steps which is enough.
		m_cycleLatch.GetForModify() = 16 * GetCycle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Makes a splash when the player transitions between water states
//-----------------------------------------------------------------------------
void CVectronicPlayer::Splash( void )
{
	CEffectData data;
	data.m_fFlags = 0;
	data.m_vOrigin = GetAbsOrigin();
	data.m_vNormal = Vector(0,0,1);
	data.m_vAngles = QAngle( 0, 0, 0 );
	
	if ( GetWaterType() & CONTENTS_SLIME )
	{
		data.m_fFlags |= FX_WATER_IN_SLIME;
	}

	float flSpeed = GetAbsVelocity().Length();
	if ( flSpeed < 300 )
	{
		data.m_flScale = random->RandomFloat( 10, 12 );
		DispatchEffect( "waterripple", data );
	}
	else
	{
		data.m_flScale = random->RandomFloat( 6, 8 );
		DispatchEffect( "watersplash", data );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVectronicPlayer::UpdateClientData( void )
{
	if (m_DmgTake || m_DmgSave || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = GetLocalOrigin();
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		damageOrigin = m_DmgOrigin;

		// only send down damage type that have hud art
		int iShowHudDamage = g_pGameRules->Damage_GetShowOnHud();
		int visibleDamageBits = m_bitsDamageType & iShowHudDamage;

		m_DmgTake = clamp( m_DmgTake, 0, 255 );
		m_DmgSave = clamp( m_DmgSave, 0, 255 );

		// If we're poisoned, but it wasn't this frame, don't send the indicator
		// Without this check, any damage that occured to the player while they were
		// recovering from a poison bite would register as poisonous as well and flash
		// the whole screen! -- jdw
		if ( visibleDamageBits & DMG_POISON )
		{
			float flLastPoisonedDelta = gpGlobals->curtime - m_tbdPrev;
			if ( flLastPoisonedDelta > 0.1f )
			{
				visibleDamageBits &= ~DMG_POISON;
			}
		}

		CSingleUserRecipientFilter user( this );
		user.MakeReliable();
		UserMessageBegin( user, "Damage" );
			WRITE_BYTE( m_DmgSave );
			WRITE_BYTE( m_DmgTake );
			WRITE_LONG( visibleDamageBits );
			WRITE_FLOAT( damageOrigin.x );	//BUG: Should be fixed point (to hud) not floats
			WRITE_FLOAT( damageOrigin.y );	//BUG: However, the HUD does _not_ implement bitfield messages (yet)
			WRITE_FLOAT( damageOrigin.z );	//BUG: We use WRITE_VEC3COORD for everything else
		MessageEnd();
	
		m_DmgTake = 0;
		m_DmgSave = 0;
		m_bitsHUDDamage = m_bitsDamageType;
		
		// Clear off non-time-based damage indicators
		int iTimeBasedDamage = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType &= iTimeBasedDamage;
	}

	BaseClass::UpdateClientData();
}

void CVectronicPlayer::StartWalking( void )
{
	SetMaxSpeed( PLAYER_WALK_SPEED );
	m_fIsWalking = true;
}

void CVectronicPlayer::StopWalking( void )
{
	SetMaxSpeed( PLAYER_WALK_SPEED );
	m_fIsWalking = false;
}

void CVectronicPlayer::HandleSpeedChanges( void )
{
	bool bIsWalking = IsWalking();
	bool bWantWalking;
	
	bWantWalking = true;
	
	if( bIsWalking != bWantWalking )
	{
		if ( bWantWalking )
			StartWalking();
		else
			StopWalking();
	}
}

void CVectronicPlayer::FireBullets ( const FireBulletsInfo_t &info )
{
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( this, this->GetCurrentCommand() );

	NoteWeaponFired();

	BaseClass::FireBullets( info );

	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( this );
}

void CVectronicPlayer::NoteWeaponFired( void )
{
	Assert( m_pCurrentCommand );
	if( m_pCurrentCommand )
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
}

bool CVectronicPlayer::WantsLagCompensationOnEntity( const CBaseEntity *pEntity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	// No need to lag compensate at all if we're not attacking in this command and
	// we haven't attacked recently.
	if ( !( pCmd->buttons & IN_ATTACK ) && ( pCmd->command_number - m_iLastWeaponFireUsercmd > 5 ) )
		return false;

	return BaseClass::WantsLagCompensationOnEntity( pEntity, pCmd, pEntityTransmitBits );
}

//-----------------------------------------------------------------------------
// Purpose: Player reacts to bumping a weapon. 
// Input  : pWeapon - the weapon that the player bumped into.
// Output : Returns true if player picked up the weapon
//-----------------------------------------------------------------------------
bool CVectronicPlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( !IsAllowedToPickupWeapons() )
		return false;

	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		if ( gEvilImpulse101 )
			UTIL_Remove( pWeapon );
		return false;
	}

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if( !pWeapon->FVisible( this, MASK_SOLID ) && !(GetFlags() & FL_NOTARGET) )
		return false;

	bool bOwnsWeaponAlready = !!Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType());
	if ( bOwnsWeaponAlready == true ) 
	{
		//If we have room for the ammo, then "take" the weapon too.
		 if ( Weapon_EquipAmmoOnly( pWeapon ) )
		 {
			 pWeapon->CheckRespawn();

			 UTIL_Remove( pWeapon );
			 return true;
		 }
		 else
			 return false;
	}

	pWeapon->CheckRespawn();
	Weapon_Equip( pWeapon );

	return true;
}

bool CVectronicPlayer::ClientCommand( const CCommand &args )
{
	if ( FStrEq( args[0], "joingame" ) )
		return true;

	return BaseClass::ClientCommand( args );
}

void CVectronicPlayer::CheatImpulseCommands( int iImpulse )
{
	if( !sv_cheats->GetBool() || !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	switch ( iImpulse )
	{
		case 101:
			GiveAllItems();
			break;

		default:
			BaseClass::CheatImpulseCommands( iImpulse );
	}
}

void CVectronicPlayer::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CPredictedViewModel *vm = ( CPredictedViewModel * )CreateEntityByName( "predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

bool CVectronicPlayer::BecomeRagdollOnClient( const Vector &force )
{
	return true;
}

void CVectronicPlayer::CreateRagdollEntity( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	// If we already have a ragdoll, don't make another one.
	CVectronicRagdoll *pRagdoll = dynamic_cast< CVectronicRagdoll* >( m_hRagdoll.Get() );
	
	if ( !pRagdoll )
	{
		// create a new one
		pRagdoll = dynamic_cast< CVectronicRagdoll* >( CreateEntityByName( "vectronic_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = m_vecTotalBulletForce;
		pRagdoll->SetAbsOrigin( GetAbsOrigin() );
	}

	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CVectronicPlayer::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVectronicPlayer::FlashlightTurnOn( void )
{
	if( flashlight.GetInt() > 0 && IsAlive() )
	{
		AddEffects( EF_DIMLIGHT );
		EmitSound( "HL2Player.FlashlightOn" );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVectronicPlayer::FlashlightTurnOff( void )
{
	RemoveEffects( EF_DIMLIGHT );
	
	if( IsAlive() )
		EmitSound( "HL2Player.FlashlightOff" );
}

void CVectronicPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	//update damage info with our accumulated physics force
	CTakeDamageInfo subinfo = info;
	subinfo.SetDamageForce( m_vecTotalBulletForce );

	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.
	CreateRagdollEntity();

	BaseClass::Event_Killed( subinfo );

	if ( (info.GetDamageType() & DMG_DISSOLVE) && CanBecomeRagdoll() )
	{
		int nDissolveType = ENTITY_DISSOLVE_NORMAL;
		if ( info.GetDamageType() & DMG_SHOCK )
			nDissolveType = ENTITY_DISSOLVE_ELECTRICAL;
	
		if ( m_hRagdoll )
			m_hRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime, false, nDissolveType );

		// Also dissolve any weapons we dropped
		if ( GetActiveWeapon() )
			GetActiveWeapon()->Dissolve( NULL, gpGlobals->curtime, false, nDissolveType );
	}

	if( info.GetDamageType() & ( DMG_BLAST | DMG_BURN ) )
	{
		if( m_hRagdoll )
		{
			CBaseAnimating *pRagdoll = (CBaseAnimating *)CBaseEntity::Instance( m_hRagdoll );
			if( info.GetDamageType() & ( DMG_BURN | DMG_BLAST ) )
				pRagdoll->Ignite( 45, false, 10 );
		}
	}

	FlashlightTurnOff();

	m_lifeState = LIFE_DEAD;

	RemoveEffects( EF_NODRAW );	// still draw player body

	FirePlayerProxyOutput( "PlayerDied", variant_t(), this, this );
}

bool CVectronicPlayer::PassesDamageFilter( const CTakeDamageInfo &info )
{
	CBaseEntity *pAttacker = info.GetAttacker();
	if( pAttacker && pAttacker->MyNPCPointer() && pAttacker->MyNPCPointer()->IsPlayerAlly() )
		return false;

	if( m_hPlayerProxy && !m_hPlayerProxy->PassesDamageFilter( info ) )
		return false;

	return BaseClass::PassesDamageFilter( info );
}

int CVectronicPlayer::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo inputInfoCopy( inputInfo );

	// If you shoot yourself, make it hurt but push you less
	if ( inputInfoCopy.GetAttacker() == this && inputInfoCopy.GetDamageType() == DMG_BULLET )
	{
		inputInfoCopy.ScaleDamage( 5.0f );
		inputInfoCopy.ScaleDamageForce( 0.05f );
	}

	m_vecTotalBulletForce += inputInfoCopy.GetDamageForce();
	gamestats->Event_PlayerDamage( this, inputInfoCopy );
	m_DmgOrigin = inputInfoCopy.GetDamagePosition();

#ifdef PLAYER_IGNORE_FALLDAMAGE
	// ignore fall damage if instructed to do so by input
	if ( ( info.GetDamageType() & DMG_FALL ) )
		inputInfoCopy.SetDamage(0.0f);
#endif

	if ( GetHealth() < 100 )
		m_fTimeLastHurt = gpGlobals->curtime;

	return BaseClass::OnTakeDamage( inputInfoCopy );
}

int CVectronicPlayer::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// set damage type sustained
	m_bitsDamageType |= info.GetDamageType();

	if ( !CBaseCombatCharacter::OnTakeDamage_Alive( info ) )
		return 0;

	CBaseEntity * attacker = info.GetAttacker();

	if ( !attacker )
		return 0;

	Vector vecDir = vec3_origin;
	if ( info.GetInflictor() )
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0, 0, 10 ) - WorldSpaceCenter();
		VectorNormalize( vecDir );
	}

	if ( info.GetInflictor() && (GetMoveType() == MOVETYPE_WALK) && 
		( !attacker->IsSolidFlagSet(FSOLID_TRIGGER)) )
	{
		Vector force = vecDir;// * -DamageForce( WorldAlignSize(), info.GetBaseDamage() );
		if ( force.z > 250.0f )
		{
			force.z = 250.0f;
		}
		ApplyAbsVelocityImpulse( force );
	}

	// Burnt
	if ( info.GetDamageType() & DMG_BURN )
	{
		EmitSound( "HL2Player.BurnPain" );
	}

	// fire global game event

	IGameEvent * event = gameeventmanager->CreateEvent( "player_hurt" );
	if ( event )
	{
		event->SetInt("userid", GetUserID() );
		event->SetInt("health", MAX(0, m_iHealth) );
		event->SetInt("priority", 5 );	// HLTV event priority, not transmitted

		if ( attacker->IsPlayer() )
		{
			CBasePlayer *player = ToBasePlayer( attacker );
			event->SetInt("attacker", player->GetUserID() ); // hurt by other player
		}
		else
		{
			event->SetInt("attacker", 0 ); // hurt by "world"
		}

		gameeventmanager->FireEvent( event );
	}

	// Insert a combat sound so that nearby NPCs hear battle
	if ( attacker->IsNPC() )
	{
		CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 512, 0.5, this );
	}

	return 1;
}

void CVectronicPlayer::OnDamagedByExplosion( const CTakeDamageInfo &info )
{
	if ( info.GetInflictor() && info.GetInflictor()->ClassMatches( "mortarshell" ) )
	{
		// No ear ringing for mortar
		UTIL_ScreenShake( info.GetInflictor()->GetAbsOrigin(), 4.0, 1.0, 0.5, 1000, SHAKE_START, false );
		return;
	}
	BaseClass::OnDamagedByExplosion( info );
}

void CVectronicPlayer::DeathSound( const CTakeDamageInfo &info )
{
	if ( m_hRagdoll && m_hRagdoll->GetBaseAnimating()->IsDissolving() )
		 return;
}

CBaseEntity* CVectronicPlayer::EntSelectSpawnPoint( void )
{
	CBaseEntity *pSpot = NULL;
	CBaseEntity *pLastSpawnPoint = g_pLastSpawn;
	edict_t		*player = edict();
	const char *pSpawnpointName = "info_player_deathmatch";

	pSpot = pLastSpawnPoint;
	// Randomize the start spot
	for ( int i = random->RandomInt(1,5); i > 0; i-- )
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
	if ( !pSpot )  // skip over the null point
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );

	CBaseEntity *pFirstSpot = pSpot;

	do 
	{
		if ( pSpot )
		{
			// check if pSpot is valid
			if ( g_pGameRules->IsSpawnPointValid( pSpot, this ) )
			{
				if ( pSpot->GetLocalOrigin() == vec3_origin )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
					continue;
				}

				// if so, go to pSpot
				goto ReturnSpot;
			}
		}
		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
	} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

	// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
	if ( pSpot )
	{
		CBaseEntity *ent = NULL;
		for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
		{
			// if ent is a client, kill em (unless they are ourselves)
			if ( ent->IsPlayer() && !( ent->edict() == player ) )
				ent->TakeDamage( CTakeDamageInfo( GetContainingEntity( INDEXENT( 0 ) ), GetContainingEntity(INDEXENT( 0 ) ), 300, DMG_GENERIC ) );
		}
		goto ReturnSpot;
	}

	if ( !pSpot  )
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_start" );

		if ( pSpot )
			goto ReturnSpot;
	}

ReturnSpot:

	g_pLastSpawn = pSpot;

	return pSpot;
} 

//-----------------------------------------------------------------------------
// Purpose: multiplayer does not do autoaiming.
//-----------------------------------------------------------------------------
Vector CVectronicPlayer::GetAutoaimVector( float flScale )
{
	//No Autoaim
	Vector	forward;
	AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle, &forward );
	return	forward;
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void CVectronicPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	if ( playerAnim == PLAYER_WALK || playerAnim == PLAYER_IDLE ) 
		return;

    if ( playerAnim == PLAYER_RELOAD )
        DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
    else if ( playerAnim == PLAYER_JUMP )
        DoAnimationEvent( PLAYERANIMEVENT_JUMP );
    else
        Assert( !"CVectronicPlayer::SetAnimation OBSOLETE!" );
}

void CVectronicPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	m_PlayerAnimState->DoAnimationEvent( event, nData );
	TE_PlayerAnimEvent( this, event, nData );	// Send to any clients who can see this guy.
}

//-----------------------------------------------------------------------------
// Purpose: Override setup bones so that is uses the render angles from
//			the Vectronic animation state to setup the hitboxes.
//-----------------------------------------------------------------------------
void CVectronicPlayer::SetupBones( matrix3x4_t *pBoneToWorld, int boneMask )
{
	VPROF_BUDGET( "CVectronicPlayer::SetupBones", VPROF_BUDGETGROUP_SERVER_ANIM );

	// Set the mdl cache semaphore.
	MDLCACHE_CRITICAL_SECTION();

	// Get the studio header.
	Assert( GetModelPtr() );
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if ( !pStudioHdr )
		return;

	Vector pos[MAXSTUDIOBONES];
	Quaternion q[MAXSTUDIOBONES];

	// Adjust hit boxes based on IK driven offset.
	Vector adjOrigin = GetAbsOrigin() + Vector( 0, 0, m_flEstIkOffset );

	// FIXME: pass this into Studio_BuildMatrices to skip transforms
	CBoneBitList boneComputed;
	if ( m_pIk )
	{
		m_iIKCounter++;
		m_pIk->Init( pStudioHdr, GetAbsAngles(), adjOrigin, gpGlobals->curtime, m_iIKCounter, boneMask );
		GetSkeleton( pStudioHdr, pos, q, boneMask );

		m_pIk->UpdateTargets( pos, q, pBoneToWorld, boneComputed );
		CalculateIKLocks( gpGlobals->curtime );
		m_pIk->SolveDependencies( pos, q, pBoneToWorld, boneComputed );
	}
	else
	{
		GetSkeleton( pStudioHdr, pos, q, boneMask );
	}

	CBaseAnimating *pParent = dynamic_cast< CBaseAnimating* >( GetMoveParent() );
	if ( pParent )
	{
		// We're doing bone merging, so do special stuff here.
		CBoneCache *pParentCache = pParent->GetBoneCache();
		if ( pParentCache )
		{
			BuildMatricesWithBoneMerge( 
				pStudioHdr, 
				m_PlayerAnimState->GetRenderAngles(),
				adjOrigin, 
				pos, 
				q, 
				pBoneToWorld, 
				pParent, 
				pParentCache );

			return;
		}
	}

	Studio_BuildMatrices( 
		pStudioHdr, 
		m_PlayerAnimState->GetRenderAngles(),
		adjOrigin, 
		pos, 
		q, 
		-1,
		GetModelScale(), // Scaling
		pBoneToWorld,
		boneMask );
}

void CVectronicPlayer::CreateSounds()
{
	if ( !m_pAirSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		CPASAttenuationFilter filter( this );

		m_pAirSound = controller.SoundCreate( filter, entindex(), SOUND_WHOOSH );
		controller.Play( m_pAirSound, 0, 100 );
	}
}

void CVectronicPlayer::StopLoopingSounds()
{
	if ( m_pAirSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		controller.SoundDestroy( m_pAirSound );
		m_pAirSound = NULL;
	}

	BaseClass::StopLoopingSounds();
}

void CVectronicPlayer::UpdateWooshSounds( void )
{
	if ( m_pAirSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		float fWooshVolume = GetAbsVelocity().Length() - 300;

		if ( fWooshVolume < 0.0f )
		{
			controller.SoundChangeVolume( m_pAirSound, 0.0f, 0.1f );
			return;
		}

		fWooshVolume /= 2000.0f;
		if ( fWooshVolume > 1.0f )
			fWooshVolume = 1.0f;

		controller.SoundChangeVolume( m_pAirSound, fWooshVolume, 0.1f );
	}
}

void CVectronicPlayer::HintMessage( const char *pMessage, bool bDisplayIfDead, bool bOverrideClientSettings, bool bQuiet )
{
	if (!hud_show_annotations.GetBool())
		return;

	if ( !bDisplayIfDead && !IsAlive() || !IsNetClient() || !m_pHintMessageQueue )
		return;

	//Are we gonna play a sound?
	if(!bQuiet)
	{
		EmitSound(SOUND_HINT);
	}

	if ( bOverrideClientSettings || m_bShowHints )
		m_pHintMessageQueue->AddMessage( pMessage );
}

// All other mouse over hints.
#ifdef PLAYER_MOUSEOVER_HINTS
void CVectronicPlayer::UpdateMouseoverHints()
{
	// Don't show if we are DEAD!
	if ( !IsAlive())
		return;

	if( m_bPlayerPickedUpObject )
		return;

	Vector forward, up;
	EyeVectors( &forward, NULL, &up );

	trace_t tr;
	// Search for objects in a sphere (tests for entities that are not solid, yet still useable)
	Vector searchStart = EyePosition();
	Vector searchEnd = searchStart + forward * 2048;

	int useableContents = MASK_NPCSOLID_BRUSHONLY | MASK_VISIBLE_AND_NPCS;

	bool bItemGlowing = false;

	UTIL_TraceLine( searchStart, searchEnd, useableContents, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 1.0f )
	{
		if ( tr.DidHitNonWorldEntity() && tr.m_pEnt )
		{
			CBaseEntity *pObject = tr.m_pEnt;

			int classify = pObject->Classify();

			// You can make it so that when the player roles over a class type, it will activate. Like
			// if we run into another player or an ally, tell the user via hint that they're friends. <3
			if( classify == CLASS_PLAYER || classify == CLASS_PLAYER_ALLY )
			{
				if ( g_pGameRules->PlayerRelationship( this, pObject ) == GR_TEAMMATE )
				{
					if ( !( m_iDisplayHistoryBits & DHF_FRIEND_SEEN ) )
					{
						m_iDisplayHistoryBits |= DHF_FRIEND_SEEN;
						HintMessage( "#Hint_spotted_a_friend", true );
					}
				}

				return;
			}

			// You can also be entity specific. Like here, if the player roles over any weapon class,
			// it will use the engine's glow system. Keep in mind ASW's and This branch of source uses
			// diffrent glow systems. Plus, both are called via server this way. For SP, this is fine,
			// But MP needs to do this cliently somehow. 
			if ( pObject->ClassMatches( "weapon_*" ) )
			{
				// Ok, check to see if we have glown any other entities.
				if( m_hLastSeenEntity.Get() )
				{
					// If we did (!= null), then make the last get entity unglow.
					if (m_hLastSeenEntity != NULL)
					{
						m_hLastSeenEntity->SetGlow( false );
						bItemGlowing = false;
						SetLastSeenEntity ( NULL );
					}
				}

				if ( !bItemGlowing )
				{
					pObject->SetGlow( true );
					bItemGlowing = true;
					SetLastSeenEntity ( pObject );
				} 
				return;
			}
		}

		// Reset everything!
		else if ( m_bPlayerPickedUpObject || tr.DidHitWorld() && tr.m_pEnt || tr.fraction >= 1.0f ) // We are looking at the world.
		{
			if( m_hLastSeenEntity.Get() )
			{
				m_hLastSeenEntity->SetGlow(false);
				bItemGlowing = false;
				m_bMouseOverEnemy = false;
				SetLastSeenEntity ( NULL );
			}

			m_bMouseOverEnemy = false;

			return;
		}
	}
}
#endif

CLogicPlayerProxy *CVectronicPlayer::GetPlayerProxy( void )
{
	CLogicPlayerProxy *pProxy = dynamic_cast< CLogicPlayerProxy* > ( m_hPlayerProxy.Get() );

	if ( pProxy == NULL )
	{
		pProxy = (CLogicPlayerProxy*)gEntList.FindEntityByClassname(NULL, "logic_playerproxy" );

		if ( pProxy == NULL )
			return NULL;

		pProxy->m_hPlayer = this;
		m_hPlayerProxy = pProxy;
	}

	return pProxy;
}

void CVectronicPlayer::FirePlayerProxyOutput( const char *pszOutputName, variant_t variant, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	if ( GetPlayerProxy() == NULL )
		return;

	GetPlayerProxy()->FireNamedOutput( pszOutputName, variant, pActivator, pCaller );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( logic_playerproxy, CLogicPlayerProxy );

BEGIN_DATADESC( CLogicPlayerProxy )
	DEFINE_OUTPUT( m_OnFlashlightOn, "OnFlashlightOn" ),
	DEFINE_OUTPUT( m_OnFlashlightOff, "OnFlashlightOff" ),
	DEFINE_OUTPUT( m_RequestedPlayerHealth, "PlayerHealth" ),
	DEFINE_OUTPUT( m_PlayerHasAmmo, "PlayerHasAmmo" ),
	DEFINE_OUTPUT( m_PlayerHasNoAmmo, "PlayerHasNoAmmo" ),
	DEFINE_OUTPUT( m_PlayerDied,	"PlayerDied" ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"RequestPlayerHealth",	InputRequestPlayerHealth ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetFlashlightSlowDrain",	InputSetFlashlightSlowDrain ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetFlashlightNormalDrain",	InputSetFlashlightNormalDrain ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetPlayerHealth",	InputSetPlayerHealth ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"RequestAmmoState", InputRequestAmmoState ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SuppressCrosshair", InputSuppressCrosshair ),

	DEFINE_FIELD( m_hPlayer, FIELD_EHANDLE ),
END_DATADESC()

void CLogicPlayerProxy::Activate( void )
{
	BaseClass::Activate();

	if ( m_hPlayer == NULL )
		m_hPlayer = UTIL_GetLocalPlayer();
}

bool CLogicPlayerProxy::PassesDamageFilter( const CTakeDamageInfo &info )
{
	if ( m_hDamageFilter )
	{
		CBaseFilter *pFilter = (CBaseFilter *)( m_hDamageFilter.Get() );
		return pFilter->PassesDamageFilter( info );
	}

	return true;
}

void CLogicPlayerProxy::InputSetPlayerHealth( inputdata_t &inputdata )
{
	if ( m_hPlayer == NULL )
		return;

	m_hPlayer->SetHealth( inputdata.value.Int() );

}

void CLogicPlayerProxy::InputRequestPlayerHealth( inputdata_t &inputdata )
{
	if ( m_hPlayer == NULL )
		return;

	m_RequestedPlayerHealth.Set( m_hPlayer->GetHealth(), inputdata.pActivator, inputdata.pCaller );
}

void CLogicPlayerProxy::InputSetFlashlightSlowDrain( inputdata_t &inputdata )
{
	/*if( m_hPlayer == NULL )
		return;

	CVectronicPlayer *pPlayer = To_VectronicPlayer( m_hPlayer.Get() );
	if( pPlayer )
		pPlayer->SetFlashlightPowerDrainScale( hl2_darkness_flashlight_factor.GetFloat() );*/
}

void CLogicPlayerProxy::InputSetFlashlightNormalDrain( inputdata_t &inputdata )
{
	/*if( m_hPlayer == NULL )
		return;

	CVectronicPlayer *pPlayer = To_VectronicPlayer( m_hPlayer.Get() );
	if( pPlayer )
		pPlayer->SetFlashlightPowerDrainScale( 1.0f );*/
}

void CLogicPlayerProxy::InputRequestAmmoState( inputdata_t &inputdata )
{
	if( m_hPlayer == NULL )
		return;

	CVectronicPlayer *pPlayer = To_VectronicPlayer(m_hPlayer.Get());

	for ( int i = 0 ; i < pPlayer->WeaponCount(); ++i )
	{
		CBaseCombatWeapon* pCheck = pPlayer->GetWeapon( i );
		if ( pCheck )
		{
			if ( pCheck->HasAnyAmmo() && ( pCheck->UsesPrimaryAmmo() || pCheck->UsesSecondaryAmmo() ) )
			{
				m_PlayerHasAmmo.FireOutput( this, this, 0 );
				return;
			}
		}
	}

	m_PlayerHasNoAmmo.FireOutput( this, this, 0 );
}

void CLogicPlayerProxy::InputSuppressCrosshair( inputdata_t &inputdata )
{
	if( m_hPlayer == NULL )
		return;

	CVectronicPlayer *pPlayer = To_VectronicPlayer( m_hPlayer.Get() );
	if( pPlayer )
		pPlayer->m_Local.m_iHideHUD |= HIDEHUD_CROSSHAIR;;
}

//-----------------------------------------------------------------------------
// Here we have our hud hint entity, since it only works in SP due to
// UTIL_GetLocalPlayer(), we will put it here.
//-----------------------------------------------------------------------------
#define SF_HUDHINT_ALLPLAYERS			0x0001

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudAnnotation : public CPointEntity
{
public:
	DECLARE_CLASS( CHudAnnotation, CPointEntity );

	void	Spawn( void );
	void	Precache( void );

	inline	void	MessageSet( const char *pMessage ) { m_iszMessage = AllocPooledString(pMessage); }
	inline	const char *MessageGet( void )	{ return STRING( m_iszMessage ); }

private:

	inline	bool	AllPlayers( void ) { return (m_spawnflags & SF_HUDHINT_ALLPLAYERS) != 0; }

	CHandle<CBasePlayer> m_pPlayer;
	bool m_bWriteOnScreen;
	bool m_bHintQuiet;

	void InputShowHudHint( inputdata_t &inputdata );

	string_t m_iszMessage;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( hud_annotation, CHudAnnotation );

BEGIN_DATADESC( CHudAnnotation )

	DEFINE_KEYFIELD( m_iszMessage, FIELD_STRING, "display_text" ),
	DEFINE_KEYFIELD( m_bWriteOnScreen, FIELD_BOOLEAN, "simpledisplay" ),
	DEFINE_KEYFIELD( m_bHintQuiet, FIELD_BOOLEAN, "quiet" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Show", InputShowHudHint ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAnnotation::Spawn( void )
{
	Precache();

	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAnnotation::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for showing the message and/or playing the sound.
//-----------------------------------------------------------------------------
void CHudAnnotation::InputShowHudHint( inputdata_t &inputdata )
{
	CBaseEntity *pGetPlayer = NULL;
	if ( inputdata.pActivator && inputdata.pActivator->IsPlayer() )
	{
		pGetPlayer = inputdata.pActivator;
	}
	else
	{
		pGetPlayer = UTIL_GetLocalPlayer();
	}

	CVectronicPlayer *pPlayer = To_VectronicPlayer(pGetPlayer);
	if (!m_bWriteOnScreen)
	{
		pPlayer->HintMessage( MessageGet(), false, false, m_bHintQuiet );
	}
	else
	{
		//Display on screen only for the player. 
		CSingleUserRecipientFilter user( (CBasePlayer *)pPlayer );
		user.MakeReliable();

		if (!AllPlayers())
		{
			UTIL_ClientPrintFilter( user, HUD_PRINTCENTER, MessageGet() );
		}
		else
		{
			UTIL_ClientPrintAll( HUD_PRINTCENTER, MessageGet() );
		}
	}
}
