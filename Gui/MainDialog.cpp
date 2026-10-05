/*
 MainDialog.cpp : The main dialog
*/

#include "Pch.h"

#include <wx/filename.h>
#include <algorithm>

#include "Gui/App.h"
#include "Gui/MainDialog.h"
#include "Gui/FilesListView.h"
#include "Gui/SegmentsListView.h"
#include "Gui/AddManuallyDialog.h"
#include "Gui/SetInfoDialog.h"
#include "Functionality/FilesList.h"
#include "Functionality/SegmentsList.h"
#include "Functionality/Segment.h"
#include "Functionality/SegmentStream.h"
#include "Functionality/Output.h"
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

	EVT_BUTTON(ID_SaveAsButton, NDecGui::CMainDialog::OnSaveAsButtonClicked)
	EVT_BUTTON(ID_SaveToWavButton, NDecGui::CMainDialog::OnSaveToWavButtonClicked)
	EVT_BUTTON(ID_SpliceToWavButton, NDecGui::CMainDialog::OnSpliceToWavButtonClicked)
	EVT_BUTTON(ID_ExtractAsButton, NDecGui::CMainDialog::OnExtractAsButtonClicked)

	EVT_LIST_ITEM_SELECTED(ID_FileList, NDecGui::CMainDialog::OnFileListSelChange)
	EVT_LIST_ITEM_DESELECTED(ID_FileList, NDecGui::CMainDialog::OnFileListSelChange)
	EVT_LIST_ITEM_FOCUSED(ID_FileList, NDecGui::CMainDialog::OnFileListSelChange)
	EVT_LIST_ITEM_SELECTED(ID_SegmentList, NDecGui::CMainDialog::OnSegmentListSelChange)
	EVT_LIST_ITEM_DESELECTED(ID_SegmentList, NDecGui::CMainDialog::OnSegmentListSelChange)
	EVT_LIST_ITEM_FOCUSED(ID_SegmentList, NDecGui::CMainDialog::OnSegmentListSelChange)
	EVT_LIST_ITEM_ACTIVATED(ID_SegmentList, NDecGui::CMainDialog::OnSegmentListItemActivated)
END_EVENT_TABLE()

