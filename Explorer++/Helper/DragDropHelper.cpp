// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DragDropHelper.h"
#include "DataObjectWrapper.h"
#include "Macros.h"
#include <wil/com.h>
#include <winrt/base.h>

STGMEDIUM GetStgMediumForGlobal(HGLOBAL global)
{
	STGMEDIUM storage;
	storage.tymed = TYMED_HGLOBAL;
	storage.hGlobal = global;
	storage.pUnkForRelease = nullptr;
	return storage;
}

STGMEDIUM GetStgMediumForStream(IStream *stream)
{
	STGMEDIUM storage;
	storage.tymed = TYMED_ISTREAM;
	storage.pstm = stream;
	storage.pUnkForRelease = nullptr;
	return storage;
}

HRESULT SetPreferredDropEffect(IDataObject *dataObject, DWORD effect)
{
	return SetBlobData(dataObject,
		static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT)), effect);
}

HRESULT SetDropDescription(IDataObject *dataObject, DROPIMAGETYPE type, const std::wstring &message,
	const std::wstring &insert)
{
	DROPDESCRIPTION dropDescription;
	dropDescription.type = type;
	StringCchCopy(dropDescription.szMessage, SIZEOF_ARRAY(dropDescription.szMessage),
		message.c_str());
	StringCchCopy(dropDescription.szInsert, SIZEOF_ARRAY(dropDescription.szInsert), insert.c_str());

	return SetBlobData(dataObject,
		static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_DROPDESCRIPTION)), dropDescription);
}

// Returns an IDataObject instance that can be used for clipboard operations and drag and drop.
HRESULT CreateDataObjectForShellTransfer(const std::vector<PCIDLIST_ABSOLUTE> &items,
	IDataObject **dataObjectOut)
{
	wil::com_ptr_nothrow<IShellItemArray> shellItemArray;
	RETURN_IF_FAILED(SHCreateShellItemArrayFromIDLists(static_cast<UINT>(items.size()),
		items.data(), &shellItemArray));

	wil::com_ptr_nothrow<IDataObject> shellDataObject;
	RETURN_IF_FAILED(
		shellItemArray->BindToHandler(nullptr, BHID_DataObject, IID_PPV_ARGS(&shellDataObject)));

	// Although it's possible to retrieve the IDataObjectAsyncCapability interface from the shell
	// IDataObject instance and call SetAsyncMode(), it appears that doesn't actually enable
	// asynchronous transfer. That's the reason the shell IDataObject instance is wrapped here.
	auto dataObject = winrt::make<DataObjectWrapper>(shellDataObject.get());

	wil::com_ptr_nothrow<IDataObjectAsyncCapability> asyncCapability;
	RETURN_IF_FAILED(dataObject->QueryInterface(IID_PPV_ARGS(&asyncCapability)));
	RETURN_IF_FAILED(asyncCapability->SetAsyncMode(TRUE));

	*dataObjectOut = dataObject.detach();

	return S_OK;
}
