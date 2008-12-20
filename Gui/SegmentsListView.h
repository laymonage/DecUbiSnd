/*
 SegmentsListView.h : A ListView for the list of segments
*/

#pragma once

#include "Gui/AppListView.h"

namespace NDecFunc
{
	class CSegmentsList;
};

namespace NDecGui
{
	class CSegmentsListView : public CAppListView
	{
		DECLARE_CLASS(CSegmentsListView);
		DECLARE_EVENT_TABLE();

	protected:
		NDecFunc::CSegmentsList* m_SegmentsList;
		NDecFunc::CSegmentsList* m_PlayingSegmentsList;
		TListSelection m_PlayingRange;
		unsigned long m_CurrentlyPlaying;
		wxListItemAttr m_NormalAttr;
		wxListItemAttr m_InRangeAttr;
		wxListItemAttr m_PlayingAttr;

	public:
		CSegmentsListView(wxWindow* Parent, wxWindowID ID, const wxPoint& Pos=wxDefaultPosition, \
			const wxSize& Size=wxDefaultSize);
		virtual ~CSegmentsListView();

		virtual wxString OnGetItemText(long Item, long Column) const;
		virtual wxListItemAttr* NDecGui::CSegmentsListView::OnGetItemAttr(long Item) const;

		virtual void RefreshGui();
		virtual void RefreshData();
		virtual void SetSegmentsList(NDecFunc::CSegmentsList* List);
		virtual NDecFunc::CSegmentsList* GetSegmentsList() const;
		virtual void Play(const TListSelection& Range);
		virtual void Play();
		virtual void SetCurrentlyPlaying(unsigned long Item=-1);
		virtual void Stop();
	};
};