// CMainDialog Implementation
NDecGui::CMainDialog::CMainDialog(wxWindow* Parent, const wxPoint& Pos, const wxSize& Size) :
	wxDialog(Parent, wxID_ANY, _("Decode UbiSoft Sounds/Music version " DECUBISND_VERSION), Pos, Size, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxMAXIMIZE_BOX|wxMINIMIZE_BOX),
	m_FilesList(NULL),
	m_Update(0),
	m_NoMagic(false),
	m_Player(this, wxID_ANY)
{
	// Create the controls
	SelectionSizer_staticbox = new wxStaticBox(this, -1, wxT("Selection"));
	EditSizer_staticbox = new wxStaticBox(this, -1, wxT("Edit"));
	LayersSizer_staticbox = new wxStaticBox(this, -1, wxT("Layers"));
	PlaySizer_staticbox = new wxStaticBox(this, -1, wxT("Play"));
	OutputSizer_staticbox = new wxStaticBox(this, -1, wxT("Output"));
	InputSizer_staticbox = new wxStaticBox(this, -1, wxT("Input"));
	m_ScanDirectoryButton = new wxButton(this, ID_ScanDirectoryButton, wxT("Scan &Directory..."));
	m_ScanFileButton = new wxButton(this, ID_ScanFileButton, wxT("Scan &File..."));
	m_AddManuallyButton = new wxButton(this, ID_AddManuallyButton, wxT("Add &Manually..."));
	m_LoadBankMapButton = new wxButton(this, ID_LoadBankButton, wxT("L&oad Bank/Map..."));
	m_ClearButton = new wxButton(this, ID_ClearButton, wxT("&Clear"));
	m_SelectAllButton = new wxButton(this, ID_SelectAllButton, wxT("Select &All"));
	m_SelectNoneButton = new wxButton(this, ID_SelectNoneButton, wxT("Select &None"));
	m_SelectAllFilesButton = new wxButton(this, ID_SelectAllFilesButton, wxT("Select All F&iles"));
	m_DuplicateButton = new wxButton(this, ID_DuplicateButton, wxT("&Duplicate"));
	m_RemoveButton = new wxButton(this, ID_RemoveButton, wxT("&Remove"));
	m_SetInfoButton = new wxButton(this, ID_SetInfoButton, wxT("Set &Info..."));
	m_MixLayersButton = new wxButton(this, ID_MixLayersButton, wxT("Mi&x Layers"));
	m_UnmixLayersButton = new wxButton(this, ID_UnmixLayersButton, wxT("&Unmix Layers"));
	m_PlayButton = new wxButton(this, ID_PlayButton, wxT("&Play"));
	m_LoopButton = new wxButton(this, ID_LoopButton, wxT("&Loop"));
	m_StopButton = new wxButton(this, ID_StopButton, wxT("&Stop"));
	m_PlayLabel = new wxStaticText(this, wxID_ANY, wxT("m:ss"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
	m_NextButton = new wxButton(this, ID_NextButton, wxT("Ne&xt"));
	m_ContinuousButton = new wxButton(this, ID_ContinuousButton, wxT("Continuous"));
	m_SaveAsButton = new wxButton(this, ID_SaveAsButton, wxT("Save As..."));
	m_SaveToWavButton = new wxButton(this, ID_SaveToWavButton, wxT("Save to WAV(s).."));
	m_SpliceToWavButton = new wxButton(this, ID_SpliceToWavButton, wxT("Splice to WAV..."));
	m_ExtractAsButton = new wxButton(this, ID_ExtractAsButton, wxT("Extract As..."));
    m_FileList = new CFilesListView(this, ID_FileList, wxDefaultPosition, wxDefaultSize);
    m_SegmentList = new CSegmentsListView(this, ID_SegmentList, wxDefaultPosition, wxDefaultSize);

	// Set some properties
	m_LoadBankMapButton->Hide();
	m_ContinuousButton->Hide();

	// Add some tool tips
	m_NextButton->SetToolTip(wxT("Move the selection down on track and play it."));
	m_SaveAsButton->SetToolTip(wxT("Save the selected track(s) in the best output format."));
	m_SaveToWavButton->SetToolTip(wxT("Save the selected track(s) as Microsoft Wave files."));
	m_SpliceToWavButton->SetToolTip(wxT("Splice all the selected tracks together and save them as a single WAV file."));
	m_ExtractAsButton->SetToolTip(wxT("Save the track(s) in their original forms (potentially not playable with common audio software)."));

	// Do the layout
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* SecondarySizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* ButtonsSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticBoxSizer* OutputSizer = new wxStaticBoxSizer(OutputSizer_staticbox, wxVERTICAL);
	wxStaticBoxSizer* PlaySizer = new wxStaticBoxSizer(PlaySizer_staticbox, wxHORIZONTAL);
	wxBoxSizer* PlaySizerCol2 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* PlaySizerCol1 = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* LayersSizer = new wxStaticBoxSizer(LayersSizer_staticbox, wxVERTICAL);
	wxStaticBoxSizer* EditSizer = new wxStaticBoxSizer(EditSizer_staticbox, wxVERTICAL);
	wxStaticBoxSizer* SelectionSizer = new wxStaticBoxSizer(SelectionSizer_staticbox, wxVERTICAL);
	wxStaticBoxSizer* InputSizer = new wxStaticBoxSizer(InputSizer_staticbox, wxVERTICAL);
	InputSizer->Add(m_ScanDirectoryButton, 0, wxBOTTOM|wxEXPAND, 5);
	InputSizer->Add(m_ScanFileButton, 0, wxBOTTOM|wxEXPAND, 5);
	InputSizer->Add(m_AddManuallyButton, 0, wxBOTTOM|wxEXPAND, 5);
	InputSizer->Add(m_LoadBankMapButton, 0, wxBOTTOM|wxEXPAND, 5);
	InputSizer->Add(m_ClearButton, 0, wxEXPAND, 5);
	ButtonsSizer->Add(InputSizer, 0, wxRIGHT, 5);
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
	PlaySizerCol1->Add(m_PlayButton, 0, wxBOTTOM|wxEXPAND, 5);
	PlaySizerCol1->Add(m_LoopButton, 0, wxBOTTOM|wxEXPAND, 5);
	PlaySizerCol1->Add(m_StopButton, 0, wxBOTTOM|wxEXPAND, 5);
	PlaySizerCol1->Add(m_PlayLabel, 0, wxEXPAND, 5);
	PlaySizer->Add(PlaySizerCol1, 1, wxRIGHT|wxEXPAND, 5);
	PlaySizerCol2->Add(m_NextButton, 0, wxBOTTOM|wxEXPAND, 5);
	PlaySizerCol2->Add(m_ContinuousButton, 0, wxBOTTOM|wxEXPAND, 5);
	PlaySizer->Add(PlaySizerCol2, 1, wxEXPAND, 5);
	ButtonsSizer->Add(PlaySizer, 0, wxRIGHT, 5);
	OutputSizer->Add(m_SaveAsButton, 0, wxBOTTOM|wxEXPAND, 5);
	OutputSizer->Add(m_SaveToWavButton, 0, wxBOTTOM|wxEXPAND, 5);
	OutputSizer->Add(m_SpliceToWavButton, 0, wxBOTTOM|wxEXPAND, 5);
	OutputSizer->Add(m_ExtractAsButton, 0, wxEXPAND, 5);
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
	if(m_NextButton->IsEnabled()!=EnableState)
	{
		m_NextButton->Enable(EnableState);
	}
	if(m_ContinuousButton->IsEnabled()!=EnableState)
	{
		m_ContinuousButton->Enable(EnableState);
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
	if(m_SaveAsButton->IsEnabled()!=EnableState)
	{
		m_SaveAsButton->Enable(EnableState);
	}
	if(m_SaveToWavButton->IsEnabled()!=EnableState)
	{
		m_SaveToWavButton->Enable(EnableState);
	}
	if(m_SpliceToWavButton->IsEnabled()!=EnableState)
	{
		m_SpliceToWavButton->Enable(EnableState);
	}
	if(m_ExtractAsButton->IsEnabled()!=EnableState)
	{
		m_ExtractAsButton->Enable(EnableState);
	}

	UpdatePlayingTime();
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
	UpdatePlayingTime();
	if (!m_Player.isPlaying())
	{
		m_SegmentList->Stop();
		RefreshGui();
	}
}

void NDecGui::CMainDialog::OnRefreshGui(wxCommandEvent& Event)
{
	RefreshGui();
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
		m_FilesList->Scan(std::string(Filedir.GetPath(true).mb_str()));

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
	wxFileDialog OpenDlg(this, _("Scan file"), m_InputDir, wxEmptyString, \
		_("All Files (*.*)|*.*"), wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

	if(OpenDlg.ShowModal()==wxID_OK)
	{
		// Save the default directory
		m_InputDir=OpenDlg.GetDirectory();

		wxArrayString paths;
		OpenDlg.GetPaths(paths);

		for (wxArrayString::const_iterator iter = paths.begin(); iter != paths.end(); ++iter)
		{
			// See if this file is already in the list
			wxFileName NewFile(*iter);
			bool Found = false;
			unsigned int i = 0;
			for(i = 0; i < m_FilesList->GetCount(); i++)
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
				SegmentList->SetFilename(std::string(iter->mb_str()));

				// Scan into this file
				SegmentList->Clear();
				SegmentList->ScanFile();

				// Set the selection to this file and this segment
				TListSelection Sel;
				Sel.push_back(m_FilesList->GetCount()-1);
				m_FileList->RefreshGui();
				m_FileList->SetSelection(Sel);
			}
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
		unsigned int i = 0;
		for(i = 0; i < m_FilesList->GetCount(); i++)
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
			SegmentList->SetFilename(std::string(Dlg.GetFilename().mb_str()));

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
			Sel.push_back(m_FilesList->GetCount()-1);
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
	Play();
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
	return;
}

void NDecGui::CMainDialog::OnStopButtonClicked(wxCommandEvent& Event)
{
	m_StopButton->Enable(false);
	Stop();
	return;
}

void NDecGui::CMainDialog::OnSaveAsButtonClicked(wxCommandEvent& Event)
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
	wxFileDialog Dlg(this, _("Save As"), m_OutputDir, wxEmptyString, \
		_("Common audio formats (*.wav;*.ogg)|*.wav;*.ogg|All Files (*.*)|*.*"), wxFD_SAVE | \
		wxFD_OVERWRITE_PROMPT);

	if(Dlg.ShowModal()==wxID_OK)
	{
		// Save the default directory
		m_OutputDir=Dlg.GetDirectory();

		NDecFunc::OutputBest(std::string(Dlg.GetPath().mb_str()), Segments);
	}
}

void NDecGui::CMainDialog::OnSaveToWavButtonClicked(wxCommandEvent& Event)
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
	wxFileDialog Dlg(this, _("Save to WAV(s)"), m_OutputDir, wxEmptyString, \
		_("Microsoft Wave Files (*.wav)|*.wav|All Files (*.*)|*.*"), wxFD_SAVE | \
		wxFD_OVERWRITE_PROMPT);

	if(Dlg.ShowModal()==wxID_OK)
	{
		// Save the default directory
		m_OutputDir=Dlg.GetDirectory();

		NDecFunc::OutputSeparate(std::string(Dlg.GetPath().mb_str()), Segments);
	}
	return;
}

void NDecGui::CMainDialog::OnSpliceToWavButtonClicked(wxCommandEvent& Event)
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
	wxFileDialog Dlg(this, _("Splice to WAV"), m_OutputDir, wxEmptyString, \
		_("Microsoft Wave Files (*.wav)|*.wav|All Files (*.*)|*.*"), wxFD_SAVE | \
		wxFD_OVERWRITE_PROMPT);

	if(Dlg.ShowModal()==wxID_OK)
	{
		// Save the default directory
		m_OutputDir=Dlg.GetDirectory();

		// Output it
		NDecFunc::OutputConcatenated(std::string(Dlg.GetPath().mb_str()), Segments);
	}
	return;
}

void NDecGui::CMainDialog::OnExtractAsButtonClicked(wxCommandEvent& Event)
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
	wxFileDialog Dlg(this, _("Extract As"), m_OutputDir, wxEmptyString, \
		_("Raw audio layers (*.layer)|*.layer|All Files (*.*)|*.*"), wxFD_SAVE | \
		wxFD_OVERWRITE_PROMPT);

	if(Dlg.ShowModal()==wxID_OK)
	{
		// Save the default directory
		m_OutputDir=Dlg.GetDirectory();

		// Output it
		NDecFunc::OutputLayerExtract(std::string(Dlg.GetPath().mb_str()), Segments);
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

void NDecGui::CMainDialog::OnSegmentListItemActivated(wxListEvent& Event)
{
	Play();
}

void NDecGui::CMainDialog::Play(bool Looping)
{
	// Check the state
	if(!m_SegmentList->GetSegmentsList())
	{
		return;
	}

	// Make sure there is something to play
	if(!m_SegmentList->GetSelectedItemCount())
	{
		return;
	}

	m_Player.stop();

	// Remove any currently playing items
	m_SegmentList->Stop();

	// Create a new segment stream
	NDecFunc::CSegmentStream* stream = new NDecFunc::CSegmentStream;

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
		stream->Add(Segments.Get(*Iter)); // TODO Really, we should be making a copy of the segment here for thread safety
		stream->Get(stream->GetCount()-1)->SetListView(m_SegmentList);
		stream->Get(stream->GetCount()-1)->SetListViewIndex(*Iter);
	}

	m_Player.setLoop(Looping);
	m_Player.play(stream);

	// Update
	RefreshGui();
	return;
}

bool NDecGui::CMainDialog::IsPlaying() const
{
	return m_Player.isPlaying();
}

void NDecGui::CMainDialog::Stop()
{
	m_Player.stop();

	// Remove any currently playing items
	m_SegmentList->Stop();

	// Update
	RefreshGui();
	return;
}

void NDecGui::CMainDialog::UpdatePlayingTime()
{
	// Check playing time
	wxString label(_("Stopped"));
	if (IsPlaying())
	{
		long ms = m_Player.getPlayingTime();
		long minutes = ms / (1000 * 60);
		long seconds = (ms - minutes * 1000 * 60) / 1000; // Weird, but I'll go with it
		label = wxString::Format(wxT("%lu:%02lu"), minutes, seconds);
	}
	if (m_PlayLabel->GetLabel() != label)
	{
		m_PlayLabel->SetLabel(label);
	}
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

