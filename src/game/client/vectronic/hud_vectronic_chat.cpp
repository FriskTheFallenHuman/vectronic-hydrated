//=========== Copyright � 2014, rHetorical, All rights reserved. =============
//
// Purpose: 
//		
//=============================================================================
#include "cbase.h"
#include "hud_vectronic_chat.h"
#include "hud_macros.h"
#include "text_message.h"
#include "vguicenterprint.h"
#include "vgui/ILocalize.h"
#include "vgui/IScheme.h"
#include "engine/IEngineSound.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "c_vectronic_player.h"
#include "vectronic_gamerules.h"
#include "ihudlcd.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudChat );
DECLARE_HUD_MESSAGE( CHudChat, SayText );
DECLARE_HUD_MESSAGE( CHudChat, SayText2 );
DECLARE_HUD_MESSAGE( CHudChat, TextMsg );
DECLARE_HUD_MESSAGE( CHudChat, VoiceSubtitle );


//=====================
//CHudChatLine
//=====================

void CHudChatLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_hFont = pScheme->GetFont( "ChatFont" );
	SetBorder( NULL );
	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetFgColor( Color( 0, 0, 0, 0 ) );

	SetFont( m_hFont );
}

CHudChatLine::CHudChatLine( vgui::Panel *parent, const char *panelName ) : CBaseHudChatLine( parent, panelName )
{
	m_text = NULL;
}

//=====================
//CHudChatInputLine
//=====================
void CHudChatInputLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

//=====================
//CHudChat
//=====================
CHudChat::CHudChat( const char *pElementName ) : BaseClass( pElementName )
{
}

void CHudChat::CreateChatInputLine( void )
{
	m_pChatInput = new CHudChatInputLine( this, "ChatInputLine" );
	m_pChatInput->SetVisible( false );
}

void CHudChat::CreateChatLines( void )
{
	m_ChatLine = new CHudChatLine( this, "ChatLine1" );
	m_ChatLine->SetVisible( false );		
}

void CHudChat::Init( void )
{
	BaseClass::Init();

	HOOK_HUD_MESSAGE( CHudChat, SayText );
	HOOK_HUD_MESSAGE( CHudChat, SayText2 );
	HOOK_HUD_MESSAGE( CHudChat, TextMsg );
	HOOK_HUD_MESSAGE( CHudChat, VoiceSubtitle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CHudChat::GetChatInputOffset( void )
{
	if ( m_pChatInput->IsVisible() )
		return m_iFontHeight;
	else
		return 0;
}

int CHudChat::GetFilterForString( const char *pString )
{
	int iFilter = BaseClass::GetFilterForString( pString );

	if ( iFilter == CHAT_FILTER_NONE )
	{
		if ( !Q_stricmp( pString, "#Vectronic_Name_Change" ) ) 
			return CHAT_FILTER_NAMECHANGE;
	}

	return iFilter;
}

//-----------------------------------------------------------------------------
Color CHudChat::GetDefaultTextColor( void )
{
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	if ( pScheme == NULL )
		return Color( 255, 255, 255, 255 );

	return pScheme->GetColor( "Colors.ChatTextWhite", Color( 255, 255, 255, 255 ) );
}

//-----------------------------------------------------------------------------
Color CHudChat::GetClientColor( int clientIndex )
{
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	if ( pScheme == NULL )
		return Color( 255, 255, 255, 255 );

	if ( clientIndex == 0 ) // console msg
	{
		return pScheme->GetColor( "Colors.ChatTextCyan", Color( 255, 255, 255, 255 ) );
	}
	else if( g_PR )
	{
		int iTeam = g_PR->GetTeam( clientIndex );

		switch ( iTeam )
		{
			case TEAM_UNASSIGNED : 
				return pScheme->GetColor( "Colors.ChatTextBlue", Color( 255, 255, 255, 255 ) );

			default	: 
				return pScheme->GetColor( "Colors.ChatTextNightSky", Color( 255, 255, 255, 255 ) );
		}
	}

	return pScheme->GetColor( "Colors.ChatTextWhite", Color( 255, 255, 255, 255 ) );
}

const char *CHudChat::GetDisplayedSubtitlePlayerName( int clientIndex )
{
	C_VectronicPlayer *pPlayer = To_VectronicPlayer( UTIL_PlayerByIndex( clientIndex ) );
	C_VectronicPlayer *pLocalPlayer = C_VectronicPlayer::GetLocalVectronicPlayer();

	if ( !pPlayer || !pLocalPlayer )
		return BaseClass::GetDisplayedSubtitlePlayerName( clientIndex );

	return BaseClass::GetDisplayedSubtitlePlayerName( clientIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CHudChat::GetTextColorForClient( TextColor colorNum, int clientIndex )
{
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	if ( pScheme == NULL )
		return Color( 255, 255, 255, 255 );

	Color c;
	switch ( colorNum )
	{
	case COLOR_CUSTOM:
		c = m_ColorCustom;
		break;

	case COLOR_PLAYERNAME:
		c = GetClientColor( clientIndex );
		break;

	case COLOR_LOCATION:
		c = g_ColorDarkGreen;
		break;

	case COLOR_ACHIEVEMENT:
		{
			IScheme *pSourceScheme = scheme()->GetIScheme( scheme()->GetScheme( "SourceScheme" ) ); 
			if ( pSourceScheme )
				c = pSourceScheme->GetColor( "SteamLightGreen", GetBgColor() );
			else
				c = pScheme->GetColor( "Colors.ChatTextWhite", GetBgColor() );
		}
		break;

	default:
		c = pScheme->GetColor( "Colors.ChatTextWhite", GetBgColor() );
	}

	return Color( c[0], c[1], c[2], 255 );
}

int CHudChat::GetFilterFlags( void )
{
	// Don't display messages on singleplayer
	if ( rHGameRules() && rHGameRules()->IsMultiplayer() == false )
		return CHAT_FILTER_PUBLICCHAT;

	int iFlags = BaseClass::GetFilterFlags();	
	return iFlags;
}
