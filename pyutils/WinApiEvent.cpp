#include "Stdafx.h"
#include "WinApiEvent.h"

#include <Windows.h>
#include <Python.h>
#include <structmember.h>

typedef struct {
	PyObject_HEAD;
	HANDLE hEvent;
} winapievent_object;

void winapievent_dealloc(winapievent_object* self)
{
	if (self->hEvent != NULL)
	{
		CloseHandle(self->hEvent);
	}
	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* winapievent_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	static char* kwlist[] = { "name", "auto_reset", "initial_state", NULL };

	PyObject* name = NULL;
	bool autoReset = false;
	bool initialState = false;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Upp", &kwlist[0], &name, &autoReset, &initialState))
		return NULL;

	winapievent_object* self = (winapievent_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		self->hEvent = NULL;

		wchar_t* szName = NULL;
		if (name != NULL)
			szName = PyUnicode_AsWideCharString(name, NULL);

		self->hEvent = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, szName);
		int err = GetLastError();
		if (self->hEvent == NULL &&
			err != ERROR_FILE_NOT_FOUND)
		{
			if (szName)
				PyMem_FREE(szName);
			Py_DECREF(self);
			PyErr_SetFromWindowsErr(err);
			return NULL;
		}
		else if (self->hEvent == NULL)
		{
			self->hEvent = CreateEventW(NULL, (BOOL)(!autoReset), (BOOL)initialState, szName);
			err = GetLastError();
		}

		if (szName != NULL)
			PyMem_FREE(szName);

		if (self->hEvent == NULL)
		{
			Py_DECREF(self);
			PyErr_SetFromWindowsErr(err);
			return NULL;
		}
	}
	return (PyObject*)self;
}
int winapievent_init(winapievent_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyObject* winapievent_set(PyObject* selfObj)
{
	winapievent_object* self = (winapievent_object*)selfObj;
	if (self->hEvent == NULL)
	{
		PyErr_SetString(PyExc_OSError, "Invalid event handle.");
		return NULL;
	}

	SetEvent(self->hEvent);
	Py_RETURN_NONE;
}
PyObject* winapievent_reset(PyObject* selfObj)
{
	winapievent_object* self = (winapievent_object*)selfObj;
	if (self->hEvent == NULL)
	{
		PyErr_SetString(PyExc_OSError, "Invalid event handle.");
		return NULL;
	}

	ResetEvent(self->hEvent);

	Py_RETURN_NONE;
}
PyObject* winapievent_wait(PyObject* selfObj, PyObject* args)
{
	winapievent_object* self = (winapievent_object*)selfObj;
	if (self->hEvent == NULL)
	{
		PyErr_SetString(PyExc_OSError, "Invalid event handle.");
		return NULL;
	}

	int timeout = INFINITE;
	if (!PyArg_ParseTuple(args, "|i", &timeout))
		return NULL;

	DWORD result = WaitForSingleObject(self->hEvent, timeout);
	if (result == WAIT_OBJECT_0)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}
PyMethodDef winapievent_methods[] =
{
	{ "set", (PyCFunction)winapievent_set, METH_NOARGS, "Sets the event" },
	{ "reset", (PyCFunction)winapievent_reset, METH_NOARGS, "Resets the event" },
	{ "wait", (PyCFunction)winapievent_wait, METH_VARARGS, "Waits for the event to be signalled." },
	{ NULL, NULL, 0, NULL },
};
PyMemberDef winapievent_members[] =
{
	{ NULL }
};

PyTypeObject winapievent_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.WinApiEvent",					/* tp_name */
	sizeof(winapievent_object),				/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)winapievent_dealloc,		/* tp_dealloc */
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
	"Windows API Event wrapper",			/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	winapievent_methods,					/* tp_methods */
	winapievent_members,					/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)winapievent_init,				/* tp_init */
	0,										/* tp_alloc */
	winapievent_new,						/* tp_new */
};