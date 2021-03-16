//=====================================================================================//
//
// Purpose:
//
//=====================================================================================//

#include "cbase.h"
#include "vblogpanel.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

BlogScreen::BlogScreen( Panel *parent, const char *panelName ): BaseClass( parent, panelName, true, true )
{
	GameUI().PreventEngineHideGameUI();

	SetDeleteSelfOnClose( true );

	SetProportional( true );

	SetUpperGarnishEnabled( true );
	SetLowerGarnishEnabled( true );

	m_pHTMLPanel = new vgui::HTML( this, "HTMLPanel" );
}

//=============================================================================
BlogScreen::~BlogScreen()
{
	GameUI().AllowEngineHideGameUI();
}

//=============================================================================
void BlogScreen::PerformLayout()
{
	BaseClass::PerformLayout();

	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );

	if ( m_pHTMLPanel )
	{
		m_pHTMLPanel->SetVisible( true );
		m_pHTMLPanel->OpenURL( "https://www.example.com", NULL );
	}
}

//=============================================================================
void BlogScreen::Activate()
{
	BaseClass::Activate();
}

//=============================================================================
void BlogScreen::PaintBackground()
{
	BaseClass::DrawDialogBackground( "#GameUI_GameMenu_News", NULL, "#GameUI_GameMenu_News_Tip", NULL, NULL, true );
}

//=============================================================================
void BlogScreen::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// required for new style
	SetPaintBackgroundEnabled( true );
	SetupAsDialogStyle();

	if ( m_pHTMLPanel )
		m_pHTMLPanel->SetScheme( vgui::scheme()->LoadSchemeFromFileEx( 0, "resource/MenuScheme.res", "MenuScheme" ) );
}

//=============================================================================
void BlogScreen::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "Back" ) )
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
	else if ( !Q_stricmp( command, "refresh" ) )
		if ( m_pHTMLPanel )
			m_pHTMLPanel->Refresh();
	else
		BaseClass::OnCommand( command );
}

//=============================================================================
void BlogScreen::OnKeyCodeTyped( KeyCode code )
{
	switch ( code )
	{
	case KEY_ESCAPE:
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
		break;
	case KEY_F4:
		if ( m_pHTMLPanel )
			m_pHTMLPanel->Refresh();
	}

	BaseClass::OnKeyTyped( code );
}