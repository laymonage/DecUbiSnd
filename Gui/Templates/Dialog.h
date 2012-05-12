/*
 Dialog.h : A template for a dialog
*/

#pragma once

namespace NDecGui
{
	class CDialog : public wxDialog
	{
		DECLARE_CLASS(CDialog);
		DECLARE_EVENT_TABLE();

	public:
		CDialog(wxWindow* Parent, const wxPoint& Pos=wxDefaultPosition/*, \
			const wxSize& Size=wxDefaultSize*/);
		virtual ~CDialog();
	};
};
