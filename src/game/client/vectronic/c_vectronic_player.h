//=========== Copyright © 2014, rHetorical, All rights reserved. =============
//
// Purpose: 
//		
//=============================================================================
#ifndef C_VECTRONIC_PLAYER_H
#define C_VECTRONIC_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "vectronic_playeranimstate.h"
#include "c_baseplayer.h"
#include "c_baseflex.h"
#include "baseparticleentity.h"
#include "vectronic_player_shared.h"
#include "beamdraw.h"
#include "flashlighteffect.h"
#include "colorcorrectionmgr.h"

class CVectronicFlashlightEffect : public CFlashlightEffect
{
public:
	CVectronicFlashlightEffect(int nIndex = 0) : CFlashlightEffect( nIndex  ) {	}
	~CVectronicFlashlightEffect() {};

	virtual void UpdateLight( const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance );
};

//=============================================================================
// >> C_VectronicPlayer
//=============================================================================
class C_VectronicPlayer : public C_BasePlayer
{
public:
	DECLARE_CLASS( C_VectronicPlayer, C_BasePlayer );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();


	C_VectronicPlayer();
	~C_VectronicPlayer( void );

	virtual bool ShouldRegenerateOriginFromCellBits() const { return true; }

	// Player avoidance
	bool ShouldCollide( int collisionGroup, int contentsMask ) const;
	void AvoidPlayers( CUserCmd *pCmd );
	float m_fNextThinkPushAway;

	void ClientThink( void );

	static C_VectronicPlayer* GetLocalVectronicPlayer();
	
	virtual int DrawModel( int flags );
	virtual void AddEntity( void );

	// Should this object cast shadows?
	virtual ShadowType_t		ShadowCastType( void );
	virtual C_BaseAnimating *BecomeRagdollOnClient();
	virtual const QAngle& GetRenderAngles();
	virtual bool ShouldDraw( void );
	virtual void OnDataChanged( DataUpdateType_t type );
	virtual float GetFOV( void );
	virtual float GetMinFOV() const;
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual void ItemPreFrame( void );
	virtual void ItemPostFrame( void );
	virtual Vector GetAutoaimVector( float flDelta );
	virtual void NotifyShouldTransmit( ShouldTransmitState_t state );
	virtual void CreateLightEffects( void ) {}
	virtual bool ShouldReceiveProjectedTextures( int flags );
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void PreThink( void );
	virtual void DoImpactEffect( trace_t &tr, int nDamageType );
	IRagdoll* GetRepresentativeRagdoll() const;
	virtual void CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );
	virtual const QAngle& EyeAngles( void );

	void	UpdateLookAt( void );
	int		GetIDTarget() const;
	void	UpdateIDTarget( void );

	virtual void UpdateClientSideAnimation();
	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	virtual void CalculateIKLocks( float currentTime );

	static void RecvProxy_CycleLatch( const CRecvProxyData *pData, void *pStruct, void *pOut );

	virtual float GetServerIntendedCycle() { return m_flServerCycle; }
	virtual void SetServerIntendedCycle( float cycle ) { m_flServerCycle = cycle; }

	//Tony; when model is changed, need to init some stuff.
	virtual CStudioHdr *OnNewModel( void );
	void InitializePoseParams( void );

	virtual	void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );

	// Input handling
	virtual bool CreateMove( float flInputSampleTime, CUserCmd *pCmd );
	void PerformClientSideObstacleAvoidance( float flFrameTime, CUserCmd *pCmd );
	void PerformClientSideNPCSpeedModifiers( float flFrameTime, CUserCmd *pCmd );

	Activity TranslateActivity( Activity ActToTranslate, bool *pRequired = NULL );

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

	// Are we looking over a useable object?
//	bool IsCursorOverUseable() { return m_bMouseOverUseable; }
//	bool m_bMouseOverUseable;

	// Do we have an object?
	bool PlayerHasObject() { return m_bPlayerPickedUpObject; }
	bool m_bPlayerPickedUpObject;

	// Are we looking at an enemy?
//	bool IsCursorOverEnemy() { return m_bMouseOverEnemy; }
//	bool m_bMouseOverEnemy;

	EHANDLE m_hClosestNPC;
	float m_flSpeedModTime;
	float m_flExitSpeedMod;

	CNetworkVar( int, m_iShotsFired );	// number of shots fired recently

private:
	
	C_VectronicPlayer( const C_VectronicPlayer & );
	CVectronicPlayerAnimState *m_PlayerAnimState;

	QAngle	m_angEyeAngles;

	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;

	EHANDLE	m_hRagdoll;

	int	m_headYawPoseParam;
	int	m_headPitchPoseParam;
	float m_headYawMin;
	float m_headYawMax;
	float m_headPitchMin;
	float m_headPitchMax;

	bool m_isInit;
	Vector m_vLookAtTarget;

	float m_flLastBodyYaw;
	float m_flCurrentHeadYaw;
	float m_flCurrentHeadPitch;

	int	  m_iIDEntIndex;

	CountdownTimer m_blinkTimer;

	int	  m_iSpawnInterpCounter;
	int	  m_iSpawnInterpCounterCache;

	virtual void	UpdateFlashlight( void ); //Tony; override.
	void ReleaseFlashlight( void );
	Beam_t	*m_pFlashlightBeam;

	CVectronicFlashlightEffect *m_pVectronicFlashLightEffect;

	int m_cycleLatch; // The animation cycle goes out of sync very easily. Mostly from the player entering/exiting PVS. Server will frequently update us with a new one.
	float m_flServerCycle;

	virtual bool TestMove( const Vector &pos, float fVertDist, float radius, const Vector &objPos, const Vector &objDir );

	float	m_flSpeedMod;
	QAngle m_angRender;

	ClientCCHandle_t	m_CCDeathHandle;	// handle to death cc effect
	float				m_flDeathCCWeight;	// for fading in cc effect
};

inline C_VectronicPlayer *To_VectronicPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<C_VectronicPlayer*>( pEntity );
}

// -------------------------------------------------------------------------------- //
// Ragdoll entities.
// -------------------------------------------------------------------------------- //
class C_VectronicRagdoll : public C_BaseFlex
{
public:
	DECLARE_CLASS( C_VectronicRagdoll, C_BaseFlex );
	DECLARE_CLIENTCLASS();
	
	C_VectronicRagdoll();
	~C_VectronicRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	int GetPlayerEntIndex() const;
	IRagdoll* GetIRagdoll() const;

	void ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );
	void UpdateOnRemove( void );
	virtual void SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	
private:
	
	C_VectronicRagdoll( const C_VectronicRagdoll & ) {}

	void Interp_Copy( C_BaseAnimatingOverlay *pDestinationEntity );
	void CreateVectronicRagdoll( void );

private:

	EHANDLE	m_hPlayer;
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

#endif // C_VECTRONIC_PLAYER_H
