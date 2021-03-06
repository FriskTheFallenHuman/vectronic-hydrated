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
	SetDeleteSelfOnClose( true );
	SetProportional( true );

	SetUpperGarnishEnabled( false );
	SetLowerGarnishEnabled( false );

	m_pProperty = new BlogPanel( this );
	m_pProperty->AddActionSignalTarget( this );
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
}

//=============================================================================
void BlogScreen::Activate()
{
	BaseClass::Activate();

	m_pProperty->SetVisible( true );
	m_pProperty->MoveToFront();
}

//=============================================================================
void BlogScreen::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// required for new style
	SetPaintBackgroundEnabled( true );
	SetupAsDialogStyle();

	m_pProperty->InvalidateLayout( true, true );
}

//=============================================================================
void BlogScreen::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "vguicancel" ) )
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
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
	}

	BaseClass::OnKeyTyped( code );
}

//=============================================================================
BlogPanel::BlogPanel( vgui::Panel *parent ) : EditablePanel( parent, "BlogPanel" )
{
	SetScheme( "ClientScheme" );

	SetProportional( true );

	m_pHTMLPanel = new vgui::HTML( this, "HTMLPanel" );
}

//=============================================================================
BlogPanel::~BlogPanel()
{
	GameUI().AllowEngineHideGameUI();
}

//=============================================================================
void BlogPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/BaseModUI/BlogPanel.res");
}

//=============================================================================
void BlogPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_pHTMLPanel )
	{
		m_pHTMLPanel->SetVisible( true );
		m_pHTMLPanel->OpenURL( "https://www.example.com", NULL );
	}
}

//=============================================================================
void BlogPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "vguicancel" ) )
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
	else if ( !Q_stricmp( command, "refresh" ) )
		if ( m_pHTMLPanel )
			m_pHTMLPanel->Refresh();
	else
		BaseClass::OnCommand( command );
}

//=============================================================================
void BlogPanel::OnKeyCodeTyped( KeyCode code )
{
	switch ( code )
	{
	case KEY_ESCAPE:
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
		break;

	case KEY_F4:
		if ( m_pHTMLPanel )
			m_pHTMLPanel->Refresh();
		break;
	}

	BaseClass::OnKeyTyped( code );
}