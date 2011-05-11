/******************************************************************************
/ SnM_Dlg.cpp
/
/ Copyright (c) 2009-2011 Tim Payne (SWS), Jeffos
/ http://www.standingwaterstudios.com/reaper
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/ 
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/ 
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

#include "stdafx.h"
#include "SnM_Actions.h"


///////////////////////////////////////////////////////////////////////////////
// WDL UIs
///////////////////////////////////////////////////////////////////////////////

LICE_CachedFont* SNM_GetThemeFont()
{
	static LICE_CachedFont themeFont;
	int sz;
	ColorTheme* ct = (ColorTheme*)GetColorThemeStruct(&sz);
	if (!themeFont.GetHFont())
	{
		LOGFONT lf = {
			14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,SWSDLG_TYPEFACE
		};
		if (ct) 
			lf = ct->mediaitem_font;
		themeFont.SetFromHFont(CreateFontIndirect(&lf),LICE_FONT_FLAG_OWNS_HFONT);                 
	}
	themeFont.SetBkMode(TRANSPARENT);
	if (ct)	
		themeFont.SetTextColor(LICE_RGBA_FROMNATIVE(ct->main_text,255));
	else 
		themeFont.SetTextColor(LICE_RGBA(255,255,255,255));
	return &themeFont;
}


///////////////////////////////////////////////////////////////////////////////
// SNM_FastListView
// "Brutal force" update to make list views' updates run much faster, but 
// subbtle thing like selection restorations aren't managed anymore..
///////////////////////////////////////////////////////////////////////////////

void SNM_FastListView::Update()
{
	// Fill in the data by pulling it from the derived class
	if (m_iEditingItem == -1 && !m_bDisableUpdates)
	{
		m_bDisableUpdates = true;
		char str[256];

		WDL_TypedBuf<LPARAM> items;
		GetItemList(&items);

/*JFB original code
		// Check for deletions - items in the lstwnd are quite likely out of order so gotta do a full O(n^2) search
		int lvItemCount = ListView_GetItemCount(m_hwndList);
		for (int i = 0; i < lvItemCount; i++)
		{
			LPARAM item = GetListItem(i);
			bool bFound = false;
			for (int j = 0; j < items.GetSize(); j++)
				if (items.Get()[j] == item)
				{
					bFound = true;
					break;
				}

			if (!bFound)
			{
				ListView_DeleteItem(m_hwndList, i--);
				lvItemCount--;
			}
		}

		// Check for additions
		lvItemCount = ListView_GetItemCount(m_hwndList);
		for (int i = 0; i < items.GetSize(); i++)
		{
			bool bFound = false;
			int j;
			for (j = 0; j < lvItemCount; j++)
			{
				if (items.Get()[i] == GetListItem(j))
				{
					bFound = true;
					break;
				}
			}

			// Update the list, no matter what, because text may have changed
			LVITEM item;
			item.mask = 0;
			int iNewState = GetItemState(items.Get()[i]);
			if (iNewState >= 0)
			{
				int iCurState = bFound ? ListView_GetItemState(m_hwndList, j, LVIS_SELECTED | LVIS_FOCUSED) : 0;
				if (iNewState && !(iCurState & LVIS_SELECTED))
				{
					item.mask |= LVIF_STATE;
					item.state = LVIS_SELECTED;
					item.stateMask = LVIS_SELECTED;
				}
				else if (!iNewState && (iCurState & LVIS_SELECTED))
				{
					item.mask |= LVIF_STATE;
					item.state = 0;
					item.stateMask = LVIS_SELECTED | ((iCurState & LVIS_FOCUSED) ? LVIS_FOCUSED : 0);
				}
			}

			item.iItem = j;
			item.pszText = str;

			int iCol = 0;
			for (int k = 0; k < m_iCols; k++)
				if (m_pCols[k].iPos != -1)
				{
					item.iSubItem = iCol;
					GetItemText(items.Get()[i], k, str, 256);
					if (!iCol && !bFound)
					{
						item.mask |= LVIF_PARAM | LVIF_TEXT;
						item.lParam = items.Get()[i];
						ListView_InsertItem(m_hwndList, &item);
						lvItemCount++;
					}
					else
					{
						char curStr[256];
						ListView_GetItemText(m_hwndList, j, iCol, curStr, 256);
						if (strcmp(str, curStr))
							item.mask |= LVIF_TEXT;
						if (item.mask)
						{
							// Only set if there's changes
							// May be less efficient here, but less messages get sent for sure!
							ListView_SetItem(m_hwndList, &item);
						}
					}
					item.mask = 0;
					iCol++;
				}
		}
*/

