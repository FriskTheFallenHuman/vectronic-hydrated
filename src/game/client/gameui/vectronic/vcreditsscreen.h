//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef __VCREDITSSCREEN_H__
#define __VCREDITSSCREEN_H__

#include "basemodui.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/RichText.h>

namespace BaseModUI 
{
	//-----------------------------------------------------------------------------
	// Purpose: dialog for the window
	//-----------------------------------------------------------------------------
	class CreditsPanel : public vgui::EditablePanel
	{
		DECLARE_CLASS_SIMPLE( CreditsPanel,  vgui::EditablePanel );

	public:
		CreditsPanel( vgui::Panel *parent );
		~CreditsPanel();

		virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
		virtual void	OnCommand( const char *command );
		virtual void	OnKeyCodeTyped( vgui::KeyCode code );

	private:
		vgui::RichText* m_RichText;
	};

	//-----------------------------------------------------------------------------
	// Purpose:
	//-----------------------------------------------------------------------------
	class CreditsScreen : public CBaseModFrame
	{
		DECLARE_CLASS_SIMPLE( CreditsScreen, CBaseModFrame );

	public:
		CreditsScreen( vgui::Panel *parent, const char *panelName );
		~CreditsScreen();

		virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
		virtual void	PaintBackground() {}
		virtual void	PerformLayout();
		virtual void	Activate();
		virtual void	OnCommand( const char *command );
		virtual void	OnKeyCodeTyped( vgui::KeyCode code );

	private:
		CreditsPanel* m_pProperty;
	};
};

#endif // __VCREDITSSCREEN_H__