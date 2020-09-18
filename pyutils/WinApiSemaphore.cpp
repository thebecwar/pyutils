#include "Stdafx.h"
#include "WinApiSemaphore.h"

#include <Windows.h>
#include <Python.h>
#include <structmember.h>

typedef struct {
	PyObject_HEAD;
	HANDLE hSemaphore;
} winapisemaphore_object;

void winapisemaphore_dealloc(winapisemaphore_object* self)
{
	if (self->hSemaphore != NULL)
	{
		CloseHandle(self->hSemaphore);
	}
	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* winapisemaphore_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	static char* kwlist[] = { "name", "initial_count", "maximum_count", NULL };

	PyObject* name = NULL;
	int initialCount = 0;
	int maxCount = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "Uii", &kwlist[0], &name, &initialCount, &maxCount))
		return NULL;

	winapisemaphore_object* self = (winapisemaphore_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		self->hSemaphore = NULL;

		wchar_t* szName = NULL;
		if (name != NULL)
			szName = PyUnicode_AsWideCharString(name, NULL);

		self->hSemaphore = CreateSemaphoreW(NULL, initialCount, maxCount, szName);
		int err = GetLastError();

		if (szName != NULL)
			PyMem_FREE(szName);

		if (self->hSemaphore == NULL)
		{
			Py_DECREF(self);
			PyErr_SetFromWindowsErr(err);
			return NULL;
		}
	}
	return (PyObject*)self;
}
int winapisemaphore_init(winapisemaphore_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyObject* winapisemaphore_open(PyObject* selfObj, PyObject* args)
{
	PyObject* name = NULL;
	if (!PyArg_ParseTuple(args, "U", &name))
		return NULL;

	winapisemaphore_object* self = (winapisemaphore_object*)winapisemaphore_type.tp_alloc(&winapisemaphore_type, 0);
	if (self != NULL)
	{
		self->hSemaphore = NULL;

		wchar_t* szName = NULL;
		if (name != NULL)
			szName = PyUnicode_AsWideCharString(name, NULL);

		self->hSemaphore = OpenSemaphoreW(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, szName);
		int err = GetLastError();

		if (szName != NULL)
			PyMem_FREE((void*)szName);

		if (self->hSemaphore == NULL)
		{
			Py_DECREF(self);

			if (err != ERROR_FILE_NOT_FOUND)
			{
				PyErr_SetFromWindowsErr(err);
				return NULL;
			}
			else
			{
				Py_RETURN_NONE;
			}
		}
	}
	return (PyObject*)self;
}

PyObject* winapisemaphore_acquire(PyObject* selfObj, PyObject* args)
{
	winapisemaphore_object* self = (winapisemaphore_object*)selfObj;
	if (self->hSemaphore == NULL)
	{
		PyErr_SetString(PyExc_OSError, "Semaphore not acquired.");
		return NULL;
	}

	int timeout = INFINITE;
	if (!PyArg_ParseTuple(args, "|i", &timeout))
		return NULL;

	DWORD result = WaitForSingleObject(self->hSemaphore, (DWORD)timeout);
	if (result == WAIT_TIMEOUT)
	{
		PyErr_SetString(PyExc_TimeoutError, "Timed out waiting for semaphore.");
		return NULL;
	}
	else if (result == WAIT_FAILED)
	{
		PyErr_SetExcFromWindowsErr(PyExc_OSError, GetLastError());
		return NULL;
	}
	Py_RETURN_NONE;
}
PyObject* winapisemaphore_release(PyObject* selfObj, PyObject* args)
{
	winapisemaphore_object* self = (winapisemaphore_object*)selfObj;
	if (self->hSemaphore == NULL)
	{
		PyErr_SetString(PyExc_OSError, "Semaphore not acquired.");
		return NULL;
	}

	int releaseCount = 1;
	if (!PyArg_ParseTuple(args, "|i", &releaseCount))
		return NULL;

	long previousCount = 0;

	BOOL ok = ReleaseSemaphore(self->hSemaphore, releaseCount, &previousCount);
	if (!ok)
	{
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
		return NULL;
	}

	return PyLong_FromLong(previousCount);
}

PyMethodDef winapisemaphore_methods[] =
{
	{ "acquire", (PyCFunction)winapisemaphore_acquire, METH_VARARGS, "Acquires the semaphore. Optional timeout parameter in MS." },
	{ "release", (PyCFunction)winapisemaphore_release, METH_VARARGS, "Releases the semaphore. Optional count parameter." },
	{ "open", (PyCFunction)winapisemaphore_open, METH_CLASS | METH_VARARGS, "Attempts to open an existing semaphore. Returns the semaphore object on success, None if the semaphore doesn't exist, and raises if there is another error." },
	{ NULL, NULL, 0, NULL },
};

PyMemberDef winapisemaphore_members[] =
{
	{ NULL }
};

PyTypeObject winapisemaphore_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.WinApiSemaphore",				/* tp_name */
	sizeof(winapisemaphore_object),			/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)winapisemaphore_dealloc,	/* tp_dealloc */
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
	"Windows API Semaphore wrapper",		/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	winapisemaphore_methods,				/* tp_methods */
	winapisemaphore_members,				/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)winapisemaphore_init,			/* tp_init */
	0,										/* tp_alloc */
	winapisemaphore_new,					/* tp_new */
};