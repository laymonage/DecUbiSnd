/*
 AppListView.cpp : Standard ListView for this application
*/

#include "Pch.h"

#include "Gui/AppListView.h"

DEFINE_EVENT_TYPE(wxEVT_REFRESH_GUI)

// CAppListView Event Table
IMPLEMENT_CLASS(NDecGui::CAppListView, wxListView)
BEGIN_EVENT_TABLE(NDecGui::CAppListView, wxListView)
	EVT_LEFT_DOWN(OnMouseDown)
	EVT_RIGHT_DOWN(OnMouseDown)
END_EVENT_TABLE()

// CAppListView Implementation
NDecGui::CAppListView::CAppListView(wxWindow* Parent, wxWindowID ID, const wxPoint& Pos, const wxSize& Size) :
	wxListView(Parent, ID, Pos, Size, wxLC_REPORT|wxSUNKEN_BORDER|wxLC_VIRTUAL)
{
	return;
}

NDecGui::CAppListView::~CAppListView()
{
	return;
}

void NDecGui::CAppListView::OnMouseDown(wxMouseEvent& Event)
{
	wxCommandEvent NewEvent(wxEVT_REFRESH_GUI, GetId());
	GetEventHandler()->ProcessEvent(NewEvent);
	Event.Skip();
	return;
}

void NDecGui::CAppListView::SetSelection(const TListSelection& Selection)
{
	SelectNone();
	for(TListSelection::const_iterator Iter=Selection.begin();Iter!=Selection.end();++Iter)
	{
		Select(*Iter);
	}
	return;
}

void NDecGui::CAppListView::GetSelection(TListSelection& Selection) const
{
	// Search for selected items
	long Sel;
	Sel=GetFirstSelected();
	while(Sel!=-1)
	{
		Selection.push_back((unsigned long)Sel);
		Sel=GetNextSelected(Sel);
	}
	return;
}

void NDecGui::CAppListView::SelectAll()
{
	for(unsigned long i=0;i<(unsigned long)GetItemCount();i++)
	{
		Select(i, true);
	}
	return;
}

void NDecGui::CAppListView::SelectNone()
{
	for(unsigned long i=0;i<(unsigned long)GetItemCount();i++)
	{
		Select(i, false);
	}
	return;
}

void NDecGui::CAppListView::SelectNext()
{
	// Search for selected items
	long Sel;
	Sel=GetFirstSelected();
	while(Sel!=-1)
	{
		Select(Sel, false);
		if(GetNextSelected(Sel)==-1)
		{
			if(Sel+1>=GetItemCount())
			{
				Sel=0;
			}
			else
			{
				Sel++;
			}
			Select(Sel);
			break;
		}
		else
		{
			Sel=GetNextSelected(Sel);
		}
	}
	return;
}
