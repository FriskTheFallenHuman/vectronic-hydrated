//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "EngineInterface.h"
#include "vgenericpanellist.h"
#include "iachievementmgr.h"
#include "KeyValues.h"
#include "fmtstr.h"

#include "vgui/IBorder.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Divider.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/ProgressBar.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/TextImage.h"

#include "filesystem.h"
#include "cdll_util.h"
#include "vgui/ISurface.h"
#include "vachievements.h"
#include "ui_defines.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

KeyValues *g_pPreloadedAchievementListItemLayout = NULL;

#pragma warning( disable : 4800 ) // warning C4800: 'uint64' : forcing value to bool 'true' or 'false' (performance warning)

//-----------------------------------------------------------------------------
// Purpose: Sets the parameter pIconPanel to display the specified achievement's icon file.
//-----------------------------------------------------------------------------
bool LoadNewAchievementIcon( vgui::ImagePanel* pIconPanel, IAchievement *pAchievement, const char *pszExt /*= NULL*/ )
{
	char imagePath[_MAX_PATH];
	Q_strncpy( imagePath, "achievements\\", sizeof(imagePath) );
	Q_strncat( imagePath, pAchievement->GetName(), sizeof(imagePath), COPY_ALL_CHARACTERS );
	if ( pszExt )
	{
		Q_strncat( imagePath, pszExt, sizeof(imagePath), COPY_ALL_CHARACTERS );
	}
	Q_strncat( imagePath, ".vtf", sizeof(imagePath), COPY_ALL_CHARACTERS );

	char checkFile[_MAX_PATH];
	Q_snprintf( checkFile, sizeof(checkFile), "materials\\vgui\\%s", imagePath );
	if ( !g_pFullFileSystem->FileExists( checkFile ) )
	{
		Q_snprintf( imagePath, sizeof(imagePath), "hud\\icon_locked.vtf" );
	}

	pIconPanel->SetShouldScaleImage( true );
	pIconPanel->SetImage( imagePath ? imagePath : "vgui/white" );
	pIconPanel->SetVisible( true );

	return pIconPanel->IsVisible();
}

AchievementListItem::AchievementListItem( IAchievement *pAchievement ) : BaseClass( NULL, "AchievementListItem" )
{
	SetProportional( true );

	m_LblName = new Label( this, "LblName", "" );
	m_LblProgress = new Label( this, "LblProgress", "#GameUI_Achievement_Awarded" );
	m_DivTitleDivider = new Divider( this, "DivTitleDivider" );
	m_ImgAchievementIcon = new ImagePanel( this, "ImgAchievementIcon" );
	m_LblHowTo = new Label( this, "LblHowTo", "" );
	m_PrgProgress = new ContinuousProgressBar(this, "PrgProgress" );
	m_LblCurrProgress = new Label( this, "LblCurrProgress", "0" );

	m_pAchievement = NULL;

	m_bShowingDetails = false;

	SetAchievement( pAchievement );
	SetAchievementName( ACHIEVEMENT_LOCALIZED_NAME( pAchievement ) );
	SetAchievementHowTo( ACHIEVEMENT_LOCALIZED_DESC( pAchievement ) );
	SetAchievementIcon( pAchievement );
	SetAchievementGoal( pAchievement->GetGoal() );
	SetAchievementProgress( pAchievement->GetGoal() );	
}

//=============================================================================
void AchievementListItem::SetAchievement( IAchievement *pAchievement )
{
	if ( !pAchievement )
		return;

	m_pAchievement = pAchievement;

	InvalidateLayout();
}

//=============================================================================
void AchievementListItem::SetAchievementName( const wchar_t* name )
{
	m_LblName->SetText(name);
}

//=============================================================================
void AchievementListItem::SetAchievementHowTo( const wchar_t* howTo )
{
	m_LblHowTo->SetText(howTo);
}

//=============================================================================
void AchievementListItem::SetAchievementIcon( IAchievement *pAchievement )
{
	if ( pAchievement->IsAchieved() )
	{
		LoadNewAchievementIcon( m_ImgAchievementIcon, pAchievement );
	}
	else
	{
		LoadNewAchievementIcon( m_ImgAchievementIcon, pAchievement, "_bw" );
	}
}

