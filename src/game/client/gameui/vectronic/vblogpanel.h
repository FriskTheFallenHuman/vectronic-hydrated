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
	// Purpose:
	//-----------------------------------------------------------------------------
	class BlogScreen : public CBaseModFrame
	{
		DECLARE_CLASS_SIMPLE( BlogScreen, CBaseModFrame );

	public:
		BlogScreen( vgui::Panel *parent, const char *panelName );
		~BlogScreen();

		virtual void PerformLayout();

	protected:
		virtual void Activate();
		virtual void PaintBackground();
		virtual void ApplySchemeSettings( vgui::IScheme* pScheme );
		virtual void OnKeyCodeTyped( vgui::KeyCode code );
		virtual void OnCommand( const char *command );

	private:
		vgui::HTML* m_pHTMLPanel;
	};
};

#endif // __VBlogScreen_H__