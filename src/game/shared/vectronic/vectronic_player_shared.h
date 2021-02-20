//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef VECTRONIC_PLAYER_SHARED_H
#define VECTRONIC_PLAYER_SHARED_H
#pragma once

#include "studio.h"

#if defined( CLIENT_DLL )
	#define CVectronicPlayer C_VectronicPlayer
#endif

// Player avoidance
#define PUSHAWAY_THINK_INTERVAL		(1.0f / 20.0f)

#endif // VECTRONIC_PLAYER_SHARED_H
