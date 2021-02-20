#include "cbase.h"
#include "multiplay_gamerules.h"
#include "vectronic_gamerules.h"
#include "ammodef.h"
#include "gamevars_shared.h"

#ifdef GAME_DLL
	#include "voice_gamemgr.h"
#endif

// =====Convars=============

// This convar is for storing a small bit for the hub.
ConVar	vectronic_onlvl	( "vectronic_onlvl","0", FCVAR_REPLICATED| FCVAR_DEVELOPMENTONLY );

REGISTER_GAMERULES_CLASS( CVectronicGameRules );

void InitBodyQue() { }

// Not using this
CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;
	
	if ( !bInitted )
		bInitted = true;

	return &def;
}

//=========================================================
//=========================================================
CVectronicGameRules::CVectronicGameRules( void )
{
#ifdef GAME_DLL
	ConColorMsg( Color( 77, 116, 85, 255 ), "[CVectronicGameRules] Creating gamerules.\n" );

	InitDefaultAIRelationships();
#else
	ConColorMsg( Color( 77, 116, 85, 255 ), "[C_VectronicGameRules] Creating gamerules.\n" );
#endif // GAME_DLL
}

//=========================================================
//=========================================================
CVectronicGameRules::~CVectronicGameRules()
{
#ifdef GAME_DLL
	ConColorMsg( Color( 77, 116, 85, 255 ), "[CVectronicGameRules] Destroying gamerules.\n" );
#else
	ConColorMsg( Color( 77, 116, 85, 255 ), "[C_VectronicGameRules] Destroying gamerules.\n" );
#endif // GAME_DLL
}

//=========================================================
//=========================================================
bool CVectronicGameRules::IsMultiplayer( void )
{
	return gpGlobals->maxClients > 1;
}

//=========================================================
//=========================================================
bool CVectronicGameRules::IsCoOp( void )
{
	return IsMultiplayer();
}

//=========================================================
//=========================================================
bool CVectronicGameRules::IsDeathmatch( void )
{
	return IsMultiplayer() && friendlyfire.GetBool();
}

//=========================================================
//=========================================================
bool CVectronicGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	// The smaller number is always first
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		int tmp = collisionGroup0;
		collisionGroup0 = collisionGroup1;
		collisionGroup1 = tmp;
	}

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		collisionGroup0 = COLLISION_GROUP_PLAYER;
	}

	if( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		collisionGroup1 = COLLISION_GROUP_PLAYER;
	}

	//If collisionGroup0 is not a player then NPC_ACTOR behaves just like an NPC.
	if ( collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 != COLLISION_GROUP_PLAYER )
	{
		collisionGroup1 = COLLISION_GROUP_NPC;
	}

	//players don't collide against NPC Actors.
	//I could've done this up where I check if collisionGroup0 is NOT a player but I decided to just
	//do what the other checks are doing in this function for consistency sake.
	if ( collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 == COLLISION_GROUP_PLAYER )
		return false;
		
	// In cases where NPCs are playing a script which causes them to interpenetrate while riding on another entity,
	// such as a train or elevator, you need to disable collisions between the actors so the mover can move them.
	if ( collisionGroup0 == COLLISION_GROUP_NPC_SCRIPTED && collisionGroup1 == COLLISION_GROUP_NPC_SCRIPTED )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_PROJECTILE )
	{
		if ( collisionGroup1 == COLLISION_GROUP_PROJECTILE)
			return false;
	}

	if ( ( collisionGroup0 == COLLISION_GROUP_WEAPON ) ||
		( collisionGroup0 == COLLISION_GROUP_PLAYER ) ||
		( collisionGroup0 == COLLISION_GROUP_PROJECTILE ) )
	{
		if ( collisionGroup1 == COLLISION_GROUP_PROJECTILE ) //COLLISION_GROUP_VECTRONIC_BALL
			return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS )
	{
		if ( collisionGroup1 == COLLISION_GROUP_PROJECTILE )
			return true;
	}

	return true;
}


#ifdef GAME_DLL

// This being required here is a bug. It should be in shared\BaseGrenade_shared.cpp
ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade","0" );		

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool	CanPlayerHearPlayer( CBasePlayer* pListener, CBasePlayer* pTalker, bool &bProximity ) { return true; }
};

CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper* g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

//=========================================================
//=========================================================
void CVectronicGameRules::Activate()
{
}

//=========================================================
//==================================================== =====
void CVectronicGameRules::PlayerThink( CBasePlayer *pPlayer )
{
	if ( !IsMultiplayer() )
		return;

	BaseClass::PlayerThink( pPlayer );
}

//=========================================================
//=========================================================
void CVectronicGameRules::PlayerSpawn( CBasePlayer *pPlayer )
{
	// Player no longer gets all weapons to start.
	// He has to pick them up now.  Use impulse 101
	// to give him all weapons
	if ( !IsMultiplayer() )
		return;

	BaseClass::PlayerSpawn( pPlayer );
}

//=========================================================
//=========================================================
const char *CVectronicGameRules::GetGameDescription( void )
{
	if ( IsCoOp() )
		return "Vectronic Hydrated - CoOp";

	if ( IsDeathmatch() )
		return "Vectronic Hydrated - CoOp DM";

	return "Vectronic Hydrated - Single Player";
}

//=========================================================
//=========================================================
void CVectronicGameRules::InitDefaultAIRelationships( void )
{
	int i, j;

	//  Allocate memory for default relationships
	CBaseCombatCharacter::AllocateDefaultRelationships();

	// --------------------------------------------------------------
	// First initialize table so we can report missing relationships
	// --------------------------------------------------------------
	for ( i = 0; i < NUM_AI_CLASSES; i++ )
	{
		for ( j = 0; j < NUM_AI_CLASSES; j++ )
		{
			// By default all relationships are neutral of priority zero
			CBaseCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
		}
	}

		// ------------------------------------------------------------
		//	> CLASS_PLAYER
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER,			CLASS_NONE,				D_NU, 0 );			
		CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER,			CLASS_PLAYER,			D_NU, 0 );			
}

//=========================================================
//=========================================================
const char* CVectronicGameRules::AIClassText( int classType )
{
	switch (classType)
	{
		case CLASS_NONE:			return "CLASS_NONE";
		case CLASS_PLAYER:			return "CLASS_PLAYER";
		default:					return "MISSING CLASS in ClassifyText()";
	}
}
#endif // #ifdef GAME_DLL