//=============================================================================
void AchievementListItem::SetAchievementProgress(int progress)
{
	m_AchievementProgress = progress;

	float fProgress = static_cast<float>(progress) / static_cast<float>(m_AchievementGoal);

	char buffer[64];
	Q_snprintf(buffer, 63, "%d / %d", m_AchievementProgress, m_AchievementGoal);
	m_LblCurrProgress->SetText(buffer);

	m_PrgProgress->SetProgress(fProgress);

	if(fProgress < 1.0f)
	{
		fProgress *= 100.0f;

		char buffer[8];
		Q_snprintf(buffer, 7, "%2.0f%%", fProgress);
	}

	// For achievements that don't have multiple steps do not display progress bar or progress label
	m_PrgProgress->SetVisible( m_AchievementGoal > 1 );
	m_LblCurrProgress->SetVisible( m_AchievementGoal > 1 );

	InvalidateLayout();
}

//=============================================================================
void AchievementListItem::SetAchievementGoal( int goal )
{
	m_AchievementGoal = goal;

	// reset the achievement progress to refresh the labels
	SetAchievementProgress( m_AchievementProgress );
}

//=============================================================================
int AchievementListItem::GetGoal() const
{
	return m_AchievementGoal;
}

//=============================================================================
int AchievementListItem::GetProgress() const
{
	return m_AchievementProgress;
}

//=============================================================================
bool AchievementListItem::GetCompleted() const
{
	return m_AchievementGoal == m_AchievementProgress;
}

//=============================================================================
void AchievementListItem::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( !m_pAchievement )
		return;

	if ( !g_pPreloadedAchievementListItemLayout )
	{
		const char *pszResource = "Resource/UI/BaseModUI/AchievementListItem.res";
		g_pPreloadedAchievementListItemLayout = new KeyValues( pszResource );
		g_pPreloadedAchievementListItemLayout->LoadFromFile(g_pFullFileSystem, pszResource);
	}

	LoadControlSettings( "", NULL, g_pPreloadedAchievementListItemLayout );

	m_iOriginalTall = GetTall();

	KeyValues *pListItem = g_pPreloadedAchievementListItemLayout->FindKey( "AchievementListItem" );
	if ( pListItem )
	{
		if ( m_pAchievement->IsAchieved() )
		{
			SetBgColor( pScheme->GetColor( "AchievementsLightGrey", Color(79, 79, 79, 255) ) );

			m_LblName->SetFgColor( pScheme->GetColor( "SteamLightGreen", Color(157, 194, 80, 255) ) );
			m_LblHowTo->SetFgColor( pScheme->GetColor( "AchievementsLightGrey", Color(229, 223, 211, 90) ) );
		}
		else
		{
			SetBgColor( pScheme->GetColor( "AchievementsDarkGrey", Color(55, 55, 55, 255) ) );

			Color fgColor = pScheme->GetColor( "AchievementsInactiveFG", Color(130, 130, 130, 255) );
			m_LblName->SetFgColor( fgColor );
			m_LblHowTo->SetFgColor( fgColor );
		}	

	}

	SetAchievementName( ACHIEVEMENT_LOCALIZED_NAME( m_pAchievement ) );
	SetAchievementHowTo( ACHIEVEMENT_LOCALIZED_DESC( m_pAchievement ) );
	SetAchievementIcon( m_pAchievement );
	SetAchievementGoal( m_pAchievement->GetGoal() );
	SetAchievementProgress( m_pAchievement->IsAchieved() ? m_pAchievement->GetGoal() : m_pAchievement->GetCount() );
}

//=============================================================================
void AchievementListItem::NavigateTo()
{
	BaseClass::NavigateTo();
}


//=============================================================================
void AchievementListItem::PerformLayout( void )
{
	BaseClass::PerformLayout();
}

//=============================================================================
void AchievementListItem::OnCommand( const char *command )
{
	if ( !Q_strcmp( command, "toggle_details" ) )
	{
		m_bShowingDetails = !m_bShowingDetails;
		InvalidateLayout();
	}
}

//=============================================================================
void AchievementListItem::OnSizeChanged( int newWide, int newTall )
{
	BaseClass::OnSizeChanged( newWide, newTall );

	PostActionSignal( new KeyValues( "ChildResized" ) );
}

//=============================================================================
void AchievementListItem::Paint(void)
{
	BaseClass::Paint();
}

//=============================================================================
AchievementListItemLabel::AchievementListItemLabel(Panel *parent, const char *panelName):
BaseClass(parent, panelName)
{
	SetProportional( true );

	m_LblCategory = new Label( this, "LblCategory", "#GameUI_Achievement_Unlocked" );

	LoadControlSettings("Resource/UI/BaseModUI/AchievementListItemLabel.res");
}

//=============================================================================
AchievementListItemLabel::~AchievementListItemLabel()
{
	delete m_LblCategory;
}

//=============================================================================
void AchievementListItemLabel::SetCategory( const wchar_t* category )
{
	m_LblCategory->SetText( category );
}

