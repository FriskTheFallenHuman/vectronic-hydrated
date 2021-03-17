//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//
#include "cbase.h"
#include "vcreategame.h"
#include "vhybridbutton.h"
#include "FileSystem.h"
#include <KeyValues.h>
#include "tier1/convar.h"
#include "EngineInterface.h"
#include "CvarToggleCheckButton.h"
#include "ModInfo.h"
#include <stdio.h>
#include <time.h>
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>
#include "PanelListPanel.h"
#include "ScriptObject.h"

// for SRC
#include <vstdlib/random.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

#define RANDOM_MAP "#GameUI_RandomMap"

#define OPTIONS_DIR "cfg"
#define DEFAULT_OPTIONS_FILE OPTIONS_DIR "/settings_default.scr"
#define OPTIONS_FILE OPTIONS_DIR "/settings.scr"

//=============================================================================
// Purpose: class for loading/saving server config file
class CServerDescription : public CDescription
{
public:
	CServerDescription( CPanelListPanel *panel );

	void WriteScriptHeader( FileHandle_t fp );
	void WriteFileHeader( FileHandle_t fp ); 
};

//=============================================================================
CreateGame::CreateGame( Panel *parent, const char *panelName ): BaseClass( parent, panelName )
{
	SetDeleteSelfOnClose( true );
	SetProportional( true );

	// we can use this if we decide we want to put "listen server" at the end of the game name
	m_pMapList = new ComboBox( this, "MapList", 12, false );

	LoadMapList();
	m_szMapName[0]  = 0;

	// initialize hostname
	SetControlString( "ServerNameEdit", ModInfo().GetGameName() );

	// initialize password
	ConVarRef var( "sv_password" );
	if ( var.IsValid() )
		SetControlString( "PasswordEdit", var.GetString() );

	SetUpperGarnishEnabled( true );
	SetLowerGarnishEnabled( true );

	m_pOptionsList = new CPanelListPanel( this, "GameOptions" );

	m_pDescription = new CServerDescription( m_pOptionsList );
	m_pDescription->InitFromFile( DEFAULT_OPTIONS_FILE );
	m_pDescription->InitFromFile( OPTIONS_FILE );
	m_pList = NULL;

	LoadGameOptionsList();

	// create KeyValues object to load/save config options
	m_pSavedData = new KeyValues( "ServerConfig" );

	// load the config data
	if ( m_pSavedData )
	{
		m_pSavedData->LoadFromFile( g_pFullFileSystem, "ServerConfig.vdf", "GAME" ); // this is game-specific data, so it should live in GAME, not CONFIG

		const char *startMap = m_pSavedData->GetString( "map", "" );
		if ( startMap[0] )
			SetMap( startMap );
	}
}

//=============================================================================
CreateGame::~CreateGame()
{
	if ( m_pSavedData )
	{
		m_pSavedData->deleteThis();
		m_pSavedData = NULL;
	}

	GameUI().AllowEngineHideGameUI();

	delete m_pDescription;
}

//=============================================================================
void CreateGame::PerformLayout()
{
	BaseClass::PerformLayout();

	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
}

//=============================================================================
void CreateGame::Activate()
{
	BaseClass::Activate();
}

//=============================================================================
void CreateGame::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// required for new style
	SetPaintBackgroundEnabled( true );
	SetupAsDialogStyle();
}

