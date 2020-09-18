#include "Stdafx.h"
#include "MemoryMappedFile.h"
#include <Windows.h>
#include <Python.h>
#include <structmember.h>

typedef struct {
	PyObject_HEAD;
	HANDLE hFile;
	void* lpvMemoryBlock;
	ssize_t size;
} memorymappedfile_object;

void memorymappedfile_dealloc(memorymappedfile_object* self)
{
	if (self->lpvMemoryBlock)
	{
		UnmapViewOfFile(self->lpvMemoryBlock);
	}
	if (self->hFile != NULL)
	{
		CloseHandle(self->hFile);
	}
	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* memorymappedfile_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	HANDLE hFile = NULL;

	static char* kwlist[] = { "name", "size", NULL };

	PyObject* name = NULL;
	int size = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "Ui", &kwlist[0], &name, &size))
		return NULL;

	memorymappedfile_object* self = (memorymappedfile_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		LPCWSTR szName = NULL;
		if (name != NULL)
		{
			szName = PyUnicode_AsWideCharString(name, NULL);
		}
		else
		{
			PyErr_SetString(PyExc_NameError, "File Name must be specified");
			Py_DECREF(self);
			return NULL;
		}

		self->hFile = CreateFileMappingW(NULL, NULL, PAGE_READWRITE, 0, size, szName);
		if (self->hFile == NULL)
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
			Py_DECREF(self);
			PyMem_FREE((void*)szName);
			return NULL;
		}
		PyMem_FREE((void*)szName);
		self->lpvMemoryBlock = MapViewOfFile(self->hFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
		self->size = size;
	}
	return (PyObject*)self;
}
int memorymappedfile_init(memorymappedfile_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyObject* memorymappedfile_readbool(PyObject* selfObj, PyObject* args)
{
	memorymappedfile_object* self = (memorymappedfile_object*)selfObj;
	if (self->lpvMemoryBlock == NULL)
	{
		PyErr_SetString(PyExc_IndexError, "Memory block is null.");
		return NULL;
	}

	int offset = 0;
	if (!PyArg_ParseTuple(args, "i", &offset))
		return NULL;

	if (offset > self->size || offset < 0)
	{
		PyErr_SetString(PyExc_IndexError, "Index out of range");
		return NULL;
	}

	if (*(char*)self->lpvMemoryBlock)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}
Py_ssize_t memorymappedfile_length(PyObject* selfObj)
{
	return ((memorymappedfile_object*)selfObj)->size;
}
PyObject* memorymappedfile_item(PyObject* selfObj, Py_ssize_t index)
{
	memorymappedfile_object* self = (memorymappedfile_object*)selfObj;
	if (index > self->size || index < 0)
	{
		PyErr_SetString(PyExc_IndexError, "Index out of bounds");
		return NULL;
	}
	return Py_BuildValue("b", ((char*)self->lpvMemoryBlock)[index]);
}
PyObject* memorymappedfile_readstring(PyObject* selfObj)
{
	memorymappedfile_object* self = (memorymappedfile_object*)selfObj;
	if (self->lpvMemoryBlock == NULL)
	{
		PyErr_SetString(PyExc_IndexError, "Memory block is null.");
		return NULL;
	}

	char* memory = (char*)self->lpvMemoryBlock;
	int end = 0;
	while (end < self->size && memory[end] != '\0') end++;

	if (end == 0)
		Py_RETURN_NONE;

	int buffersize = end + 1;
	if (end == self->size)
		buffersize++;

	char* tempBuffer = (char*)calloc(buffersize, 1);
	memcpy_s(tempBuffer, buffersize, memory, buffersize);
	PyObject* result = PyUnicode_FromString(tempBuffer);
	free(tempBuffer);
	return result;
}
PyObject* memorymappedfile_writestring(PyObject* selfObj, PyObject* args)
{
	memorymappedfile_object* self = (memorymappedfile_object*)selfObj;
	if (self->lpvMemoryBlock == NULL)
	{
		PyErr_SetString(PyExc_IndexError, "Memory block is null.");
		return NULL;
	}

	PyObject* unicode = NULL;
	if (!PyArg_ParseTuple(args, "U", &unicode))
		return NULL;

	Py_ssize_t size = 0;
	char* buff = PyUnicode_AsUTF8AndSize(unicode, &size); // Not responsible for the buffer, Unicode Object owns it
	memcpy_s(self->lpvMemoryBlock, self->size, buff, size);

	Py_RETURN_NONE;
}
PyObject* memorymappedfile_clear(PyObject* selfObj)
{
	memorymappedfile_object* self = (memorymappedfile_object*)selfObj;
	if (self->lpvMemoryBlock == NULL)
	{
		PyErr_SetString(PyExc_IndexError, "Memory block is null.");
		return NULL;
	}

	ZeroMemory(self->lpvMemoryBlock, self->size);
	Py_RETURN_NONE;
}
int memorymappedfile_assign(PyObject* selfObj, Py_ssize_t index, PyObject* args)
{
	memorymappedfile_object* self = (memorymappedfile_object*)selfObj;
	if (index > self->size || index < 0)
	{
		PyErr_SetString(PyExc_IndexError, "Index out of bounds");
		return -1;
	}

	if (!PyLong_Check(args))
	{
		PyErr_SetString(PyExc_TypeError, "Must be a numeric value.");
		return -1;
	}
	long value = PyLong_AsLong(args);
	if (value < 0 || value > 255)
	{
		PyErr_SetString(PyExc_TypeError, "Value must be between 0 and 255 inclusive");
		return -1;
	}

	((char*)self->lpvMemoryBlock)[index] = (unsigned char)value;
	return 0;
}
PyObject* memorymappedfile_mapget(PyObject* selfObj, PyObject* key)
{
	memorymappedfile_object* self = (memorymappedfile_object*)selfObj;
	if (PySlice_Check(key))
	{
		Py_ssize_t start = 0;
		Py_ssize_t end = 0;
		Py_ssize_t stride = 0;

		PySlice_GetIndices(key, self->size, &start, &end, &stride);

		if (start > self->size || end > self->size || start < 0 || end < 0)
		{
			PyErr_SetString(PyExc_IndexError, "Indices out of range");
			return NULL;
		}

		if (stride == 1)
		{
			char* ptrToStart = &((char*)self->lpvMemoryBlock)[start];
			return PyBytes_FromStringAndSize(ptrToStart, end - start);
		}
		else
		{
			char* tmp = (char*)calloc((end - start) % stride, 1);
			int cur = 0;
			for (Py_ssize_t i = start; i < end; i += stride)
			{
				tmp[cur++] = ((char*)self->lpvMemoryBlock)[i];
			}
			PyObject* result = PyBytes_FromStringAndSize(tmp, (end - start) % stride);
			free(tmp);
			return result;
		}

	}
	else if (PyLong_Check(key))
	{
		return memorymappedfile_item(selfObj, PyLong_AsSsize_t(key));
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Unknown Type");
		return NULL;
	}
}
PyMappingMethods memorymappedfile_map = {
	memorymappedfile_length, /* mp_length */
	memorymappedfile_mapget, /* mp_subscript */
	0, /* mp_ass_subscript */
};
static PySequenceMethods memorymappedfile_sequence = {
	memorymappedfile_length,	/* sq_length */
	0,							/* sq_concat */
	0,							/* sq_repeat */
	memorymappedfile_item,		/* sq_item */
	0,							/* *was_sq_slice */
	memorymappedfile_assign,	/* sq_ass_item */
	0,							/* *was_sq_ass_slice */
	0,							/* sq_contains */
	0,							/* sq_inplace_concat */
	0,							/* sq_inplace_repeat */
};
PyMethodDef memorymappedfile_methods[] = {
	{ "read_bool", (PyCFunction)memorymappedfile_readbool, METH_VARARGS, "Gets a bool at the specified index." },
	{ "read_string", (PyCFunction)memorymappedfile_readstring, METH_NOARGS, "Reads the buffer as a UTF-8 string, starting at index 0." },
	{ "write_string", (PyCFunction)memorymappedfile_writestring, METH_VARARGS, "Writes a UTF-8 string to the buffer starting at index 0." },
	{ "clear", (PyCFunction)memorymappedfile_clear, METH_NOARGS, "Clears the buffer by writing zeroes to all positions." },
	{ NULL, NULL, 0, NULL },
};
PyMemberDef memorymappedfile_members[] = {
	{ NULL }
};
PyTypeObject memorymappedfile_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.MemoryMappedFile",				/* tp_name */
	sizeof(memorymappedfile_object),		/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)memorymappedfile_dealloc,	/* tp_dealloc */
	0,										/* tp_print */
	0,										/* tp_getattr */
	0,										/* tp_setattr */
	0,										/* tp_reserved */
	0,										/* tp_repr */
	0,										/* tp_as_number */
	&memorymappedfile_sequence,				/* tp_as_sequence */
	&memorymappedfile_map,					/* tp_as_mapping */
	0,										/* tp_hash  */
	0,										/* tp_call */
	0,										/* tp_str */
	0,										/* tp_getattro */
	0,										/* tp_setattro */
	0,										/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |
	Py_TPFLAGS_BASETYPE,					/* tp_flags */
	"Windows API Memory Mapped File Wrapper",	/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	memorymappedfile_methods,				/* tp_methods */
	memorymappedfile_members,				/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)memorymappedfile_init,		/* tp_init */
	0,										/* tp_alloc */
	memorymappedfile_new,					/* tp_new */
};