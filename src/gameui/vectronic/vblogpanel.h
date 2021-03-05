//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef __VBlogScreen_H__
#define __VBlogScreen_H__

#include "basemodui.h"
#include <vgui_controls/HTML.h>

namespace BaseModUI 
{
	//-----------------------------------------------------------------------------
	// Purpose: dialog for the window
	//-----------------------------------------------------------------------------
	class BlogPanel : public vgui::EditablePanel
	{
		DECLARE_CLASS_SIMPLE( BlogPanel,  vgui::EditablePanel );

	public:
		BlogPanel( vgui::Panel *parent );
		~BlogPanel();

		virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
		virtual void	OnCommand( const char *command );
		virtual void	PerformLayout();
		virtual void	OnKeyCodeTyped( vgui::KeyCode code );

	private:
		vgui::HTML* m_pHTMLPanel;
	};

	//-----------------------------------------------------------------------------
	// Purpose:
	//-----------------------------------------------------------------------------
	class BlogScreen : public CBaseModFrame
	{
		DECLARE_CLASS_SIMPLE( BlogScreen, CBaseModFrame );

	public:
		BlogScreen( vgui::Panel *parent, const char *panelName );
		~BlogScreen();

		virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
		virtual void	PaintBackground() {}
		virtual void	PerformLayout();
		virtual void	Activate();
		virtual void	OnCommand( const char *command );
		virtual void	OnKeyCodeTyped( vgui::KeyCode code );

	private:
		BlogPanel* m_pProperty;
	};
};

#endif // __VBlogScreen_H__