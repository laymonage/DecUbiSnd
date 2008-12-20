/*
 FilesListView.cpp : A ListView for the list of files
*/

#include "Pch.h"

#include <wx/filename.h>

#include "Gui/FilesListView.h"
#include "Gui/SegmentsListView.h"
#include "Functionality/FilesList.h"
#include "Functionality/SegmentsList.h"

// Test image list
class CAttachImageList : public wxImageList
{
public:
	CAttachImageList(HIMAGELIST hImageList)
	{
		m_hImageList=hImageList;
	}

	virtual ~CAttachImageList()
	{
		m_hImageList=NULL;
	}
};

// CFilesListView Event Table
IMPLEMENT_CLASS(NDecGui::CFilesListView, NDecGui::CAppListView)
BEGIN_EVENT_TABLE(NDecGui::CFilesListView, NDecGui::CAppListView)
END_EVENT_TABLE()

// CFilesListView Implementation
NDecGui::CFilesListView::CFilesListView(wxWindow* Parent, wxWindowID ID, \
										const wxPoint& Pos, const wxSize& Size) :
	CAppListView(Parent, ID, Pos, Size),
	m_SegmentsListView(NULL),
	m_FilesList(NULL)
{
	InsertColumn(0, _("Filename"), wxLIST_FORMAT_LEFT, 125);
	InsertColumn(1, _("Size"), wxLIST_FORMAT_RIGHT, 100);
	InsertColumn(2, _("Number"), wxLIST_FORMAT_RIGHT, 50);
	return;
}

NDecGui::CFilesListView::~CFilesListView()
{
	return;
}

static wxString FileSizeToString(std::streamsize FileSize)
{
	if(FileSize==-1)
	{
		return wxT("Invalid");
	}
	else if(FileSize==1)
	{
		return wxT("1 Byte");
	}
	else if(FileSize<1000)
	{
		return wxString::Format(wxT("%lu Bytes"), FileSize);
	}
	else if(((float)FileSize/1024)<1000)
	{
		return wxString::Format(wxT("%.2f KB"), ((float)FileSize/1024));
	}
	else if(((float)FileSize/(1024*1024))<1000)
	{
		return wxString::Format(wxT("%.2f MB"), ((float)FileSize/(1024*1024)));
	}
	else
	{
		return wxString::Format(wxT("%.2f GB"), ((float)FileSize/(1024*1024*1024)));
	}
	return wxEmptyString;
}

wxString NDecGui::CFilesListView::OnGetItemText(long Item, long Column) const
{
	// Check some things
	if(!m_FilesList)
	{
		return wxEmptyString;
	}
	if(!m_FilesList->IsValid(Item))
	{
		return wxEmptyString;
	}

	// Get the item
	NDecFunc::CSegmentsList& SegmentsList=*m_FilesList->Get(Item);
	
	// Go through the columns
	wxString Text;
	wxFileName Filename(SegmentsList.GetFilename());
	switch(Column)
	{
		case 0:
			Text=Filename.GetFullName();
		break;
		case 1:
			Text=FileSizeToString((std::streamoff)wxFileName::GetSize(SegmentsList.GetFilename()).GetValue());
		break;
		case 2:
			Text=wxString::Format(wxT("%lu"), SegmentsList.GetCount());
		break;
	}
	return Text;
}

int NDecGui::CFilesListView::OnGetItemImage(long Item) const
{
	// Check some things
	if(!m_FilesList)
	{
		return -1;
	}
	if(!m_FilesList->IsValid(Item))
	{
		return -1;
	}

	// Get the item
	NDecFunc::CSegmentsList& SegmentsList=*m_FilesList->Get(Item);

	// Get the icon
	SHFILEINFO FileInfo;
	HIMAGELIST hImageList;
	hImageList=(HIMAGELIST)SHGetFileInfo(SegmentsList.GetFilename().c_str(), 0, &FileInfo, \
		sizeof(SHFILEINFO), SHGFI_SYSICONINDEX);

	// Create a new image list
	if(!GetImageList(wxIMAGE_LIST_SMALL))
	{
		wxImageList* SystemImageList=new CAttachImageList(hImageList);
		((CFilesListView*)this)->AssignImageList(SystemImageList, wxIMAGE_LIST_SMALL);
	}
	return FileInfo.iIcon;
}

void NDecGui::CFilesListView::RefreshGui()
{
	// Make sure we have some items
	if(!m_FilesList)
	{
		SetItemCount(0);
		Refresh();
		return;
	}

	// Set the item count and redraw if needed
	if(GetItemCount()!=m_FilesList->GetCount())
	{
		RefreshData();
	}

	// Update the segment list with the selection
	if(m_SegmentsListView)
	{
		TListSelection Sel;
		NDecFunc::CSegmentsList* Old=m_SegmentsListView->GetSegmentsList();
		GetSelection(Sel);
		if(Sel.size()==1)
		{
			unsigned long CurrentSel;
			CurrentSel=Sel[0];
			m_SegmentsListView->SetSegmentsList(m_FilesList->Get(CurrentSel));
		}
		else
		{
			m_SegmentsListView->SetSegmentsList(NULL);
		}
	}
	return;
}

void NDecGui::CFilesListView::RefreshData()
{
	// Make sure we have some items
	if(!m_FilesList)
	{
		SetItemCount(0);
		Refresh();
		return;
	}

	// Set the item count and redraw
	if(GetItemCount()!=m_FilesList->GetCount())
	{
		SetItemCount(m_FilesList->GetCount());
	}
	Refresh();
	return;
}

void NDecGui::CFilesListView::SetSegmentsListView(CSegmentsListView* ListView)
{
	m_SegmentsListView=ListView;
	RefreshGui();
	return;
}

NDecGui::CSegmentsListView* NDecGui::CFilesListView::GetSegmentsListView() const
{
	return m_SegmentsListView;
}

void NDecGui::CFilesListView::SetFilesList(NDecFunc::CFilesList* List)
{
	m_FilesList=List;
	RefreshGui();
	return;
}

NDecFunc::CFilesList* NDecGui::CFilesListView::GetFilesList() const
{
	return m_FilesList;
}
