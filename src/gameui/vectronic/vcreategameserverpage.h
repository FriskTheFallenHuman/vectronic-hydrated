//=====================================================================================//
//
// Purpose:
//
//=====================================================================================//

#ifndef __VCREATEGAMESERVERPAGE_H__
#define __VCREATEGAMESERVERPAGE_H__
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include "cvartogglecheckbutton.h"

//-----------------------------------------------------------------------------
// Purpose: server options page of the create game server dialog
//-----------------------------------------------------------------------------
class CreateGameServerPage : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( CreateGameServerPage, vgui::PropertyPage );

public:
	CreateGameServerPage(vgui::Panel *parent, const char *name);
	~CreateGameServerPage();

	// returns currently entered information about the server
	void			SetMap( const char *name );
	bool			IsRandomMapSelected();
	const char		*GetMapName();

	virtual void	OnTick();

protected:
	virtual void	OnApplyChanges();

private:
	void			LoadMapList();
	void			LoadMaps( const char *pszPathID );

	vgui::ComboBox *m_pMapList;

	enum { DATA_STR_LENGTH = 64 };
	char m_szMapName[DATA_STR_LENGTH];
};


#endif // CREATEMULTIPLAYERGAMESERVERPAGE_H
