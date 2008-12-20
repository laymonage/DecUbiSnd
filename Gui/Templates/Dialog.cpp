/*
 Dialog.cpp : A template for a dialog
*/

#include "Pch.h"

#include "Gui/Templates/Dialog.h"

// CDialog Event Table
IMPLEMENT_CLASS(NDecGui::CDialog, wxDialog)
BEGIN_EVENT_TABLE(NDecGui::CDialog, wxDialog)
	// TODO: Events go here
END_EVENT_TABLE()

// CDialog Implementation
NDecGui::CDialog::CDialog(wxWindow* Parent, const wxPoint& Pos/*, const wxSize& Size*/) :
	wxDialog(Parent, wxID_ANY, _("Dialog"), Pos, Size, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxTHICK_FRAME)
{
	return;
}

NDecGui::CDialog::~CDialog()
{
	return;
}
