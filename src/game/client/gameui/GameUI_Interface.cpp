//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Implements all the functions exported by the GameUI dll
//
// $NoKeywords: $
//===========================================================================//

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#endif

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <tier0/dbg.h>

#ifdef SendMessage
#undef SendMessage
#endif

#include "filesystem.h"
#include "GameUI_Interface.h"
#include "Sys_Utils.h"
#include "string.h"
#include "tier0/icommandline.h"

// interface to engine
#include "EngineInterface.h"
#include "VGuiSystemModuleLoader.h"
#include "bitmap/tgaloader.h"
#include "GameConsole.h"
#include "ModInfo.h"
#include "game/client/IGameClientExports.h"
#include "materialsystem/imaterialsystem.h"
#include "ixboxsystem.h"
#include "iachievementmgr.h"
#include "IGameUIFuncs.h"
#include "ienginevgui.h"
#include "video/ivideoservices.h"

// vgui2 interface
// note that GameUI project uses ..\vgui2\include, not ..\utils\vgui\include
#include "vgui/Cursor.h"
#include "tier1/KeyValues.h"
#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/IScheme.h"
#include "vgui/IVGui.h"
#include "vgui/ISystem.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/PHandle.h"
#include "tier3/tier3.h"
#include "matsys_controls/matsyscontrols.h"
#include "steam/steam_api.h"
#include "game/server/iplayerinfo.h"
#include "avi/iavi.h"

#include "basemodpanel.h"
#include "basemodui.h"
typedef BaseModUI::CBaseModPanel UI_BASEMOD_PANEL_CLASS;
inline UI_BASEMOD_PANEL_CLASS & GetUiBaseModPanelClass() { return UI_BASEMOD_PANEL_CLASS::GetSingleton(); }
inline UI_BASEMOD_PANEL_CLASS & ConstructUiBaseModPanelClass() { return * new UI_BASEMOD_PANEL_CLASS(); }

#include "tier0/dbg.h"
#include "engine/IEngineSound.h"
#include <SoundEmitterSystem/isoundemittersystembase.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IVEngineClient *engine = NULL;
IGameUIFuncs *gameuifuncs = NULL;
IEngineSound *enginesound = NULL;
ISoundEmitterSystemBase *soundemitterbase = NULL;
IXboxSystem *xboxsystem = NULL;
IVideoServices *g_pVideo = NULL;

IEngineVGui *enginevguifuncs = NULL;
vgui::ISurface *enginesurfacefuncs = NULL;
IAchievementMgr *achievementmgr = NULL;
IGameEventManager2 *gameeventmanager = nullptr;

class CGameUI;
CGameUI *g_pGameUI = NULL;

static CGameUI g_GameUI;

#ifdef _WIN32
static WHANDLE g_hMutex = NULL;
static WHANDLE g_hWaitMutex = NULL;
#endif

static IGameClientExports *g_pGameClientExports = NULL;
IGameClientExports *GameClientExports()
{
	return g_pGameClientExports;
}

//-----------------------------------------------------------------------------
// Purpose: singleton accessor
//-----------------------------------------------------------------------------
CGameUI &GameUI()
{
	return g_GameUI;
}

