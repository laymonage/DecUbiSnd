/*
 Pch.h : The precompiled header, shared by the CLI and the GUI
*/

#pragma once

#include <tchar.h>
#include <ios>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include <memory.h>
#include <stdlib.h>

#ifdef __WXMSW__
#include <wx/wxprec.h>
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
