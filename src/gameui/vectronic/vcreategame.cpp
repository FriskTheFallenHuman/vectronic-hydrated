//=====================================================================================//
//
// Purpose:
//
//=====================================================================================//
#include "cbase.h"
#include "vcreategame.h"
#include "vcreategameserverpage.h"
#include "vcreategamegameplaypage.h"
#include "EngineInterface.h"
#include "ModInfo.h"
#include "GameUI_Interface.h"
#include <stdio.h>
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "FileSystem.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;
using namespace BaseModUI;

//=============================================================================
CreateGame::CreateGame( vgui::Panel *parent, const char *panelName ) : BaseClass( parent, panelName, true, true )
{
	SetDeleteSelfOnClose( true );
	SetProportional( true );

	SetUpperGarnishEnabled( false );
	SetLowerGarnishEnabled( false );

	m_pProperty = new CreateGameProperty( this );
	m_pProperty->AddActionSignalTarget( this );
}

//=============================================================================
CreateGame::~CreateGame()
{
	GameUI().AllowEngineHideGameUI();
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

	m_pProperty->SetVisible( true );
	m_pProperty->Activate();
	m_pProperty->MoveToFront();
}

//=============================================================================
void CreateGame::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// required for new style
	SetPaintBackgroundEnabled( true );
	SetupAsDialogStyle();

	m_pProperty->InvalidateLayout( true, true );
}

//=============================================================================
void CreateGame::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );
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

	// hax
	case MOUSE_LEFT:
		m_pProperty->Activate();
		break;
	}

	BaseClass::OnKeyTyped( code );
}

//=============================================================================
CreateGameProperty::CreateGameProperty( vgui::Panel *parent ) : PropertyDialog( parent, "CreateGameProperty" )
{
	SetScheme( "ClientScheme" );

	SetTitle( "#GameUI_CreateServer", true );
	SetOKButtonText( "#GameUI_Start" );

	SetDeleteSelfOnClose( true );
	SetMoveable( false );
	SetSizeable( false );

	SetProportional( true );

	m_pServerPage = new CreateGameServerPage(this, "ServerPage");
	m_pGameplayPage = new CreateGameGameplayPage(this, "GameplayPage");

	// create KeyValues object to load/save config options
	m_pSavedData = new KeyValues( "ServerConfig" );

	// load the config data
	if ( m_pSavedData )
	{
		m_pSavedData->LoadFromFile( g_pFullFileSystem, "ServerConfig.vdf", "GAME" ); // this is game-specific data, so it should live in GAME, not CONFIG

		const char *startMap = m_pSavedData->GetString( "map", "" );
		if ( startMap[0] )
		{
			m_pServerPage->SetMap( startMap );
		}
	}
}

//=============================================================================
CreateGameProperty::~CreateGameProperty()
{
	if ( m_pSavedData )
	{
		m_pSavedData->deleteThis();
		m_pSavedData = NULL;
	}

	GameUI().AllowEngineHideGameUI();
}

//=============================================================================
void CreateGameProperty::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "Resource/UI/BaseModUI/CreateGameProperty.res");

	AddPage( m_pServerPage, "#GameUI_Server" );
	AddPage( m_pGameplayPage, "#GameUI_Game" );

	m_pServerPage->InvalidateLayout( true, true );
	m_pGameplayPage->InvalidateLayout( true, true );

	SetOKButtonVisible( false );
	SetCancelButtonVisible( false );
}

//=============================================================================
void CreateGameProperty::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "vguicancel" ) )
	{
		//Close();
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
	}
	else
		BaseClass::OnCommand( command );
}

//=============================================================================
void CreateGameProperty::OnKeyCodeTyped( KeyCode code )
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
bool CreateGameProperty::OnOK( bool applyOnly )
{
	// reset server enforced cvars
	g_pCVar->RevertFlaggedConVars( FCVAR_REPLICATED );	

	// Cheats were disabled; revert all cheat cvars to their default values.
	// This must be done heading into multiplayer games because people can play
	// demos etc and set cheat cvars with sv_cheats 0.
	g_pCVar->RevertFlaggedConVars( FCVAR_CHEAT );

	DevMsg( "FCVAR_CHEAT cvars reverted to defaults.\n" );

	BaseClass::OnOK(applyOnly);

	// get these values from m_pServerPage and store them temporarily
	char szMapName[64], szHostName[64], szPassword[64];
	Q_strncpy(szMapName, m_pServerPage->GetMapName(), sizeof( szMapName ));
	Q_strncpy(szHostName, m_pGameplayPage->GetHostName(), sizeof( szHostName ));
	Q_strncpy(szPassword, m_pGameplayPage->GetPassword(), sizeof( szPassword ));

	// save the config data
	if (m_pSavedData)
	{
		if (m_pServerPage->IsRandomMapSelected())
		{
			// it's set to random map, just save an
			m_pSavedData->SetString( "map", "" );
		}
		else
		{
			m_pSavedData->SetString( "map", szMapName );
		}

		// save config to a file
		m_pSavedData->SaveToFile( g_pFullFileSystem, "ServerConfig.vdf", "GAME" );
	}

	char szMapCommand[1024];

	// create the command to execute
	Q_snprintf( szMapCommand, sizeof( szMapCommand ), "disconnect\nwait\nwait\nsetmaster enable\nmaxplayers %i\nsv_password \"%s\"\nhostname \"%s\"\nprogress_enable\nmap %s\n",
		m_pGameplayPage->GetMaxPlayers(),
		szPassword,
		szHostName,
		szMapName
	);

	// exec
	engine->ClientCmd_Unrestricted( szMapCommand );
	Close();

	return true;
}