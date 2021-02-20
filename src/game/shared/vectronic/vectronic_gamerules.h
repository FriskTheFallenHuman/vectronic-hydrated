//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: Game rules for Scratch
//
//=============================================================================

#ifndef VECTRONIC_GAMERULES_H
#define VECTRONIC_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "gamerules.h"
#include "multiplay_gamerules.h"

#ifdef CLIENT_DLL
	#define CVectronicGameRules C_VectronicGameRules
#endif

class CVectronicGameRules : public CMultiplayRules
{
	DECLARE_CLASS( CVectronicGameRules, CMultiplayRules );

public:
			CVectronicGameRules();
	virtual ~CVectronicGameRules();

	virtual bool IsMultiplayer( void );
	virtual bool IsDeathmatch( void );
	virtual bool IsCoOp( void );

	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

#else
	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.

	virtual void Activate();

	virtual const char *GetGameDescription( void );

	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void InitDefaultAIRelationships( void );
	virtual const char*	AIClassText(int classType);
#endif // CLIENT_DLL
};

//-----------------------------------------------------------------------------
// Gets us at the SDK game rules
//-----------------------------------------------------------------------------
inline CVectronicGameRules* rHGameRules()
{
	return static_cast<CVectronicGameRules*>(g_pGameRules);
}

#endif // rh_GAMERULES_H