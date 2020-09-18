#pragma once

#include <Windows.h>
#include <Python.h>

// Shared Helper functions
HANDLE OpenProcessByName(LPWSTR szProcessName);
DWORD GetProcessIdByName(LPWSTR szProcessName);
HANDLE ParseProcessArgs(PyObject* args);

extern PyTypeObject process_type;

bool Process_InitTypes(PyObject* module);