/*
 AppListView.h : Standard ListView for this application
*/

#pragma once

#include <wx/listctrl.h>

DECLARE_EVENT_TYPE(wxEVT_REFRESH_GUI, -1)

namespace NDecGui
{
	typedef std::vector<unsigned long> TListSelection;

	class CAppListView : public wxListView
	{
		DECLARE_CLASS(CAppListView);
		DECLARE_EVENT_TABLE();

	public:
		CAppListView(wxWindow* Parent, wxWindowID ID, const wxPoint& Pos=wxDefaultPosition, \
			const wxSize& Size=wxDefaultSize);
		virtual ~CAppListView();

		void OnMouseDown(wxMouseEvent& Event);

		virtual void SetSelection(const TListSelection& Selection);
		virtual void GetSelection(TListSelection& Selection) const;
		virtual void SelectAll();
		virtual void SelectNone();
		virtual void SelectNext();
	};
};
