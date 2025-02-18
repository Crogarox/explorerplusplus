// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Navigation.h"
#include "CoreInterface.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainer.h"
#include "../Helper/ShellHelper.h"

Navigation::Navigation(IExplorerplusplus *expp) : m_expp(expp), m_tabContainer(nullptr)
{
	m_expp->AddTabsInitializedObserver(
		[this]
		{
			m_tabContainer = m_expp->GetTabContainer();
		});
}

void Navigation::OnNavigateUp()
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	unique_pidl_absolute directory = tab.GetShellBrowser()->GetDirectoryIdl();

	HRESULT hr = E_FAIL;
	int resultingTabId = -1;

	if (tab.GetLockState() != Tab::LockState::AddressLocked)
	{
		hr = tab.GetShellBrowser()->GetNavigationController()->GoUp();

		resultingTabId = tab.GetId();
	}
	else
	{
		unique_pidl_absolute pidlParent;
		hr = GetVirtualParentPath(tab.GetShellBrowser()->GetDirectoryIdl().get(),
			wil::out_param(pidlParent));

		if (SUCCEEDED(hr))
		{
			m_tabContainer->CreateNewTab(pidlParent.get(), TabSettings(_selected = true), nullptr,
				nullptr, &resultingTabId);
		}
	}

	if (SUCCEEDED(hr))
	{
		const Tab &resultingTab = m_tabContainer->GetTab(resultingTabId);
		resultingTab.GetShellBrowser()->SelectItems({ directory.get() });
	}
}

HRESULT Navigation::BrowseFolderInCurrentTab(const TCHAR *szPath)
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	return tab.GetShellBrowser()->GetNavigationController()->BrowseFolder(szPath);
}

HRESULT Navigation::BrowseFolderInCurrentTab(PCIDLIST_ABSOLUTE pidlDirectory)
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	return tab.GetShellBrowser()->GetNavigationController()->BrowseFolder(pidlDirectory);
}

void Navigation::OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory)
{
	/* Create a new instance of this program, with the
	specified path as an argument. */
	std::wstring path;
	GetDisplayName(pidlDirectory, SHGDN_FORPARSING, path);

	TCHAR szParameters[512];
	StringCchPrintf(szParameters, SIZEOF_ARRAY(szParameters), _T("\"%s\""), path.c_str());

	ExecuteAndShowCurrentProcess(m_expp->GetMainWindow(), szParameters);
}
