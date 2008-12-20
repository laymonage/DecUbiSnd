/*
 SetInfoDialog.h : Set information about each segment
*/

#pragma once

namespace NDecGui
{
	class CSetInfoDialog : public wxDialog
	{
		DECLARE_CLASS(CSetInfoDialog);
		DECLARE_EVENT_TABLE();

	protected:
		wxStaticBox* m_ChannelsSampleRateSizer_staticbox;
		wxSpinCtrl* m_ChannelsSpinctrl;
		wxComboBox* m_SampleRateCombobox;
		wxTextCtrl* m_StreamSizeTextbox;

	public:
		CSetInfoDialog(wxWindow* Parent, const wxPoint& Pos=wxDefaultPosition);
		virtual ~CSetInfoDialog();

		virtual void SetChannels(unsigned char Channels);
		virtual unsigned char GetChannels() const;
		virtual void SetSampleRate(unsigned long SampleRate);
		virtual unsigned long GetSampleRate() const;
		virtual void SetStreamSize(std::streamsize StreamSize);
		virtual std::streamsize GetStreamSize() const;
	};
};
