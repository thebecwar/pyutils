#include "Stdafx.h"
#ifdef ENABLE_WIN8
#include "ProcessSnap.h" // Windows 8.1+
#endif

#pragma once

// WINDOWS 8.1+

#include <Windows.h>
#include <TlHelp32.h>
#include <ProcessSnapshot.h>

#include <Python.h>
#include <structmember.h>
#include <vector>

#include "Process.h" // Helpers for process wrangling
#include "NamedTuple.h"

typedef DWORD(*fp_pss_capture)(HANDLE, PSS_CAPTURE_FLAGS, DWORD, HPSS*);
typedef DWORD(*fp_pss_walk_create)(PSS_ALLOCATOR*, HPSSWALK*);
typedef DWORD(*fp_pss_walk_free)(HPSSWALK);
typedef DWORD(*fp_pss_walk)(HPSS, PSS_WALK_INFORMATION_CLASS, HPSSWALK, void*, DWORD);
typedef DWORD(*fp_pss_free)(HANDLE, HPSS);

HMODULE hmodulePss = NULL;
fp_pss_capture pss_capture = NULL;
fp_pss_walk_create pss_walk_create = NULL;
fp_pss_walk_free pss_walk_free = NULL;
fp_pss_walk pss_walk = NULL;
fp_pss_free pss_free = NULL;
bool pssAvailable = false;
bool pssInitialized = false;
bool TryInit()
{
	if (pssInitialized) return pssAvailable;

	hmodulePss = LoadLibraryW(L"Kernel32.dll");
	pss_capture = (fp_pss_capture)GetProcAddress(hmodulePss, "PssCaptureSnapshot");
	pss_walk_create = (fp_pss_walk_create)GetProcAddress(hmodulePss, "PssWalkMarkerCreate");
	pss_walk_free = (fp_pss_walk_free)GetProcAddress(hmodulePss, "PssWalkMarkerFree");
	pss_walk = (fp_pss_walk)GetProcAddress(hmodulePss, "PssWalkSnapshot");
	pss_free = (fp_pss_free)GetProcAddress(hmodulePss, "PssFreeSnapshot");

	pssAvailable = (pss_capture != NULL) &&
		(pss_walk_create != NULL) &&
		(pss_walk_free != NULL) &&
		(pss_walk != NULL) &&
		(pss_free != NULL);

	pssInitialized = true;
	return pssAvailable;
}


PyStructSequence_Field process_handle_info_fields[] = {
	{ "exit_status", "" },
	{ "base_priority", "" },
	{ "process_id", "" },
	{ "parent_process_id", "" },
	{ NULL, NULL }
};
PyStructSequence_Desc process_handle_info_desc = {
	"ProcessHandleInfo",
	"Information about a process handle",
	process_handle_info_fields,
	4
};
PyStructSequence_Field thread_handle_info_fields[] = {
	{ "exit_status", "" },
	{ "process_id", "" },
	{ "thread_id", "" },
	{ "priority", "" },
	{ "base_priority", "" },
	{ NULL, NULL }
};
PyStructSequence_Desc thread_handle_info_desc = {
	"ThreadHandleInfo",
	"Information about a thread handle",
	thread_handle_info_fields,
	5
};
PyStructSequence_Field mutant_handle_info_fields[] = {
	{ "current_count", "" },
	{ "abandoned", "" },
	{ "owner_process_id", "" },
	{ "owner_thread_id", "" },
	{ NULL, NULL },
};
PyStructSequence_Desc mutant_handle_info_desc = {
	"MutantHandleInfo",
	"Information about a mutant handle",
	mutant_handle_info_fields,
	4
};
PyStructSequence_Field event_handle_info_fields[] = {
	{ "manual_reset", "" },
	{ "signaled", "" },
	{ NULL, NULL },
};
PyStructSequence_Desc event_handle_info_desc = {
	"EventHandleInfo",
	"Information about a event handle",
	event_handle_info_fields,
	2
};
PyStructSequence_Field allocation_handle_info_fields[] = {
	{ "maximum_size", "" },
	{ NULL, NULL },
};
PyStructSequence_Desc allocation_handle_info_desc = {
	"AllocationHandleInfo",
	"Information about an allocation handle",
	allocation_handle_info_fields,
	1
};
PyStructSequence_Field handle_info_fields[] = {
	{ "handle_count", "" },
	{ "pointer_count", "" },
	{ "paged_pool_charge", "" },
	{ "non_paged_pool_chargs", "" },
	{ "creation_time", "" },
	{ "snapshot_time", "" },
	{ "type_name", "" },
	{ "object_name", "" },
	{ "details", "" },
	{ NULL, NULL },
};
PyStructSequence_Desc handle_info_desc = {
	"HandleInfo",
	"Information about a specified handle",
	handle_info_fields,
	9
};

