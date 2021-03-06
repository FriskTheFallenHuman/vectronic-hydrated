//=====================================================================================//
//
// Purpose:
//
//=====================================================================================//

#include "cbase.h"
#include "vcreategameserverpage.h"

using namespace vgui;

#include <KeyValues.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/CheckButton.h>
#include "FileSystem.h"
#include "tier1/convar.h"
#include "EngineInterface.h"
#include "CvarToggleCheckButton.h"
#include <vgui/ISurface.h>
#include <vgui_controls/ImagePanel.h>
#include "ModInfo.h"
#include <materialsystem/imaterial.h>

// for SRC
#include <vstdlib/random.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define RANDOM_MAP "#GameUI_RandomMap"

extern const char* GetMapDisplayName( const char* mapName );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CreateGameServerPage::CreateGameServerPage(vgui::Panel *parent, const char *name) : PropertyPage(parent, name)
{
	// we can use this if we decide we want to put "listen server" at the end of the game name
	m_pMapList = new ComboBox( this, "MapList", 12, false );

	LoadControlSettings( "Resource/UI/BaseModUI/CreateGameServerPage.res" );

	LoadMapList();
	m_szMapName[0]  = 0;

	// initialize hostname
	SetControlString( "ServerNameEdit", ModInfo().GetGameName() );

	// initialize password
	ConVarRef var( "sv_password" );
	if ( var.IsValid() )
		SetControlString("PasswordEdit", var.GetString() );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CreateGameServerPage::~CreateGameServerPage()
{
}

//-----------------------------------------------------------------------------
// Purpose: called to get the info from the dialog
//-----------------------------------------------------------------------------
void CreateGameServerPage::OnApplyChanges()
{
	KeyValues *kv = m_pMapList->GetActiveItemUserData();
	Q_strncpy(m_szMapName, kv->GetString("mapname", ""), DATA_STR_LENGTH);
}

//-----------------------------------------------------------------------------
// Purpose: loads the list of available maps into the map list
//-----------------------------------------------------------------------------
void CreateGameServerPage::LoadMaps( const char *pszPathID )
{
	FileFindHandle_t findHandle = NULL;

	KeyValues *hiddenMaps = ModInfo().GetHiddenMaps();

	const char *pszFilename = g_pFullFileSystem->FindFirst( "maps/*.bsp", &findHandle );
	while ( pszFilename )
	{
		char mapname[256];

		// FindFirst ignores the pszPathID, so check it here
		// TODO: this doesn't find maps in fallback dirs
		Q_snprintf( mapname, sizeof(mapname), "maps/%s", pszFilename );
		if ( !g_pFullFileSystem->FileExists( mapname, pszPathID ) )
		{
			pszFilename = g_pFullFileSystem->FindNext(findHandle);
			continue;
		}

		// remove the text 'maps/' and '.bsp' from the file name to get the map name
		const char *str = Q_strstr( pszFilename, "maps" );
		if ( str )
			Q_strncpy( mapname, str + 5, sizeof(mapname) - 1 );	// maps + \\ = 5
		else
			Q_strncpy( mapname, pszFilename, sizeof(mapname) - 1 );

		char *ext = Q_strstr( mapname, ".bsp" );
		if ( ext )
			*ext = 0;

		// strip out maps that shouldn't be displayed
		if ( hiddenMaps )
		{
			if ( hiddenMaps->GetInt( mapname, 0 ) )
			{
				pszFilename = g_pFullFileSystem->FindNext(findHandle);
				continue;
			}
		}

		// add to the map list
		m_pMapList->AddItem( mapname, new KeyValues( "data", "mapname", mapname ) );
		pszFilename = g_pFullFileSystem->FindNext( findHandle );
	}
	g_pFullFileSystem->FindClose( findHandle );
}

//-----------------------------------------------------------------------------
// Purpose: loads the list of available maps into the map list
//-----------------------------------------------------------------------------
void CreateGameServerPage::LoadMapList()
{
	// clear the current list (if any)
	m_pMapList->DeleteAllItems();

	// add special "name" to represent loading a randomly selected map
	m_pMapList->AddItem( RANDOM_MAP, new KeyValues( "data", "mapname", RANDOM_MAP ) );

	// Load the GameDir maps
	LoadMaps( "GAME" ); 

	// set the first item to be selected
	m_pMapList->ActivateItem( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CreateGameServerPage::IsRandomMapSelected()
{
	const char *mapname = m_pMapList->GetActiveItemUserData()->GetString("mapname");
	if (!stricmp( mapname, RANDOM_MAP ))
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CreateGameServerPage::GetMapName()
{
	int count = m_pMapList->GetItemCount();

	// if there is only one entry it's the special "select random map" entry
	if( count <= 1 )
		return NULL;

	const char *mapname = m_pMapList->GetActiveItemUserData()->GetString( "mapname" );
	if (!strcmp( mapname, RANDOM_MAP ))
	{
		int which = RandomInt( 1, count - 1 );
		mapname = m_pMapList->GetItemUserData( which )->GetString( "mapname" );
	}

	return mapname;
}

//-----------------------------------------------------------------------------
// Purpose: Sets currently selected map in the map combobox
//-----------------------------------------------------------------------------
void CreateGameServerPage::SetMap(const char *mapName)
{
	for (int i = 0; i < m_pMapList->GetItemCount(); i++)
	{
		if (!m_pMapList->IsItemIDValid(i))
			continue;

		if (!stricmp(m_pMapList->GetItemUserData(i)->GetString("mapname"), mapName))
		{
			m_pMapList->ActivateItem(i);
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CreateGameServerPage::OnTick()
{
	BaseClass::OnTick();
	
	if ( !IsVisible() )
		return;

	// set the map name in the UI
	/*wchar_t wzMapName[255]=L"";
	g_pVGuiLocalize->ConvertANSIToUnicode( GetMapDisplayName( GetMapName() ), wzMapName, sizeof( wzMapName ) );

	SetDialogVariable( "maplabel", wzMapName );*/

	// set the background image
	vgui::ImagePanel *pImagePanel = dynamic_cast<ImagePanel *>( FindChildByName( "MapImage" ) );

	char szMapImage[ MAX_PATH ];
	Q_snprintf( szMapImage, sizeof( szMapImage ),  "VGUI/maps/menu_loading_%s_widescreen", GetMapName() );
	Q_strlower( szMapImage );
	IMaterial *pMapMaterial = materials->FindMaterial( szMapImage, TEXTURE_GROUP_VGUI, false );
	if ( pMapMaterial && !IsErrorMaterial( pMapMaterial ) )
	{
		if ( !pImagePanel->IsVisible() )
			pImagePanel->SetVisible( true );

		// take off the vgui/ at the beginning when we set the image
		Q_snprintf( szMapImage, sizeof( szMapImage ), "maps/menu_loading_%s_widescreen", GetMapName() );
		Q_strlower( szMapImage );

		pImagePanel->SetImage( szMapImage );
		//InvalidateLayout(true, true);
	}
	else
	{
		pImagePanel->SetImage( "maps/menu_loading_random" );
	}
};