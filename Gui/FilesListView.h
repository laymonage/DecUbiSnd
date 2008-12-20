/*
 FilesListView.h : A ListView for the list of files
*/

#pragma once

#include "Gui/AppListView.h"

namespace NDecFunc
{
	class CFilesList;
};

namespace NDecGui
{
	class CSegmentsListView;

	class CFilesListView : public CAppListView
	{
		DECLARE_CLASS(CFilesListView);
		DECLARE_EVENT_TABLE();

	protected:
		CSegmentsListView* m_SegmentsListView;
		NDecFunc::CFilesList* m_FilesList;

	public:
		CFilesListView(wxWindow* Parent, wxWindowID ID, const wxPoint& Pos=wxDefaultPosition, \
			const wxSize& Size=wxDefaultSize);
		virtual ~CFilesListView();

		virtual wxString OnGetItemText(long Item, long Column) const;
		virtual int OnGetItemImage(long Item) const;

		virtual void RefreshGui();
		virtual void RefreshData();
		virtual void SetSegmentsListView(CSegmentsListView* ListView);
		virtual CSegmentsListView* GetSegmentsListView() const;
		virtual void SetFilesList(NDecFunc::CFilesList* List);
		virtual NDecFunc::CFilesList* GetFilesList() const;
	};
};