//=============================================================================
void CreateGame::OnCommand( const char *command )
{
	if ( UI_IsDebug() )
		ConColorMsg( Color( 77, 116, 85, 255 ), "[GAMEUI] Handling CreateGame main menu command %s\n", command );

	if( !Q_strcmp( command, "ok" ) )
	{
		// reset server enforced cvars
		g_pCVar->RevertFlaggedConVars( FCVAR_REPLICATED );	

		// Cheats were disabled; revert all cheat cvars to their default values.
		// This must be done heading into multiplayer games because people can play
		// demos etc and set cheat cvars with sv_cheats 0.
		g_pCVar->RevertFlaggedConVars( FCVAR_CHEAT );

		DevMsg( "FCVAR_CHEAT cvars reverted to defaults.\n" );

		//BaseClass::OnOK( applyOnly );

		// get these values from m_pServerPage and store them temporarily
		char szMapName[64], szHostName[64], szPassword[64];
		Q_strncpy( szMapName, GetMapName(), sizeof( szMapName ) );
		Q_strncpy( szHostName, GetHostName(), sizeof( szHostName ) );
		Q_strncpy( szPassword, GetPassword(), sizeof( szPassword ) );

		// save the config data
		if ( m_pSavedData )
		{
			if ( IsRandomMapSelected() )
				m_pSavedData->SetString( "map", "" );	// it's set to random map, just save an
			else
				m_pSavedData->SetString( "map", szMapName );

			// save config to a file
			m_pSavedData->SaveToFile( g_pFullFileSystem, "ServerConfig.vdf", "GAME" );
		}

		char szMapCommand[1024];

		// create the command to execute
		Q_snprintf( szMapCommand, sizeof( szMapCommand ), "disconnect\nwait\nwait\nsetmaster enable\nmaxplayers %i\nsv_password \"%s\"\nhostname \"%s\"\nprogress_enable\nmap %s\n",
			GetMaxPlayers(),
			szPassword,
			szHostName,
			szMapName
		);

		// exec
		engine->ClientCmd_Unrestricted( szMapCommand );
		Close();
	}
	else if( V_strcmp( command, "Back" ) == 0 )
	{
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
	}
}

//=============================================================================
void CreateGame::OnKeyCodeTyped( KeyCode code )
{
	switch ( code )
	{
	case KEY_ESCAPE:
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
		break;

	case KEY_ENTER:
		OnCommand( "ok" );
		break;
	}

	BaseClass::OnKeyTyped( code );
}

//=============================================================================
void CreateGame::PaintBackground() 
{
	vgui::Panel *m_pFooter = FindChildByName( "PnlBackground" );
	if ( m_pFooter )
	{
		int screenWidth, screenHeight;
		CBaseModPanel::GetSingleton().GetSize( screenWidth, screenHeight );

		int x, y, wide, tall;
		m_pFooter->GetBounds( x, y, wide, tall );
		surface()->DrawSetColor( m_pFooter->GetBgColor() );
		surface()->DrawFilledRect( 0, y, x+screenWidth, y+tall );	
	}
}


//=============================================================================
// Purpose: called to get the info from the dialog
void CreateGame::OnApplyChanges()
{
	// Get the values from the controls
	GatherCurrentValues();

	// Create the game.cfg file
	if ( m_pDescription )
	{
		FileHandle_t fp;

		// Add settings to config.cfg
		m_pDescription->WriteToConfig();

		g_pFullFileSystem->CreateDirHierarchy( OPTIONS_DIR, "MOD" );
		fp = g_pFullFileSystem->Open( OPTIONS_FILE, "wb", "MOD" );
		if ( fp )
		{
			m_pDescription->WriteToScriptFile( fp );
			g_pFullFileSystem->Close( fp );
		}
	}

	KeyValues *kv = m_pMapList->GetActiveItemUserData();
	Q_strncpy( m_szMapName, kv->GetString( "mapname", "" ), DATA_STR_LENGTH );
}

