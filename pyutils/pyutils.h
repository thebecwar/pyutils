// pyutils.h

#pragma once

#include <Windows.h>

#include <Python.h>

#include "WinApiMutex.h"
#include "WinApiEvent.h"
#include "MemoryMappedFile.h"
#include "WinApiSemaphore.h"
#include "PerformanceCounter.h"
#include "AutomationCondition.h"
#include "UIAutomation.h"
#include "Process.h"
#include "ExeInfo.h"

#ifdef ENABLE_WIN8
#include "ProcessSnap.h" // Windows 8.1+ only
#endif

#pragma region Python Interfaces

static PyObject* pyutils_getpid(PyObject* self, PyObject* args)
{
	PyObject* procName = NULL;
	if (!PyArg_ParseTuple(args, "U", &procName))
		return NULL;

	wchar_t* szProcess = PyUnicode_AsWideCharString(procName, NULL);
	DWORD result = GetProcessIdByName(szProcess);
	PyMem_FREE(szProcess);

	return PyLong_FromLong(result);
}
static PyObject* pyutils_gethandles(PyObject* self, PyObject* args)
{
	HANDLE hProcess = ParseProcessArgs(args);
	if (hProcess == NULL)
		return NULL;

	PyObject* result = NULL;

	DWORD count = 0;
	if (!GetProcessHandleCount(hProcess, &count))
	{
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
	}
	else
	{
		result = PyLong_FromLong(count);
	}
	CloseHandle(hProcess);
	return result;
}
static PyObject* pyutils_getgdiobjects(PyObject* self, PyObject* args)
{
	HANDLE hProcess = ParseProcessArgs(args);
	if (hProcess == NULL)
		return NULL;

	DWORD count = GetGuiResources(hProcess, GR_GDIOBJECTS);
	CloseHandle(hProcess);
	return PyLong_FromLong(count);
}
static PyObject* pyutils_getgdiobjectspeak(PyObject* self, PyObject* args)
{
	HANDLE hProcess = ParseProcessArgs(args);
	if (hProcess == NULL)
		return NULL;

	DWORD count = GetGuiResources(hProcess, GR_GDIOBJECTS_PEAK);
	CloseHandle(hProcess);
	return PyLong_FromLong(count);
}
static PyObject* pyutils_getuserobjects(PyObject* self, PyObject* args)
{
	HANDLE hProcess = ParseProcessArgs(args);
	if (hProcess == NULL)
		return NULL;

	DWORD count = GetGuiResources(hProcess, GR_USEROBJECTS);
	CloseHandle(hProcess);
	return PyLong_FromLong(count);
}
static PyObject* pyutils_getuserobjectspeak(PyObject* self, PyObject* args)
{
	HANDLE hProcess = ParseProcessArgs(args);
	if (hProcess == NULL)
		return NULL;

	DWORD count = GetGuiResources(hProcess, GR_USEROBJECTS_PEAK);
	CloseHandle(hProcess);
	return PyLong_FromLong(count);
}
static PyObject* pyutils_getclipboard(PyObject* self, PyObject* args)
{
	bool unicode = true;
	if (!PyArg_ParseTuple(args, "|p", &unicode))
		return NULL;


	PyObject* result = NULL;
	
	if (unicode && IsClipboardFormatAvailable(CF_UNICODETEXT))
	{
		OpenClipboard(NULL);
		HANDLE hGlob = GetClipboardData(CF_UNICODETEXT);
		wchar_t* data = (wchar_t*)GlobalLock(hGlob);
		result = PyUnicode_FromWideChar(data, -1);
		GlobalUnlock(hGlob);
		CloseClipboard();
	}
	else if (!unicode && IsClipboardFormatAvailable(CF_TEXT))
	{
		OpenClipboard(NULL);
		HANDLE hGlob = GetClipboardData(CF_TEXT);
		char* data = (char*)GlobalLock(hGlob);
		result = PyUnicode_DecodeASCII(data, strlen(data), "?");
		GlobalUnlock(hGlob);
		CloseClipboard();
	}
	else
	{
		result = Py_None;
		Py_INCREF(result);
	}
	return result;
}
static PyObject* pyutils_setclipboard(PyObject* self, PyObject* args)
{
	PyObject* text = NULL;
	if (!PyArg_ParseTuple(args, "U", &text))
		return NULL;

	OpenClipboard(NULL);
	Py_ssize_t len = 0;
	wchar_t* szText = PyUnicode_AsWideCharString(text, &len);
	len = len + 1;

	HANDLE hGlob = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(wchar_t));
	LPVOID target = GlobalLock(hGlob);
	
	memcpy_s(target, len * sizeof(wchar_t), szText, len * sizeof(wchar_t));
	
	GlobalUnlock(hGlob);
	PyMem_FREE(szText);
	
	SetClipboardData(CF_UNICODETEXT, hGlob);

	CloseClipboard();
	Py_RETURN_NONE;
}
static PyObject* pyutils_waitforinputidle(PyObject* self, PyObject* args)
{
	DWORD dwPid = 0;
	int timeout = INFINITE;
	if (!PyArg_ParseTuple(args, "ki", &dwPid, &timeout))
		return NULL;

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwPid);
	DWORD waitResult = WaitForInputIdle(hProcess, INFINITE);
	CloseHandle(hProcess);
	
	if (waitResult == WAIT_FAILED)
	{
		PyErr_SetString(PyExc_WindowsError, "An error occurred.");
		return NULL;
	}
	else if (waitResult == WAIT_TIMEOUT)
	{
		PyErr_SetString(PyExc_TimeoutError, "The wait was terminated because the time-out interval elapsed.");
		return NULL;
	}

	Py_RETURN_NONE;
}

