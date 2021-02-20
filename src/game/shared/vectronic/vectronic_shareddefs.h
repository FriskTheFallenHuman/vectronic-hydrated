//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: Here are tags to enable/disable things throughout the source code.
//
//=============================================================================//

#ifndef VECTRONIC_SHAREDDEFS_H
#define VECTRONIC_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

// The default glow color.
#define GLOW_COLOR 0,128,255

// Gravity defined in movevars_shared.cpp
#define GAME_GRAVITY "600"

#define BALL0_COLOR 134,199,197,255
#define BALL1_COLOR 112,176,109,255
#define BALL2_COLOR 136,123,193,255
#define BALL3_COLOR 193,124,123,255
#define BALL4_COLOR 176,169,109,255
#define BALL5_COLOR 193,155,123,255

//-----Optional Defines-----

// PLAYER_HEALTH_REGEN : Regen the player's health much like it does in Portal/COD
// PLAYER_WOOSH_SOUNDS : Play air simulation sounds when falling.
// PLAYER_MOUSEOVER_HINTS : When the player has their crosshair over whatever we put in UpdateMouseoverHints() it will display a hint
// PLAYER_IGNORE_FALLDAMAGE : Ingnore fall damage.
// PLAYER_DISABLE_THROWING : Disables throwing in the player pickup controller.

//------------------
// WEAPON_QUIET_PICKUP : Don't play a sound when picking up a weapon.
// WEAPON_NOHUDSHOW_PICKUP : Don't display pickup in the hud.
// WEAPON_DLIGHT_MUZZLEFLASH : Have a light flash when firing. (Source 2013 only!)
// WEAPON_ALLOW_VIEWMODEL_FLIP : Enables cl_righthand for viewmodel.
// WEAPON_DISABLE_DYNAIMIC_CROSSHAIR : Disables the CSS like crosshair.
// GLOWS_ENABLE : Uses the item glow system. (WIP)
// GE_GLOWS_ENABLE : Uses the GE item glow system.

//========================
// PLAYER RELATED OPTIONS
//========================
	#define PLAYER_HEALTH_REGEN
	#define PLAYER_WOOSH_SOUNDS
	#define PLAYER_DISABLE_THROWING

//========================
// WEAPON RELATED OPTIONS
//========================
	#define WEAPON_QUIET_PICKUP
	#define WEAPON_NOHUDSHOW_PICKUP
	#define WEAPON_DISABLE_DYNAIMIC_CROSSHAIR

#endif // SDK_SHAREDDEFS_H