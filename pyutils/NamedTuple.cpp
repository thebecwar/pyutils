#include "Stdafx.h"
#include "NamedTuple.h"

#include <Python.h>
#include <structmember.h>

#include <string>
#include <sstream>
#include <map>

using std::string;
using std::stringstream;
using std::map;

typedef struct
{
	PyTypeObject pyType;
	int n_objects;
	map<string, int>* propertyMap;
	map<int, string>* indexMap;
} NamedTupleTypeObject;

typedef struct {
	PyObject_HEAD;
	PyObject* internalObjects[1];
} namedtuple_object;

void namedtuple_dealloc(namedtuple_object* self)
{
	int n_objects = ((NamedTupleTypeObject*)Py_TYPE(self))->n_objects;
	for (int i = 0; i < n_objects; i++)
		Py_XDECREF(self->internalObjects[i]);

	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* namedtuple_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	NamedTupleTypeObject* namedType = (NamedTupleTypeObject*)type;

	namedtuple_object* self = (namedtuple_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		if (args != NULL)
		{
			size_t count = PySequence_Length(args);
			if (count > namedType->n_objects)
			{
				PyErr_SetString(PyExc_IndexError, "Too many initializers");
				Py_DECREF(self);
				return NULL;
			}
			for (int i = 0; i < count; i++)
			{
				self->internalObjects[i] = PySequence_GetItem(args, i);
			}
		}
		if (kwds != NULL)
		{
			for (auto iter = namedType->propertyMap->begin(); iter != namedType->propertyMap->end(); iter++)
			{
				PyObject* arg = PyDict_GetItemString(kwds, iter->first.c_str());
				Py_XINCREF(arg);
				self->internalObjects[iter->second] = arg;
			}
		}

		for (int i = 0; i < namedType->n_objects; i++)
		{
			if (self->internalObjects[i] == NULL)
			{
				Py_INCREF(Py_None);
				self->internalObjects[i] = Py_None;
			}
		}
	}
	return (PyObject*)self;
}
int namedtuple_init(namedtuple_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
Py_ssize_t namedtuple_length(namedtuple_object* self)
{
	return ((NamedTupleTypeObject*)Py_TYPE(self))->n_objects;
}

#pragma region Sequence Functions

PyObject* namedtuple_concat(PyObject* a, PyObject* b)
{
	PyErr_SetString(PyExc_TypeError, "NamedTuple doesn't support this operation.");
	return NULL;
}
PyObject* namedtuple_repeat(namedtuple_object* self, Py_ssize_t count)
{
	PyErr_SetString(PyExc_TypeError, "NamedTuple doesn't support this operation.");
	return NULL;
}
PyObject* namedtuple_item(namedtuple_object* self, Py_ssize_t index)
{
	// validate index
	int n = ((NamedTupleTypeObject*)Py_TYPE(self))->n_objects;
	if (index < 0)
	{
		index += n;
		if (index < 0) goto err;
	}
	if (index >= n) goto err;

	return self->internalObjects[index];

err:
	PyErr_SetString(PyExc_IndexError, "tuple index out of range");
	return NULL;
}
int namedtuple_assignitem(namedtuple_object* self, Py_ssize_t index, PyObject* value)
{
	int n = ((NamedTupleTypeObject*)Py_TYPE(self))->n_objects;
	if (index < 0)
	{
		index += n;
		if (index < 0) goto err;
	}
	if (index >= n) goto err;

	if (value == NULL) value = Py_None;

	Py_INCREF(value);
	Py_DECREF(self->internalObjects[index]);
	self->internalObjects[index] = value;

	return 0;
err:
	PyErr_SetString(PyExc_IndexError, "tuple index out of range");
	return -1;
}
PySequenceMethods namedtuple_sequence = {
	(lenfunc)namedtuple_length,						//sq_length;
	namedtuple_concat,								//sq_concat;
	(ssizeargfunc)namedtuple_repeat,				//sq_repeat;
	(ssizeargfunc)namedtuple_item,					//sq_item;
	0,												//*was_sq_slice
	(ssizeobjargproc)namedtuple_assignitem,			//sq_ass_item
	0,												//*was_sq_ass_slice
	0,												//sq_contains

	0,												//sq_inplace_concat
	0,												//sq_inplace_repeat
};

#pragma endregion

#pragma region Mapping Functions

PyObject* namedtuple_mapsubscript(namedtuple_object* self, PyObject* key)
{
	ssize_t index = 0;
	NamedTupleTypeObject* type = (NamedTupleTypeObject*)Py_TYPE(self);

	if (PyUnicode_Check(key))
	{
		const char* tmp = PyUnicode_AsUTF8(key);
		string keyVal(tmp);
		if (type->propertyMap->find(keyVal) == type->propertyMap->end())
		{
			PyErr_SetString(PyExc_KeyError, "Invalid Key.");
			return NULL;
		}
		index = type->propertyMap->at(keyVal);
	}
	else if (PyLong_Check(key))
	{
		index = PyLong_AsSsize_t(key);
		if (index < 0)
			index += type->n_objects;
		if (index < 0 || index >= type->n_objects)
		{
			PyErr_SetString(PyExc_IndexError, "Index out of range");
			return NULL;
		}
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Unknown key type.");
		return NULL;
	}
	Py_INCREF(self->internalObjects[index]);
	return self->internalObjects[index];
}
int namedtuple_mapassign(namedtuple_object* self, PyObject* key, PyObject* value)
{
	ssize_t index = 0;
	NamedTupleTypeObject* type = (NamedTupleTypeObject*)Py_TYPE(self);

	if (PyUnicode_Check(key))
	{
		const char* tmp = PyUnicode_AsUTF8(key);
		string keyVal(tmp);
		if (type->propertyMap->find(keyVal) == type->propertyMap->end())
		{
			PyErr_SetString(PyExc_KeyError, "Invalid Key.");
			return -1;
		}
		index = type->propertyMap->at(keyVal);
	}
	else if (PyLong_Check(key))
	{
		index = PyLong_AsSsize_t(key);
		if (index < 0)
			index += type->n_objects;
		if (index < 0 || index >= type->n_objects)
		{
			PyErr_SetString(PyExc_IndexError, "Index out of range");
			return -1;
		}
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Unknown key type.");
		return -1;
	}

	Py_DECREF(self->internalObjects[index]);
	if (value == NULL)
		value = Py_None;
	Py_INCREF(value);
	self->internalObjects[index] = value;
	return 0;
}

PyMappingMethods namedtuple_mapping = {
	(lenfunc)namedtuple_length,				// mp_length
	(binaryfunc)namedtuple_mapsubscript,	// mp_subscript
	(objobjargproc)namedtuple_mapassign,	// mp_ass_subscript
};

#pragma endregion

PyObject* namedtuple_repr(namedtuple_object* self) 
{
	NamedTupleTypeObject* tp = (NamedTupleTypeObject*)Py_TYPE(self);
	stringstream ss;

	ss << "{" << tp->pyType.tp_name << ": ";

	for (int i = 0; i < tp->n_objects; i++)
	{
		PyObject* repr = PyObject_Repr(self->internalObjects[i]);
		const char* value = PyUnicode_AsUTF8AndSize(repr, NULL);
		ss << tp->indexMap->at(i) << "=" << value << ((i < tp->n_objects - 1) ? ", " : " ");
		Py_DECREF(repr);
	}
	ss << "}";

	return PyUnicode_FromString(ss.str().c_str());
}
PyTypeObject namedtuple_type_template = {
	PyVarObject_HEAD_INIT(NULL, 0)
	0,										/* tp_name */
	sizeof(namedtuple_object),				/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)namedtuple_dealloc,			/* tp_dealloc */
	0,										/* tp_print */
	0,										/* tp_getattr */
	0,										/* tp_setattr */
	0,										/* tp_reserved */
	(reprfunc)namedtuple_repr,				/* tp_repr */
	0,										/* tp_as_number */
	&namedtuple_sequence,					/* tp_as_sequence */
	&namedtuple_mapping,					/* tp_as_mapping */
	0,										/* tp_hash  */
	0,										/* tp_call */
	0,										/* tp_str */
	0,										/* tp_getattro */
	0,										/* tp_setattro */
	0,										/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |
	Py_TPFLAGS_BASETYPE,					/* tp_flags */
	0,										/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	0,										/* tp_methods */
	0,										/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)namedtuple_init,				/* tp_init */
	0,										/* tp_alloc */
	namedtuple_new,							/* tp_new */
};
PyTypeObject* NamedTuple_NewType(PyStructSequence_Desc* descriptor)
{
	NamedTupleTypeObject* result = new NamedTupleTypeObject();
	memcpy_s(result, sizeof(PyTypeObject), &namedtuple_type_template, sizeof(PyTypeObject));

	result->propertyMap = new map<string, int>();
	result->indexMap = new map<int, string>();

	// Calculate basic size. (PyObject_HEAD macro evaluates to an item of type PyObject)
	result->pyType.tp_basicsize = sizeof(PyObject) + sizeof(int) + descriptor->n_in_sequence * sizeof(PyObject*);

	result->pyType.tp_name = descriptor->name;
	result->pyType.tp_doc = descriptor->doc;

	result->n_objects = descriptor->n_in_sequence;
	result->pyType.tp_members = new PyMemberDef[descriptor->n_in_sequence + 1];
	for (int i = 0; i < descriptor->n_in_sequence; i++)
	{
		result->pyType.tp_members[i].name = descriptor->fields[i].name;
		result->pyType.tp_members[i].doc = descriptor->fields[i].doc;
		result->pyType.tp_members[i].type = T_OBJECT;
		result->pyType.tp_members[i].flags = 0;
		result->pyType.tp_members[i].offset = offsetof(namedtuple_object, internalObjects) + (i * sizeof(PyObject*));

		result->propertyMap->emplace(string(descriptor->fields[i].name), i);
		result->indexMap->emplace(i, string(descriptor->fields[i].name));
	}
	memset(&result->pyType.tp_members[descriptor->n_in_sequence], 0, sizeof(PyMemberDef));

	if (PyType_Ready(&result->pyType) < 0)
	{
		delete result;
		return NULL;
	}

	return (PyTypeObject*)result;
}
void NamedTuple_DeleteType(PyTypeObject* typeObj)
{
	NamedTupleTypeObject* type = (NamedTupleTypeObject*)typeObj;
	if (type->propertyMap)
	{
		delete type->propertyMap;
		type->propertyMap = NULL;
	}
	if (type->pyType.tp_members)
	{
		delete[] type->pyType.tp_members;
		type->pyType.tp_members = NULL;
	}
}
PyObject* NamedTuple_New(PyTypeObject* type)
{
	return namedtuple_new(type, NULL, NULL);
}
void NamedTuple_SetItem(PyObject* o, Py_ssize_t index, PyObject* value)
{
	namedtuple_assignitem((namedtuple_object*)o, index, value);
}
void NamedTuple_SetItemString(PyObject* o, const char* item, PyObject* value)
{
	NamedTupleTypeObject* type = (NamedTupleTypeObject*)Py_TYPE(o);
	string key(item);
	int index = type->propertyMap->at(key);
	namedtuple_assignitem((namedtuple_object*)o, index, value);
}