/*
 AddManuallyDialog.h : Add a segment manually
*/

#pragma once

namespace NDecGui
{
	class CAddManuallyDialog : public wxDialog
	{
		DECLARE_CLASS(CAddManuallyDialog);
		DECLARE_EVENT_TABLE();

	protected:
		enum
		{
			ID_BrowseButton=1000
		};

	protected:
		wxStaticBox* m_FilenameOffsetSizerSizer_staticbox;
		wxTextCtrl* m_FilenameTextbox;
		wxButton* m_BrowseButton;
		wxTextCtrl* m_OffsetTextbox;
		wxTextCtrl* m_SizeTextbox;

	public:
		CAddManuallyDialog(wxWindow* Parent, const wxPoint& Pos=wxDefaultPosition);
		virtual ~CAddManuallyDialog();

		void OnBrowseButtonClicked(wxCommandEvent& Event);

		virtual wxString GetFilename() const;
		virtual std::streamoff GetOffset() const;
		virtual std::streamsize GetSize() const;
	};
};