//JFB mod -------------------------------------------------------------------->
		ListView_DeleteAllItems(m_hwndList);
		for (int i = 0; i < items.GetSize(); i++)
		{
			LVITEM item;
			item.mask = 0;
			item.iItem = i;
			item.pszText = str;

			int iCol = 0;
			for (int k = 0; k < m_iCols; k++)
				if (m_pCols[k].iPos != -1)
				{
					item.iSubItem = iCol;
					GetItemText(items.Get()[i], k, str, 256);
					if (!iCol)
					{
						item.mask |= LVIF_PARAM | LVIF_TEXT;
						item.lParam = items.Get()[i];
						ListView_InsertItem(m_hwndList, &item);
					}
					else
					{
						item.mask |= LVIF_TEXT;
						ListView_SetItem(m_hwndList, &item);
					}
					item.mask = 0;
					iCol++;
				}
		}
//JFB mod <--------------------------------------------------------------------

		ListView_SortItems(m_hwndList, sListCompare, (LPARAM)this);
		int iCol = abs(m_iSortCol) - 1;
		iCol = DataToDisplayCol(iCol) + 1;
		if (m_iSortCol < 0)
			iCol = -iCol;
		SetListviewColumnArrows(iCol);

#ifdef _WIN32
		if (m_hwndTooltip)
		{
			TOOLINFO ti = { sizeof(TOOLINFO), };
			ti.lpszText = str;
			ti.hwnd = m_hwndList;
			ti.uFlags = TTF_SUBCLASS;
			ti.hinst  = g_hInst;

			// Delete all existing tools
			while (SendMessage(m_hwndTooltip, TTM_ENUMTOOLS, 0, (LPARAM)&ti))
				SendMessage(m_hwndTooltip, TTM_DELTOOL, 0, (LPARAM)&ti);

			RECT r;
			// Add tooltips after sort
			for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
			{
				// Get the rect of the line
				ListView_GetItemRect(m_hwndList, i, &r, LVIR_BOUNDS);
				memcpy(&ti.rect, &r, sizeof(RECT));
				ti.uId = i;
				GetItemTooltip(GetListItem(i), str, 100);
				SendMessage(m_hwndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
			}
		}
#endif
		m_bDisableUpdates = false;
	}
}


///////////////////////////////////////////////////////////////////////////////
// Cue buss dialog box
///////////////////////////////////////////////////////////////////////////////

int GetComboSendIdxType(int _reaType) 
{
	switch(_reaType)
	{
		case 0: return 1;
		case 3: return 2; 
		case 1: return 3; 
		default: return 1;
	}
	return 1; // in case _reaType comes from mars
}

const char* GetSendTypeStr(int _type) 
{
	switch(_type)
	{
		case 1: return "Post-Fader (Post-Pan)";
		case 2: return "Pre-Fader (Post-FX)";
		case 3: return "Pre-FX";
		default: return NULL;
	}
}

void fillHWoutDropDown(HWND _hwnd, int _idc)
{
	int x=0, x0=0;
	char buffer[BUFFER_SIZE] = "<None>";
	x0 = (int)SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)buffer);
	SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,x0,0);
	
	// get mono outputs
	WDL_PtrList<WDL_String> monos;
	int monoIdx=0;
	while (GetOutputChannelName(monoIdx))
	{
		monos.Add(new WDL_String(GetOutputChannelName(monoIdx)));
		monoIdx++;
	}

	// add stereo outputs
	WDL_PtrList<WDL_String> stereos;
	if (monoIdx)
	{
		for(int i=0; i < (monoIdx-1); i++)
		{
			WDL_String* hw = new WDL_String();
			hw->SetFormatted(256, "%s / %s", monos.Get(i)->Get(), monos.Get(i+1)->Get());
			stereos.Add(hw);
		}
	}

	// fill dropdown
	for(int i=0; i < stereos.GetSize(); i++)
	{
		x = (int)SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)stereos.Get(i)->Get());
		SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,x,i+1); // +1 for <none>
	}
	for(int i=0; i < monos.GetSize(); i++)
	{
		x = (int)SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)monos.Get(i)->Get());
		SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,x,i+1); // +1 for <none>
	}

