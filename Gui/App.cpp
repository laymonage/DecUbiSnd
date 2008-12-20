/*
 App.cpp : The application
*/

#include "Pch.h"

#include "Gui/App.h"
#include "Gui/MainDialog.h"

IMPLEMENT_APP(NDecGui::CApp);

bool NDecGui::CApp::OnInit()
{
	// Create the main window
	CMainDialog* Main;
	Main=new CMainDialog(NULL);

	// Show it
	SetTopWindow(Main);
	Main->ShowModal();

	// Just exit
	return false;
}

int NDecGui::CApp::OnExit()
{
	return 0;
}