PyTypeObject* process_handle_info_type = NULL;
PyTypeObject* thread_handle_info_type = NULL;
PyTypeObject* mutant_handle_info_type = NULL;
PyTypeObject* event_handle_info_type = NULL;
PyTypeObject* allocation_handle_info_type = NULL;
PyTypeObject* handle_info_type = NULL;

#define INIT_TYPE(tp, desc, name) \
	tp = NamedTuple_NewType(&desc); \
	if (tp != NULL) { \
	Py_INCREF(tp); \
	PyModule_AddObject(targetModule, name, (PyObject*)tp); }

bool processsnapshot_init_types(PyObject* targetModule)
{
	INIT_TYPE(process_handle_info_type, process_handle_info_desc, "ProcessHandleInfo");
	INIT_TYPE(thread_handle_info_type, thread_handle_info_desc, "ThreadHandleInfo");
	INIT_TYPE(mutant_handle_info_type, mutant_handle_info_desc, "MutantHandleInfo");
	INIT_TYPE(event_handle_info_type, event_handle_info_desc, "EventHandleInfo");
	INIT_TYPE(allocation_handle_info_type, allocation_handle_info_desc, "AllocationHandleInfo");
	INIT_TYPE(handle_info_type, handle_info_desc, "HandleInfo");

	return (process_handle_info_type != NULL) &&
		(thread_handle_info_type != NULL) &&
		(mutant_handle_info_type != NULL) &&
		(event_handle_info_type != NULL) &&
		(allocation_handle_info_type != NULL) &&
		(handle_info_type != NULL);
};

#undef INIT_TYPE


typedef struct {
	PyObject_HEAD;
	PyObject* handleList;
} processsnapshot_object;

PyObject* GetFileTimeAsString(LPFILETIME fileTime)
{
	SYSTEMTIME sysTime = { 0 };
	FileTimeToSystemTime(fileTime, &sysTime);

	int cch = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, NULL, NULL, NULL, 0);
	wchar_t* buffer = new wchar_t[cch];
	cch = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, NULL, NULL, buffer, cch);
	PyObject* timestamp = PyUnicode_FromWideChar(buffer, -1);
	delete[] buffer;

	return timestamp;
}

