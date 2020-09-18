#include "Stdafx.h"
#include "WinApiMutex.h"

#include <Windows.h>
#include <Python.h>
#include <structmember.h>

typedef struct {
	PyObject_HEAD;
	HANDLE hMutex;
	bool bOwned;
} winapimutex_object;

void winapimutex_dealloc(winapimutex_object* self)
{
	if (self->hMutex != NULL)
	{
		CloseHandle(self->hMutex);
	}
	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* winapimutex_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	static char* kwlist[] = { "name", "take_ownership", NULL };

	PyObject* name = NULL;
	bool initialOwner = false;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Up", &kwlist[0], &name, &initialOwner))
		return NULL;

	winapimutex_object* self = (winapimutex_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		self->hMutex = NULL;

		LPCWSTR szName = NULL;
		if (name != NULL)
			szName = (LPCWSTR)PyUnicode_2BYTE_DATA(name);

		self->hMutex = CreateMutexW(NULL, (BOOL)initialOwner, szName);
		self->bOwned = initialOwner;
		int err = GetLastError();
		if (self->hMutex == NULL && err != ERROR_ACCESS_DENIED)
		{
			Py_DECREF(self);
			PyErr_SetFromWindowsErr(err);
			return NULL;
		}
		else if (self->hMutex == NULL && err == ERROR_ACCESS_DENIED && szName != NULL)
		{
			self->hMutex = OpenMutexW(GENERIC_READ | GENERIC_WRITE, (bool)initialOwner, szName);
			err = GetLastError();
			if (self->hMutex == NULL)
			{
				Py_DECREF(self);
				PyErr_SetFromWindowsErr(err);
				return NULL;
			}
		}
		else if (err == ERROR_ALREADY_EXISTS)
		{
			self->bOwned = false;
		}
	}
	return (PyObject*)self;
}
int winapimutex_init(winapimutex_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyObject* winapimutex_acquire(PyObject* selfObj, PyObject* args)
{
	winapimutex_object* self = (winapimutex_object*)selfObj;
	if (self->hMutex == NULL)
	{
		PyErr_SetString(PyExc_OSError, "Mutex not acquired.");
		return NULL;
	}
	if (self->bOwned)
		Py_RETURN_NONE;

	int timeout = INFINITE;
	if (!PyArg_ParseTuple(args, "|i", &timeout))
		return NULL;

	DWORD result = WaitForSingleObject(self->hMutex, (DWORD)timeout);
	if (result == WAIT_ABANDONED)
	{
		PyErr_SetString(PyExc_OSError, "Mutex was abandoned by previous owner.");
		self->bOwned = true;
		return NULL;
	}
	else if (result == WAIT_TIMEOUT)
	{
		PyErr_SetString(PyExc_TimeoutError, "Timed out waiting for mutex.");
		self->bOwned = false;
		return NULL;
	}
	else if (result == WAIT_FAILED)
	{
		PyErr_SetExcFromWindowsErr(PyExc_OSError, GetLastError());
		self->bOwned = false;
		return NULL;
	}

	self->bOwned = true;
	Py_RETURN_NONE;
}
PyObject* winapimutex_release(PyObject* selfObj)
{
	winapimutex_object* self = (winapimutex_object*)selfObj;
	if (self->hMutex == NULL)
	{
		PyErr_SetString(PyExc_OSError, "Mutex not acquired.");
		return NULL;
	}
	else if (!self->bOwned)
	{
		Py_RETURN_NONE;
	}

	BOOL ok = ReleaseMutex(self->hMutex);
	if (!ok)
	{
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
		return NULL;
	}

	self->bOwned = false;
	Py_RETURN_NONE;
}

PyMethodDef winapimutex_methods[] =
{
	{ "acquire", (PyCFunction)winapimutex_acquire, METH_VARARGS, "Acquires the mutex. Optional timeout parameter in MS" },
	{ "release", (PyCFunction)winapimutex_release, METH_NOARGS, "Releases the mutex" },
	{ NULL, NULL, 0, NULL },
};

PyMemberDef winapimutex_members[] =
{
	{ "is_acquired", T_BOOL, offsetof(winapimutex_object, bOwned), READONLY, "Whether the mutex is currently owned by this object." },
	{ NULL }
};

PyTypeObject winapimutex_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.WinApiMutex",					/* tp_name */
	sizeof(winapimutex_object),				/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)winapimutex_dealloc,		/* tp_dealloc */
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
	"Windows API Mutex wrapper",			/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	winapimutex_methods,					/* tp_methods */
	winapimutex_members,					/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)winapimutex_init,				/* tp_init */
	0,										/* tp_alloc */
	winapimutex_new,						/* tp_new */
};