class BaseModUI::AchievementGenericPanelList : public GenericPanelList
{
	DECLARE_CLASS_SIMPLE( AchievementGenericPanelList, GenericPanelList );

public:
	AchievementGenericPanelList::AchievementGenericPanelList( vgui::Panel *parent, const char *panelName, ITEM_SELECTION_MODE selectionMode, int iControllingSlot ) :
	    BaseClass( parent, panelName, selectionMode ),
		m_iControllingUserSlot( iControllingSlot )
	{
	}

protected:
	void OnKeyCodePressed(KeyCode code)
	{
		if ( m_iControllingUserSlot != GetJoystickForCode( code ) )
			return;

		BaseClass::OnKeyCodePressed(code);
	}
	int m_iControllingUserSlot;
};

//=============================================================================
//
//=============================================================================
Achievements::Achievements(Panel *parent, const char *panelName):
BaseClass(parent, panelName, false, true)
{
	GameUI().PreventEngineHideGameUI();

	// Determine the slot and controller of the player who opened the dialog
	m_iStartingUserSlot = CBaseModPanel::GetSingleton().GetLastActiveUserId();

	memset( m_wAchievementsTitle, 0, sizeof( m_wAchievementsTitle ) );

	SetDeleteSelfOnClose( true );
	SetProportional( true );

	m_LblComplete = new Label(this, "LblComplete", ""); 
	m_GplAchievements = new AchievementGenericPanelList( this, "GplAchievements", GenericPanelList::ISM_ELEVATOR, m_iStartingUserSlot );
	m_GplAchievements->ShowScrollProgress( true );
	m_GplAchievements->SetScrollBarVisible( IsPC() );
	m_GplAchievements->SetBgColor( Color( 0, 0, 0, 0 ) );

	m_GplAwards = NULL;

	m_pProgressBar = new ContinuousProgressBar( this, "ProTotalProgress" );

	SetUpperGarnishEnabled( false );
	SetLowerGarnishEnabled( false );

	SetOkButtonEnabled( false );

	m_ActiveControl = m_GplAchievements;

	LoadControlSettings("Resource/UI/BaseModUI/Achievements.res");

	m_bShowingAssets = false;
	m_iAwardCompleteCount = 0;
	m_iAchCompleteCount = 0;
}

//=============================================================================
Achievements::~Achievements()
{
	GameUI().AllowEngineHideGameUI();
}

//=============================================================================
void Achievements::PerformLayout()
{
	BaseClass::PerformLayout();

	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
}

//=============================================================================
void Achievements::Activate()
{
	BaseClass::Activate();

	// Populate the achievements list.
	m_GplAchievements->RemoveAllPanelItems();

	if ( !achievementmgr )
		return;

	m_iAwardCompleteCount = 0;
	m_iAchCompleteCount = 0;
	int incompleteCount= 0;
	int gamerScore = 0;

	//
	// Add the "Achieved" ones
	//
	for( int i = 0; i < achievementmgr->GetAchievementCount(); i++ )
	{
		IAchievement* achievement = achievementmgr->GetAchievementByIndex( i );

		if ( achievement && achievement->IsAchieved() )
		{
			AchievementListItem *panelItem = new AchievementListItem( achievement );
			if ( panelItem )
			{
				m_GplAchievements->AddPanelItem( panelItem, true );
			}

			gamerScore += achievement->GetPointValue();
			++m_iAchCompleteCount;
		}
	}

	//
	// Add the "Unattained" ones
	//
	for(int i = 0; i < achievementmgr->GetAchievementCount(); i++)
	{
		IAchievement* achievement = achievementmgr->GetAchievementByIndex( i );

		if ( achievement && !achievement->IsAchieved() && !achievement->ShouldHideUntilAchieved() )
		{
			AchievementListItem *panelItem = new AchievementListItem( achievement );
			if ( panelItem )
				m_GplAchievements->AddPanelItem( panelItem, true );

			++incompleteCount;
		}
	} 

	if ( m_GplAwards )
	{
		m_GplAwards->SetVisible( false );
	}

	//
	// Update achievement and gamerscore progress
	//
	char buffer[64];
	wchar_t wNumAchieved[64];
	wchar_t wTotalAchievements[64];

	// Construct achievement progress string
	itoa( achievementmgr->GetAchievementCount(), buffer, 10 );
	V_UTF8ToUnicode( buffer, wTotalAchievements, sizeof( wNumAchieved ) );
	itoa( m_iAchCompleteCount, buffer, 10 );
	V_UTF8ToUnicode( buffer, wNumAchieved, sizeof( wTotalAchievements ) );
	g_pVGuiLocalize->ConstructString( m_wAchievementsProgress, sizeof( m_wAchievementsProgress ), g_pVGuiLocalize->Find( "#GameUI_Achievement_Progress_Fmt" ), 2, wNumAchieved, wTotalAchievements );
	m_LblComplete->SetText( m_wAchievementsProgress );

	// Focus on the first item in the list
	m_GplAchievements->NavigateTo();
	m_GplAchievements->SelectPanelItem( 0 );

	// Set the progress bar
	m_flTotalProgress = static_cast<float>(m_iAchCompleteCount) / static_cast<float>(achievementmgr->GetAchievementCount());

	ToggleDisplayType( m_bShowingAssets );
}

