/*
 SegmentsListView.cpp : A ListView for the list of segments
*/

#include "Pch.h"

#include "Gui/SegmentsListView.h"
#include "Functionality/SegmentsList.h"
#include "Functionality/Segment.h"
#include "Decoding/UbiFormats.h"

// CSegmentsListView Event Table
IMPLEMENT_CLASS(NDecGui::CSegmentsListView, NDecGui::CAppListView)
BEGIN_EVENT_TABLE(NDecGui::CSegmentsListView, NDecGui::CAppListView)
END_EVENT_TABLE()

// CSegmentsListView Implementation
NDecGui::CSegmentsListView::CSegmentsListView(wxWindow* Parent, wxWindowID ID, \
										const wxPoint& Pos, const wxSize& Size) :
	CAppListView(Parent, ID, Pos, Size),
	m_SegmentsList(NULL),
	m_PlayingSegmentsList(NULL),
	m_CurrentlyPlaying(-1)
{
	// Insert the columns
	InsertColumn(0, _("Type"), wxLIST_FORMAT_LEFT, 100);
	InsertColumn(1, _("Layers"), wxLIST_FORMAT_LEFT, 50);
	InsertColumn(2, _("Channels"), wxLIST_FORMAT_LEFT, 40);
	InsertColumn(3, _("Sample Rate"), wxLIST_FORMAT_LEFT, 60);
	InsertColumn(4, _("Offset"), wxLIST_FORMAT_LEFT, 75);
	InsertColumn(5, _("Size"), wxLIST_FORMAT_LEFT, 75);

	// Create the playing attributes
	m_InRangeAttr.SetBackgroundColour(*wxCYAN);
	m_PlayingAttr.SetBackgroundColour(*wxCYAN);
	wxFont BoldFont(m_PlayingAttr.GetFont());
	BoldFont.SetWeight(wxFONTWEIGHT_BOLD);
	m_PlayingAttr.SetFont(BoldFont);
	return;
}

NDecGui::CSegmentsListView::~CSegmentsListView()
{
	return;
}

static wxString UbiFormatToString(EUbiFormat Format)
{
	switch(Format)
	{
		case EUF_NULL:
		return wxEmptyString;
		case EUF_UBI_V3:
		return wxT("Old Simple Stream");
		case EUF_UBI_V5:
		return wxT("Simple Stream");
		case EUF_UBI_V6:
		return wxT("Simple Stream 6");
		case EUF_UBI_IV2:
		return wxT("Old Interleaved Stream");
		case EUF_UBI_IV8:
		return wxT("Interleaved Stream");
		case EUF_UBI_IV9:
		return wxT("Interleaved 9 Stream");
		case EUF_UBI_6OR4:
		return wxT("Old 6-Or-4 Bit Stream");
		case EUF_UBI_RAW:
		return wxT("Raw UbiSoft ADPCM");
		case EUF_RAW:
		return wxT("Raw 16-Bit Uncompressed");
		case EUF_OGG:
		return wxT("Ogg Vorbis");
	}
	return wxEmptyString;
}

wxString NDecGui::CSegmentsListView::OnGetItemText(long Item, long Column) const
{
	// Check some things
	if(!m_SegmentsList)
	{
		return wxEmptyString;
	}
	if(!m_SegmentsList->IsValid(Item))
	{
		return wxEmptyString;
	}

	// Get the item
	const NDecFunc::CSegment& Segment=*m_SegmentsList->Get(Item);

	// Go through the columns
	wxString Text;
	bool First;
	bool sampleRateUncertain;
	switch(Column)
	{
		case 0:
			Text=UbiFormatToString(Segment.GetType());

			switch (Segment.GetType())
			{
			case EUF_UBI_IV2:
			case EUF_UBI_IV8:
			case EUF_UBI_IV9:
				if (Segment.GetLayerTypes().size() > 0)
				{
					Text += wxT(" (");
					Text += UbiFormatToString(Segment.GetLayerTypes().at(0));
					Text += wxT(")");
				}
				break;
			}
		break;
		case 1:
			First=true;
			Text=wxEmptyString;
			for(std::vector<unsigned long>::const_iterator Iter=Segment.GetLayers().begin();Iter!=Segment.GetLayers().end();++Iter)
			{
				if(First)
				{
					First=false;
				}
				else
				{
					Text.Append(wxT(", "));
				}
				Text.Append(wxString::Format(wxT("%lu"), (*Iter)+1));
			}
		break;
		case 2:
			Text=wxString::Format(wxT("%lu"), (unsigned long)Segment.GetChannels());
		break;
		case 3:
			// Depending on the format, we might want to indicate it has been guessed
			sampleRateUncertain = false;
			switch (Segment.GetType())
			{
			case EUF_UBI_V3:
			case EUF_UBI_V5:
			case EUF_UBI_V6:
				sampleRateUncertain = true;
				break;

			case EUF_UBI_IV2:
			case EUF_UBI_IV8:
			case EUF_UBI_IV9:
				if (Segment.GetLayerTypes().size() > 0)
				{
					switch (Segment.GetLayerTypes().at(0))
					{
					case EUF_UBI_V3:
					case EUF_UBI_V5:
					case EUF_UBI_V6:
						sampleRateUncertain = true;
						break;

					default:
						sampleRateUncertain = false;
						break;
					}
				}
				break;

			default:
				sampleRateUncertain = false;
				break;
			}

			if (sampleRateUncertain)
				Text = wxString::Format(wxT("[%lu]"), Segment.GetSampleRate());
			else
				Text = wxString::Format(wxT("%lu"), Segment.GetSampleRate());
		break;
		case 4:
			Text=wxString::Format(wxT("%lu"), Segment.GetOffset());
		break;
		case 5:
			Text=wxString::Format(wxT("%lu"), Segment.GetSize());
		break;
	}
	return Text;
}

