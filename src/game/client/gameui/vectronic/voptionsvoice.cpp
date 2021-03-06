//=====================================================================================//
//
// Purpose:
//
//=====================================================================================//

#include "cbase.h"
#include "vOptionsVoice.h"
#include "CvarSlider.h"
#include <vgui/IVGui.h>
#include "ExMenuButton.h"
#include "ExMenuLabel.h"
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Slider.h>
#include "EngineInterface.h"
#include "ivoicetweak.h"
#include "CvarToggleCheckButton.h"
#include "tier1/KeyValues.h"
#include "tier1/convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
OptionsVoice::OptionsVoice( vgui::Panel *parent, const char *name ) : PropertyPage( parent, name )
{
#if !defined( NO_VOICE ) //#ifndef _XBOX
	m_pVoiceTweak = engine->GetVoiceTweakAPI();
#endif

	m_pMicMeter = new ImagePanel(this, "MicMeter");
	m_pMicMeter2 = new ImagePanel(this, "MicMeter2");

	m_pReceiveSliderLabel = new CExMenuLabel(this, "ReceiveLabel", "#GameUI_VoiceReceiveVolume");
	m_pReceiveVolume = new CCvarSlider( this, "VoiceReceive", "#GameUI_ReceiveVolume",
		0.0f, 1.0f, "voice_scale" );

	m_pMicrophoneSliderLabel = new CExMenuLabel(this, "MicrophoneLabel", "#GameUI_VoiceTransmitVolume");
	m_pMicrophoneVolume = new Slider( this, "#GameUI_MicrophoneVolume" );
	m_pMicrophoneVolume->SetRange( 0, 100 );
	m_pMicrophoneVolume->AddActionSignalTarget( this );

	m_pVoiceEnableCheckButton = new CCvarToggleCheckButton( this, "voice_modenable", "#GameUI_EnableVoice", "voice_modenable" );

	m_pMicBoost = new CheckButton(this, "MicBoost", "#GameUI_BoostMicrophone" );
	m_pMicBoost->AddActionSignalTarget( this );

	m_pTestMicrophoneButton = new CExMenuButton(this, "TestMicrophone", "#GameUI_TestMicrophone");

	LoadControlSettings( "Resource/UI/BaseModUI/OptionsVoice.res" );

	m_bVoiceOn = false;
	m_pMicMeter2->SetVisible(false);
	// no voice tweak - then disable all buttons
	if (!m_pVoiceTweak)
	{
		m_pReceiveVolume->SetEnabled(false);
		m_pMicrophoneVolume->SetEnabled(false);
		m_pVoiceEnableCheckButton->SetEnabled(false);
		m_pMicBoost->SetEnabled(false);
		m_pTestMicrophoneButton->SetEnabled(false);
	}
	else
	{
		OnResetData();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
OptionsVoice::~OptionsVoice()
{
	// turn off voice if it was on, since we're leaving this page
	if (m_bVoiceOn)
	{
		EndTestMicrophone();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::OnResetData()
{
	if (!m_pVoiceTweak)
		return;

	float micVolume = m_pVoiceTweak->GetControlFloat( MicrophoneVolume );
	m_pMicrophoneVolume->SetValue( (int)( 100.0f * micVolume ) );
	m_nMicVolumeValue = m_pMicrophoneVolume->GetValue();

	float fMicBoost = m_pVoiceTweak->GetControlFloat( MicBoost );
	m_pMicBoost->SetSelected( fMicBoost != 0.0f );
	m_bMicBoostSelected = m_pMicBoost->IsSelected();

	m_pReceiveVolume->Reset();
	m_fReceiveVolume = m_pReceiveVolume->GetSliderValue();

	m_pVoiceEnableCheckButton->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::OnSliderMoved( int position )
{
	if (m_pVoiceTweak)
	{
		if (m_pMicrophoneVolume->GetValue() != m_nMicVolumeValue)
		{
			PostActionSignal(new KeyValues("ApplyButtonEnable"));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::OnCheckButtonChecked( int state )
{
	if (m_pVoiceTweak)
	{
		// if our state is different
		if ( m_pMicBoost->IsSelected() != m_bMicBoostSelected)
		{
			PostActionSignal(new KeyValues("ApplyButtonEnable"));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::OnApplyChanges()
{
	if (!m_pVoiceTweak)
		return;

	m_nMicVolumeValue = m_pMicrophoneVolume->GetValue();
	float fMicVolume = (float) m_nMicVolumeValue / 100.0f;
	m_pVoiceTweak->SetControlFloat( MicrophoneVolume, fMicVolume );

	m_bMicBoostSelected = m_pMicBoost->IsSelected();
	m_pVoiceTweak->SetControlFloat( MicBoost, m_bMicBoostSelected ? 1.0f : 0.0f );

	m_pReceiveVolume->ApplyChanges();
	m_fReceiveVolume = m_pReceiveVolume->GetSliderValue();

	m_pVoiceEnableCheckButton->ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::StartTestMicrophone()
{
	if (!m_pVoiceTweak || m_bVoiceOn)
		return;

	m_bVoiceOn = true;

	UseCurrentVoiceParameters();

	if (m_pVoiceTweak->StartVoiceTweakMode())
	{
		m_pTestMicrophoneButton->SetText("#GameUI_StopTestMicrophone");

		m_pReceiveVolume->SetEnabled(false);
		m_pMicrophoneVolume->SetEnabled(false);
		m_pVoiceEnableCheckButton->SetEnabled(false);
		m_pMicBoost->SetEnabled(false);
		m_pMicrophoneSliderLabel->SetEnabled(false);
		m_pReceiveSliderLabel->SetEnabled(false);

		m_pMicMeter2->SetVisible(true);
	}
	else
	{
		ResetVoiceParameters();

		// we couldn't start it
		m_bVoiceOn = false;
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::UseCurrentVoiceParameters()
{
	int nVal = m_pMicrophoneVolume->GetValue();
	float val = (float) nVal / 100.0f;
	m_pVoiceTweak->SetControlFloat( MicrophoneVolume, val );

	bool bSelected = m_pMicBoost->IsSelected();
	val = bSelected ? 1.0f : 0.0f;
	m_pVoiceTweak->SetControlFloat( MicBoost, val );

	// get where the current slider is
	m_nReceiveSliderValue = m_pReceiveVolume->GetValue();
	m_pReceiveVolume->ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::ResetVoiceParameters()
{
	float fMicVolume = (float) m_nMicVolumeValue / 100.0f;
	m_pVoiceTweak->SetControlFloat( MicrophoneVolume, fMicVolume );
	m_pVoiceTweak->SetControlFloat( MicBoost, m_bMicBoostSelected ? 1.0f : 0.0f );

	// restore the old value
	ConVarRef voice_scale( "voice_scale" );
	voice_scale.SetValue( m_fReceiveVolume );
	
	m_pReceiveVolume->Reset();
	// set the slider to 'new' value, but we've reset the 'start' value where it was
	m_pReceiveVolume->SetValue(m_nReceiveSliderValue);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::EndTestMicrophone()
{
	if (!m_pVoiceTweak || !m_bVoiceOn)
		return;

	if ( m_pVoiceTweak->IsStillTweaking() )
	{
		m_pVoiceTweak->EndVoiceTweakMode();
	}
	ResetVoiceParameters();
	m_pTestMicrophoneButton->SetText("#GameUI_TestMicrophone");
	m_bVoiceOn = false;

	m_pReceiveVolume->SetEnabled(true);
	m_pMicrophoneVolume->SetEnabled(true);
	m_pVoiceEnableCheckButton->SetEnabled(true);
	m_pMicBoost->SetEnabled(true);
	m_pMicrophoneSliderLabel->SetEnabled(true);
	m_pReceiveSliderLabel->SetEnabled(true);
	m_pMicMeter2->SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::OnCommand( const char *command)
{
	if (!stricmp(command, "TestMicrophone"))
	{
		if (!m_bVoiceOn)
		{
			StartTestMicrophone();
		}
		else
		{
			EndTestMicrophone();
		}
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::OnPageHide()
{
	// turn off voice if it was on, since we're leaving this page
	if (m_bVoiceOn)
	{
		EndTestMicrophone();
	}
	BaseClass::OnPageHide();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::OnControlModified()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OptionsVoice::OnThink()
{
	BaseClass::OnThink();
	if (m_bVoiceOn)
	{
		if ( !m_pVoiceTweak->IsStillTweaking() )
		{
			DevMsg( 1, "Lost Voice Tweak channels, resetting\n" );
			EndTestMicrophone();
		}
		else
		{
			float val = m_pVoiceTweak->GetControlFloat( SpeakingVolume );
			int nValue = static_cast<int>( val*32768.0f + 0.5f );

			int width = (BAR_WIDTH * nValue) / 32768;
			width = ((width + (BAR_INCREMENT-1)) / BAR_INCREMENT) * BAR_INCREMENT;  // round to nearest BAR_INCREMENT

			int wide, tall;
			m_pMicMeter2->GetSize(wide, tall);
			m_pMicMeter2->SetSize(width, tall);
			m_pMicMeter2->Repaint();
		}
	}
}