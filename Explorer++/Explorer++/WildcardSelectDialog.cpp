// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WildcardSelectDialog.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"

const TCHAR WildcardSelectDialogPersistentSettings::SETTINGS_KEY[] = _T("WildcardSelect");

const TCHAR WildcardSelectDialogPersistentSettings::SETTING_PATTERN_LIST[] = _T("Pattern");
const TCHAR WildcardSelectDialogPersistentSettings::SETTING_CURRENT_TEXT[] = _T("CurrentText");

WildcardSelectDialog::WildcardSelectDialog(HINSTANCE hInstance, HWND hParent, BOOL bSelect,
	IExplorerplusplus *pexpp) :
	DarkModeDialogBase(hInstance, IDD_WILDCARDSELECT, hParent, true)
{
	m_bSelect = bSelect;
	m_pexpp = pexpp;

	m_pwsdps = &WildcardSelectDialogPersistentSettings::GetInstance();
}

INT_PTR WildcardSelectDialog::OnInitDialog()
{
	m_icon.reset(LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_MAIN)));
	SetClassLongPtr(m_hDlg, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(m_icon.get()));

	HWND hComboBox = GetDlgItem(m_hDlg, IDC_SELECTGROUP_COMBOBOX);

	for (const auto &strPattern : m_pwsdps->m_PatternList)
	{
		ComboBox_InsertString(hComboBox, -1, strPattern.c_str());
	}

	ComboBox_SetText(hComboBox, m_pwsdps->m_szPattern);

	if (!m_bSelect)
	{
		std::wstring deselectTitle =
			ResourceHelper::LoadString(GetInstance(), IDS_WILDCARDDESELECTION);
		SetWindowText(m_hDlg, deselectTitle.c_str());
	}

	SetFocus(hComboBox);

	AllowDarkModeForComboBoxes({ IDC_SELECTGROUP_COMBOBOX });

	m_pwsdps->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

void WildcardSelectDialog::GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc,
	std::list<ResizableDialog::Control> &controlList)
{
	dsc = BaseDialog::DialogSizeConstraint::X;

	ResizableDialog::Control control;

	control.iID = IDC_SELECTGROUP_COMBOBOX;
	control.Type = ResizableDialog::ControlType::Resize;
	control.Constraint = ResizableDialog::ControlConstraint::X;
	controlList.push_back(control);

	control.iID = IDOK;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::None;
	controlList.push_back(control);

	control.iID = IDCANCEL;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::None;
	controlList.push_back(control);
}

INT_PTR WildcardSelectDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDOK:
		OnOk();
		break;

	case IDCANCEL:
		OnCancel();
		break;
	}

	return 0;
}

void WildcardSelectDialog::OnOk()
{
	TCHAR szPattern[512];

	GetDlgItemText(m_hDlg, IDC_SELECTGROUP_COMBOBOX, szPattern, SIZEOF_ARRAY(szPattern));

	if (lstrcmp(szPattern, EMPTY_STRING) != 0)
	{
		SelectItems(szPattern);

		bool bStorePattern = true;

		/* If the current text isn't the same as the
		most recent text (if any), add it to the history
		list. */
		auto itr = m_pwsdps->m_PatternList.begin();

		if (itr != m_pwsdps->m_PatternList.end())
		{
			if (lstrcmp(itr->c_str(), szPattern) == 0)
			{
				bStorePattern = false;
			}
		}

		if (bStorePattern)
		{
			m_pwsdps->m_PatternList.push_front(szPattern);
		}
	}

	EndDialog(m_hDlg, 1);
}

void WildcardSelectDialog::SelectItems(TCHAR *szPattern)
{
	HWND hListView = m_pexpp->GetActiveListView();

	int nItems = ListView_GetItemCount(hListView);

	for (int i = 0; i < nItems; i++)
	{
		std::wstring filename = m_pexpp->GetActiveShellBrowser()->GetItemName(i);

		if (CheckWildcardMatch(szPattern, filename.c_str(), FALSE) == 1)
		{
			ListViewHelper::SelectItem(hListView, i, m_bSelect);
		}
	}
}

void WildcardSelectDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

INT_PTR WildcardSelectDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void WildcardSelectDialog::SaveState()
{
	m_pwsdps->SaveDialogPosition(m_hDlg);

	HWND hComboBox = GetDlgItem(m_hDlg, IDC_SELECTGROUP_COMBOBOX);
	ComboBox_GetText(hComboBox, m_pwsdps->m_szPattern, SIZEOF_ARRAY(m_pwsdps->m_szPattern));

	m_pwsdps->m_bStateSaved = TRUE;
}

WildcardSelectDialogPersistentSettings::WildcardSelectDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
	StringCchCopy(m_szPattern, SIZEOF_ARRAY(m_szPattern), EMPTY_STRING);
}

WildcardSelectDialogPersistentSettings &WildcardSelectDialogPersistentSettings::GetInstance()
{
	static WildcardSelectDialogPersistentSettings wsdps;
	return wsdps;
}

void WildcardSelectDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::SaveStringList(hKey, SETTING_PATTERN_LIST, m_PatternList);
	RegistrySettings::SaveString(hKey, SETTING_CURRENT_TEXT, m_szPattern);
}

void WildcardSelectDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::ReadStringList(hKey, SETTING_PATTERN_LIST, m_PatternList);
	RegistrySettings::ReadString(hKey, SETTING_CURRENT_TEXT, m_szPattern,
		SIZEOF_ARRAY(m_szPattern));
}

void WildcardSelectDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddStringListToNode(pXMLDom, pParentNode, SETTING_PATTERN_LIST, m_PatternList);
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_CURRENT_TEXT, m_szPattern);
}

void WildcardSelectDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue)
{
	if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, bstrName, lstrlen(SETTING_PATTERN_LIST),
			SETTING_PATTERN_LIST, lstrlen(SETTING_PATTERN_LIST))
		== CSTR_EQUAL)
	{
		m_PatternList.emplace_back(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_CURRENT_TEXT) == 0)
	{
		StringCchCopy(m_szPattern, SIZEOF_ARRAY(m_szPattern), bstrValue);
	}
}