void processsnapshot_dealloc(processsnapshot_object* self)
{
	Py_XDECREF(self->handleList);
	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* processsnapshot_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	if (!TryInit())
	{
		PyErr_SetString(PyExc_WindowsError, "Process Snapshots are only available on Windows 8.1 and above.");
		return NULL;
	}

	PyObject* name = NULL;
	int processId = -1;
	static char* kwlist[] = { "name", "process_id", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Ui", &kwlist[0], &name, &processId))
		return NULL;

	if (name == NULL && processId < 0)
	{
		PyErr_SetString(PyExc_AssertionError, "Either name or process_id must be specified");
	}

	HANDLE hProcess = NULL;
	if (processId > 0)
	{
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
	}
	else
	{
		wchar_t* procName = PyUnicode_AsWideCharString(name, NULL);
		hProcess = OpenProcessByName(procName);
		PyMem_FREE(procName);
	}

	if (hProcess == NULL)
	{
		PyErr_SetString(PyExc_WindowsError, "Couldn't open process.");
		return NULL;
	}

	processsnapshot_object* self = (processsnapshot_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		self->handleList = PyList_New(0);

		HPSS snapshot = NULL;

		DWORD res = pss_capture(hProcess,
			PSS_CAPTURE_HANDLES | PSS_CAPTURE_HANDLE_NAME_INFORMATION | PSS_CAPTURE_HANDLE_TYPE_SPECIFIC_INFORMATION,
			0,
			&snapshot);
		if (res != ERROR_SUCCESS)
		{
			Py_DECREF(self);
			CloseHandle(hProcess);

			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, res);
			return NULL;
		}

		HPSSWALK walkMarker = NULL;
		res = pss_walk_create(NULL, &walkMarker);
		if (res != ERROR_SUCCESS)
		{
			Py_DECREF(self);
			pss_free(hProcess, snapshot);
			CloseHandle(hProcess);

			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, res);
			return NULL;
		}

		PSS_HANDLE_ENTRY entry = { 0 };
		while ((res = pss_walk(snapshot, PSS_WALK_HANDLES, walkMarker, &entry, sizeof(PSS_HANDLE_ENTRY))) != ERROR_NO_MORE_ITEMS)
		{
			PyObject* item = NamedTuple_New(handle_info_type);

			NamedTuple_SetItem(item, 0, PyLong_FromLong(entry.HandleCount));
			NamedTuple_SetItem(item, 1, PyLong_FromLong(entry.PointerCount));
			NamedTuple_SetItem(item, 2, PyLong_FromLong(entry.PagedPoolCharge));
			NamedTuple_SetItem(item, 3, PyLong_FromLong(entry.NonPagedPoolCharge));
			NamedTuple_SetItem(item, 4, GetFileTimeAsString(&entry.CreationTime));
			NamedTuple_SetItem(item, 5, GetFileTimeAsString(&entry.CaptureTime));
			NamedTuple_SetItem(item, 6, PyUnicode_FromWideChar(entry.TypeName, entry.TypeNameLength));
			NamedTuple_SetItem(item, 7, PyUnicode_FromWideChar(entry.ObjectName, entry.ObjectNameLength));

			PyObject* detailObj;
#define SET_LONG(idx, val) NamedTuple_SetItem(detailObj, idx, PyLong_FromLong(val));
#define SET_LONGLONG(idx, val) NamedTuple_SetItem(detailObj, idx, PyLong_FromLongLong(val));
#define SET_BOOL(idx, val) NamedTuple_SetItem(detailObj, idx, PyBool_FromLong(val));
			switch (entry.ObjectType)
			{
			case PSS_OBJECT_TYPE_PROCESS:
				detailObj = NamedTuple_New(process_handle_info_type);
				SET_LONG(0, entry.TypeSpecificInformation.Process.ExitStatus);
				SET_LONG(1, entry.TypeSpecificInformation.Process.BasePriority);
				SET_LONG(2, entry.TypeSpecificInformation.Process.ProcessId);
				SET_LONG(3, entry.TypeSpecificInformation.Process.ParentProcessId);
				break;
			case PSS_OBJECT_TYPE_THREAD:
				detailObj = NamedTuple_New(thread_handle_info_type);
				SET_LONG(0, entry.TypeSpecificInformation.Thread.ExitStatus);
				SET_LONG(1, entry.TypeSpecificInformation.Thread.ProcessId);
				SET_LONG(2, entry.TypeSpecificInformation.Thread.ThreadId);
				SET_LONG(3, entry.TypeSpecificInformation.Thread.Priority);
				SET_LONG(4, entry.TypeSpecificInformation.Thread.BasePriority);
				break;
			case PSS_OBJECT_TYPE_MUTANT:
				detailObj = NamedTuple_New(mutant_handle_info_type);
				SET_LONG(0, entry.TypeSpecificInformation.Mutant.CurrentCount);
				SET_BOOL(1, entry.TypeSpecificInformation.Mutant.Abandoned);
				SET_LONG(2, entry.TypeSpecificInformation.Mutant.OwnerProcessId);
				SET_LONG(3, entry.TypeSpecificInformation.Mutant.OwnerThreadId);
				break;
			case PSS_OBJECT_TYPE_EVENT:
				detailObj = NamedTuple_New(event_handle_info_type);
				SET_BOOL(0, entry.TypeSpecificInformation.Event.ManualReset);
				SET_BOOL(1, entry.TypeSpecificInformation.Event.Signaled);
				break;
			case PSS_OBJECT_TYPE_SECTION:
				detailObj = NamedTuple_New(allocation_handle_info_type);
				SET_LONGLONG(0, entry.TypeSpecificInformation.Section.MaximumSize.QuadPart);
				break;
			default:
				Py_INCREF(Py_None);
				detailObj = Py_None;
				break;
			}

#undef SET_LONG
#undef SET_LONGLONG
#undef SET_BOOL

			NamedTuple_SetItem(item, 8, detailObj);

			PyList_Append(self->handleList, item);
		}

		pss_walk_free(walkMarker);
		pss_free(hProcess, snapshot);
	}

	CloseHandle(hProcess);

	return (PyObject*)self;
}
int processsnapshot_init(processsnapshot_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyObject* processsnapshot_isavailable(PyObject*)
{
	if (TryInit())
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}
PyMethodDef processsnapshot_methods[] = {
	{ "is_available", (PyCFunction)processsnapshot_isavailable, METH_NOARGS | METH_CLASS, "Checks to see if process snapshotting is available" },
	{ NULL, NULL, 0, NULL },
};
PyMemberDef processsnapshot_members[] = {
	{ "handles", T_OBJECT, offsetof(processsnapshot_object, handleList), READONLY, "Process' open handles." },
	{ NULL }
};
PyTypeObject processsnapshot_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.ProcessSnapshot",				/* tp_name */
	sizeof(processsnapshot_object),		/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)processsnapshot_dealloc,	/* tp_dealloc */
	0,										/* tp_print */
	0,										/* tp_getattr */
	0,										/* tp_setattr */
	0,										/* tp_reserved */
	0,										/* tp_repr */
	0,										/* tp_as_number */
	0,										/* tp_as_sequence */
	0,										/* tp_as_mapping */
	0,										/* tp_hash  */
	0,										/* tp_call */
	0,										/* tp_str */
	0,										/* tp_getattro */
	0,										/* tp_setattro */
	0,										/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |
	Py_TPFLAGS_BASETYPE,					/* tp_flags */
	"Windows API Process Snapshot",			/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	processsnapshot_methods,				/* tp_methods */
	processsnapshot_members,				/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)processsnapshot_init,			/* tp_init */
	0,										/* tp_alloc */
	processsnapshot_new,					/* tp_new */
};