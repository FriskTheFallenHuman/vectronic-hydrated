//=====================================================================================//
//
// Purpose:
//
//=====================================================================================//
#include "cbase.h"
#include "vcreditsscreen.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

CreditsScreen::CreditsScreen( Panel *parent, const char *panelName ): BaseClass( parent, panelName, true, true )
{
	SetDeleteSelfOnClose( true );
	SetProportional( true );

	SetUpperGarnishEnabled( false );
	SetLowerGarnishEnabled( false );

	m_pProperty = new CreditsPanel( this );
	m_pProperty->AddActionSignalTarget( this );
}

//=============================================================================
CreditsScreen::~CreditsScreen()
{
	GameUI().AllowEngineHideGameUI();
}

//=============================================================================
void CreditsScreen::PerformLayout()
{
	BaseClass::PerformLayout();

	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
}

//=============================================================================
void CreditsScreen::Activate()
{
	BaseClass::Activate();

	m_pProperty->SetVisible( true );
	m_pProperty->MoveToFront();
}

//=============================================================================
void CreditsScreen::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// required for new style
	SetPaintBackgroundEnabled( true );
	SetupAsDialogStyle();

	m_pProperty->InvalidateLayout( true, true );
}

//=============================================================================
void CreditsScreen::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );
}

//=============================================================================
void CreditsScreen::OnKeyCodeTyped( KeyCode code )
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
CreditsPanel::CreditsPanel( vgui::Panel *parent ) : EditablePanel( parent, "CreditsPanel" )
{
	SetScheme( "ClientScheme" );

	SetProportional( true );

	vgui::Button* pBack = dynamic_cast< vgui::Button* >( FindChildByName( "BtnBack" ) );
	if ( pBack )
		pBack->SetScheme( "ClientScheme" );

	m_RichText = dynamic_cast< vgui::RichText* >( FindChildByName( "CreditText" ) );
	if ( m_RichText )
		m_RichText->SetScheme( "ClientScheme" );
}

//=============================================================================
CreditsPanel::~CreditsPanel()
{
	GameUI().AllowEngineHideGameUI();
}

//=============================================================================
void CreditsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/BaseModUI/CreditsPanel.res");
}

//=============================================================================
void CreditsPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "vguicancel" ) )
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
	else
		BaseClass::OnCommand( command );
}

//=============================================================================
void CreditsPanel::OnKeyCodeTyped( KeyCode code )
{
	switch ( code )
	{
	case KEY_ESCAPE:
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
		break;
	}

	BaseClass::OnKeyTyped( code );
}