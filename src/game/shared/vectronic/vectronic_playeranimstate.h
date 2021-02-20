//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifndef VECTRONIC_PLAYERANIMSTATE_H
#define VECTRONIC_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "multiplayer_animstate.h"
#include "base_playeranimstate.h"

#if defined( CLIENT_DLL )
	class C_VectronicPlayer;
	#define CVectronicPlayer C_VectronicPlayer
#else
	class CVectronicPlayer;
#endif

// ------------------------------------------------------------------------------------------------ //
// CVectronicPlayerAnimState declaration.
// ------------------------------------------------------------------------------------------------ //
class CVectronicPlayerAnimState : public CMultiPlayerAnimState
{
public:
	
	DECLARE_CLASS( CVectronicPlayerAnimState, CMultiPlayerAnimState );

			CVectronicPlayerAnimState();
			CVectronicPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData );
	virtual ~CVectronicPlayerAnimState();

	void InitPlayerAnimState( CVectronicPlayer *pPlayer );
	CVectronicPlayer *GetThePlayer( void ) { return m_pVectronicPlayer; }

	virtual void ClearAnimationState();
	virtual Activity TranslateActivity( Activity actDesired );
	virtual void Update( float eyeYaw, float eyePitch );

	virtual void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );

	virtual bool HandleMoving( Activity &idealActivity );
	virtual bool HandleJumping( Activity &idealActivity );
	virtual bool HandleDucking( Activity &idealActivity );
	virtual bool HandleSwimming( Activity &idealActivity );

	virtual float GetCurrentMaxGroundSpeed();

protected:
	CModAnimConfig m_AnimConfig;

private:
	//Tony; temp till 9way!
	virtual bool SetupPoseParameters( CStudioHdr *pStudioHdr );
	virtual void EstimateYaw( void );
	virtual void ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr );
	virtual void ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr );
	virtual void ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr );
	virtual void ComputePlaybackRate();
	
	CVectronicPlayer *m_pVectronicPlayer;
	bool m_bInAirWalk;
	float m_flHoldDeployedPoseUntilTime;
};

CVectronicPlayerAnimState *CreatePlayerAnimState( CVectronicPlayer *pPlayer );

#endif // VECTRONIC_PLAYERANIMSTATE_H