//=============================================================================
void Achievements::OnCommand(const char *command)
{
	if( V_strcmp( command, "Back" ) == 0 )
	{
		// Act as though 360 back button was pressed
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
		m_bShowingAssets = false;
	}
	else
	{
		BaseClass::OnCommand( command );
	}	
}

//=============================================================================
void Achievements::OnKeyCodePressed(KeyCode code)
{
	if ( m_iStartingUserSlot != GetJoystickForCode( code ) )
		return;

	switch( GetBaseButtonCode( code ) )
	{
	case KEY_XBUTTON_Y:
		ToggleDisplayType( !m_bShowingAssets );
		break;
	}

	BaseClass::OnKeyCodePressed(code);
}

//=============================================================================
void Achievements::ToggleDisplayType( bool bDisplayType )
{
	if ( IsPC() )
		return;

	m_bShowingAssets = bDisplayType;

	if ( m_bShowingAssets )
	{
		m_GplAwards->SetVisible( true );
		m_GplAchievements->SetVisible( false );

		m_GplAwards->NavigateTo();
		m_GplAwards->SelectPanelItem( 0 );

		m_ActiveControl = m_GplAwards;
	}
	else
	{
		m_GplAwards->SetVisible( false );
		m_GplAchievements->SetVisible( true );

		m_GplAchievements->NavigateTo();
		m_GplAchievements->SelectPanelItem( 0 );

		m_ActiveControl = m_GplAchievements;
	}

	m_ActiveControl->SetBgColor( Color( 0, 0, 0, 0 ) );

	char buffer[64];
	wchar_t wNumAchieved[64];
	wchar_t wTotalAchievements[64];

	itoa( achievementmgr->GetAchievementCount(), buffer, 10 );
	V_UTF8ToUnicode( buffer, wTotalAchievements, sizeof( wNumAchieved ) );
	itoa( m_bShowingAssets ? m_iAwardCompleteCount : m_iAchCompleteCount, buffer, 10 );
	V_UTF8ToUnicode( buffer, wNumAchieved, sizeof( wTotalAchievements ) );
	g_pVGuiLocalize->ConstructString( m_wAchievementsProgress, sizeof( m_wAchievementsProgress ), g_pVGuiLocalize->Find( "#GameUI_Achievement_Progress_Fmt" ), 2, wNumAchieved, wTotalAchievements );
	m_LblComplete->SetText( m_wAchievementsProgress );
	m_flTotalProgress = static_cast<float>(m_bShowingAssets ? m_iAwardCompleteCount : m_iAchCompleteCount) / static_cast<float>(achievementmgr->GetAchievementCount());
	m_pProgressBar->SetProgress( m_flTotalProgress );
}

//=============================================================================
void Achievements::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetPaintBackgroundEnabled( true );
	SetupAsDialogStyle();

	m_pProgressBar->SetProgress( m_flTotalProgress );

	// Set up total completion percentage bar
	float flCompletion = 0.0f;
	if ( achievementmgr && achievementmgr->GetAchievementCount() > 0 )
	{
		flCompletion = (((float)m_iAchCompleteCount) / ((float)achievementmgr->GetAchievementCount()));
	}

	Color clrHighlight = pScheme->GetColor( "NewGame.SelectionColor", Color(255, 255, 255, 255) );
	Color clrWhite(255, 255, 255, 255);

	Color cProgressBar = Color( static_cast<float>( clrHighlight.r() ) * ( 1.0f - flCompletion ) + static_cast<float>( clrWhite.r() ) * flCompletion,
		static_cast<float>( clrHighlight.g() ) * ( 1.0f - flCompletion ) + static_cast<float>( clrWhite.g() ) * flCompletion,
		static_cast<float>( clrHighlight.b() ) * ( 1.0f - flCompletion ) + static_cast<float>( clrWhite.b() ) * flCompletion,
		static_cast<float>( clrHighlight.a() ) * ( 1.0f - flCompletion ) + static_cast<float>( clrWhite.a() ) * flCompletion );

	m_pProgressBar->SetFgColor( cProgressBar );
}