wxListItemAttr* NDecGui::CSegmentsListView::OnGetItemAttr(long Item) const
{
	if(m_SegmentsList==m_PlayingSegmentsList)
	{
		// See if this item is in the playing range
		bool IsInRange=false;
		for(TListSelection::const_iterator Iter=m_PlayingRange.begin();Iter!=m_PlayingRange.end();++Iter)
		{
			if((*Iter)==Item)
			{
				IsInRange=true;
				break;
			}
		}

		// Branch out
		if(IsInRange && Item==m_CurrentlyPlaying)
		{
			return (wxListItemAttr*)&m_PlayingAttr;
		}
		else if(IsInRange)
		{
			return (wxListItemAttr*)&m_InRangeAttr;
		}
	}
	return (wxListItemAttr*)&m_NormalAttr;
}

void NDecGui::CSegmentsListView::RefreshGui()
{
	// Make sure we have some items
	if(!m_SegmentsList)
	{
		SetItemCount(0);
		Refresh();
		return;
	}

	// Set the item count and redraw if needed
	if(GetItemCount()!=m_SegmentsList->GetCount())
	{
		RefreshData();
	}
	return;
}

void NDecGui::CSegmentsListView::RefreshData()
{
	// Make sure we have some items
	if(!m_SegmentsList)
	{
		SetItemCount(0);
		Refresh();
		return;
	}

	// Set the item count and redraw
	if(GetItemCount()!=m_SegmentsList->GetCount())
	{
		SetItemCount(m_SegmentsList->GetCount());
	}
	Refresh();
	return;
}

void NDecGui::CSegmentsListView::SetSegmentsList(NDecFunc::CSegmentsList* List)
{
	if(m_SegmentsList!=List)
	{
		m_SegmentsList=List;
		RefreshData();
	}
	return;
}

NDecFunc::CSegmentsList* NDecGui::CSegmentsListView::GetSegmentsList() const
{
	return m_SegmentsList;
}

void NDecGui::CSegmentsListView::Play(const TListSelection& Range)
{
	// Update the old range
	Stop();

	// Check the segments list
	if(!m_SegmentsList)
	{
		return;
	}

	// Set the new range
	m_PlayingRange=Range;
	m_PlayingSegmentsList=m_SegmentsList;

	// Update the new range
	for(TListSelection::const_iterator Iter=m_PlayingRange.begin();Iter!=m_PlayingRange.end();++Iter)
	{
		RefreshItem(*Iter);
	}
	return;
}

void NDecGui::CSegmentsListView::Play()
{
	// Update the old range
	Stop();

	// Check the segments list
	if(!m_SegmentsList)
	{
		return;
	}

	// Set the new range
	m_PlayingSegmentsList=m_SegmentsList;
	return;
}

void NDecGui::CSegmentsListView::SetCurrentlyPlaying(unsigned long Item)
{
	// Check the segments list
	if(!m_PlayingSegmentsList)
	{
		return;
	}

	// Save the old one
	unsigned long OldPlaying=m_CurrentlyPlaying;

	// Make sure it's in bounds
	if(Item<=m_PlayingSegmentsList->GetCount())
	{
		m_CurrentlyPlaying=Item;
	}
	else
	{
		m_CurrentlyPlaying=-1;
	}

	// Only if it has the one that's currently playing
	if(m_SegmentsList==m_PlayingSegmentsList)
	{
		// Refresh
		if(OldPlaying<=(unsigned long)GetItemCount())
		{
			RefreshItem(OldPlaying);
		}
		if(m_CurrentlyPlaying<=(unsigned long)GetItemCount())
		{
			RefreshItem(m_CurrentlyPlaying);
		}
	}
	return;
}

void NDecGui::CSegmentsListView::Stop()
{
	// Remove anything that's currently playing
	m_CurrentlyPlaying=-1;

	// Save the old one and wipe out new one
	TListSelection OldRange(m_PlayingRange);
	m_PlayingRange.clear();

	// Refresh
	if(m_SegmentsList==m_PlayingSegmentsList)
	{
		for(TListSelection::const_iterator Iter=OldRange.begin();Iter!=OldRange.end();++Iter)
		{
			RefreshItem(*Iter);
		}
	}

	// Reset the currently playing list
	m_PlayingSegmentsList=NULL;
	return;
}
