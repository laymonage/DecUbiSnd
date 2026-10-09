/*
 MainDialog.h : The main dialog
*/

#pragma once

#include <wx/listctrl.h>
#include "../Sound/AudioPlayer.h"

namespace NDecFunc
{
	class CFilesList;
	class CSegmentStream;
};

namespace NDecGui
{
	class CFilesListView;
	class CSegmentsListView;
	class CSegmentStreamSound;

	class CMainDialog : public wxDialog
	{
		DECLARE_CLASS(CMainDialog);
		DECLARE_EVENT_TABLE();

	protected:
		enum EID
		{
			ID_ScanDirectoryButton=1000,
			ID_ScanFileButton,
			ID_AddManuallyButton,
			ID_LoadBankButton,
			ID_ClearButton,
			ID_SelectAllButton,
			ID_SelectNoneButton,
			ID_SelectAllFilesButton,
			ID_DuplicateButton,
			ID_RemoveButton,
			ID_SetInfoButton,
			ID_MixLayersButton,
			ID_UnmixLayersButton,
			ID_PlayButton,
			ID_LoopButton,
			ID_StopButton,
			ID_PlayLabel,
			ID_NextButton,
			ID_ContinuousButton,
			ID_SaveAsButton,
			ID_SaveToWavButton,
			ID_SpliceToWavButton,
			ID_ExtractAsButton,
			ID_FileList,
			ID_SegmentList
		};

	protected:
		NDecFunc::CFilesList* m_FilesList;
		unsigned long m_Update;
		bool m_NoMagic;
		wxString m_InputDir;
		wxString m_OutputDir;

		AudioPlayer m_Player;

		wxStaticBox* OutputSizer_staticbox;
		wxStaticBox* PlaySizer_staticbox;
		wxStaticBox* LayersSizer_staticbox;
		wxStaticBox* EditSizer_staticbox;
		wxStaticBox* SelectionSizer_staticbox;
		wxStaticBox* InputSizer_staticbox;
		wxButton* m_ScanDirectoryButton;
		wxButton* m_ScanFileButton;
		wxButton* m_AddManuallyButton;
		wxButton* m_LoadBankMapButton;
		wxButton* m_ClearButton;
		wxButton* m_SelectAllButton;
		wxButton* m_SelectNoneButton;
		wxButton* m_SelectAllFilesButton;
		wxButton* m_DuplicateButton;
		wxButton* m_RemoveButton;
		wxButton* m_SetInfoButton;
		wxButton* m_MixLayersButton;
		wxButton* m_UnmixLayersButton;
		wxButton* m_PlayButton;
		wxButton* m_LoopButton;
		wxButton* m_StopButton;
		wxStaticText* m_PlayLabel;
		wxButton* m_NextButton;
		wxButton* m_ContinuousButton;
		wxButton* m_SaveAsButton;
		wxButton* m_SaveToWavButton;
		wxButton* m_SpliceToWavButton;
		wxButton* m_ExtractAsButton;

		CFilesListView* m_FileList;
		CSegmentsListView* m_SegmentList;

	protected:
		void UpdatePlayingTime();
		virtual void FreezeUpdate();
		virtual void ThawUpdate();

	public:
		CMainDialog(wxWindow* Parent, const wxPoint& Pos=wxDefaultPosition, \
			const wxSize& Size=wxDefaultSize);
		virtual ~CMainDialog();

		virtual void RefreshGui();
		void OnClose(wxCloseEvent& Event);
		void OnTimer(wxTimerEvent& Event);
		void OnRefreshGui(wxCommandEvent& Event);
		void OnScanDirectoryButtonClicked(wxCommandEvent& Event);
		void OnScanFileButtonClicked(wxCommandEvent& Event);
		void OnAddManuallyButtonClicked(wxCommandEvent& Event);
		void OnLoadBankMapButtonClicked(wxCommandEvent& Event);
		void OnClearButtonClicked(wxCommandEvent& Event);
		void OnNextButtonClicked(wxCommandEvent& Event);
		void OnSelectAllButtonClicked(wxCommandEvent& Event);
		void OnSelectNoneButtonClicked(wxCommandEvent& Event);
		void OnSelectAllFilesButtonClicked(wxCommandEvent& Event);
		void OnDuplicateButtonClicked(wxCommandEvent& Event);
		void OnRemoveButtonClicked(wxCommandEvent& Event);
		void OnSetInfoButtonClicked(wxCommandEvent& Event);
		void OnMixLayersButtonClicked(wxCommandEvent& Event);
		void OnUnmixLayersButtonClicked(wxCommandEvent& Event);
		void OnPlayButtonClicked(wxCommandEvent& Event);
		void OnLoopButtonClicked(wxCommandEvent& Event);
		void OnStopButtonClicked(wxCommandEvent& Event);

		void OnSaveAsButtonClicked(wxCommandEvent& Event);
		void OnSaveToWavButtonClicked(wxCommandEvent& Event);
		void OnSpliceToWavButtonClicked(wxCommandEvent& Event);
		void OnExtractAsButtonClicked(wxCommandEvent& Event);

		void OnFileListSelChange(wxListEvent& Event);
		void OnSegmentListSelChange(wxListEvent& Event);
		void OnSegmentListItemActivated(wxListEvent& Event);

		virtual void Play(bool Looping=false);
		virtual bool IsPlaying() const;
		virtual void Stop();
	};
};