//-----------------------------------------------------------------------------
// Purpose: hack function to give the module loader access to the main panel handle
//			only used in VguiSystemModuleLoader
//-----------------------------------------------------------------------------
vgui::VPANEL GetGameUIBasePanel()
{
	return GetUiBaseModPanelClass().GetVPanel();
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameUI, IGameUI, GAMEUI_INTERFACE_VERSION, g_GameUI);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameUI::CGameUI()
{
	g_pGameUI = this;
	m_bTryingToLoadFriends = false;
	m_iFriendsLoadPauseFrames = 0;
	m_iGameIP = 0;
	m_iGameConnectionPort = 0;
	m_iGameQueryPort = 0;
	m_bActivatedUI = false;
	m_szPreviousStatusText[0] = 0;
	m_bIsConsoleUI = false;
	m_bHasSavedThisMenuSession = false;
	m_bOpenProgressOnStart = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameUI::~CGameUI()
{
	g_pGameUI = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Initialization
//-----------------------------------------------------------------------------
void CGameUI::Initialize( CreateInterfaceFn factory )
{
	MEM_ALLOC_CREDIT();
	ConnectTier1Libraries( &factory, 1 );
	ConnectTier2Libraries( &factory, 1 );
	ConVar_Register( FCVAR_CLIENTDLL );
	ConnectTier3Libraries( &factory, 1 );

	gameuifuncs = (IGameUIFuncs *)factory(VENGINE_GAMEUIFUNCS_VERSION, NULL);
	soundemitterbase = (ISoundEmitterSystemBase *)factory(SOUNDEMITTERSYSTEM_INTERFACE_VERSION, NULL);
	xboxsystem = (IXboxSystem *)factory(XBOXSYSTEM_INTERFACE_VERSION, NULL);

	enginesound = (IEngineSound *)factory(IENGINESOUND_CLIENT_INTERFACE_VERSION, NULL);
	engine = (IVEngineClient *)factory( VENGINE_CLIENT_INTERFACE_VERSION, NULL );

	gameeventmanager = (IGameEventManager2 *)factory(INTERFACEVERSION_GAMEEVENTSMANAGER2, nullptr);

	g_pVideo = (IVideoServices *)factory(VIDEO_SERVICES_INTERFACE_VERSION, NULL);

#if !defined NO_STEAM
	SteamAPI_InitSafe();
	steamapicontext->Init();
#endif

	ConVarRef var( "gameui_xbox" );
	m_bIsConsoleUI = var.IsValid() && var.GetBool();

	vgui::VGui_InitInterfacesList( "GameUI", &factory, 1 );
	vgui::VGui_InitMatSysInterfacesList( "GameUI", &factory, 1 );

	// load localization file
	g_pVGuiLocalize->AddFile( "Resource/gameui_%language%.txt", "GAME", true );

	// load mod info
	ModInfo().LoadCurrentGameInfo();

	// load localization file for kb_act.lst
	g_pVGuiLocalize->AddFile( "Resource/valve_%language%.txt", "GAME", true );

	//bool bFailed = false;
	enginevguifuncs = (IEngineVGui *)factory( VENGINE_VGUI_VERSION, NULL );
	enginesurfacefuncs = (vgui::ISurface *)factory(VGUI_SURFACE_INTERFACE_VERSION, NULL);
	gameuifuncs = (IGameUIFuncs *)factory( VENGINE_GAMEUIFUNCS_VERSION, NULL );
	xboxsystem = (IXboxSystem *)factory( XBOXSYSTEM_INTERFACE_VERSION, NULL );

	if (!enginesurfacefuncs || !gameuifuncs || !enginevguifuncs || !xboxsystem)
	{
		Warning( "CGameUI::Initialize() failed to get necessary interfaces\n" );
	}

	// setup base panel
	UI_BASEMOD_PANEL_CLASS& factoryBasePanel = ConstructUiBaseModPanelClass(); // explicit singleton instantiation

	factoryBasePanel.SetBounds( 0, 0, 640, 480 );
	factoryBasePanel.SetPaintBorderEnabled( false );
	factoryBasePanel.SetPaintBackgroundEnabled( true );
	factoryBasePanel.SetPaintEnabled( true );
	factoryBasePanel.SetVisible( true );

	factoryBasePanel.SetMouseInputEnabled( IsPC() );
	// factoryBasePanel.SetKeyBoardInputEnabled( IsPC() );
	factoryBasePanel.SetKeyBoardInputEnabled( true );

	vgui::VPANEL rootpanel = enginevguifuncs->GetPanel( PANEL_GAMEUIDLL );
	factoryBasePanel.SetParent( rootpanel );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGameUI::PostInit()
{
	// to know once client dlls have been loaded
	BaseModUI::CUIGameData::Get()->OnGameUIPostInit();
}

//-----------------------------------------------------------------------------
// Purpose: connects to client interfaces
//-----------------------------------------------------------------------------
void CGameUI::Connect( CreateInterfaceFn gameFactory )
{
	g_pGameClientExports = (IGameClientExports *)gameFactory(GAMECLIENTEXPORTS_INTERFACE_VERSION, NULL);

	achievementmgr = engine->GetAchievementMgr();

	if (!g_pGameClientExports)
	{
		Warning("CGameUI::Initialize() failed to get necessary interfaces\n");
	}

	m_GameFactory = gameFactory;
}

//-----------------------------------------------------------------------------
// Purpose: Callback function; sends platform Shutdown message to specified window
//-----------------------------------------------------------------------------
int __stdcall SendShutdownMsgFunc(WHANDLE hwnd, int lparam)
{
	Sys_PostMessage(hwnd, Sys_RegisterWindowMessage("ShutdownValvePlatform"), 0, 1);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Searches for GameStartup*.mp3 files in the sound/ui folder and plays one
//-----------------------------------------------------------------------------
void CGameUI::PlayGameStartupSound()
{
	if ( CommandLine()->FindParm( "-nostartupsound" ) )
		return;

	FileFindHandle_t fh;

	CUtlVector<char *> fileNames;

	char path[ 512 ];
	Q_snprintf( path, sizeof( path ), "sound/ui/gamestartup*.mp3" );
	Q_FixSlashes( path );

	char const *fn = g_pFullFileSystem->FindFirstEx( path, "MOD", &fh );
	if ( fn )
	{
		do
		{
			char ext[ 10 ];
			Q_ExtractFileExtension( fn, ext, sizeof( ext ) );

			if ( !Q_stricmp( ext, "mp3" ) )
			{
				char temp[ 512 ];
				Q_snprintf( temp, sizeof( temp ), "ui/%s", fn );

				char *found = new char[ strlen( temp ) + 1 ];
				Q_strncpy( found, temp, strlen( temp ) + 1 );

				Q_FixSlashes( found );
				fileNames.AddToTail( found );
			}
	
			fn = g_pFullFileSystem->FindNext( fh );

		} while ( fn );

		g_pFullFileSystem->FindClose( fh );
	}

	// did we find any?
	if ( fileNames.Count() > 0 )
	{
		SYSTEMTIME SystemTime;
		GetSystemTime( &SystemTime );
		int index = SystemTime.wMilliseconds % fileNames.Count();

		if ( fileNames.IsValidIndex( index ) && fileNames[index] )
		{
			char found[ 512 ];

			// escape chars "*#" make it stream, and be affected by snd_musicvolume
			Q_snprintf( found, sizeof( found ), "play *#%s", fileNames[index] );

			engine->ClientCmd_Unrestricted( found );
		}

		fileNames.PurgeAndDeleteElements();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called to setup the game UI
//-----------------------------------------------------------------------------
void CGameUI::Start()
{
	// determine Steam location for configuration
	if ( !FindPlatformDirectory( m_szPlatformDir, sizeof( m_szPlatformDir ) ) )
		return;

	if ( IsPC() )
	{
		// setup config file directory
		char szConfigDir[512];
		Q_strncpy( szConfigDir, m_szPlatformDir, sizeof( szConfigDir ) );
		Q_strncat( szConfigDir, "config", sizeof( szConfigDir ), COPY_ALL_CHARACTERS );

		ConColorMsg( Color( 43, 62, 206, 255 ),  "Steam config directory: %s\n", szConfigDir );

		g_pFullFileSystem->AddSearchPath(szConfigDir, "CONFIG");
		g_pFullFileSystem->CreateDirHierarchy("", "CONFIG");

		// user dialog configuration
		vgui::system()->SetUserConfigFile("InGameDialogConfig.vdf", "CONFIG");

		g_pFullFileSystem->AddSearchPath( "platform", "PLATFORM" );
	}

	// localization
	g_pVGuiLocalize->AddFile( "Resource/platform_%language%.txt");
	g_pVGuiLocalize->AddFile( "Resource/vgui_%language%.txt");

#ifdef _WIN32
	Sys_SetLastError( SYS_NO_ERROR );

	if ( IsPC() )
	{
		g_hMutex = Sys_CreateMutex( "ValvePlatformUIMutex" );
		g_hWaitMutex = Sys_CreateMutex( "ValvePlatformWaitMutex" );
		if ( g_hMutex == NULL || g_hWaitMutex == NULL || Sys_GetLastError() == SYS_ERROR_INVALID_HANDLE )
		{
			// error, can't get handle to mutex
			if (g_hMutex)
			{
				Sys_ReleaseMutex(g_hMutex);
			}
			if (g_hWaitMutex)
			{
				Sys_ReleaseMutex(g_hWaitMutex);
			}
			g_hMutex = NULL;
			g_hWaitMutex = NULL;
			Warning("Steam Error: Could not access Steam, bad mutex\n");
			return;
		}
		unsigned int waitResult = Sys_WaitForSingleObject(g_hMutex, 0);
		if (!(waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED))
		{
			// mutex locked, need to deactivate Steam (so we have the Friends/ServerBrowser data files)
			// get the wait mutex, so that Steam.exe knows that we're trying to acquire ValveTrackerMutex
			waitResult = Sys_WaitForSingleObject(g_hWaitMutex, 0);
			if (waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED)
			{
				Sys_EnumWindows(SendShutdownMsgFunc, 1);
			}
		}

		// Delay playing the startup music until two frames
		// this allows cbuf commands that occur on the first frame that may start a map

		// now we are set up to check every frame to see if we can friends/server browser
		m_bTryingToLoadFriends = true;
		m_iFriendsLoadPauseFrames = 1;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Finds which directory the platform resides in
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameUI::FindPlatformDirectory(char *platformDir, int bufferSize)
{
	platformDir[0] = '\0';

	if ( platformDir[0] == '\0' )
	{
#ifdef _WIN32
		// we're not under steam, so setup using path relative to game
		if ( IsPC() )
		{
			if ( ::GetModuleFileName( ( HINSTANCE )GetModuleHandle( NULL ), platformDir, bufferSize ) )
			{
				char *lastslash = strrchr(platformDir, '\\'); // this should be just before the filename
				if ( lastslash )
				{
					*lastslash = 0;
					Q_strncat(platformDir, "\\platform\\", bufferSize, COPY_ALL_CHARACTERS );
					return true;
				}
			}
		}
		else
		{
			// xbox fetches the platform path from exisiting platform search path
			// path to executeable is not correct for xbox remote configuration
			if ( g_pFullFileSystem->GetSearchPath( "PLATFORM", false, platformDir, bufferSize ) )
			{
				char *pSeperator = strchr( platformDir, ';' );
				if ( pSeperator )
					*pSeperator = '\0';
				return true;
			}
		}

		Warning( "Unable to determine platform directory\n" );
		return false;
#endif
	}

	return (platformDir[0] != 0);
}

//-----------------------------------------------------------------------------
// Purpose: Called to Shutdown the game UI system
//-----------------------------------------------------------------------------
void CGameUI::Shutdown()
{
	// notify all the modules of Shutdown
	g_VModuleLoader.ShutdownPlatformModules();

	// unload the modules them from memory
	g_VModuleLoader.UnloadPlatformModules();

	ModInfo().FreeModInfo();

#ifdef _WIN32	
	// release platform mutex
	// close the mutex
	if (g_hMutex)
	{
		Sys_ReleaseMutex(g_hMutex);
	}
	if (g_hWaitMutex)
	{
		Sys_ReleaseMutex(g_hWaitMutex);
	}
#endif

	steamapicontext->Clear();
	
	ConVar_Unregister();
	DisconnectTier3Libraries();
	DisconnectTier2Libraries();
	DisconnectTier1Libraries();
}

//-----------------------------------------------------------------------------
// Purpose: just wraps an engine call to activate the gameUI
//-----------------------------------------------------------------------------
void CGameUI::ActivateGameUI()
{
	engine->ExecuteClientCmd("gameui_activate");
}

//-----------------------------------------------------------------------------
// Purpose: just wraps an engine call to hide the gameUI
//-----------------------------------------------------------------------------
void CGameUI::HideGameUI()
{
	engine->ExecuteClientCmd("gameui_hide");
}

//-----------------------------------------------------------------------------
// Purpose: Toggle allowing the engine to hide the game UI with the escape key
//-----------------------------------------------------------------------------
void CGameUI::PreventEngineHideGameUI()
{
	engine->ExecuteClientCmd("gameui_preventescape");
}

//-----------------------------------------------------------------------------
// Purpose: Toggle allowing the engine to hide the game UI with the escape key
//-----------------------------------------------------------------------------
void CGameUI::AllowEngineHideGameUI()
{
	engine->ExecuteClientCmd("gameui_allowescape");
}

//-----------------------------------------------------------------------------
// Purpose: Activate the game UI
//-----------------------------------------------------------------------------
void CGameUI::OnGameUIActivated()
{
	//bool bWasActive = m_bActivatedUI;
	m_bActivatedUI = true;

	SetSavedThisMenuSession( false );

	UI_BASEMOD_PANEL_CLASS &ui = GetUiBaseModPanelClass();
	bool bNeedActivation = true;
	if ( ui.IsVisible() )
	{
		// Already visible, maybe don't need activation
		if ( !IsInLevel() && IsInBackgroundLevel() )
			bNeedActivation = false;
	}

	if ( bNeedActivation )
	{
		GetUiBaseModPanelClass().OnGameUIActivated();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hides the game ui, in whatever state it's in
//-----------------------------------------------------------------------------
void CGameUI::OnGameUIHidden()
{
	//bool bWasActive = m_bActivatedUI;
	m_bActivatedUI = false;

	GetUiBaseModPanelClass().OnGameUIHidden();
}

//-----------------------------------------------------------------------------
// Purpose: paints all the vgui elements
//-----------------------------------------------------------------------------
void CGameUI::RunFrame()
{
	int wide, tall;
#if defined( TOOLFRAMEWORK_VGUI_REFACTOR )
	// resize the background panel to the screen size
	vgui::VPANEL clientDllPanel = enginevguifuncs->GetPanel( PANEL_ROOT );

	int x, y;
	vgui::ipanel()->GetPos( clientDllPanel, x, y );
	vgui::ipanel()->GetSize( clientDllPanel, wide, tall );
	staticPanel->SetBounds( x, y, wide,tall );
#else
	vgui::surface()->GetScreenSize(wide, tall);

	GetUiBaseModPanelClass().SetSize(wide, tall);
#endif

	// Run frames
	g_VModuleLoader.RunFrame();

	GetUiBaseModPanelClass().RunFrame();

#ifdef _WIN32
	if ( IsPC() && m_bTryingToLoadFriends && m_iFriendsLoadPauseFrames-- < 1 && g_hMutex && g_hWaitMutex )
	{
		// try and load Steam platform files
		unsigned int waitResult = Sys_WaitForSingleObject(g_hMutex, 0);
		if (waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED)
		{
			// we got the mutex, so load Friends/Serverbrowser
			// clear the loading flag
			m_bTryingToLoadFriends = false;
			g_VModuleLoader.LoadPlatformModules(&m_GameFactory, 1, false);

			// release the wait mutex
			Sys_ReleaseMutex(g_hWaitMutex);

			// notify the game of our game name
			const char *fullGamePath = engine->GetGameDirectory();
			const char *pathSep = strrchr( fullGamePath, '/' );
			if ( !pathSep )
			{
				pathSep = strrchr( fullGamePath, '\\' );
			}
			if ( pathSep )
			{
				KeyValues *pKV = new KeyValues("ActiveGameName" );
				pKV->SetString( "name", pathSep + 1 );
				pKV->SetInt( "appid", engine->GetAppID() );
				KeyValues *modinfo = new KeyValues("ModInfo");
				if ( modinfo->LoadFromFile( g_pFullFileSystem, "gameinfo.txt" ) )
				{
					pKV->SetString( "game", modinfo->GetString( "game", "" ) );
				}
				modinfo->deleteThis();
				
				g_VModuleLoader.PostMessageToAllModules( pKV );
			}

			// notify the ui of a game connect if we're already in a game
			if (m_iGameIP)
			{
				SendConnectedToGameMessage();
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called when the game connects to a server
//-----------------------------------------------------------------------------
void CGameUI::OLD_OnConnectToServer(const char *game, int IP, int port)
{
	// Nobody should use this anymore because the query port and the connection port can be different.
	// Use OnConnectToServer2 instead.
	Assert( false );
	OnConnectToServer2( game, IP, port, port );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the game connects to a server
//-----------------------------------------------------------------------------
void CGameUI::OnConnectToServer2(const char *game, int IP, int connectionPort, int queryPort)
{
	m_iGameIP = IP;
	m_iGameConnectionPort = connectionPort;
	m_iGameQueryPort = queryPort;

	SendConnectedToGameMessage();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGameUI::SendConnectedToGameMessage()
{
	MEM_ALLOC_CREDIT();
	KeyValues *kv = new KeyValues( "ConnectedToGame" );
	kv->SetInt( "ip", m_iGameIP );
	kv->SetInt( "connectionport", m_iGameConnectionPort );
	kv->SetInt( "queryport", m_iGameQueryPort );

	g_VModuleLoader.PostMessageToAllModules( kv );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the game disconnects from a server
//-----------------------------------------------------------------------------
void CGameUI::OnDisconnectFromServer( uint8 eSteamLoginFailure )
{
	m_iGameIP = 0;
	m_iGameConnectionPort = 0;
	m_iGameQueryPort = 0;

	g_VModuleLoader.PostMessageToAllModules( new KeyValues( "DisconnectedFromGame" ) );
}

//-----------------------------------------------------------------------------
// Purpose: activates the loading dialog on level load start
//-----------------------------------------------------------------------------
void CGameUI::OnLevelLoadingStarted( bool bShowProgressDialog )
{
	g_VModuleLoader.PostMessageToAllModules( new KeyValues( "LoadingStarted" ) );

	GetUiBaseModPanelClass().OnLevelLoadingStarted( bShowProgressDialog );
	ShowLoadingBackgroundDialog();

	if ( bShowProgressDialog )
		StartProgressBar();
}

//-----------------------------------------------------------------------------
// Purpose: closes any level load dialog
//-----------------------------------------------------------------------------
void CGameUI::OnLevelLoadingFinished(bool bError, const char *failureReason, const char *extendedReason)
{
	StopProgressBar( bError, failureReason, extendedReason );

	// notify all the modules
	g_VModuleLoader.PostMessageToAllModules( new KeyValues( "LoadingFinished" ) );

	// Need to call this function in the Base mod Panel to let it know that we've finished loading the level
	// This should fix the loading screen not disappearing.
	GetUiBaseModPanelClass().OnLevelLoadingFinished( new KeyValues( "LoadingFinished" ) );
	HideLoadingBackgroundDialog();
}

//-----------------------------------------------------------------------------
// Purpose: Updates progress bar
// Output : Returns true if screen should be redrawn
//-----------------------------------------------------------------------------
bool CGameUI::UpdateProgressBar(float progress, const char *statusText)
{
	return GetUiBaseModPanelClass().UpdateProgressBar(progress, statusText);
}

//-----------------------------------------------------------------------------
// Purpose: sets loading info text
//-----------------------------------------------------------------------------
bool CGameUI::SetProgressBarStatusText( const char *statusText )
{
	if ( !statusText )
		return false;

	if ( !stricmp( statusText, m_szPreviousStatusText ) )
		return false;

	if ( !GetUiBaseModPanelClass().SetProgressBarStatusText( statusText ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're currently playing the game
//-----------------------------------------------------------------------------
bool CGameUI::IsInLevel()
{
	const char *levelName = engine->GetLevelName();
	if (levelName && levelName[0] && !engine->IsLevelMainMenuBackground())
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're at the main menu and a background level is loaded
//-----------------------------------------------------------------------------
bool CGameUI::IsInBackgroundLevel()
{
	const char *levelName = engine->GetLevelName();
	if (levelName && levelName[0] && engine->IsLevelMainMenuBackground())
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're in a multiplayer game
//-----------------------------------------------------------------------------
bool CGameUI::IsInMultiplayer()
{
	return (IsInLevel() && engine->GetMaxClients() > 1);
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're console ui
//-----------------------------------------------------------------------------
bool CGameUI::IsConsoleUI()
{
	return m_bIsConsoleUI;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we've saved without closing the menu
//-----------------------------------------------------------------------------
bool CGameUI::HasSavedThisMenuSession()
{
	return m_bHasSavedThisMenuSession;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGameUI::SetSavedThisMenuSession( bool bState )
{
	m_bHasSavedThisMenuSession = bState;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGameUI::NeedConnectionProblemWaitScreen()
{
	BaseModUI::CUIGameData::Get()->NeedConnectionProblemWaitScreen();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGameUI::ShowPasswordUI( char const *pchCurrentPW )
{
	BaseModUI::CUIGameData::Get()->ShowPasswordUI( pchCurrentPW );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGameUI::SetProgressOnStart()
{
	m_bOpenProgressOnStart = true;
}
