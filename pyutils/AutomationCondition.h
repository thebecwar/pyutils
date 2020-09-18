#pragma once

#include <Python.h>
#include <Windows.h>
#include <UIAutomation.h>

PROPERTYID GetPropertyIdForName(const wchar_t* val);
const wchar_t* GetNameForPropertyId(PROPERTYID val);
VARTYPE GetPropertyType(PROPERTYID id);

void AddControlTypeConstants(PyObject* module);

extern PyTypeObject AutomationConditionType;
extern PyTypeObject PropertyConditionType;
extern PyTypeObject AndConditionType;
extern PyTypeObject OrConditionType;
extern PyTypeObject NotConditionType;

typedef struct
{
	PyObject_HEAD;
} automationcondition_object;
typedef struct
{
	automationcondition_object base;
	PyObject* key;
	PyObject* value;
} propertycondition_object;
typedef struct
{
	automationcondition_object base;
	PyObject* conditionList;
} andcondition_object;
typedef struct
{
	automationcondition_object base;
	PyObject* conditionList;
} orcondition_object;
typedef struct
{
	automationcondition_object base;
	PyObject* condition;
} notcondition_object;