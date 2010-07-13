/*
 MainDialog.cpp : The main dialog
*/

#include "Pch.h"

#include <wx/filename.h>
#include <wx/mmedia/sndwin.h>

#include "Gui/App.h"
#include "Gui/MainDialog.h"
#include "Gui/FilesListView.h"
#include "Gui/SegmentsListView.h"
#include "Gui/AddManuallyDialog.h"
#include "Gui/SetInfoDialog.h"
#include "Gui/SegmentStreamSound.h"
#include "Functionality/FilesList.h"
#include "Functionality/SegmentsList.h"
#include "Functionality/Segment.h"
#include "Functionality/SegmentStream.h"
#include "Functionality/Output.h"
#include "Sound/Player.h"
#include "Sound/Stream.h"
#include "Version.h"

// CMainDialog Event Table
IMPLEMENT_CLASS(NDecGui::CMainDialog, wxDialog)
BEGIN_EVENT_TABLE(NDecGui::CMainDialog, wxDialog)
	EVT_CLOSE(OnClose)
	EVT_TIMER(wxID_ANY, OnTimer)
	EVT_COMMAND(wxID_ANY, wxEVT_REFRESH_GUI, OnRefreshGui)

	EVT_BUTTON(ID_ScanDirectoryButton, NDecGui::CMainDialog::OnScanDirectoryButtonClicked)
	EVT_BUTTON(ID_ScanFileButton, NDecGui::CMainDialog::OnScanFileButtonClicked)
	EVT_BUTTON(ID_AddManuallyButton, NDecGui::CMainDialog::OnAddManuallyButtonClicked)
	EVT_BUTTON(ID_ClearButton, NDecGui::CMainDialog::OnClearButtonClicked)
	EVT_BUTTON(ID_NextButton, NDecGui::CMainDialog::OnNextButtonClicked)
	EVT_BUTTON(ID_SelectAllButton, NDecGui::CMainDialog::OnSelectAllButtonClicked)
	EVT_BUTTON(ID_SelectNoneButton, NDecGui::CMainDialog::OnSelectNoneButtonClicked)
	EVT_BUTTON(ID_SelectAllFilesButton, NDecGui::CMainDialog::OnSelectAllFilesButtonClicked)
	EVT_BUTTON(ID_DuplicateButton, NDecGui::CMainDialog::OnDuplicateButtonClicked)
	EVT_BUTTON(ID_RemoveButton, NDecGui::CMainDialog::OnRemoveButtonClicked)
	EVT_BUTTON(ID_SetInfoButton, NDecGui::CMainDialog::OnSetInfoButtonClicked)
	EVT_BUTTON(ID_MixLayersButton, NDecGui::CMainDialog::OnMixLayersButtonClicked)
	EVT_BUTTON(ID_UnmixLayersButton, NDecGui::CMainDialog::OnUnmixLayersButtonClicked)
	EVT_BUTTON(ID_PlayButton, NDecGui::CMainDialog::OnPlayButtonClicked)
	EVT_BUTTON(ID_LoopButton, NDecGui::CMainDialog::OnLoopButtonClicked)
	EVT_BUTTON(ID_StopButton, NDecGui::CMainDialog::OnStopButtonClicked)
	EVT_BUTTON(ID_ConcatenatedButton, NDecGui::CMainDialog::OnConcatenatedButtonClicked)
	EVT_BUTTON(ID_SeparateButton, NDecGui::CMainDialog::OnSeparateButtonClicked)
	EVT_BUTTON(ID_LayerExtractButton, OnLayerExtractButtonClicked)

	EVT_LIST_ITEM_SELECTED(ID_FileList, NDecGui::CMainDialog::OnFileListSelChange)
	EVT_LIST_ITEM_DESELECTED(ID_FileList, NDecGui::CMainDialog::OnFileListSelChange)
	EVT_LIST_ITEM_FOCUSED(ID_FileList, NDecGui::CMainDialog::OnFileListSelChange)
	EVT_LIST_ITEM_SELECTED(ID_SegmentList, NDecGui::CMainDialog::OnSegmentListSelChange)
	EVT_LIST_ITEM_DESELECTED(ID_SegmentList, NDecGui::CMainDialog::OnSegmentListSelChange)
	EVT_LIST_ITEM_FOCUSED(ID_SegmentList, NDecGui::CMainDialog::OnSegmentListSelChange)
END_EVENT_TABLE()

