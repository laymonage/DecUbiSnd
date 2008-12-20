/*
 SetInfoDialog.cpp : Set information about each segment
*/

#include "Pch.h"

#include <wx/spinctrl.h>

#include "Gui/SetInfoDialog.h"

// CSetInfoDialog Event Table
IMPLEMENT_CLASS(NDecGui::CSetInfoDialog, wxDialog)
BEGIN_EVENT_TABLE(NDecGui::CSetInfoDialog, wxDialog)
END_EVENT_TABLE()

// CSetInfoDialog Implementation
NDecGui::CSetInfoDialog::CSetInfoDialog(wxWindow* Parent, const wxPoint& Pos) :
	wxDialog(Parent, wxID_ANY, _("Set Information"), Pos, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	// Create the controls
	m_ChannelsSampleRateSizer_staticbox = new wxStaticBox(this, -1, wxT("Information"));
	m_ChannelsSpinctrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 32);
	const wxString SampleRateCombobox_choices[] = {
		wxT("4000"),
		wxT("6000"),
		wxT("8000"),
		wxT("10000"),
		wxT("12000"),
		wxT("14000"),
		wxT("16000"),
		wxT("18000"),
		wxT("20000"),
		wxT("22000"),
		wxT("22050"),
		wxT("24000"),
		wxT("26000"),
		wxT("28000"),
		wxT("30000"),
		wxT("32000"),
		wxT("33000"),
		wxT("34000"),
		wxT("36000"),
		wxT("44100"),
		wxT("48000")
	};
	m_SampleRateCombobox = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 21, SampleRateCombobox_choices, wxCB_DROPDOWN);
	m_StreamSizeTextbox = new wxTextCtrl(this, wxID_ANY, wxEmptyString);

	// Do the layout
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxSizer* ButtonsSizer = CreateButtonSizer(wxOK | wxCANCEL);
	wxStaticBoxSizer* ChannelsSampleRateSizer = new wxStaticBoxSizer(m_ChannelsSampleRateSizer_staticbox, wxVERTICAL);
	wxBoxSizer* StreamSizeSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* SampleRateSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* ChannelsSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* ChannelsLabel = new wxStaticText(this, wxID_ANY, wxT("Number of channels:"));
	ChannelsSizer->Add(ChannelsLabel, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
	ChannelsSizer->Add(m_ChannelsSpinctrl, 1, wxEXPAND, 0);
	ChannelsSampleRateSizer->Add(ChannelsSizer, 0, wxBOTTOM|wxEXPAND, 5);
	wxStaticText* SampleRateLabel = new wxStaticText(this, wxID_ANY, wxT("Sample rate:"));
	SampleRateSizer->Add(SampleRateLabel, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
	SampleRateSizer->Add(m_SampleRateCombobox, 1, wxEXPAND, 0);
	ChannelsSampleRateSizer->Add(SampleRateSizer, 0, wxBOTTOM|wxEXPAND, 5);
	wxStaticText* StreamSizeLabel = new wxStaticText(this, wxID_ANY, wxT("Stream size:"));
	StreamSizeSizer->Add(StreamSizeLabel, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
	StreamSizeSizer->Add(m_StreamSizeTextbox, 1, wxEXPAND, 0);
	ChannelsSampleRateSizer->Add(StreamSizeSizer, 1, wxEXPAND, 0);
	MainSizer->Add(ChannelsSampleRateSizer, 0, wxALL|wxEXPAND, 5);
	MainSizer->Add(ButtonsSizer, 1, wxBOTTOM|wxRIGHT|wxEXPAND, 5);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	Layout();
	Center();
	return;
}

NDecGui::CSetInfoDialog::~CSetInfoDialog()
{
	return;
}

void NDecGui::CSetInfoDialog::SetChannels(unsigned char Channels)
{
	m_ChannelsSpinctrl->SetValue(Channels);
	return;
}

unsigned char NDecGui::CSetInfoDialog::GetChannels() const
{
	return (unsigned char)m_ChannelsSpinctrl->GetValue();
}

void NDecGui::CSetInfoDialog::SetSampleRate(unsigned long SampleRate)
{
	if(SampleRate)
	{
		m_SampleRateCombobox->SetValue(wxString::Format(wxT("%lu"), SampleRate));
	}
	else
	{
		m_SampleRateCombobox->SetValue(wxEmptyString);
	}
	return;
}

unsigned long NDecGui::CSetInfoDialog::GetSampleRate() const
{
	unsigned long SampleRate=0;
	m_SampleRateCombobox->GetValue().ToULong(&SampleRate);
	return SampleRate;
}

void NDecGui::CSetInfoDialog::SetStreamSize(std::streamsize StreamSize)
{
	if(StreamSize)
	{
		m_StreamSizeTextbox->SetValue(wxString::Format(wxT("%lu"), StreamSize));
	}
	else
	{
		m_StreamSizeTextbox->SetValue(wxEmptyString);
	}
	return;
}

std::streamsize NDecGui::CSetInfoDialog::GetStreamSize() const
{
	std::streamsize StreamSize=0;
	m_StreamSizeTextbox->GetValue().ToULong((unsigned long*)&StreamSize);
	return StreamSize;
}
