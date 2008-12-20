/*
 App.h : The application
*/

#pragma once
#include <wx/app.h>

namespace NDecGui
{
	class CApp : public wxApp
	{
	public:
		virtual bool OnInit();
		virtual int OnExit();
	};
}

DECLARE_APP(NDecGui::CApp);
