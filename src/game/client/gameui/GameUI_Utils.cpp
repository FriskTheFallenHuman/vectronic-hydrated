//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include <stdarg.h>
#include "GameUI_Utils.h"
#include "strtools.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Background settings
//-----------------------------------------------------------------------------
KeyValues* gBackgroundSettings;
KeyValues* BackgroundSettings()
{
	return gBackgroundSettings;
}

void InitBackgroundSettings()
{
	if ( gBackgroundSettings )
		gBackgroundSettings->deleteThis();

	gBackgroundSettings = new KeyValues( "MenuBackgrounds" );
	gBackgroundSettings->LoadFromFile( g_pFullFileSystem, "scripts/menu_backgrounds.txt" );
}