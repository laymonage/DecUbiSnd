/*
 AddManuallyDialog.cpp : Add a segment manually
*/

#include "Pch.h"

#include "Gui/AddManuallyDialog.h"

// CAddManuallyDialog Event Table
IMPLEMENT_CLASS(NDecGui::CAddManuallyDialog, wxDialog)
BEGIN_EVENT_TABLE(NDecGui::CAddManuallyDialog, wxDialog)
	EVT_BUTTON(ID_BrowseButton, OnBrowseButtonClicked)
END_EVENT_TABLE()

// CAddManuallyDialog Implementation
NDecGui::CAddManuallyDialog::CAddManuallyDialog(wxWindow* Parent, const wxPoint& Pos) :
	wxDialog(Parent, wxID_ANY, _("Add Segment Manually"), Pos, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	// Create the controls
	m_FilenameOffsetSizerSizer_staticbox = new wxStaticBox(this, -1, _("Filename, Offset, and Size"));
	m_FilenameTextbox = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	m_BrowseButton = new wxButton(this, ID_BrowseButton, _("&Browse..."));
	m_OffsetTextbox = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	m_SizeTextbox = new wxTextCtrl(this, wxID_ANY, wxEmptyString);

	// Do the layout
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxSizer* ButtonsSizer = CreateButtonSizer(wxOK | wxCANCEL);
	wxStaticBoxSizer* FilenameOffsetSizerSizer = new wxStaticBoxSizer(m_FilenameOffsetSizerSizer_staticbox, wxVERTICAL);
	wxBoxSizer* OffsetSizeSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* FilenameSizer = new wxBoxSizer(wxHORIZONTAL);
	FilenameSizer->Add(m_FilenameTextbox, 1, wxRIGHT|wxEXPAND, 5);
	FilenameSizer->Add(m_BrowseButton, 0, 0, 0);
	FilenameOffsetSizerSizer->Add(FilenameSizer, 0, wxBOTTOM|wxEXPAND, 5);
	wxStaticText* OffsetLabel = new wxStaticText(this, wxID_ANY, wxT("Offset:"));
	OffsetSizeSizer->Add(OffsetLabel, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
	OffsetSizeSizer->Add(m_OffsetTextbox, 1, wxRIGHT|wxEXPAND, 5);
	wxStaticText* SizeLabel = new wxStaticText(this, wxID_ANY, wxT("Size:"));
	OffsetSizeSizer->Add(SizeLabel, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
	OffsetSizeSizer->Add(m_SizeTextbox, 1, wxEXPAND, 0);
	FilenameOffsetSizerSizer->Add(OffsetSizeSizer, 0, wxEXPAND, 4);
	MainSizer->Add(FilenameOffsetSizerSizer, 0, wxALL|wxEXPAND, 5);
	MainSizer->Add(ButtonsSizer, 1, wxBOTTOM|wxRIGHT|wxEXPAND, 5);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	Layout();
	Center();
	return;
}

NDecGui::CAddManuallyDialog::~CAddManuallyDialog()
{
	return;
}

void NDecGui::CAddManuallyDialog::OnBrowseButtonClicked(wxCommandEvent& Event)
{
	wxFileDialog Dlg(this, _("Choose a file"), wxEmptyString, m_FilenameTextbox->GetValue(), \
		_("Standard UbiSoft Sound Files (*.ss?;*.ls?)|*.ss?;*.ls?|All Files (*.*)|*.*"), \
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if(Dlg.ShowModal()==wxID_OK)
	{
		m_FilenameTextbox->SetValue(Dlg.GetPath());
	}
	return;
}

wxString NDecGui::CAddManuallyDialog::GetFilename() const
{
	return m_FilenameTextbox->GetValue();
}

std::streamoff NDecGui::CAddManuallyDialog::GetOffset() const
{
	std::streamoff StreamOffset=0;
	m_OffsetTextbox->GetValue().ToULong((unsigned long*)&StreamOffset);
	return StreamOffset;
}

std::streamsize NDecGui::CAddManuallyDialog::GetSize() const
{
	std::streamsize StreamSize=0;
	m_SizeTextbox->GetValue().ToULong((unsigned long*)&StreamSize);
	return StreamSize;
}
