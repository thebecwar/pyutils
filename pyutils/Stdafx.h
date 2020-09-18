// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifdef PYUTILS_EXPORTS
#define PYUTILS_API __declspec(dllexport)
#else
#define PYUTILS_API __declspec(dllimport)
#endif

#define ENABLE_WIN8

#include <Python.h>

#define ASSIGN_NONE (Py_INCREF(Py_None), Py_None)
#define CBOOL(a) ((a) ? (Py_INCREF(Py_True), Py_True) : (Py_INCREF(Py_False), Py_False))
#define CWSTRING(a) ((a) ? PyUnicode_FromWideChar(a, -1) : PyUnicode_FromString(""))
#define CSTRING(a) ((a) ? PyUnicode_FromString(a) : PyUnicode_FromString(""))
#define CLONG(a) (PyLong_FromLong(a))
#define CULONG(a) (PyLong_FromUnsignedLong(a))
#define CLONGLONG(a) (PyLong_FromLongLong(a))
#define CULONGLONG(a) (PyLong_FromUnsignedLongLong(a))
#define CFLOAT(a) (PyFloat_FromDouble((double)a))