static PyMethodDef pyutils_methods[] =
{
	{ "get_pid", (PyCFunction)pyutils_getpid, METH_VARARGS, "Gets the PID for the specified process name" },
	{ "get_handles", (PyCFunction)pyutils_gethandles, METH_VARARGS, "Gets the number of handles held by the specified process" },
	{ "get_gdi_objects", (PyCFunction)pyutils_getgdiobjects, METH_VARARGS, "Return the count of GDI objects." },
	{ "get_gdi_objects_peak", (PyCFunction)pyutils_getgdiobjectspeak, METH_VARARGS, "Return the peak count of GDI objects." },
	{ "get_user_objects", (PyCFunction)pyutils_getuserobjects, METH_VARARGS, "Return the count of USER objects." },
	{ "get_user_objects_peak", (PyCFunction)pyutils_getuserobjectspeak, METH_VARARGS, "Return the peak count of USER objects." },

	{ "get_clipboard", (PyCFunction)pyutils_getclipboard, METH_VARARGS, "Get the text from the clipboard" },
	{ "set_clipboard", (PyCFunction)pyutils_setclipboard, METH_VARARGS, "Set the text on the clipboard" },

	{ "wait_for_input_idle", (PyCFunction)pyutils_waitforinputidle, METH_VARARGS, "Waits for the specified process to become idle." },
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef pyutils_def =
{
	PyModuleDef_HEAD_INIT,
	"pyutils",
	"Python Utilities for Windows Debugging and SQA",
	-1,
	pyutils_methods,
};

PyMODINIT_FUNC PyInit_pyutils()
{
	PyObject* module;
	if (PyType_Ready(&winapimutex_type) < 0)
		return NULL;
	if (PyType_Ready(&winapievent_type) < 0)
		return NULL;
	if (PyType_Ready(&memorymappedfile_type) < 0)
		return NULL;
	if (PyType_Ready(&winapisemaphore_type) < 0)
		return NULL;
	if (PyType_Ready(&performancecounter_type) < 0)
		return NULL;
	if (PyType_Ready(&process_type) < 0)
		return NULL;
	if (PyType_Ready(&uiautomation_type) < 0)
		return NULL;
	if (PyType_Ready(&exeinfo_type) < 0)
		return NULL; 

	if (PyType_Ready(&AutomationConditionType) < 0)
		return NULL;
	if (PyType_Ready(&PropertyConditionType) < 0)
		return NULL;
	if (PyType_Ready(&AndConditionType) < 0)
		return NULL;
	if (PyType_Ready(&OrConditionType) < 0)
		return NULL;
	if (PyType_Ready(&NotConditionType) < 0)
		return NULL;
	
#ifdef ENABLE_WIN8
	if (PyType_Ready(&processsnapshot_type) < 0)
		return NULL;
#endif

	module = PyModule_Create(&pyutils_def);
	if (module == NULL)
		return NULL;

	Py_INCREF(&winapimutex_type);
	PyModule_AddObject(module, "WinApiMutex", (PyObject*)&winapimutex_type);
	
	Py_INCREF(&winapievent_type);
	PyModule_AddObject(module, "WinApiEvent", (PyObject*)&winapievent_type);

	Py_INCREF(&memorymappedfile_type);
	PyModule_AddObject(module, "MemoryMappedFile", (PyObject*)&memorymappedfile_type);

	Py_INCREF(&winapisemaphore_type);
	PyModule_AddObject(module, "WinApiSemaphore", (PyObject*)&winapisemaphore_type);

	Py_INCREF(&performancecounter_type);
	PyModule_AddObject(module, "PerformanceCounter", (PyObject*)&performancecounter_type);

	Py_INCREF(&process_type);
	PyModule_AddObject(module, "Process", (PyObject*)&process_type);
	Process_InitTypes(module);

	Py_INCREF(&exeinfo_type);
	PyModule_AddObject(module, "ExeInfo", (PyObject*)&exeinfo_type);

	Py_INCREF(&uiautomation_type);
	PyModule_AddObject(module, "AutomationElement", (PyObject*)&uiautomation_type);

	Py_INCREF(&PropertyConditionType);
	PyModule_AddObject(module, "PropertyCondition", (PyObject*)&PropertyConditionType);
	Py_INCREF(&AndConditionType);
	PyModule_AddObject(module, "AndCondition", (PyObject*)&AndConditionType);
	Py_INCREF(&OrConditionType);
	PyModule_AddObject(module, "OrCondition", (PyObject*)&OrConditionType);
	Py_INCREF(&NotConditionType);
	PyModule_AddObject(module, "NotCondition", (PyObject*)&NotConditionType);

	AddControlTypeConstants(module);

#ifdef ENABLE_WIN8
	if (!processsnapshot_init_types(module))
	{
		return NULL;
	}
	Py_INCREF(&processsnapshot_type);
	PyModule_AddObject(module, "ProcessSnapshot", (PyObject*)&processsnapshot_type);
#endif

	return module;
}

#pragma endregion

using namespace System;

namespace pyutils {
}
