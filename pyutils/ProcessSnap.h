#pragma once

#include <Python.h>

extern PyTypeObject* process_handle_info_type;
extern PyTypeObject* thread_handle_info_type;
extern PyTypeObject* mutant_handle_info_type;
extern PyTypeObject* event_handle_info_type;
extern PyTypeObject* allocation_handle_info_type;
extern PyTypeObject* handle_info_type;
extern PyTypeObject processsnapshot_type;

bool processsnapshot_init_types(PyObject* targetModule);