// CMainDialog Implementation
NDecGui::CMainDialog::CMainDialog(wxWindow* Parent, const wxPoint& Pos, const wxSize& Size) :
	wxDialog(Parent, wxID_ANY, _("Decode UbiSoft Sounds/Music version " DECUBISND_VERSION), Pos, Size, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxTHICK_FRAME),
	m_FilesList(NULL),
	m_Update(0),
	m_NoMagic(false),
	m_Playback(NULL),
	m_Stream(NULL),
	m_Sound(NULL),
	m_SoundUpdate(this)
{
	// Create the controls
	m_SelectionSizer_staticbox = new wxStaticBox(this, -1, wxT("Selection"));
	m_EditSizer_staticbox = new wxStaticBox(this, -1, wxT("Edit"));
	m_LayersSizer_staticbox = new wxStaticBox(this, -1, wxT("Layers"));
	m_PlaySizer_staticbox = new wxStaticBox(this, -1, wxT("Play"));
	m_OutputSizer_staticbox = new wxStaticBox(this, -1, wxT("Output"));
	m_InputSizer_staticbox = new wxStaticBox(this, -1, wxT("Input"));
	m_ScanDirectoryButton = new wxButton(this, ID_ScanDirectoryButton, wxT("Scan &Directory..."));
	m_ScanFileButton = new wxButton(this, ID_ScanFileButton, wxT("Scan &File..."));
	m_AddManuallyButton = new wxButton(this, ID_AddManuallyButton, wxT("Add &Manually..."));
	m_ClearButton = new wxButton(this, ID_ClearButton, wxT("&Clear"));
	m_NextButton = new wxButton(this, ID_NextButton, wxT("Nex&t"));
	m_SelectAllButton = new wxButton(this, ID_SelectAllButton, wxT("Select &All"));
	m_SelectNoneButton = new wxButton(this, ID_SelectNoneButton, wxT("Select &None"));
	m_SelectAllFilesButton = new wxButton(this, ID_SelectAllFilesButton, wxT("Select All F&iles"));
	m_DuplicateButton = new wxButton(this, ID_DuplicateButton, wxT("Dup&licate"));
	m_RemoveButton = new wxButton(this, ID_RemoveButton, wxT("&Remove"));
	m_SetInfoButton = new wxButton(this, ID_SetInfoButton, wxT("Set I&nfo..."));
	m_MixLayersButton = new wxButton(this, ID_MixLayersButton, wxT("Mi&x Layers"));
	m_UnmixLayersButton = new wxButton(this, ID_UnmixLayersButton, wxT("&Unmix Layers"));
	m_PlayButton = new wxButton(this, ID_PlayButton, wxT("&Play"));
	m_LoopButton = new wxButton(this, ID_LoopButton, wxT("L&oop"));
	m_StopButton = new wxButton(this, ID_StopButton, wxT("&Stop"));
	m_PlayLabel = new wxStaticText(this, ID_PlayLabel, wxT("m:ss"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
	m_ConcatenatedButton = new wxButton(this, ID_ConcatenatedButton, wxT("&Concatenated..."));
	m_SeparateButton = new wxButton(this, ID_SeparateButton, wxT("S&eparate..."));
	m_LayerExtractButton = new wxButton(this, ID_LayerExtractButton, wxT("Layer Ex&tract..."));
	m_FileList = new CFilesListView(this, ID_FileList, wxDefaultPosition, wxDefaultSize);
	m_SegmentList = new CSegmentsListView(this, ID_SegmentList, wxDefaultPosition, wxDefaultSize);

	// Do the layout
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* SecondarySizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* ButtonsSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticBoxSizer* OutputSizer = new wxStaticBoxSizer(m_OutputSizer_staticbox, wxVERTICAL);
	wxStaticBoxSizer* PlaySizer = new wxStaticBoxSizer(m_PlaySizer_staticbox, wxVERTICAL);
	wxStaticBoxSizer* LayersSizer = new wxStaticBoxSizer(m_LayersSizer_staticbox, wxVERTICAL);
	wxStaticBoxSizer* EditSizer = new wxStaticBoxSizer(m_EditSizer_staticbox, wxVERTICAL);
	wxStaticBoxSizer* SelectionSizer = new wxStaticBoxSizer(m_SelectionSizer_staticbox, wxVERTICAL);
	wxStaticBoxSizer* InputSizer = new wxStaticBoxSizer(m_InputSizer_staticbox, wxVERTICAL);
	InputSizer->Add(m_ScanDirectoryButton, 0, wxBOTTOM|wxEXPAND, 5);
	InputSizer->Add(m_ScanFileButton, 0, wxBOTTOM|wxEXPAND, 5);
	InputSizer->Add(m_AddManuallyButton, 0, wxBOTTOM|wxEXPAND, 5);
	InputSizer->Add(m_ClearButton, 0, wxEXPAND, 5);
	ButtonsSizer->Add(InputSizer, 0, wxRIGHT, 5);
	SelectionSizer->Add(m_NextButton, 0, wxBOTTOM|wxEXPAND, 5);
	SelectionSizer->Add(m_SelectAllButton, 0, wxBOTTOM|wxEXPAND, 5);
	SelectionSizer->Add(m_SelectNoneButton, 0, wxBOTTOM|wxEXPAND, 5);
	SelectionSizer->Add(m_SelectAllFilesButton, 0, wxEXPAND, 5);
	ButtonsSizer->Add(SelectionSizer, 0, wxRIGHT, 5);
	EditSizer->Add(m_DuplicateButton, 0, wxBOTTOM|wxEXPAND, 5);
	EditSizer->Add(m_RemoveButton, 0, wxBOTTOM|wxEXPAND, 5);
	EditSizer->Add(m_SetInfoButton, 0, wxEXPAND, 5);
	ButtonsSizer->Add(EditSizer, 0, wxRIGHT, 5);
	LayersSizer->Add(m_MixLayersButton, 0, wxBOTTOM|wxEXPAND, 5);
	LayersSizer->Add(m_UnmixLayersButton, 0, wxEXPAND, 5);
	ButtonsSizer->Add(LayersSizer, 0, wxRIGHT, 5);
	PlaySizer->Add(m_PlayButton, 0, wxBOTTOM|wxEXPAND, 5);
	PlaySizer->Add(m_LoopButton, 0, wxBOTTOM|wxEXPAND, 5);
	PlaySizer->Add(m_StopButton, 0, wxBOTTOM|wxEXPAND, 5);
	PlaySizer->Add(m_PlayLabel, 0, wxEXPAND, 5);
	ButtonsSizer->Add(PlaySizer, 0, wxRIGHT, 5);
	OutputSizer->Add(m_ConcatenatedButton, 0, wxBOTTOM|wxEXPAND, 5);
	OutputSizer->Add(m_SeparateButton, 0, wxBOTTOM|wxEXPAND, 5);
	OutputSizer->Add(m_LayerExtractButton, 0, wxEXPAND, 5);
	ButtonsSizer->Add(OutputSizer, 0, 0, 0);
	MainSizer->Add(ButtonsSizer, 0, wxALL, 7);
	SecondarySizer->Add(m_FileList, 1, wxRIGHT|wxEXPAND, 5);
	SecondarySizer->Add(m_SegmentList, 1, wxEXPAND, 5);
	MainSizer->Add(SecondarySizer, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 7);
	SetSizer(MainSizer);

	// Allocate the files list
	m_FilesList=new NDecFunc::CFilesList();
	m_FileList->SetFilesList(m_FilesList);
	m_FileList->SetSegmentsListView(m_SegmentList);

	// Update
	RefreshGui();

	// Layout the controls
	MainSizer->SetMinSize(1, 480);
	MainSizer->Fit(this);
	Layout();
	Center();
	return;
}

NDecGui::CMainDialog::~CMainDialog()
{
	// Clean up
	delete m_Sound;
	m_Sound=NULL;
	delete m_Stream;
	m_Stream=NULL;
	delete m_Playback;
	m_Playback=NULL;
	delete m_FilesList;
	m_FilesList=NULL;
	return;
}

void NDecGui::CMainDialog::RefreshGui()
{
	bool EnableState;

	// See if we're frozen
	if(m_Update)
	{
		return;
	}

	// Check for this event
	if(!m_NoMagic && m_FileList->GetSelectedItemCount()==0 && m_SegmentList->GetSegmentsList())
	{
		// Some "magic" - figure out which file the segment list belongs to
		for(unsigned long i=0;i<m_FilesList->GetCount();i++)
		{
			if(m_SegmentList->GetSegmentsList()==m_FilesList->Get(i))
			{
				m_FileList->SelectNone();
				m_FileList->Select(i);
				break;
			}
		}
	}

	// Refresh the file list (will refresh the segment list)
	m_FileList->RefreshGui();

	// Check clear button
	EnableState=m_FilesList->GetCount()>0;
	if(m_ClearButton->IsEnabled()!=EnableState)
	{
		m_ClearButton->Enable(EnableState);
	}

	// Check file selection button
	if(m_FilesList->GetCount())
	{
		EnableState=true;
	}
	else
	{
		EnableState=false;
	}
	if(m_SelectAllFilesButton->IsEnabled()!=EnableState)
	{
		m_SelectAllFilesButton->Enable(EnableState);
	}

	// Check the segment selection buttons
	if(m_SegmentList->GetSegmentsList())
	{
		if(m_SegmentList->GetSegmentsList()->GetCount())
		{
			EnableState=true;
		}
		else
		{
			EnableState=false;
		}
	}
	else
	{
		EnableState=false;
	}
	if(m_SelectAllButton->IsEnabled()!=EnableState)
	{
		m_SelectAllButton->Enable(EnableState);
	}
	if(m_SelectNoneButton->IsEnabled()!=EnableState)
	{
		m_SelectNoneButton->Enable(EnableState);
	}

	// Next, Duplicate, Remove, and Set Info buttons
	if(m_SegmentList->GetSelectedItemCount())
	{
		EnableState=true;
	}
	else
	{
		EnableState=false;
	}
	if(m_NextButton->IsEnabled()!=EnableState)
	{
		m_NextButton->Enable(EnableState);
	}
	if(m_DuplicateButton->IsEnabled()!=EnableState)
	{
		m_DuplicateButton->Enable(EnableState);
	}
	if(m_RemoveButton->IsEnabled()!=EnableState)
	{
		m_RemoveButton->Enable(EnableState);
	}
	if(m_SetInfoButton->IsEnabled()!=EnableState)
	{
		m_SetInfoButton->Enable(EnableState);
	}

	// Mix button
	if(m_SegmentList->GetSegmentsList())
	{
		std::vector<unsigned long> Indicies;
		m_SegmentList->GetSelection(Indicies);
		EnableState=m_SegmentList->GetSegmentsList()->CanMix(Indicies);
	}
	else
	{
		EnableState=false;
	}
	if(m_MixLayersButton->IsEnabled()!=EnableState)
	{
		m_MixLayersButton->Enable(EnableState);
	}

	// Unmix button
	if(m_SegmentList->GetSegmentsList())
	{
		std::vector<unsigned long> Indicies;
		m_SegmentList->GetSelection(Indicies);
		EnableState=m_SegmentList->GetSegmentsList()->CanUnmix(Indicies);
	}
	else
	{
		EnableState=false;
	}
	if(m_UnmixLayersButton->IsEnabled()!=EnableState)
	{
		m_UnmixLayersButton->Enable(EnableState);
	}

	// Play and Loop buttons
	if(m_SegmentList->GetSelectedItemCount())
	{
		EnableState=true;
	}
	else
	{
		EnableState=false;
	}
	if(m_PlayButton->IsEnabled()!=EnableState)
	{
		m_PlayButton->Enable(EnableState);
	}
	if(m_LoopButton->IsEnabled()!=EnableState)
	{
		m_LoopButton->Enable(EnableState);
	}

	// Stop button
	EnableState=IsPlaying();
	if(m_StopButton->IsEnabled()!=EnableState)
	{
		m_StopButton->Enable(EnableState);
	}

	// Output buttons
	if(m_SegmentList->GetSelectedItemCount() || m_FileList->GetSelectedItemCount())
	{
		EnableState=true;
	}
	else
	{
		EnableState=false;
	}
	if(m_ConcatenatedButton->IsEnabled()!=EnableState)
	{
		m_ConcatenatedButton->Enable(EnableState);
	}
	if(m_SeparateButton->IsEnabled()!=EnableState)
	{
		m_SeparateButton->Enable(EnableState);
	}
	if(m_LayerExtractButton->IsEnabled()!=EnableState)
	{
		m_LayerExtractButton->Enable(EnableState);
	}

	// Check playing time
	wxString PlayingTime(_("Stopped"));
	if(IsPlaying())
	{
		unsigned long Minutes=m_SoundTimer.Time()/(1000*60);
		unsigned long Seconds=(m_SoundTimer.Time()-Minutes*1000*60)/1000;
		PlayingTime=wxString::Format(wxT("%lu:%02lu"), Minutes, Seconds);
	}
	if(m_PlayLabel->GetLabel()!=PlayingTime)
	{
		m_PlayLabel->SetLabel(PlayingTime);
	}
	return;
}

void NDecGui::CMainDialog::OnClose(wxCloseEvent& Event)
{
	Stop();
	Event.Skip();
	return;
}

void NDecGui::CMainDialog::OnTimer(wxTimerEvent& Event)
{
	unsigned long Minutes=m_SoundTimer.Time()/(1000*60);
	unsigned long Seconds=(m_SoundTimer.Time()-Minutes*1000*60)/1000;
	m_PlayLabel->SetLabel(wxString::Format(wxT("%lu:%02lu"), Minutes, Seconds));
	if(!IsPlaying())
	{
		// I believe that this is not what's causing the stopping bug
		Stop();
	}
	return;
}

void NDecGui::CMainDialog::OnRefreshGui(wxCommandEvent& Event)
{
	RefreshGui();
	return;
}

void NDecGui::CMainDialog::OnScanDirectoryButtonClicked(wxCommandEvent& Event)
{
	wxDirDialog Dlg(this, _("Scan directory"), m_InputDir, wxDD_DEFAULT_STYLE | \
		wxDD_DIR_MUST_EXIST);

	if(Dlg.ShowModal()==wxID_OK)
	{
		// Save the default directory
		m_InputDir=Dlg.GetPath();

		// Clear the list
		Stop();
		FreezeUpdate();
		m_FilesList->Clear();
		m_FileList->RefreshGui();
		ThawUpdate();

		// Scan
		wxFileName Filedir(Dlg.GetPath(), wxT("*"), wxT("*"));
		m_FilesList->Scan(Filedir.GetPath(true).mb_str());

		// Update
		m_FileList->RefreshData();
		m_FileList->RefreshGui();
		m_SegmentList->RefreshData();
		RefreshGui();
	}
	return;
}

void NDecGui::CMainDialog::OnScanFileButtonClicked(wxCommandEvent& Event)
{
	wxFileDialog Dlg(this, _("Scan file"), m_InputDir, wxEmptyString, \
		_("All Files (*.*)|*.*"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if(Dlg.ShowModal()==wxID_OK)
	{
		// Save the default directory
		m_InputDir=Dlg.GetDirectory();

		// See if this file is already in the list
		wxFileName NewFile(Dlg.GetPath());
		bool Found=false;
		for(unsigned long i=0;i<m_FilesList->GetCount();i++)
		{
			// Get it
			NDecFunc::CSegmentsList& SegmentList=*m_FilesList->Get(i);

			// Check with a filename object
			wxFileName ExistingFile(SegmentList.GetFilename());

			// If they are equal add it here
			if(NewFile==ExistingFile)
			{
				// Scan into this file
				SegmentList.Clear();
				SegmentList.ScanFile();

				// Set the selection to this file and this segment
				TListSelection Sel;
				Sel.push_back(i);
				m_FileList->SetSelection(Sel);
				Found=true;
				break;
			}
		}

		// If it wasn't found we need to make a new one
		if(!Found)
		{
			// Create the new segment list
			NDecFunc::CSegmentsList* SegmentList=new NDecFunc::CSegmentsList;
			m_FilesList->Add(SegmentList);
			SegmentList->SetFilename(Dlg.GetPath().mb_str());

			// Scan into this file
			SegmentList->Clear();
			SegmentList->ScanFile();

			// Set the selection to this file and this segment
			TListSelection Sel;
			Sel.push_back(i);
			m_FileList->RefreshGui();
			m_FileList->SetSelection(Sel);
		}

		// Update
		m_FileList->RefreshData();
		m_FileList->RefreshGui();
		m_SegmentList->RefreshData();
		RefreshGui();
	}
	return;
}

void NDecGui::CMainDialog::OnAddManuallyButtonClicked(wxCommandEvent& Event)
{
	CAddManuallyDialog Dlg(this);

	if(Dlg.ShowModal()==wxID_OK)
	{
		// Work out the size
		std::streamoff Offset=Dlg.GetOffset();
		std::streamsize Size=Dlg.GetSize();
		std::streamsize FileSize=wxFileName::GetSize(Dlg.GetFilename()).ToULong();
		if(Offset>=FileSize)
		{
			wxMessageBox(_("The offset entered exceeds the length of the file."), _("Error"), wxICON_EXCLAMATION);
			return;
		}
		if(Size==0)
		{
			Size=FileSize-Offset;
		}

		// See if this file is already in the list
		wxFileName NewFile(Dlg.GetFilename());
		bool Found=false;
		for(unsigned long i=0;i<m_FilesList->GetCount();i++)
		{
			// Get it
			NDecFunc::CSegmentsList& SegmentList=*m_FilesList->Get(i);

			// Check with a filename object
			wxFileName ExistingFile(SegmentList.GetFilename());

			// If they are equal add it here
			if(NewFile==ExistingFile)
			{
				// Insert the segment
				unsigned long InsertPos;
				NDecFunc::CSegment* NewSegment;
				NewSegment=SegmentList.CreateSegment(Offset, Size);
				InsertPos=SegmentList.Insert(NewSegment);

				// Unmix the layers, if necessary
				std::vector<unsigned long> List;
				List.push_back(InsertPos);
				if(SegmentList.CanUnmix(List))
				{
					SegmentList.Unmix(List);
				}

				// Set the selection to this file and this segment
				TListSelection Sel;
				Sel.push_back(i);
				m_FileList->SetSelection(Sel);
				m_FileList->RefreshGui();
				Sel.clear();
				Sel.push_back(InsertPos);
				m_SegmentList->SetSelection(Sel);
				Found=true;
				break;
			}
		}

		// If it wasn't found we need to make a new one
		if(!Found)
		{
			// Create the new segment list
			NDecFunc::CSegmentsList* SegmentList=new NDecFunc::CSegmentsList;
			m_FilesList->Add(SegmentList);
			SegmentList->SetFilename(Dlg.GetFilename().mb_str());

			// Insert a new segment
			unsigned long InsertPos;
			NDecFunc::CSegment* NewSegment;
			NewSegment=SegmentList->CreateSegment(Offset, Size);
			InsertPos=SegmentList->Insert(NewSegment);

			// Unmix the layers, if necessary
			std::vector<unsigned long> List;
			List.push_back(InsertPos);
			if(SegmentList->CanUnmix(List))
			{
				SegmentList->Unmix(List);
			}

			// Set the selection to this file and this segment
			TListSelection Sel;
			Sel.push_back(i);
			m_FileList->RefreshGui();
			m_FileList->SetSelection(Sel);
			m_FileList->RefreshGui();
			Sel.clear();
			Sel.push_back(InsertPos);
			m_SegmentList->SetSelection(Sel);
		}

		// Update
		m_FileList->RefreshData();
		m_SegmentList->RefreshData();
		RefreshGui();
	}
	return;
}

void NDecGui::CMainDialog::OnClearButtonClicked(wxCommandEvent& Event)
{
	Stop();
	FreezeUpdate();
	m_FilesList->Clear();
	m_FileList->RefreshGui();
	ThawUpdate();
	return;
}

void NDecGui::CMainDialog::OnNextButtonClicked(wxCommandEvent& Event)
{
	FreezeUpdate();
	m_SegmentList->SelectNext();
	ThawUpdate();
	//if(IsPlaying())
	//{
		//Stop();
		Play();
	//}
	return;
}

void NDecGui::CMainDialog::OnSelectAllButtonClicked(wxCommandEvent& Event)
{
	FreezeUpdate();
	m_SegmentList->SelectAll();
	ThawUpdate();
	return;
}

void NDecGui::CMainDialog::OnSelectNoneButtonClicked(wxCommandEvent& Event)
{
	FreezeUpdate();
	m_SegmentList->SelectNone();
	ThawUpdate();
	return;
}

void NDecGui::CMainDialog::OnSelectAllFilesButtonClicked(wxCommandEvent& Event)
{
	FreezeUpdate();
	m_FileList->SelectAll();
	ThawUpdate();
	return;
}

void NDecGui::CMainDialog::OnDuplicateButtonClicked(wxCommandEvent& Event)
{
	if(m_SegmentList->GetSegmentsList())
	{
		FreezeUpdate();
		std::vector<unsigned long> Selection;
		m_SegmentList->GetSelection(Selection);
		m_SegmentList->GetSegmentsList()->Duplicate(Selection);
		m_SegmentList->SelectNone();
		m_FileList->RefreshData();
		m_SegmentList->RefreshData();
		ThawUpdate();
	}
	return;
}

void NDecGui::CMainDialog::OnRemoveButtonClicked(wxCommandEvent& Event)
{
	if(m_SegmentList->GetSegmentsList())
	{
		FreezeUpdate();
		std::vector<unsigned long> Selection;
		m_SegmentList->GetSelection(Selection);
		m_SegmentList->GetSegmentsList()->Remove(Selection);
		m_SegmentList->SelectNone();
		m_FileList->RefreshData();
		m_SegmentList->RefreshData();
		ThawUpdate();
	}
	return;
}

void NDecGui::CMainDialog::OnSetInfoButtonClicked(wxCommandEvent& Event)
{
	CSetInfoDialog Dlg(this);

	// Just checking
	if(!m_SegmentList->GetSegmentsList())
	{
		return;
	}

	// Some variables
	unsigned char NumberChannels=0;
	unsigned long SampleRate=0;
	std::streamsize StreamSize=0;
	TListSelection Sel;
	m_SegmentList->GetSelection(Sel);

	// Go through the items and set our variables
	for(TListSelection::const_iterator Iter=Sel.begin();Iter!=Sel.end();++Iter)
	{
		// Get it
		NDecFunc::CSegment& Segment=*m_SegmentList->GetSegmentsList()->Get(*Iter);

		// Check number of channels
		if(Iter==Sel.begin())
		{
			NumberChannels=Segment.GetChannels();
			SampleRate=Segment.GetSampleRate();
			StreamSize=Segment.GetSize();
		}
		else
		{
			if(NumberChannels!=Segment.GetChannels())
			{
				NumberChannels=0;
			}
			if(SampleRate!=Segment.GetSampleRate())
			{
				SampleRate=0;
			}
			if(StreamSize!=Segment.GetSize())
			{
				StreamSize=0;
			}
		}
	}

	// Set these properties
	Dlg.SetChannels(NumberChannels);
	Dlg.SetSampleRate(SampleRate);
	Dlg.SetStreamSize(StreamSize);

	if(Dlg.ShowModal()==wxID_OK)
	{
		// Get from the dialog box
		NumberChannels=Dlg.GetChannels();
		SampleRate=Dlg.GetSampleRate();
		StreamSize=Dlg.GetStreamSize();

		// Go through the items and set them
		for(TListSelection::const_iterator Iter=Sel.begin();Iter!=Sel.end();++Iter)
		{
			// Get it
			NDecFunc::CSegment& Segment=*m_SegmentList->GetSegmentsList()->Get(*Iter);

			// Set it if needed
			if(NumberChannels)
			{
				Segment.SetChannels(NumberChannels);
			}
			if(SampleRate)
			{
				Segment.SetSampleRate(SampleRate);
			}
			if(StreamSize)
			{
				Segment.SetSize(StreamSize);
			}
		}

		// Update
		m_SegmentList->RefreshData();
	}
	return;
}

void NDecGui::CMainDialog::OnMixLayersButtonClicked(wxCommandEvent& Event)
{
	if(m_SegmentList->GetSegmentsList())
	{
		FreezeUpdate();
		std::vector<unsigned long> Indicies;
		m_SegmentList->GetSelection(Indicies);
		m_SegmentList->GetSegmentsList()->Mix(Indicies);
		m_SegmentList->SelectNone();
		m_FileList->RefreshData();
		m_SegmentList->RefreshData();
		ThawUpdate();
	}
	return;
}

void NDecGui::CMainDialog::OnUnmixLayersButtonClicked(wxCommandEvent& Event)
{
	if(m_SegmentList->GetSegmentsList())
	{
		FreezeUpdate();
		std::vector<unsigned long> Indicies;
		m_SegmentList->GetSelection(Indicies);
		m_SegmentList->GetSegmentsList()->Unmix(Indicies);
		m_SegmentList->SelectNone();
		m_FileList->RefreshData();
		m_SegmentList->RefreshData();
		ThawUpdate();
	}
	return;
}

void NDecGui::CMainDialog::OnPlayButtonClicked(wxCommandEvent& Event)
{
	Play(false);
	return;
}

void NDecGui::CMainDialog::OnLoopButtonClicked(wxCommandEvent& Event)
{
	Play(true);
	/*
	NSound::CPlayer* Player;
	NSound::CTestStream TestStream;
	Player=NSound::CPlayer::CreatePlayer();
	Player->Play(&TestStream);
	wxSleep(1);
	Player->Stop();
	NSound::CPlayer::DestroyPlayer(Player);
	Player=NULL;*/
	return;
}

void NDecGui::CMainDialog::OnStopButtonClicked(wxCommandEvent& Event)
{
	if(m_Playback)
	{
		m_Playback->ForceStop(true);
	}
	m_StopButton->Enable(false);
	Stop();
	return;
}

void NDecGui::CMainDialog::OnConcatenatedButtonClicked(wxCommandEvent& Event)
{
	RefreshGui();

	// Collect all of the selected items
	std::vector<NDecFunc::CSegment*> Segments;
	if(m_FileList->GetSelectedItemCount()>1)
	{
		TListSelection Sel;
		m_FileList->GetSelection(Sel);
		for(TListSelection::const_iterator Iter=Sel.begin();Iter!=Sel.end();++Iter)
		{
			NDecFunc::CSegmentsList* List=m_FilesList->Get(*Iter);
			if(!List)
			{
				continue;
			}
			for(unsigned long i=0;i<List->GetCount();i++)
			{
				Segments.push_back(List->Get(i));
			}
		}
	}
	else if(m_SegmentList->GetSegmentsList())
	{
		TListSelection Sel;
		m_SegmentList->GetSelection(Sel);
		for(TListSelection::const_iterator Iter=Sel.begin();Iter!=Sel.end();++Iter)
		{
			Segments.push_back(m_SegmentList->GetSegmentsList()->Get(*Iter));
		}
	}

	// Now prompt for an output filename
	wxFileDialog Dlg(this, _("Decode to file"), m_OutputDir, wxEmptyString, \
		_("Microsoft Wave Files (*.wav)|*.wav|All Files (*.*)|*.*"), wxFD_SAVE | \
		wxFD_OVERWRITE_PROMPT);

	if(Dlg.ShowModal()==wxID_OK)
	{
		// Save the default directory
		m_OutputDir=Dlg.GetDirectory();

		// Output it
		NDecFunc::OutputConcatenated(Dlg.GetPath().mb_str(), Segments);
	}
	return;
}

void NDecGui::CMainDialog::OnSeparateButtonClicked(wxCommandEvent& Event)
{
	RefreshGui();

	// Collect all of the selected items
	std::vector<NDecFunc::CSegment*> Segments;
	if(m_FileList->GetSelectedItemCount()>1)
	{
		TListSelection Sel;
		m_FileList->GetSelection(Sel);
		for(TListSelection::const_iterator Iter=Sel.begin();Iter!=Sel.end();++Iter)
		{
			NDecFunc::CSegmentsList* List=m_FilesList->Get(*Iter);
			if(!List)
			{
				continue;
			}
			for(unsigned long i=0;i<List->GetCount();i++)
			{
				Segments.push_back(List->Get(i));
			}
		}
	}
	else if(m_SegmentList->GetSegmentsList())
	{
		TListSelection Sel;
		m_SegmentList->GetSelection(Sel);
		for(TListSelection::const_iterator Iter=Sel.begin();Iter!=Sel.end();++Iter)
		{
			Segments.push_back(m_SegmentList->GetSegmentsList()->Get(*Iter));
		}
	}

	// Now prompt for an output directory
	wxDirDialog Dlg(this, _("Decode to directory"), m_OutputDir, wxDD_DEFAULT_STYLE);

	if(Dlg.ShowModal()==wxID_OK)
	{
		// Save the default directory
		m_OutputDir=Dlg.GetPath();

		// Output it
		NDecFunc::OutputSeparate(Dlg.GetPath().mb_str(), Segments);
	}
	return;
}

void NDecGui::CMainDialog::OnLayerExtractButtonClicked(wxCommandEvent& Event)
{
	RefreshGui();

	// Collect all of the selected items
	std::vector<NDecFunc::CSegment*> Segments;
	if(m_FileList->GetSelectedItemCount()>1)
	{
		TListSelection Sel;
		m_FileList->GetSelection(Sel);
		for(TListSelection::const_iterator Iter=Sel.begin();Iter!=Sel.end();++Iter)
		{
			NDecFunc::CSegmentsList* List=m_FilesList->Get(*Iter);
			if(!List)
			{
				continue;
			}
			for(unsigned long i=0;i<List->GetCount();i++)
			{
				Segments.push_back(List->Get(i));
			}
		}
	}
	else if(m_SegmentList->GetSegmentsList())
	{
		TListSelection Sel;
		m_SegmentList->GetSelection(Sel);
		for(TListSelection::const_iterator Iter=Sel.begin();Iter!=Sel.end();++Iter)
		{
			Segments.push_back(m_SegmentList->GetSegmentsList()->Get(*Iter));
		}
	}

	// Now prompt for an output directory
	wxDirDialog Dlg(this, _("Layer extract to directory"), m_OutputDir, wxDD_DEFAULT_STYLE);

	if(Dlg.ShowModal()==wxID_OK)
	{
		// Save the default directory
		m_OutputDir=Dlg.GetPath();

		// Output it
		NDecFunc::OutputLayerExtract(Dlg.GetPath().mb_str(), Segments);
	}
	return;
}

void NDecGui::CMainDialog::OnFileListSelChange(wxListEvent& Event)
{
	// TODO: Update this for playing info
	m_NoMagic=true;
	RefreshGui();
	m_NoMagic=false;
	return;
}

void NDecGui::CMainDialog::OnSegmentListSelChange(wxListEvent& Event)
{
	// TODO: Update this for playing info
	m_NoMagic=true;
	RefreshGui();
	m_NoMagic=false;
	return;
}

void NDecGui::CMainDialog::Play(bool Looping)
{
	// Check the state
	if(!m_SegmentList->GetSegmentsList())
	{
		return;
	}

	// Remove any currently playing items
	m_SegmentList->Stop();

	// Check the current state
	if(m_Sound)
	{
		// Stop it
		m_Playback->ForceStop(true);
		m_SoundUpdate.Stop();
		m_SoundTimer.Pause();
		m_Sound->Stop();

		// Deallocate structures
		delete m_Sound;
		m_Sound=NULL;
		delete m_Stream;
		m_Stream=NULL;
	}

	// Make sure there is something to play
	if(!m_SegmentList->GetSelectedItemCount())
	{
		return;
	}

	// Create a new segment stream
	m_Stream=new NDecFunc::CSegmentStream;

	// Get a list of what segments to play
	NDecFunc::CSegmentsList& Segments=*m_SegmentList->GetSegmentsList();
	TListSelection Sel;
	m_SegmentList->GetSelection(Sel);

	// Set the selection to play
	if(Sel.size()==0)
	{
		return;
	}
	else if(Sel.size()==1)
	{
		m_SegmentList->Play(Sel);
	}
	else
	{
		m_SegmentList->Play(Sel);
	}

	// Add the segments to the list
	for(TListSelection::const_iterator Iter=Sel.begin();Iter!=Sel.end();++Iter)
	{
		// Get the segment
		m_Stream->Add(Segments.Get(*Iter));
		m_Stream->Get(m_Stream->GetCount()-1)->SetListView(m_SegmentList);
		m_Stream->Get(m_Stream->GetCount()-1)->SetListViewIndex(*Iter);
	}

	// Create the player if it doesn't already exist
	if(!m_Playback)
	{
		m_Playback=new wxSoundStreamWin;
	}

	// Create the segment sound stream
	m_Sound=new CSegmentStreamSound(*m_Stream, *m_Playback);
	m_Sound->SetLooping(Looping);

	// Start it playing
	m_Playback->ForceStop(false);
	m_Sound->Play();
	m_SoundUpdate.Start(1000);
	m_SoundTimer.Start();

	// Update
	RefreshGui();
	return;
}

bool NDecGui::CMainDialog::IsPlaying() const
{
	// Check the current state
	if(!m_Playback || !m_Stream || !m_Sound)
	{
		return false;
	}
	return !m_Sound->IsStopped();
}

void NDecGui::CMainDialog::Stop()
{
	// Remove any currently playing items
	m_SegmentList->Stop();

	// Check the current state
	if(!m_Sound)
	{
		return;
	}

	// Stop it
	m_Playback->ForceStop(true);
	m_SoundUpdate.Stop();
	m_SoundTimer.Pause();
	m_Sound->Stop();

	// Deallocate structures
	delete m_Sound;
	m_Sound=NULL;
	delete m_Stream;
	m_Stream=NULL;

	// Update
	RefreshGui();
	return;
}

void NDecGui::CMainDialog::FreezeUpdate()
{
	m_Update++;
	return;
}

void NDecGui::CMainDialog::ThawUpdate()
{
	if(m_Update)
	{
		m_Update--;
		RefreshGui();
	}
	return;
}