//=============================================================================
// Purpose: loads the list of available maps into the map list
void CreateGame::LoadMaps( const char *pszPathID )
{
	FileFindHandle_t findHandle = NULL;

	KeyValues *hiddenMaps = ModInfo().GetHiddenMaps();

	const char *pszFilename = g_pFullFileSystem->FindFirst( "maps/*.bsp", &findHandle );
	while ( pszFilename )
	{
		char mapname[256];

		// FindFirst ignores the pszPathID, so check it here
		// TODO: this doesn't find maps in fallback dirs
		Q_snprintf( mapname, sizeof( mapname ), "maps/%s", pszFilename );
		if ( !g_pFullFileSystem->FileExists( mapname, pszPathID ) )
		{
			pszFilename = g_pFullFileSystem->FindNext( findHandle );
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

//=============================================================================
// Purpose: loads the list of available maps into the map list
void CreateGame::LoadMapList()
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

//=============================================================================
bool CreateGame::IsRandomMapSelected()
{
	const char *mapname = m_pMapList->GetActiveItemUserData()->GetString( "mapname" );
	if ( !stricmp( mapname, RANDOM_MAP ) )
		return true;

	return false;
}

//=============================================================================
const char *CreateGame::GetMapName()
{
	int count = m_pMapList->GetItemCount();

	// if there is only one entry it's the special "select random map" entry
	if( count <= 1 )
		return NULL;

	const char *mapname = m_pMapList->GetActiveItemUserData()->GetString( "mapname" );
	if ( !strcmp( mapname, RANDOM_MAP ) )
	{
		int which = RandomInt( 1, count - 1 );
		mapname = m_pMapList->GetItemUserData( which )->GetString( "mapname" );
	}

	return mapname;
}

//=============================================================================
// Purpose: Sets currently selected map in the map combobox
void CreateGame::SetMap( const char *mapName )
{
	for ( int i = 0; i < m_pMapList->GetItemCount(); i++ )
	{
		if ( !m_pMapList->IsItemIDValid( i ) )
			continue;

		if ( !stricmp( m_pMapList->GetItemUserData( i )->GetString( "mapname" ), mapName ))
		{
			m_pMapList->ActivateItem( i );
			break;
		}
	}
}

//=============================================================================
int CreateGame::GetMaxPlayers()
{
	return atoi( GetValue( "maxplayers", "33" ) );
}

//=============================================================================
const char *CreateGame::GetPassword()
{
	return GetValue( "sv_password", "" );
}

//=============================================================================
const char *CreateGame::GetHostName()
{
	return GetValue( "hostname", ModInfo().GetGameName() );	
}

//=============================================================================
const char *CreateGame::GetValue( const char *cvarName, const char *defaultValue )
{
	for ( mpcontrol_t *mp = m_pList; mp != NULL; mp = mp->next )
	{
		Panel *control = mp->pControl;
		if ( control && !stricmp( mp->GetName(), cvarName ) )
		{
			KeyValues *data = new KeyValues( "GetText" );
			static char buf[128];
			if ( control && control->RequestInfo( data ) )
				Q_strncpy( buf, data->GetString( "text", defaultValue ), sizeof( buf ) - 1 );
			else
				Q_strncpy( buf, defaultValue, sizeof( buf ) - 1 );	// no value found, copy in default text

			// ensure null termination of string
			buf[sizeof( buf ) - 1] = 0;

			// free
			data->deleteThis();
			return buf;
		}

	}

	return defaultValue;
}

//=============================================================================
// Purpose: Creates all the controls in the game options list
void CreateGame::LoadGameOptionsList()
{
	// destroy any existing controls
	mpcontrol_t *p, *n;

	p = m_pList;
	while ( p )
	{
		n = p->next;
		//
		delete p->pControl;
		delete p->pPrompt;
		delete p;
		p = n;
	}

	m_pList = NULL;

	// Go through desciption creating controls
	CScriptObject *pObj;

	pObj = m_pDescription->pObjList;

	mpcontrol_t	*pCtrl;

	CheckButton *pBox;
	TextEntry *pEdit;
	ComboBox *pCombo;
	CScriptListItem *pListItem;

	Panel *objParent = m_pOptionsList;

	while ( pObj )
	{
		if ( pObj->type == O_OBSOLETE )
		{
			pObj = pObj->pNext;
			continue;
		}

		pCtrl = new mpcontrol_t( objParent, pObj->cvarname );
		pCtrl->type = pObj->type;

		switch ( pCtrl->type )
		{
		case O_BOOL:
			pBox = new CheckButton( pCtrl, "DescCheckButton", pObj->prompt );
			pBox->SetSelected( pObj->fdefValue != 0.0f ? true : false );
			
			pCtrl->pControl = (Panel *)pBox;
			break;
		case O_STRING:
		case O_NUMBER:
			pEdit = new TextEntry( pCtrl, "DescEdit");
			pEdit->InsertString(pObj->defValue);
			pCtrl->pControl = (Panel *)pEdit;
			break;
		case O_LIST:
			pCombo = new ComboBox( pCtrl, "DescEdit", 5, false );

			pListItem = pObj->pListItems;
			while ( pListItem )
			{
				pCombo->AddItem(pListItem->szItemText, NULL);
				pListItem = pListItem->pNext;
			}

			pCombo->ActivateItemByRow((int)pObj->fdefValue);

			pCtrl->pControl = (Panel *)pCombo;
			break;
		default:
			break;
		}

		if ( pCtrl->type != O_BOOL )
		{
			pCtrl->pPrompt = new vgui::Label( pCtrl, "DescLabel", "" );
			pCtrl->pPrompt->SetContentAlignment( vgui::Label::a_west );
			pCtrl->pPrompt->SetTextInset( 5, 0 );
			pCtrl->pPrompt->SetText( pObj->prompt );
		}

		pCtrl->pScrObj = pObj;
		pCtrl->SetSize( 100, 28 );
		//pCtrl->SetBorder( scheme()->GetBorder(1, "DepressedButtonBorder") );
		m_pOptionsList->AddItem( pCtrl );

		// Link it in
		if ( !m_pList )
		{
			m_pList = pCtrl;
			pCtrl->next = NULL;
		}
		else
		{
			mpcontrol_t *p;
			p = m_pList;
			while ( p )
			{
				if ( !p->next )
				{
					p->next = pCtrl;
					pCtrl->next = NULL;
					break;
				}
				p = p->next;
			}
		}

		pObj = pObj->pNext;
	}
}

//=============================================================================
// Purpose: applies all the values in the page
void CreateGame::GatherCurrentValues()
{
	if ( !m_pDescription )
		return;

	// OK
	CheckButton *pBox;
	TextEntry *pEdit;
	ComboBox *pCombo;

	mpcontrol_t *pList;

	CScriptObject *pObj;
	CScriptListItem *pItem;

	char szValue[256];
	char strValue[256];
	wchar_t w_szStrValue[256];

	pList = m_pList;
	while ( pList )
	{
		pObj = pList->pScrObj;

		if ( !pList->pControl )
		{
			pObj->SetCurValue( pObj->defValue );
			pList = pList->next;
			continue;
		}

		switch ( pObj->type )
		{
		case O_BOOL:
			pBox = (CheckButton *)pList->pControl;
			Q_snprintf( szValue, sizeof( szValue ), "%s", pBox->IsSelected() ? "1" : "0" );
			break;
		case O_NUMBER:
			pEdit = ( TextEntry * )pList->pControl;
			pEdit->GetText( strValue, sizeof( strValue ) );
			Q_snprintf( szValue, sizeof( szValue ), "%s", strValue );
			break;
		case O_STRING:
			pEdit = ( TextEntry * )pList->pControl;
			pEdit->GetText( strValue, sizeof( strValue ) );
			Q_snprintf( szValue, sizeof( szValue ), "%s", strValue );
			break;
		case O_LIST:
			pCombo = ( ComboBox *)pList->pControl;
			pCombo->GetText( w_szStrValue, sizeof( w_szStrValue ) / sizeof( wchar_t ) );
			
			pItem = pObj->pListItems;

			while ( pItem )
			{
				wchar_t *wLocalizedString = NULL;
				wchar_t w_szStrTemp[256];
				
				// Localized string?
				if ( pItem->szItemText[0] == '#' )
				{
					wLocalizedString = g_pVGuiLocalize->Find( pItem->szItemText );
				}

				if ( wLocalizedString )
				{
					// Copy the string we found into our temp array
					wcsncpy( w_szStrTemp, wLocalizedString, sizeof( w_szStrTemp ) / sizeof( wchar_t ) );
				}
				else
				{
					// Just convert what we have to Unicode
					g_pVGuiLocalize->ConvertANSIToUnicode( pItem->szItemText, w_szStrTemp, sizeof( w_szStrTemp ) );
				}

				if ( _wcsicmp( w_szStrTemp, w_szStrValue ) == 0 )
				{
					// Found a match!
					break;
				}

				pItem = pItem->pNext;
			}

			if ( pItem )
			{
				Q_snprintf( szValue, sizeof( szValue ), "%s", pItem->szValue );
			}
			else  //Couldn't find index
			{
				Q_snprintf( szValue, sizeof( szValue ), "%s", pObj->defValue );
			}
			break;
		}

		// Remove double quotes and % characters
		UTIL_StripInvalidCharacters( szValue, sizeof( szValue ) );

		Q_strncpy( strValue, szValue, sizeof( strValue ) );

		pObj->SetCurValue( strValue );

		pList = pList->next;
	}
}


//=============================================================================
// Purpose: Constructor, load/save server settings object
CServerDescription::CServerDescription( CPanelListPanel *panel ) : CDescription( panel )
{
	setHint( "// NOTE:  THIS FILE IS AUTOMATICALLY REGENERATED, \r\n"
"//DO NOT EDIT THIS HEADER, YOUR COMMENTS WILL BE LOST IF YOU DO\r\n"
"// Multiplayer options script\r\n"
"//\r\n"
"// Format:\r\n"
"//  Version [float]\r\n"
"//  Options description followed by \r\n"
"//  Options defaults\r\n"
"//\r\n"
"// Option description syntax:\r\n"
"//\r\n"
"//  \"cvar\" { \"Prompt\" { type [ type info ] } { default } }\r\n"
"//\r\n"
"//  type = \r\n"
"//   BOOL   (a yes/no toggle)\r\n"
"//   STRING\r\n"
"//   NUMBER\r\n"
"//   LIST\r\n"
"//\r\n"
"// type info:\r\n"
"// BOOL                 no type info\r\n"
"// NUMBER       min max range, use -1 -1 for no limits\r\n"
"// STRING       no type info\r\n"
"// LIST         "" delimited list of options value pairs\r\n"
"//\r\n"
"//\r\n"
"// default depends on type\r\n"
"// BOOL is \"0\" or \"1\"\r\n"
"// NUMBER is \"value\"\r\n"
"// STRING is \"value\"\r\n"
"// LIST is \"index\", where index \"0\" is the first element of the list\r\n\r\n\r\n" );

	setDescription ( "SERVER_OPTIONS" );
}

//=============================================================================
void CServerDescription::WriteScriptHeader( FileHandle_t fp )
{
    char am_pm[] = "AM";

	time_t timeVal;
	time(&timeVal);

	struct tm tmStruct;
	struct tm newtime = *(Plat_localtime((const time_t*)&timeVal, &tmStruct));

    if( newtime.tm_hour > 12 )        /* Set up extension. */
		Q_strncpy( am_pm, "PM", sizeof( am_pm ) );
	if( newtime.tm_hour > 12 )        /* Convert from 24-hour */
		newtime.tm_hour -= 12;    /*   to 12-hour clock.  */
	if( newtime.tm_hour == 0 )        /*Set hour to 12 if midnight. */
		newtime.tm_hour = 12;

	g_pFullFileSystem->FPrintf( fp, (char *)getHint() );

	char timeString[64];
	Plat_ctime(&timeVal, timeString, sizeof(timeString));

// Write out the comment and Cvar Info:
	g_pFullFileSystem->FPrintf( fp, "// Half-Life Server Configuration Layout Script (stores last settings chosen, too)\r\n" );
	g_pFullFileSystem->FPrintf( fp, "// File generated:  %.19s %s\r\n", timeString, am_pm );
	g_pFullFileSystem->FPrintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );

	g_pFullFileSystem->FPrintf( fp, "VERSION %.1f\r\n\r\n", SCRIPT_VERSION );

	g_pFullFileSystem->FPrintf( fp, "DESCRIPTION SERVER_OPTIONS\r\n{\r\n" );
}

//=============================================================================
void CServerDescription::WriteFileHeader( FileHandle_t fp )
{
    char am_pm[] = "AM";
   
	time_t timeVal;
	time(&timeVal);

	struct tm tmStruct;
	struct tm newtime = *(Plat_localtime((const time_t*)&timeVal, &tmStruct));

    if( newtime.tm_hour > 12 )        /* Set up extension. */
		Q_strncpy( am_pm, "PM", sizeof( am_pm ) );
	if( newtime.tm_hour > 12 )        /* Convert from 24-hour */
		newtime.tm_hour -= 12;    /*   to 12-hour clock.  */
	if( newtime.tm_hour == 0 )        /*Set hour to 12 if midnight. */
		newtime.tm_hour = 12;

	char timeString[64];
	Plat_ctime(&timeVal, timeString, sizeof(timeString));

	g_pFullFileSystem->FPrintf( fp, "// Half-Life Server Configuration Settings\r\n" );
	g_pFullFileSystem->FPrintf( fp, "// DO NOT EDIT, GENERATED BY HALF-LIFE\r\n" );
	g_pFullFileSystem->FPrintf( fp, "// File generated:  %.19s %s\r\n", timeString, am_pm );
	g_pFullFileSystem->FPrintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );
}
