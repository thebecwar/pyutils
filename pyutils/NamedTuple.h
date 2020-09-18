#pragma once

#include <Python.h>
#include <structmember.h>

PyTypeObject* NamedTuple_NewType(PyStructSequence_Desc* descriptor);
void NamedTuple_DeleteType(PyTypeObject* type);
PyObject* NamedTuple_New(PyTypeObject* type);
void NamedTuple_SetItem(PyObject* o, Py_ssize_t index, PyObject* value);
void NamedTuple_SetItemString(PyObject* o, const char* item, PyObject* value);