//	SendDlgItemMessage(_hwnd,_idc,CB_SETCURSEL,x0,0);
	monos.Empty(true);
	stereos.Empty(true);
}

HWND g_cueBussHwnd = NULL;

WDL_DLGRET CueBussDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	const char cWndPosKey[] = "CueBus Window Pos"; 
	switch(Message)
	{
        case WM_INITDIALOG :
		{
			char busName[BUFFER_SIZE] = "";
			char trTemplatePath[BUFFER_SIZE] = "";
			int reaType, userType, soloDefeat, hwOuts[8];
			bool trTemplate, showRouting, sendToMaster;
			readCueBusIniFile(busName, &reaType, &trTemplate, trTemplatePath, &showRouting, &soloDefeat, &sendToMaster, hwOuts);
			userType = GetComboSendIdxType(reaType);
			SetDlgItemText(hwnd,IDC_SNM_CUEBUS_NAME,busName);

			int x=0;
			for(int i=1; i<4; i++)
			{
				x = (int)SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_ADDSTRING,0,(LPARAM)GetSendTypeStr(i));
				SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_SETITEMDATA,x,i);
				if (i==userType) SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_SETCURSEL,x,0);
			}

			SetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,trTemplatePath);
			CheckDlgButton(hwnd, IDC_CHECK1, sendToMaster);
			CheckDlgButton(hwnd, IDC_CHECK2, showRouting);
			CheckDlgButton(hwnd, IDC_CHECK3, trTemplate);
			CheckDlgButton(hwnd, IDC_CHECK4, (soloDefeat == 1));

			for(int i=0; i < SNM_MAX_HW_OUTS; i++) 
			{
				fillHWoutDropDown(hwnd,IDC_SNM_CUEBUS_HWOUT1+i);
				SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_HWOUT1+i,CB_SETCURSEL,hwOuts[i],0);
			}

			RestoreWindowPos(hwnd, cWndPosKey, false);
			SetFocus(GetDlgItem(hwnd, IDC_SNM_CUEBUS_NAME));
			PostMessage(hwnd, WM_COMMAND, IDC_CHECK3, 0); // enable//disable state
			return 0;
		}
		break;

		case WM_CLOSE :
			g_cueBussHwnd = NULL; // for proper toggle state report, see openCueBussWnd()
			break;

		case WM_COMMAND :
		{
            switch(LOWORD(wParam))
            {
                case IDOK:
				case IDC_SAVE:
				{
					char cueBusName[BUFFER_SIZE];
					GetDlgItemText(hwnd,IDC_SNM_CUEBUS_NAME,cueBusName,BUFFER_SIZE);

					int userType = 2, reaType;
					int combo = (int)SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						userType = combo+1;
					switch(userType)
					{
						case 1: reaType=0; break;
						case 2: reaType=3; break;
						case 3: reaType=1; break;
						default: break;
					}

					int sendToMaster = IsDlgButtonChecked(hwnd, IDC_CHECK1);
					int showRouting = IsDlgButtonChecked(hwnd, IDC_CHECK2);
					int trTemplate = IsDlgButtonChecked(hwnd, IDC_CHECK3);
					int soloDefeat = IsDlgButtonChecked(hwnd, IDC_CHECK4);

					char trTemplatePath[BUFFER_SIZE];
					GetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,trTemplatePath,BUFFER_SIZE);

					int hwOuts[SNM_MAX_HW_OUTS];
					for (int i=0; i<SNM_MAX_HW_OUTS; i++)
					{
						hwOuts[i] = (int)SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_HWOUT1+i,CB_GETCURSEL,0,0);
						if(hwOuts[i] == CB_ERR)	hwOuts[i] = 0;
					}

					// *** Create cue buss ***
					if (LOWORD(wParam) == IDC_SAVE ||
						cueTrack(cueBusName, reaType, "Create cue buss",
							(showRouting == 1), soloDefeat, 
							trTemplate ? trTemplatePath : NULL, 
							(sendToMaster == 1), hwOuts))
					{
						saveCueBusIniFile(cueBusName, reaType, (trTemplate == 1), trTemplatePath, (showRouting == 1), soloDefeat, (sendToMaster == 1), hwOuts);
					}
					return 0;
				}
				break;

				case IDCANCEL:
				{
					ShowWindow(hwnd, SW_HIDE);
					return 0;
				}
				break;

				case IDC_FILES:
				{
					char currentPath[BUFFER_SIZE];
					GetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,currentPath,BUFFER_SIZE);
					if (!strlen(currentPath))
						_snprintf(currentPath, BUFFER_SIZE, "%s%c%TrackTemplates", GetResourcePath(), PATH_SLASH_CHAR);
					char* filename = BrowseForFiles("Load track template", currentPath, NULL, false, "REAPER Track Template (*.RTrackTemplate)\0*.RTrackTemplate\0");
					if (filename) {
						SetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,filename);
						free(filename);
					}
				}
				break;

				case IDC_CHECK3:
				{
					bool templateEnable = (IsDlgButtonChecked(hwnd, IDC_CHECK3) == 1);
					EnableWindow(GetDlgItem(hwnd, IDC_SNM_CUEBUS_TEMPLATE), templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_FILES), templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_SNM_CUEBUS_NAME), !templateEnable);
					for(int k=0; k < SNM_MAX_HW_OUTS ; k++)
						EnableWindow(GetDlgItem(hwnd, IDC_SNM_CUEBUS_HWOUT1+k), !templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECK1), !templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECK4), !templateEnable);
					SetFocus(GetDlgItem(hwnd, templateEnable ? IDC_SNM_CUEBUS_TEMPLATE : IDC_SNM_CUEBUS_NAME));
				}
				break;
			}
		}
		break;

		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break; 
	}

	return 0;
}

