//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: TF implementation of the IPresence interface
//
//=============================================================================
#ifndef TIP_MANAGER_H
#define TIP_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"

//-----------------------------------------------------------------------------
// Purpose: helper class for TipManager
//-----------------------------------------------------------------------------
class CTipManager : public CAutoGameSystem
{
public:
	CTipManager();

	virtual bool Init();
	virtual char const *Name() { return "CTipManager"; }

	const wchar_t *GetRandomTip();
private:
	const wchar_t *GetTip( int iTip );

	int m_iTipCountAll;		// how many tips there are total
	bool m_bInited;			// have we been initialized
};

extern CTipManager g_TipManager;

#endif // TIP_MANAGER_H