void openCueBussWnd(COMMAND_T* _ct) {
	static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_SNM_CUEBUS), g_hwndParent, CueBussDlgProc);

	// Toggle
	if (g_cueBussHwnd) {
		g_cueBussHwnd = NULL;
		ShowWindow(hwnd, SW_HIDE);
	}
	else {
		g_cueBussHwnd = hwnd;
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
	}
}

bool isCueBussWndDisplayed(COMMAND_T* _ct) {
	return (g_cueBussHwnd && IsWindow(g_cueBussHwnd) && IsWindowVisible(g_cueBussHwnd) ? true : false);
}


///////////////////////////////////////////////////////////////////////////////
// WaitDlgProc
///////////////////////////////////////////////////////////////////////////////

int g_waitDlgProcCount = 0;

WDL_DLGRET WaitDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITDIALOG:
			SetTimer(hwnd, 1, 1, NULL);
			break;
/*
		case WM_COMMAND:
			if ((LOWORD(wParam)==IDOK || LOWORD(wParam)==IDCANCEL))
			{
				EndDialog(hwnd,0);
				g_waitDlgProcCount = 0;
			}
			break;
*/
		case WM_TIMER:
			{
				SendDlgItemMessage(hwnd, IDC_EDIT, PBM_SETRANGE, 0, MAKELPARAM(0, SNM_LET_BREATHE_MS));
				if (g_waitDlgProcCount < SNM_LET_BREATHE_MS)
				{
					SendDlgItemMessage(hwnd, IDC_EDIT, PBM_SETPOS, (WPARAM) g_waitDlgProcCount, 0);
					g_waitDlgProcCount++;
				}
				else
				{
					EndDialog(hwnd,0);
					g_waitDlgProcCount = 0;
				}
			}
			break;
	}
	return 0;
}

