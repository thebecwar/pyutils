#include "Stdafx.h"
#include "UIAutomation.h"

#include <Windows.h>
#include <Python.h>
#include <structmember.h>
#include <atlbase.h>
#include <atlsafe.h>
#include <vcclr.h>
#include <UIAutomation.h>
#include <vector>

#include "AutomationCondition.h"

// forward declare this so we can use it for fetching
PyObject* uiautomation_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);
PyObject* UIAutomation_New(IUIAutomationElement* element)
{
	return uiautomation_new(&uiautomation_type, (PyObject*)element, (PyObject*)-1);
}

#pragma region Managed Utility Methods

static pyutils::AutomationManager::AutomationManager()
{
	pAutomation = NULL;
	cachedPropertyList = NULL;

	HRESULT hr = CoInitialize(NULL);
	IUIAutomation* pauto = NULL;
	hr = CoCreateInstance(CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pauto));
	if (FAILED(hr))
		throw gcnew System::ComponentModel::Win32Exception(hr);
	pAutomation = pauto;
}
void pyutils::AutomationManager::GetConditionForPyObject(PyObject* source, IUIAutomationCondition** ppCondition, std::vector<IUIAutomationCondition*>& children)
{
	HRESULT hr = S_OK;
	*ppCondition = NULL;

	if (PyErr_Occurred()) return;

	if (!PyObject_TypeCheck(source, &AutomationConditionType))
	{
		PyErr_SetString(PyExc_TypeError, "Must be an automation condition.");
		return;
	}

	if (PyObject_TypeCheck(source, &PropertyConditionType))
	{
		propertycondition_object* src = (propertycondition_object*)source;
		wchar_t* key = PyUnicode_AsWideCharString(src->key, NULL);
		PROPERTYID pid = GetPropertyIdForName(key);
		PyMem_FREE(key);

		VARIANT var = { 0 };
		BuildVariantFromPyObject(pid, &var, src->value);
		if (PyErr_Occurred())
		{
			return;
		}

		hr = pAutomation->CreatePropertyCondition(pid, var, ppCondition);

		if (var.vt == VT_BSTR)
		{
			SysFreeString(var.bstrVal);
		}

		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return;
		}
		children.push_back(*ppCondition);
	}
	else if (PyObject_TypeCheck(source, &NotConditionType))
	{
		notcondition_object* src = (notcondition_object*)source;
		IUIAutomationCondition* child = NULL;
		GetConditionForPyObject(src->condition, &child, children);
		if (PyErr_Occurred())
			return;

		hr = pAutomation->CreateNotCondition(child, ppCondition);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return;
		}
		children.push_back(*ppCondition);
	}
	else if (PyObject_TypeCheck(source, &AndConditionType))
	{
		andcondition_object* src = (andcondition_object*)source;

		Py_ssize_t len = PySequence_Length(src->conditionList);
		if (len < 2)
		{
			PyErr_SetString(PyExc_Exception, "And condition must contain at least 2 child conditions.");
			return;
		}

		IUIAutomationCondition** conditions = new IUIAutomationCondition*[len];
		for (Py_ssize_t i = 0; i < len; i++)
		{
			IUIAutomationCondition* child = NULL;
			PyObject* fromObj = PySequence_GetItem(src->conditionList, i);
			GetConditionForPyObject(fromObj, &child, children);
			if (PyErr_Occurred())
				return;
			conditions[i] = child;
		}

		hr = pAutomation->CreateAndConditionFromNativeArray(conditions, (int)len, ppCondition);
		delete[] conditions;
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return;
		}
		children.push_back(*ppCondition);
	}
	else if (PyObject_TypeCheck(source, &OrConditionType))
	{
		orcondition_object* src = (orcondition_object*)source;

		Py_ssize_t len = PySequence_Length(src->conditionList);
		if (len < 2)
		{
			PyErr_SetString(PyExc_Exception, "Or condition must contain at least 2 child conditions.");
			return;
		}

		IUIAutomationCondition** conditions = new IUIAutomationCondition*[len];
		for (Py_ssize_t i = 0; i < len; i++)
		{
			IUIAutomationCondition* child = NULL;
			PyObject* fromObj = PySequence_GetItem(src->conditionList, i);
			GetConditionForPyObject(fromObj, &child, children);
			if (PyErr_Occurred())
				return;
			conditions[i] = child;
		}

		hr = pAutomation->CreateOrConditionFromNativeArray(conditions, (int)len, ppCondition);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return;
		}
		children.push_back(*ppCondition);
	}
	return;
}

IUIAutomationElement* pyutils::AutomationManager::GetRootNode()
{
	IUIAutomationElement* result = NULL;
	HRESULT hr = pAutomation->GetRootElement(&result);
	if (FAILED(hr))
		return NULL;
	else
		return result;
}
IUIAutomationElement* pyutils::AutomationManager::GetElementsFromRoot(PyObject* searchTuple)
{
	PyObject* iter = PyObject_GetIter(searchTuple);
	PyObject* item = NULL;

	if (iter == NULL)
	{
		PyErr_SetString(PyExc_TypeError, "Argument must be iterable");
		return NULL;
	}

	IUIAutomationElement* element;
	HRESULT hr = pAutomation->GetRootElement(&element);
	if (element == NULL)
		return NULL;

	while (item = PyIter_Next(iter))
	{
		wchar_t* current = PyUnicode_AsWideCharString(item, NULL);
		CComVariant currentName;
		currentName.SetByRef(SysAllocString(current));
		PyMem_FREE(current);

		CComPtr<IUIAutomationCondition> pCondition;
		hr = pAutomation->CreatePropertyCondition(UIA_NamePropertyId, currentName, &pCondition);

		IUIAutomationElement* tmp = element;
		hr = tmp->FindFirst(TreeScope_Children, pCondition, &element);
		tmp->Release();

		Py_DECREF(item);
		if (element == NULL || FAILED(hr))
			break;
	}

	Py_DECREF(iter);

	return element;
}
IUIAutomationElement* pyutils::AutomationManager::GetProcessRoot(PyObject* process)
{
	return NULL;
}
PyObject* pyutils::AutomationManager::GetPropertyList()
{
	if (cachedPropertyList == NULL)
	{
		cachedPropertyList = PyList_New(0);
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"RuntimeId", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"BoundingRectangle", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ProcessId", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ControlType", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LocalizedControlType", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"Name", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"AcceleratorKey", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"AccessKey", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"HasKeyboardFocus", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsKeyboardFocusable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsEnabled", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"AutomationId", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ClassName", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"HelpText", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ClickablePoint", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"Culture", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsControlElement", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsContentElement", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LabeledBy", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsPassword", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"NativeWindowHandle", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ItemType", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsOffscreen", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"Orientation", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"FrameworkId", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsRequiredForForm", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ItemStatus", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsDockPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsExpandCollapsePatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsGridItemPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsGridPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsInvokePatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsMultipleViewPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsRangeValuePatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsScrollPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsScrollItemPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsSelectionItemPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsSelectionPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsTablePatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsTableItemPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsTextPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsTogglePatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsTransformPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsValuePatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsWindowPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ValueValue", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ValueIsReadOnly", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"RangeValueValue", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"RangeValueIsReadOnly", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"RangeValueMinimum", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"RangeValueMaximum", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"RangeValueLargeChange", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"RangeValueSmallChange", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ScrollHorizontalScrollPercent", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ScrollHorizontalViewSize", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ScrollVerticalScrollPercent", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ScrollVerticalViewSize", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ScrollHorizontallyScrollable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ScrollVerticallyScrollable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"SelectionSelection", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"SelectionCanSelectMultiple", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"SelectionIsSelectionRequired", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"GridRowCount", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"GridColumnCount", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"GridItemRow", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"GridItemColumn", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"GridItemRowSpan", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"GridItemColumnSpan", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"GridItemContainingGrid", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"DockDockPosition", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ExpandCollapseExpandCollapseState", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"MultipleViewCurrentView", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"MultipleViewSupportedViews", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"WindowCanMaximize", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"WindowCanMinimize", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"WindowWindowVisualState", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"WindowWindowInteractionState", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"WindowIsModal", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"WindowIsTopmost", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"SelectionItemIsSelected", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"SelectionItemSelectionContainer", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"TableRowHeaders", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"TableColumnHeaders", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"TableRowOrColumnMajor", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"TableItemRowHeaderItems", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"TableItemColumnHeaderItems", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ToggleToggleState", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"TransformCanMove", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"TransformCanResize", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"TransformCanRotate", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsLegacyIAccessiblePatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LegacyIAccessibleChildId", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LegacyIAccessibleName", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LegacyIAccessibleValue", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LegacyIAccessibleDescription", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LegacyIAccessibleRole", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LegacyIAccessibleState", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LegacyIAccessibleHelp", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LegacyIAccessibleKeyboardShortcut", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LegacyIAccessibleSelection", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LegacyIAccessibleDefaultAction", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"AriaRole", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"AriaProperties", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsDataValidForForm", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ControllerFor", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"DescribedBy", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"FlowsTo", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"ProviderDescription", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsItemContainerPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsVirtualizedItemPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsSynchronizedInputPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"OptimizeForVisualContent", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsObjectModelPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"AnnotationAnnotationTypeId", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"AnnotationAnnotationTypeName", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"AnnotationAuthor", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"AnnotationDateTime", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"AnnotationTarget", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsAnnotationPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsTextPattern2Available", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"StylesStyleId", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"StylesStyleName", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"StylesFillColor", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"StylesFillPatternStyle", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"StylesShape", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"StylesFillPatternColor", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"StylesExtendedProperties", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsStylesPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsSpreadsheetPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"SpreadsheetItemFormula", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"SpreadsheetItemAnnotationObjects", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"SpreadsheetItemAnnotationTypes", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsSpreadsheetItemPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"Transform2CanZoom", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsTransformPattern2Available", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"LiveSetting", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsTextChildPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsDragPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"DragIsGrabbed", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"DragDropEffect", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"DragDropEffects", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsDropTargetPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"DropTargetDropTargetEffect", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"DropTargetDropTargetEffects", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"DragGrabbedItems", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"Transform2ZoomLevel", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"Transform2ZoomMinimum", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"Transform2ZoomMaximum", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"FlowsFrom", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsTextEditPatternAvailable", -1));
		PyList_Append(cachedPropertyList, PyUnicode_FromWideChar(L"IsPeripheral", -1));
	}
	Py_INCREF(cachedPropertyList);
	return cachedPropertyList;
}
void pyutils::AutomationManager::BuildVariantFromPyObject(PROPERTYID pid, VARIANT* pVariant, PyObject* source)
{
	VARTYPE vt = GetPropertyType(pid);
	if (vt == VT_ILLEGAL)
	{
		PyErr_SetString(PyExc_WindowsError, "Unknown variant type.");
	}

	pVariant->vt = vt;
	switch (vt)
	{
	case VT_I1:
		pVariant->cVal = (CHAR)PyLong_AsLong(source);
		break;
	case VT_UI1:
		pVariant->bVal = (BYTE)PyLong_AsUnsignedLong(source);
		break;
	case VT_I2:
		pVariant->iVal = (SHORT)PyLong_AsLong(source);
		break;
	case VT_UI2:
		pVariant->uiVal = (USHORT)PyLong_AsUnsignedLong(source);
		break;
	case VT_I4:
		pVariant->lVal = (LONG)PyLong_AsLong(source);
		break;
	case VT_UI4:
		pVariant->ulVal = (ULONG)PyLong_AsUnsignedLong(source);
		break;
	case VT_INT:
		pVariant->intVal = (INT)PyLong_AsLong(source);
		break;
	case VT_UINT:
		pVariant->uintVal = (UINT)PyLong_AsUnsignedLong(source);
		break;
	case VT_I8:
		pVariant->llVal = (LONGLONG)PyLong_AsLongLong(source);
		break;
	case VT_UI8:
		pVariant->ullVal = (ULONGLONG)PyLong_AsUnsignedLongLong(source);
		break;
	case VT_R4:
		pVariant->fltVal = (FLOAT)PyLong_AsDouble(source);
		break;
	case VT_R8:
		pVariant->dblVal = (DOUBLE)PyLong_AsDouble(source);
		break;
	case VT_BOOL:
		if (PyObject_IsTrue(source) > 0)
			pVariant->boolVal = VARIANT_TRUE;
		else
			pVariant->boolVal = VARIANT_FALSE;
		break;
	case VT_CY:
		pVariant->cyVal.int64 = (LONGLONG)PyLong_AsLongLong(source);
		break;
	case VT_BSTR:
	{
		wchar_t* buffer = PyUnicode_AsWideCharString(source, NULL);
		if (buffer == NULL) PyErr_SetString(PyExc_TypeError, "Expected a string.");
		pVariant->bstrVal = SysAllocString(buffer);
		PyMem_FREE(buffer);
	}
	break;
	default:
		PyErr_SetString(PyExc_WindowsError, "Unsupported variant type.");
	}

}
PyObject* pyutils::AutomationManager::BuildPyObjectFromVariant(VARIANT* pVariant)
{
	PyObject* result = NULL;

	if (pVariant == NULL)
	{
		Py_RETURN_NONE;
	}

	switch (pVariant->vt)
	{
	case VT_I1:
		result = PyLong_FromLong(pVariant->cVal);
		break;
	case VT_UI1:
		result = PyLong_FromUnsignedLong(pVariant->bVal);
		break;
	case VT_I2:
		result = PyLong_FromLong(pVariant->iVal);
		break;
	case VT_UI2:
		result = PyLong_FromUnsignedLong(pVariant->uiVal);
		break;
	case VT_I4:
		result = PyLong_FromLong(pVariant->lVal);
		break;
	case VT_UI4:
		result = PyLong_FromUnsignedLong(pVariant->ulVal);
		break;
	case VT_INT:
		result = PyLong_FromLong(pVariant->intVal);
		break;
	case VT_UINT:
		result = PyLong_FromUnsignedLong(pVariant->uintVal);
		break;
	case VT_I8:
		result = PyLong_FromLongLong(pVariant->llVal);
		break;
	case VT_UI8:
		result = PyLong_FromUnsignedLongLong(pVariant->ullVal);
		break;
	case VT_R4:
		result = PyLong_FromDouble(pVariant->fltVal);
		break;
	case VT_R8:
		result = PyLong_FromDouble(pVariant->dblVal);
		break;
	case VT_BOOL:
		if (pVariant->boolVal == VARIANT_TRUE)
			result = Py_True;
		else
			result = Py_False;
		Py_INCREF(result);
		break;
	case VT_ERROR:
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, pVariant->scode);
		result = NULL;
		break;
	case VT_CY:
		result = PyLong_FromLongLong(pVariant->cyVal.int64);
		break;
	case VT_BSTR:
		result = PyUnicode_FromWideChar(pVariant->bstrVal, -1);
		break;
	case VT_UNKNOWN:
	{
		if (pVariant->punkVal)
		{
			IUIAutomationElement* element = NULL;
			HRESULT hr = pVariant->punkVal->QueryInterface(IID_PPV_ARGS(&element));
			if (SUCCEEDED(hr))
			{
				result = UIAutomation_New(element);
			}
			else if (hr == E_NOINTERFACE)
			{
				// list?
				PyErr_SetString(PyExc_TypeError, "Unsupported interface type");
				result = NULL;
			}
			else
			{
				PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
				result = NULL;
			}
		}
		else
		{
			result = Py_None;
			Py_INCREF(result);
		}
	}
	break;
	case VT_EMPTY:
		result = Py_None;
		Py_INCREF(result);
		break;
		/*case VT_SAFEARRAY:
		break;*/
	default:
		PyErr_SetString(PyExc_TypeError, "Unknown value type.");
		result = NULL;
	}

	return result;
}
HRESULT pyutils::AutomationManager::GetAutomationProperty(IUIAutomationElement* fromElement, PROPERTYID propertyId, VARIANT* pResult)
{
	if (!pResult)
		return E_FAIL;

	return fromElement->GetCurrentPropertyValue(propertyId, pResult);
}
PyObject* pyutils::AutomationManager::GetProperty(IUIAutomationElement* fromElement, PROPERTYID propertyId)
{
	CComVariant value;
	HRESULT hr = fromElement->GetCurrentPropertyValue(propertyId, &value);
	if (FAILED(hr))
	{
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
		return NULL;
	}
	return BuildPyObjectFromVariant(&value);
}
PyObject* pyutils::AutomationManager::GetProperty(IUIAutomationElement* fromElement, const wchar_t* name)
{
	PROPERTYID propId = GetPropertyIdForName(name);

	CComVariant value;
	HRESULT hr = fromElement->GetCurrentPropertyValue(propId, &value);
	if (FAILED(hr))
	{
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
		return NULL;
	}
	return BuildPyObjectFromVariant(&value);
}
PyObject* pyutils::AutomationManager::GetProperty(IUIAutomationElement* fromElement, PyObject* name)
{
	if (!PyUnicode_Check(name))
	{
		PyErr_SetString(PyExc_TypeError, "Name must be a string.");
		return NULL;
	}

	wchar_t* tmp = PyUnicode_AsWideCharString(name, NULL);
	PyObject* result = GetProperty(fromElement, tmp);
	PyMem_FREE(tmp);

	return result;
}
IUIAutomationElement* pyutils::AutomationManager::FindFirst(IUIAutomationElement* fromElement, TreeScope scope, automationcondition_object* conditions)
{
	std::vector<IUIAutomationCondition*> childConditions;
	IUIAutomationCondition* searchCondition;
	GetConditionForPyObject((PyObject*)conditions, &searchCondition, childConditions);

	IUIAutomationElement* result = NULL;
	if (!PyErr_Occurred())
	{
		HRESULT hr = fromElement->FindFirst(scope, searchCondition, &result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
		}
	}

	for (auto iter = childConditions.begin(); iter != childConditions.end(); iter++)
	{
		(*iter)->Release();
	}

	return result;
}
IUIAutomationElement* pyutils::AutomationManager::FindFirst(IUIAutomationElement* fromElement, TreeScope scope, PyObject* paramDict)
{
	HRESULT hr;
	IUIAutomationElement* result = NULL;

	if (PyObject_TypeCheck(paramDict, &AutomationConditionType))
	{
		return FindFirst(fromElement, scope, (automationcondition_object*)paramDict);
	}

	if (!PyDict_Check(paramDict))
	{
		PyErr_SetString(PyExc_TypeError, "Parameters must be in the form of a dictionary of property-value pairs");
		return NULL;
	}

	if (fromElement == NULL)
	{
		PyErr_SetString(PyExc_ReferenceError, "No element specified");
		return NULL;
	}


	int count = (int)PyDict_Size(paramDict);
	CComSafeArray<IUnknown*> conditions(count);

	PyObject* key;
	PyObject* value;
	Py_ssize_t pos = 0;
	int index = 0;

	while (PyDict_Next(paramDict, &pos, &key, &value))
	{
		CComVariant valueObj;
		bool valueSet = false;

		wchar_t* tmp = PyUnicode_AsWideCharString(key, NULL);
		PROPERTYID propId = GetPropertyIdForName(tmp);
		PyMem_FREE(tmp);

		if (PyUnicode_Check(value))
		{
			tmp = PyUnicode_AsWideCharString(value, NULL);
			valueObj.SetByRef(SysAllocString(tmp));
			PyMem_FREE(tmp);
			valueSet = true;
		}
		else if (PyBool_Check(value))
		{
			valueObj.vt = VT_BOOL;
			if (value == Py_True)
				valueObj.boolVal = VARIANT_TRUE;
			else
				valueObj.boolVal = VARIANT_FALSE;
			valueSet = true;
		}
		else if (PyLong_Check(value))
		{
			long iVal = PyLong_AsLong(value);
			double dVal = PyLong_AsDouble(value);
			if (iVal == dVal)
			{
				valueObj.vt = VT_I4;
				valueObj.iVal = (SHORT)iVal;
			}
			else
			{
				valueObj.vt = VT_R8;
				valueObj.dblVal = dVal;
			}
			valueSet = true;
		}

		if (valueSet)
		{
			IUIAutomationCondition* condition;
			hr = pAutomation->CreatePropertyCondition(propId, valueObj, &condition);
			if (FAILED(hr))
			{
				goto cleanup;
			}
			conditions.SetAt(index++, condition);
		}
	}


	if (count > 1)
	{
		IUIAutomationCondition* andCondition;
		hr = pAutomation->CreateAndConditionFromArray(conditions, &andCondition);
		if (SUCCEEDED(hr))
		{
			hr = fromElement->FindFirst(scope, andCondition, &result);
		}
	}
	else if (count == 1)
	{
		IUIAutomationCondition* cond = (IUIAutomationCondition*)conditions[0].Detach();
		hr = fromElement->FindFirst(scope, cond, &result);
		cond->Release();
	}

cleanup:
	// There's probably a leak here somewhere.

	return result;
}
IUIAutomationElementArray* pyutils::AutomationManager::FindAll(IUIAutomationElement* fromElement, TreeScope scope, automationcondition_object* conditions)
{
	std::vector<IUIAutomationCondition*> childConditions;
	IUIAutomationCondition* searchCondition;
	GetConditionForPyObject((PyObject*)conditions, &searchCondition, childConditions);

	IUIAutomationElementArray* result = NULL;
	if (!PyErr_Occurred())
	{
		HRESULT hr = fromElement->FindAll(scope, searchCondition, &result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
		}
	}

	for (auto iter = childConditions.begin(); iter != childConditions.end(); iter++)
	{
		(*iter)->Release();
	}

	return result;
}
IUIAutomationElementArray* pyutils::AutomationManager::FindAll(IUIAutomationElement* fromElement, TreeScope scope, PyObject* paramDict)
{
	HRESULT hr;
	IUIAutomationElementArray* result = NULL;

	if (PyObject_TypeCheck(paramDict, &AutomationConditionType))
	{
		return FindAll(fromElement, scope, (automationcondition_object*)paramDict);
	}

	if (!PyDict_Check(paramDict))
	{
		PyErr_SetString(PyExc_TypeError, "Parameters must be in the form of a dictionary of property-value pairs");
		return NULL;
	}

	if (fromElement == NULL)
	{
		PyErr_SetString(PyExc_ReferenceError, "No element specified");
		return NULL;
	}


	int count = (int)PyDict_Size(paramDict);
	CComSafeArray<IUnknown*> conditions(count);


	PyObject* key;
	PyObject* value;
	Py_ssize_t pos = 0;
	int index = 0;

	while (PyDict_Next(paramDict, &pos, &key, &value))
	{
		CComVariant valueObj;
		bool valueSet = false;

		wchar_t* tmp = PyUnicode_AsWideCharString(key, NULL);
		PROPERTYID propId = GetPropertyIdForName(tmp);
		PyMem_FREE(tmp);

		if (PyUnicode_Check(value))
		{
			tmp = PyUnicode_AsWideCharString(value, NULL);
			valueObj.SetByRef(SysAllocString(tmp));
			PyMem_FREE(tmp);
			valueSet = true;
		}
		else if (PyBool_Check(value))
		{
			valueObj.vt = VT_BOOL;
			if (value == Py_True)
				valueObj.boolVal = VARIANT_TRUE;
			else
				valueObj.boolVal = VARIANT_FALSE;
			valueSet = true;
		}
		else if (PyLong_Check(value))
		{
			long iVal = PyLong_AsLong(value);
			double dVal = PyLong_AsDouble(value);
			if (iVal == dVal)
			{
				valueObj.vt = VT_I4;
				valueObj.iVal = (SHORT)iVal;
			}
			else
			{
				valueObj.vt = VT_R8;
				valueObj.dblVal = dVal;
			}
			valueSet = true;
		}

		if (valueSet)
		{
			IUIAutomationCondition* condition;
			hr = pAutomation->CreatePropertyCondition(propId, valueObj, &condition);
			if (FAILED(hr))
			{
				goto cleanup;
			}
			conditions.SetAt(index++, condition);
		}
	}


	if (count > 1)
	{
		IUIAutomationCondition* andCondition;
		hr = pAutomation->CreateAndConditionFromArray(conditions, &andCondition);
		if (SUCCEEDED(hr))
		{
			hr = fromElement->FindAll(scope, andCondition, &result);
		}
	}
	else if (count == 1)
	{
		IUIAutomationCondition* cond = (IUIAutomationCondition*)conditions[0].Detach();
		hr = fromElement->FindAll(scope, cond, &result);
		cond->Release();
	}

cleanup:
	// There's probably a leak here somewhere.

	return result;
}

#pragma endregion


#pragma region UIAutomation Object

#pragma region Instance Implementation Methods

#define ISTRUE(val) (val.vt == VT_BOOL && val.boolVal == VARIANT_TRUE)

PyObject* uiautomation_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
	HRESULT hr = S_OK;
	uiautomation_object* self = (uiautomation_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		if (args != NULL && kwargs != (PyObject*)-1)
		{
			self->pElement = pyutils::AutomationManager::GetElementsFromRoot(args);
		}
		else if (args != NULL && kwargs == (PyObject*)-1)
		{
			self->pElement = (IUIAutomationElement*)args;
		}
		if (self->pElement != NULL)
		{
			CComVariant val;

			hr = self->pElement->GetCurrentPropertyValue(UIA_IsInvokePatternAvailablePropertyId, &val);
			if (FAILED(hr)) goto failed;
			if (ISTRUE(val))
			{
				hr = self->pElement->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&self->pInvoke));
				if (FAILED(hr)) goto failed;
			}

			hr = self->pElement->GetCurrentPropertyValue(UIA_IsWindowPatternAvailablePropertyId, &val);
			if (FAILED(hr)) goto failed;
			if (ISTRUE(val))
			{
				hr = self->pElement->GetCurrentPatternAs(UIA_WindowPatternId, IID_PPV_ARGS(&self->pWindow));
				if (FAILED(hr)) goto failed;
			}

			hr = self->pElement->GetCurrentPropertyValue(UIA_IsExpandCollapsePatternAvailablePropertyId, &val);
			if (FAILED(hr)) goto failed;
			if (ISTRUE(val))
			{
				hr = self->pElement->GetCurrentPatternAs(UIA_ExpandCollapsePatternId, IID_PPV_ARGS(&self->pExpandCollapse));
				if (FAILED(hr)) goto failed;
			}

			hr = self->pElement->GetCurrentPropertyValue(UIA_IsValuePatternAvailablePropertyId, &val);
			if (FAILED(hr)) goto failed;
			if (ISTRUE(val))
			{
				hr = self->pElement->GetCurrentPatternAs(UIA_ValuePatternId, IID_PPV_ARGS(&self->pValue));
				if (FAILED(hr)) goto failed;
			}

			hr = self->pElement->GetCurrentPropertyValue(UIA_IsSelectionPatternAvailablePropertyId, &val);
			if (FAILED(hr)) goto failed;
			if (ISTRUE(val))
			{
				hr = self->pElement->GetCurrentPatternAs(UIA_SelectionPatternId, IID_PPV_ARGS(&self->pSelection));
				if (FAILED(hr)) goto failed;
			}

			hr = self->pElement->GetCurrentPropertyValue(UIA_IsSelectionItemPatternAvailablePropertyId, &val);
			if (FAILED(hr)) goto failed;
			if (ISTRUE(val))
			{
				hr = self->pElement->GetCurrentPatternAs(UIA_SelectionItemPatternId, IID_PPV_ARGS(&self->pSelectionItem));
				if (FAILED(hr)) goto failed;
			}

			hr = self->pElement->GetCurrentPropertyValue(UIA_IsLegacyIAccessiblePatternAvailablePropertyId, &val);
			if (FAILED(hr)) goto failed;
			if (ISTRUE(val))
			{
				hr = self->pElement->GetCurrentPatternAs(UIA_LegacyIAccessiblePatternId, IID_PPV_ARGS(&self->pLegacy));
				if (FAILED(hr)) goto failed;
			}

			hr = self->pElement->GetCurrentPropertyValue(UIA_IsTogglePatternAvailablePropertyId, &val);
			if (FAILED(hr)) goto failed;
			if (ISTRUE(val))
			{
				hr = self->pElement->GetCurrentPatternAs(UIA_TogglePatternId, IID_PPV_ARGS(&self->pToggle));
				if (FAILED(hr)) goto failed;
			}

		}
		else
		{
			Py_DECREF(self);
			PyErr_SetString(PyExc_AssertionError, "No such element in automation tree.");
			return NULL;
		}
	}
	return (PyObject*)self;

failed:
	PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
	if (self != NULL)
	{
		if (self->pToggle)
		{
			self->pToggle->Release();
			self->pToggle = NULL;
		}
		if (self->pLegacy)
		{
			self->pLegacy->Release();
			self->pLegacy = NULL;
		}
		if (self->pSelectionItem)
		{
			self->pSelectionItem->Release();
			self->pSelectionItem = NULL;
		}
		if (self->pSelection)
		{
			self->pSelection->Release();
			self->pSelection = NULL;
		}
		if (self->pValue)
		{
			self->pValue->Release();
			self->pValue = NULL;
		}
		if (self->pExpandCollapse)
		{
			self->pExpandCollapse->Release();
			self->pExpandCollapse = NULL;
		}
		if (self->pWindow)
		{
			self->pWindow->Release();
			self->pWindow = NULL;
		}
		if (self->pInvoke)
		{
			self->pInvoke->Release();
			self->pInvoke = NULL;
		}
	}
	Py_XDECREF(self);
	return NULL;
}
int uiautomation_init(uiautomation_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
void uiautomation_dealloc(uiautomation_object* self)
{
	if (self->pToggle)
	{
		self->pToggle->Release();
		self->pToggle = NULL;
	}
	if (self->pLegacy)
	{
		self->pLegacy->Release();
		self->pLegacy = NULL;
	}
	if (self->pSelectionItem)
	{
		self->pSelectionItem->Release();
		self->pSelectionItem = NULL;
	}
	if (self->pSelection)
	{
		self->pSelection->Release();
		self->pSelection = NULL;
	}
	if (self->pValue)
	{
		self->pValue->Release();
		self->pValue = NULL;
	}
	if (self->pExpandCollapse)
	{
		self->pExpandCollapse->Release();
		self->pExpandCollapse = NULL;
	}
	if (self->pWindow)
	{
		self->pWindow->Release();
		self->pWindow = NULL;
	}
	if (self->pInvoke)
	{
		self->pInvoke->Release();
		self->pInvoke = NULL;
	}
}
PyObject* uiautomation_getattro(PyObject* selfObj, PyObject* attr_name)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pElement)
	{
		wchar_t* tmp = PyUnicode_AsWideCharString(attr_name, NULL);
		PROPERTYID propId = GetPropertyIdForName(tmp);
		PyMem_FREE(tmp);

		if (propId != 0)
		{
			PyObject* result = pyutils::AutomationManager::GetProperty(self->pElement, propId);
			return result;
		}
		return PyObject_GenericGetAttr(selfObj, attr_name);
	}
	else
	{
		PyErr_SetString(PyExc_Exception, "No current element");
	}
	return NULL;
}
PyObject* uiautomation_str(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	return pyutils::AutomationManager::GetProperty(self->pElement, UIA_NamePropertyId);
}

#pragma endregion

#pragma region Class Methods

PyObject* uiautomation_getrootnode(PyObject* typeObj, PyObject* args)
{
	IUIAutomationElement* element = pyutils::AutomationManager::GetRootNode();
	PyObject* result = uiautomation_new((PyTypeObject*)typeObj, (PyObject*)element, (PyObject*)-1);
	return result;
}

#pragma endregion

#pragma region Basic Automation Element Functionality

PyObject* uiautomation_getpropertylist(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	return pyutils::AutomationManager::GetPropertyList();
}
PyObject* uiautomation_getproperty(PyObject* selfObj, PyObject* args)
{
	PyObject* name = NULL;
	if (!PyArg_ParseTuple(args, "U", &name))
		return NULL;

	uiautomation_object* self = (uiautomation_object*)selfObj;
	return pyutils::AutomationManager::GetProperty(self->pElement, name);
}
PyObject* uiautomation_findchild(PyObject* selfObj, PyObject* args)
{
	PyObject* dict = NULL;
	if (!PyArg_ParseTuple(args, "O", &dict))
		return NULL;

	bool createdDict = false;
	if (PyUnicode_Check(dict))
	{
		PyObject* name = dict;
		dict = PyDict_New();
		PyDict_SetItemString(dict, "Name", name);
	}
	else if (!PyDict_Check(dict) && !PyObject_TypeCheck(dict, &AutomationConditionType))
	{
		PyErr_SetString(PyExc_TypeError, "Search terms must be a dictionary, or AutomationCondition object.");
		return NULL;
	}

	uiautomation_object* self = (uiautomation_object*)selfObj;
	IUIAutomationElement* foundElement = pyutils::AutomationManager::FindFirst(self->pElement, TreeScope_Children, dict);

	PyObject* result = NULL;
	if (foundElement != NULL)
	{
		result = uiautomation_new(Py_TYPE(selfObj), (PyObject*)(foundElement), (PyObject*)-1);
	}
	else
	{
		PyErr_SetString(PyExc_KeyError, "Not found");
	}

	if (createdDict) Py_DECREF(dict);

	return result;
}
PyObject* uiautomation_findchildren(PyObject* selfObj, PyObject* args)
{
	PyObject* dict = NULL;
	if (!PyArg_ParseTuple(args, "O", &dict))
		return NULL;

	if (!PyDict_Check(dict) && !PyObject_TypeCheck(dict, &AutomationConditionType))
	{
		PyErr_SetString(PyExc_TypeError, "Search terms must be a dictionary, or AutomationCondition object.");
		return NULL;
	}

	PyObject* result = PyList_New(0);

	uiautomation_object* self = (uiautomation_object*)selfObj;
	IUIAutomationElementArray* found = pyutils::AutomationManager::FindAll(self->pElement, TreeScope_Children, dict);
	if (found != NULL)
	{
		int length = 0;
		HRESULT hr = found->get_Length(&length);
		if (FAILED(hr))
		{
			Py_DECREF(result);
			found->Release();
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		for (int i = 0; i < length; i++)
		{
			IUIAutomationElement* element;
			hr = found->GetElement(i, &element);
			if (FAILED(hr))
			{
				found->Release();
				Py_DECREF(result);
				PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
				return NULL;
			}
			PyList_Append(result, uiautomation_new(Py_TYPE(selfObj), (PyObject*)element, (PyObject*)-1));
		}
		found->Release();
	}

	return result;
}
PyObject* uiautomation_finddescendant(PyObject* selfObj, PyObject* args)
{
	PyObject* dict = NULL;
	if (!PyArg_ParseTuple(args, "O", &dict))
		return NULL;

	bool createdDict = false;
	if (PyUnicode_Check(dict))
	{
		PyObject* name = dict;
		dict = PyDict_New();
		PyDict_SetItemString(dict, "Name", name);
	}
	else if (!PyDict_Check(dict) && !PyObject_TypeCheck(dict, &AutomationConditionType))
	{
		PyErr_SetString(PyExc_TypeError, "Search terms must be a dictionary, or AutomationCondition object.");
		return NULL;
	}

	uiautomation_object* self = (uiautomation_object*)selfObj;
	IUIAutomationElement* foundElement = pyutils::AutomationManager::FindFirst(self->pElement, TreeScope_Subtree, dict);

	PyObject* result = NULL;
	if (foundElement != nullptr)
	{
		result = uiautomation_new(Py_TYPE(selfObj), (PyObject*)foundElement, (PyObject*)-1);
	}
	else
	{
		PyErr_SetString(PyExc_KeyError, "Not found");
	}

	if (createdDict) Py_DECREF(dict);

	return result;
}
PyObject* uiautomation_finddescendants(PyObject* selfObj, PyObject* args)
{
	PyObject* dict = NULL;
	if (!PyArg_ParseTuple(args, "O", &dict))
		return NULL;

	if (!PyDict_Check(dict) && !PyObject_TypeCheck(dict, &AutomationConditionType))
	{
		PyErr_SetString(PyExc_TypeError, "Search terms must be a dictionary, or AutomationCondition object.");
		return NULL;
	}

	PyObject* result = PyList_New(0);

	uiautomation_object* self = (uiautomation_object*)selfObj;
	IUIAutomationElementArray* found = pyutils::AutomationManager::FindAll(self->pElement, TreeScope_Descendants, dict);
	if (found != NULL)
	{
		int length = 0;
		HRESULT hr = found->get_Length(&length);
		if (FAILED(hr))
		{
			Py_DECREF(result);
			found->Release();
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		for (int i = 0; i < length; i++)
		{
			IUIAutomationElement* element;
			hr = found->GetElement(i, &element);
			if (FAILED(hr))
			{
				found->Release();
				Py_DECREF(result);
				PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
				return NULL;
			}
			PyList_Append(result, uiautomation_new(Py_TYPE(selfObj), (PyObject*)element, (PyObject*)-1));
		}
		found->Release();
	}

	return result;
}
PyObject* uiautomation_setfocus(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pElement)
	{
		HRESULT hr = self->pElement->SetFocus();
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}
	}
	Py_RETURN_NONE;
}

#pragma endregion

#pragma region Expand/Collapse Pattern

PyObject* uiautomation_supportsexpandcollapse(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pExpandCollapse)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}
PyObject* uiautomation_expand(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pExpandCollapse)
	{
		HRESULT hr = self->pExpandCollapse->Expand();
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item cannot be expanded.");
		return NULL;
	}
}
PyObject* uiautomation_collapse(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pExpandCollapse)
	{
		HRESULT hr = self->pExpandCollapse->Collapse();
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item cannot be collapsed.");
		return NULL;
	}
}
PyObject* uiautomation_isexpanded(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pExpandCollapse)
	{
		ExpandCollapseState state = (ExpandCollapseState)-1;
		HRESULT hr = self->pExpandCollapse->get_CurrentExpandCollapseState(&state);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (state == ExpandCollapseState::ExpandCollapseState_Expanded)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item cannot be expanded/collapsed.");
		return NULL;
	}
}
PyObject* uiautomation_iscollapsed(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pExpandCollapse)
	{
		ExpandCollapseState state = (ExpandCollapseState)-1;
		HRESULT hr = self->pExpandCollapse->get_CurrentExpandCollapseState(&state);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (state == ExpandCollapseState::ExpandCollapseState_Collapsed)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item cannot be expanded/collapsed.");
		return NULL;
	}
}
PyObject* uiautomation_isleafnode(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pExpandCollapse)
	{
		ExpandCollapseState state = (ExpandCollapseState)-1;
		HRESULT hr = self->pExpandCollapse->get_CurrentExpandCollapseState(&state);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (state == ExpandCollapseState::ExpandCollapseState_PartiallyExpanded)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item cannot be expanded/collapsed.");
		return NULL;
	}
}
PyObject* uiautomation_ispartialexpanded(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pExpandCollapse)
	{
		ExpandCollapseState state = (ExpandCollapseState)-1;
		HRESULT hr = self->pExpandCollapse->get_CurrentExpandCollapseState(&state);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (state == ExpandCollapseState::ExpandCollapseState_PartiallyExpanded)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item cannot be expanded/collapsed.");
		return NULL;
	}
}
PyObject* uiautomation_getexpandstate(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pExpandCollapse)
	{
		ExpandCollapseState state = (ExpandCollapseState)-1;
		HRESULT hr = self->pExpandCollapse->get_CurrentExpandCollapseState(&state);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		switch (state)
		{
		case ExpandCollapseState::ExpandCollapseState_Collapsed:
			return PyUnicode_FromString("Collapsed");
		case ExpandCollapseState::ExpandCollapseState_Expanded:
			return PyUnicode_FromString("Expanded");
		case ExpandCollapseState::ExpandCollapseState_LeafNode:
			return PyUnicode_FromString("LeafNode");
		case ExpandCollapseState::ExpandCollapseState_PartiallyExpanded:
			return PyUnicode_FromString("PartiallyExpanded");
		default:
			return PyUnicode_FromString("Unknown");
		}
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item cannot be expanded/collapsed.");
		return NULL;
	}
}

#pragma endregion

#pragma region Invoke Pattern

PyObject* uiautomation_supportsinvoke(PyObject* selfObj)
{
	if (((uiautomation_object*)selfObj)->pInvoke)
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}
PyObject* uiautomation_invoke(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pInvoke)
	{
		HRESULT hr = self->pInvoke->Invoke();
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item is not invokable");
		return NULL;
	}
}

#pragma endregion

#pragma region Legacy IAccessible Pattern

static PyObject* uiautomation_supportslegacy(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}

// properties
PyObject* uiautomation_legacychildid(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		int childId = 0;
		HRESULT hr = self->pLegacy->get_CurrentChildId(&childId);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}
		return PyLong_FromLong(childId);
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}
PyObject* uiautomation_legacygetdefaultaction(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		BSTR action;
		HRESULT hr = self->pLegacy->get_CurrentDefaultAction(&action);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		PyObject* result;
		if (action != NULL)
		{
			result = PyUnicode_FromWideChar(action, -1);
			SysFreeString(action);
		}
		else
		{
			result = Py_None;
			Py_INCREF(result);
		}

		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}
PyObject* uiautomation_legacygetdescription(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		BSTR description;
		HRESULT hr = self->pLegacy->get_CurrentDescription(&description);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		PyObject* result;
		if (description != NULL)
		{
			result = PyUnicode_FromWideChar(description, -1);
			SysFreeString(description);
		}
		else
		{
			result = Py_None;
			Py_INCREF(result);
		}

		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}
PyObject* uiautomation_legacygethelp(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		BSTR help;
		HRESULT hr = self->pLegacy->get_CurrentHelp(&help);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		PyObject* result;
		if (help != NULL)
		{
			result = PyUnicode_FromWideChar(help, -1);
			SysFreeString(help);
		}
		else
		{
			result = Py_None;
			Py_INCREF(result);
		}

		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}
PyObject* uiautomation_legacygetkeyboardshortcut(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		BSTR key;
		HRESULT hr = self->pLegacy->get_CurrentKeyboardShortcut(&key);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		PyObject* result;
		if (key != NULL)
		{
			result = PyUnicode_FromWideChar(key, -1);
			SysFreeString(key);
		}
		else
		{
			result = Py_None;
			Py_INCREF(result);
		}

		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}
PyObject* uiautomation_legacygetname(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		BSTR name;
		HRESULT hr = self->pLegacy->get_CurrentName(&name);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		PyObject* result;
		if (name != NULL)
		{
			result = PyUnicode_FromWideChar(name, -1);
			SysFreeString(name);
		}
		else
		{
			result = Py_None;
			Py_INCREF(result);
		}

		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}
PyObject* uiautomation_legacygetrole(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		DWORD dwRole = 0;
		HRESULT hr = self->pLegacy->get_CurrentRole(&dwRole);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		PyObject* result = NULL;

		if (dwRole == ROLE_SYSTEM_TITLEBAR)
			result = PyUnicode_FromString("Titlebar");
		if (dwRole == ROLE_SYSTEM_MENUBAR)
			result = PyUnicode_FromString("Menubar");
		if (dwRole == ROLE_SYSTEM_SCROLLBAR)
			result = PyUnicode_FromString("Scrollbar");
		if (dwRole == ROLE_SYSTEM_GRIP)
			result = PyUnicode_FromString("Grip");
		if (dwRole == ROLE_SYSTEM_SOUND)
			result = PyUnicode_FromString("Sound");
		if (dwRole == ROLE_SYSTEM_CURSOR)
			result = PyUnicode_FromString("Cursor");
		if (dwRole == ROLE_SYSTEM_CARET)
			result = PyUnicode_FromString("Caret");
		if (dwRole == ROLE_SYSTEM_ALERT)
			result = PyUnicode_FromString("Alert");
		if (dwRole == ROLE_SYSTEM_WINDOW)
			result = PyUnicode_FromString("Window");
		if (dwRole == ROLE_SYSTEM_CLIENT)
			result = PyUnicode_FromString("Client");
		if (dwRole == ROLE_SYSTEM_MENUPOPUP)
			result = PyUnicode_FromString("Menupopup");
		if (dwRole == ROLE_SYSTEM_MENUITEM)
			result = PyUnicode_FromString("Menuitem");
		if (dwRole == ROLE_SYSTEM_TOOLTIP)
			result = PyUnicode_FromString("Tooltip");
		if (dwRole == ROLE_SYSTEM_APPLICATION)
			result = PyUnicode_FromString("Application");
		if (dwRole == ROLE_SYSTEM_DOCUMENT)
			result = PyUnicode_FromString("Document");
		if (dwRole == ROLE_SYSTEM_PANE)
			result = PyUnicode_FromString("Pane");
		if (dwRole == ROLE_SYSTEM_CHART)
			result = PyUnicode_FromString("Chart");
		if (dwRole == ROLE_SYSTEM_DIALOG)
			result = PyUnicode_FromString("Dialog");
		if (dwRole == ROLE_SYSTEM_BORDER)
			result = PyUnicode_FromString("Border");
		if (dwRole == ROLE_SYSTEM_GROUPING)
			result = PyUnicode_FromString("Grouping");
		if (dwRole == ROLE_SYSTEM_SEPARATOR)
			result = PyUnicode_FromString("Separator");
		if (dwRole == ROLE_SYSTEM_TOOLBAR)
			result = PyUnicode_FromString("Toolbar");
		if (dwRole == ROLE_SYSTEM_STATUSBAR)
			result = PyUnicode_FromString("Statusbar");
		if (dwRole == ROLE_SYSTEM_TABLE)
			result = PyUnicode_FromString("Table");
		if (dwRole == ROLE_SYSTEM_COLUMNHEADER)
			result = PyUnicode_FromString("Columnheader");
		if (dwRole == ROLE_SYSTEM_ROWHEADER)
			result = PyUnicode_FromString("Rowheader");
		if (dwRole == ROLE_SYSTEM_COLUMN)
			result = PyUnicode_FromString("Column");
		if (dwRole == ROLE_SYSTEM_ROW)
			result = PyUnicode_FromString("Row");
		if (dwRole == ROLE_SYSTEM_CELL)
			result = PyUnicode_FromString("Cell");
		if (dwRole == ROLE_SYSTEM_LINK)
			result = PyUnicode_FromString("Link");
		if (dwRole == ROLE_SYSTEM_HELPBALLOON)
			result = PyUnicode_FromString("Helpballoon");
		if (dwRole == ROLE_SYSTEM_CHARACTER)
			result = PyUnicode_FromString("Character");
		if (dwRole == ROLE_SYSTEM_LIST)
			result = PyUnicode_FromString("List");
		if (dwRole == ROLE_SYSTEM_LISTITEM)
			result = PyUnicode_FromString("Listitem");
		if (dwRole == ROLE_SYSTEM_OUTLINE)
			result = PyUnicode_FromString("Outline");
		if (dwRole == ROLE_SYSTEM_OUTLINEITEM)
			result = PyUnicode_FromString("Outlineitem");
		if (dwRole == ROLE_SYSTEM_PAGETAB)
			result = PyUnicode_FromString("Pagetab");
		if (dwRole == ROLE_SYSTEM_PROPERTYPAGE)
			result = PyUnicode_FromString("Propertypage");
		if (dwRole == ROLE_SYSTEM_INDICATOR)
			result = PyUnicode_FromString("Indicator");
		if (dwRole == ROLE_SYSTEM_GRAPHIC)
			result = PyUnicode_FromString("Graphic");
		if (dwRole == ROLE_SYSTEM_STATICTEXT)
			result = PyUnicode_FromString("Statictext");
		if (dwRole == ROLE_SYSTEM_TEXT)
			result = PyUnicode_FromString("Text");
		if (dwRole == ROLE_SYSTEM_PUSHBUTTON)
			result = PyUnicode_FromString("Pushbutton");
		if (dwRole == ROLE_SYSTEM_CHECKBUTTON)
			result = PyUnicode_FromString("Checkbutton");
		if (dwRole == ROLE_SYSTEM_RADIOBUTTON)
			result = PyUnicode_FromString("Radiobutton");
		if (dwRole == ROLE_SYSTEM_COMBOBOX)
			result = PyUnicode_FromString("Combobox");
		if (dwRole == ROLE_SYSTEM_DROPLIST)
			result = PyUnicode_FromString("Droplist");
		if (dwRole == ROLE_SYSTEM_PROGRESSBAR)
			result = PyUnicode_FromString("Progressbar");
		if (dwRole == ROLE_SYSTEM_DIAL)
			result = PyUnicode_FromString("Dial");
		if (dwRole == ROLE_SYSTEM_HOTKEYFIELD)
			result = PyUnicode_FromString("Hotkeyfield");
		if (dwRole == ROLE_SYSTEM_SLIDER)
			result = PyUnicode_FromString("Slider");
		if (dwRole == ROLE_SYSTEM_SPINBUTTON)
			result = PyUnicode_FromString("Spinbutton");
		if (dwRole == ROLE_SYSTEM_DIAGRAM)
			result = PyUnicode_FromString("Diagram");
		if (dwRole == ROLE_SYSTEM_ANIMATION)
			result = PyUnicode_FromString("Animation");
		if (dwRole == ROLE_SYSTEM_EQUATION)
			result = PyUnicode_FromString("Equation");
		if (dwRole == ROLE_SYSTEM_BUTTONDROPDOWN)
			result = PyUnicode_FromString("Buttondropdown");
		if (dwRole == ROLE_SYSTEM_BUTTONMENU)
			result = PyUnicode_FromString("Buttonmenu");
		if (dwRole == ROLE_SYSTEM_BUTTONDROPDOWNGRID)
			result = PyUnicode_FromString("Buttondropdowngrid");
		if (dwRole == ROLE_SYSTEM_WHITESPACE)
			result = PyUnicode_FromString("Whitespace");
		if (dwRole == ROLE_SYSTEM_PAGETABLIST)
			result = PyUnicode_FromString("Pagetablist");
		if (dwRole == ROLE_SYSTEM_CLOCK)
			result = PyUnicode_FromString("Clock");
		if (dwRole == ROLE_SYSTEM_SPLITBUTTON)
			result = PyUnicode_FromString("Splitbutton");
		if (dwRole == ROLE_SYSTEM_IPADDRESS)
			result = PyUnicode_FromString("Ipaddress");
		if (dwRole == ROLE_SYSTEM_OUTLINEBUTTON)
			result = PyUnicode_FromString("Outlinebutton");

		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}
PyObject* uiautomation_legacygetstate(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		DWORD dwState = 0;
		HRESULT hr = self->pLegacy->get_CurrentState(&dwState);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		PyObject* result = PyList_New(0);

		if (dwState & STATE_SYSTEM_UNAVAILABLE)
			PyList_Append(result, PyUnicode_FromString("Unavailable"));
		if (dwState & STATE_SYSTEM_SELECTED)
			PyList_Append(result, PyUnicode_FromString("Selected"));
		if (dwState & STATE_SYSTEM_FOCUSED)
			PyList_Append(result, PyUnicode_FromString("Focused"));
		if (dwState & STATE_SYSTEM_PRESSED)
			PyList_Append(result, PyUnicode_FromString("Pressed"));
		if (dwState & STATE_SYSTEM_CHECKED)
			PyList_Append(result, PyUnicode_FromString("Checked"));
		if (dwState & STATE_SYSTEM_MIXED)
			PyList_Append(result, PyUnicode_FromString("Mixed"));
		if (dwState & STATE_SYSTEM_INDETERMINATE)
			PyList_Append(result, PyUnicode_FromString("Indeterminate"));
		if (dwState & STATE_SYSTEM_READONLY)
			PyList_Append(result, PyUnicode_FromString("Readonly"));
		if (dwState & STATE_SYSTEM_HOTTRACKED)
			PyList_Append(result, PyUnicode_FromString("Hottracked"));
		if (dwState & STATE_SYSTEM_DEFAULT)
			PyList_Append(result, PyUnicode_FromString("Default"));
		if (dwState & STATE_SYSTEM_EXPANDED)
			PyList_Append(result, PyUnicode_FromString("Expanded"));
		if (dwState & STATE_SYSTEM_COLLAPSED)
			PyList_Append(result, PyUnicode_FromString("Collapsed"));
		if (dwState & STATE_SYSTEM_BUSY)
			PyList_Append(result, PyUnicode_FromString("Busy"));
		if (dwState & STATE_SYSTEM_FLOATING)
			PyList_Append(result, PyUnicode_FromString("Floating"));
		if (dwState & STATE_SYSTEM_MARQUEED)
			PyList_Append(result, PyUnicode_FromString("Marqueed"));
		if (dwState & STATE_SYSTEM_ANIMATED)
			PyList_Append(result, PyUnicode_FromString("Animated"));
		if (dwState & STATE_SYSTEM_INVISIBLE)
			PyList_Append(result, PyUnicode_FromString("Invisible"));
		if (dwState & STATE_SYSTEM_OFFSCREEN)
			PyList_Append(result, PyUnicode_FromString("Offscreen"));
		if (dwState & STATE_SYSTEM_SIZEABLE)
			PyList_Append(result, PyUnicode_FromString("Sizeable"));
		if (dwState & STATE_SYSTEM_MOVEABLE)
			PyList_Append(result, PyUnicode_FromString("Moveable"));
		if (dwState & STATE_SYSTEM_SELFVOICING)
			PyList_Append(result, PyUnicode_FromString("Selfvoicing"));
		if (dwState & STATE_SYSTEM_FOCUSABLE)
			PyList_Append(result, PyUnicode_FromString("Focusable"));
		if (dwState & STATE_SYSTEM_SELECTABLE)
			PyList_Append(result, PyUnicode_FromString("Selectable"));
		if (dwState & STATE_SYSTEM_LINKED)
			PyList_Append(result, PyUnicode_FromString("Linked"));
		if (dwState & STATE_SYSTEM_TRAVERSED)
			PyList_Append(result, PyUnicode_FromString("Traversed"));
		if (dwState & STATE_SYSTEM_MULTISELECTABLE)
			PyList_Append(result, PyUnicode_FromString("Multiselectable"));
		if (dwState & STATE_SYSTEM_EXTSELECTABLE)
			PyList_Append(result, PyUnicode_FromString("Extselectable"));
		if (dwState & STATE_SYSTEM_ALERT_LOW)
			PyList_Append(result, PyUnicode_FromString("Alert_Low"));
		if (dwState & STATE_SYSTEM_ALERT_MEDIUM)
			PyList_Append(result, PyUnicode_FromString("Alert_Medium"));
		if (dwState & STATE_SYSTEM_ALERT_HIGH)
			PyList_Append(result, PyUnicode_FromString("Alert_High"));
		if (dwState & STATE_SYSTEM_PROTECTED)
			PyList_Append(result, PyUnicode_FromString("Protected"));


		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}
PyObject* uiautomation_legacygetvalue(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		BSTR value;
		HRESULT hr = self->pLegacy->get_CurrentValue(&value);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		PyObject* result;
		if (value != NULL)
		{
			result = PyUnicode_FromWideChar(value, -1);
			SysFreeString(value);
		}
		else
		{
			result = Py_None;
			Py_INCREF(result);
		}

		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}

// methods
PyObject* uiautomation_legacydodefaultaction(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		HRESULT hr = self->pLegacy->DoDefaultAction();
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}
PyObject* uiautomation_legacygetcurrentselection(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		IUIAutomationElementArray* pArray;
		HRESULT hr = self->pLegacy->GetCurrentSelection(&pArray);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		PyObject* result = PyList_New(0);

		int count = 0;
		hr = pArray->get_Length(&count);
		if (FAILED(hr))
		{
			pArray->Release();
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		for (int i = 0; i < count; i++)
		{
			IUIAutomationElement* element;
			hr = pArray->GetElement(i, &element);
			if (FAILED(hr))
			{
				pArray->Release();
				PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
				return NULL;
			}

			element->AddRef();
			PyObject* obj = uiautomation_new(Py_TYPE(selfObj), (PyObject*)element, (PyObject*)-1);
			PyList_Append(result, obj);
		}

		pArray->Release();

		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}
PyObject* uiautomation_legacyselect(PyObject* selfObj, PyObject* args)
{
	long flags = 0;
	if (!PyArg_ParseTuple(args, "l", &flags))
		return NULL;

	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		HRESULT hr = self->pLegacy->Select(flags);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}
PyObject* uiautomation_legacysetvalue(PyObject* selfObj, PyObject* args)
{
	PyObject* value;
	if (!PyArg_ParseTuple(args, "U", &value))
		return NULL;

	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pLegacy)
	{
		wchar_t* val = PyUnicode_AsWideCharString(value, NULL);
		HRESULT hr = self->pLegacy->SetValue(val);
		PyMem_FREE(val);

		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the Legacy IAccessible pattern.");
		return NULL;
	}
}

#pragma endregion

#pragma region Selection Pattern

PyObject* uiautomation_supportsselection(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pSelection)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}
PyObject* uiautomation_canselectmultiple(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pSelection)
	{
		BOOL result = FALSE;
		HRESULT hr = self->pSelection->get_CurrentCanSelectMultiple(&result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}
		if (result == TRUE)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the selection pattern.");
		return NULL;
	}
	Py_RETURN_NONE;
}
PyObject* uiautomation_isselectionrequired(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pSelection)
	{
		BOOL result = FALSE;
		HRESULT hr = self->pSelection->get_CurrentIsSelectionRequired(&result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (result == TRUE)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the selection pattern.");
		return NULL;
	}
	Py_RETURN_NONE;
}
PyObject* uiautomation_getselections(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pSelection)
	{
		PyObject* result = PyList_New(0);
		/*auto selections = pyutils::AutomationManager::GetSelectionItems(self->pElement);
		for (int i = 0; i < selections->Length; i++)
		{
		gcroot<pyutils::AutomationElement^>* item = new gcroot<pyutils::AutomationElement^>(selections[i]);
		PyList_Append(result, uiautomation_new(Py_TYPE(selfObj), (PyObject*)item, (PyObject*)-1));
		delete item;
		}*/
		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the selection pattern.");
		return NULL;
	}
	Py_RETURN_NONE;
}

#pragma endregion

#pragma region Selection Item Pattern

PyObject* uiautomation_supportsselectionitem(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pSelectionItem)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}
PyObject* uiautomation_isselected(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pSelectionItem)
	{
		BOOL result = FALSE;
		HRESULT hr = self->pSelectionItem->get_CurrentIsSelected(&result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (result == TRUE)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the selection item pattern.");
		return NULL;
	}
	Py_RETURN_NONE;
}
PyObject* uiautomation_getselectioncontainer(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pSelectionItem)
	{
		IUIAutomationElement* pContainer;
		HRESULT hr = self->pSelectionItem->get_CurrentSelectionContainer(&pContainer);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}
		if (pContainer == NULL)
		{
			PyErr_SetString(PyExc_Exception, "No container element exists.");
			return NULL;
		}


		PyObject* result = uiautomation_new(Py_TYPE(selfObj), (PyObject*)pContainer, (PyObject*)-1);
		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the selection item pattern.");
		return NULL;
	}
	Py_RETURN_NONE;
}
PyObject* uiautomation_addtoselection(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pSelectionItem)
	{
		HRESULT hr = self->pSelectionItem->AddToSelection();
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the selection item pattern.");
		return NULL;
	}
	Py_RETURN_NONE;
}
PyObject* uiautomation_removefromselection(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pSelectionItem)
	{
		HRESULT hr = self->pSelectionItem->RemoveFromSelection();
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the selection item pattern.");
		return NULL;
	}
	Py_RETURN_NONE;
}
PyObject* uiautomation_select(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pSelectionItem)
	{
		HRESULT hr = self->pSelectionItem->Select();
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the selection item pattern.");
		return NULL;
	}
	Py_RETURN_NONE;
}

#pragma endregion

#pragma region Value Pattern

PyObject* uiautomation_supportsvalue(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pValue)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}
PyObject* uiautomation_valuereadonly(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pValue)
	{
		BOOL result = FALSE;
		HRESULT hr = self->pValue->get_CurrentIsReadOnly(&result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (result == TRUE)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the value pattern.");
		return NULL;
	}
}
PyObject* uiautomation_getvalue(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pValue)
	{
		BSTR result;
		HRESULT hr = self->pValue->get_CurrentValue(&result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		PyObject* retVal = PyUnicode_FromWideChar(result, -1);
		SysFreeString(result);

		return retVal;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the value pattern.");
		return NULL;
	}
}
PyObject* uiautomation_setvalue(PyObject* selfObj, PyObject* args)
{
	PyObject* val = NULL;
	if (!PyArg_ParseTuple(args, "U", &val))
		return NULL;

	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pValue)
	{
		BOOL readonly = FALSE;
		HRESULT hr = self->pValue->get_CurrentIsReadOnly(&readonly);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (readonly == TRUE)
		{
			PyErr_SetString(PyExc_Exception, "Value is read-only.");
			return NULL;
		}

		wchar_t* tmp = PyUnicode_AsWideCharString(val, NULL);
		BSTR value = SysAllocString(tmp);
		PyMem_FREE(tmp);

		hr = self->pValue->SetValue(value);
		SysFreeString(value);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the value pattern.");
		return NULL;
	}
}

#pragma endregion

#pragma region Window Pattern

PyObject* uiautomation_supportswindow(PyObject* selfObj)
{
	if (((uiautomation_object*)selfObj)->pWindow)
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}
PyObject* uiautomation_canmaximize(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pWindow)
	{
		BOOL result = FALSE;
		HRESULT hr = self->pWindow->get_CurrentCanMaximize(&result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (result == TRUE)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Element doesn't support the window pattern.");
		return NULL;
	}
}
PyObject* uiautomation_canminimize(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pWindow)
	{
		BOOL result = FALSE;
		HRESULT hr = self->pWindow->get_CurrentCanMinimize(&result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (result == TRUE)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Element doesn't support the window pattern.");
		return NULL;
	}
}
PyObject* uiautomation_ismodal(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pWindow)
	{
		BOOL result = FALSE;
		HRESULT hr = self->pWindow->get_CurrentIsModal(&result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (result == TRUE)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Element doesn't support the window pattern.");
		return NULL;
	}
}
PyObject* uiautomation_istopmost(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pWindow)
	{
		BOOL result = FALSE;
		HRESULT hr = self->pWindow->get_CurrentIsTopmost(&result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (result == TRUE)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Element doesn't support the window pattern.");
		return NULL;
	}
}
PyObject* uiautomation_getwindowvisualstate(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pWindow)
	{
		WindowVisualState result;
		HRESULT hr = self->pWindow->get_CurrentWindowVisualState(&result);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		switch (result)
		{
		case WindowVisualState::WindowVisualState_Maximized:
			return PyUnicode_FromString("Maximized");
		case WindowVisualState::WindowVisualState_Minimized:
			return PyUnicode_FromString("Minimized");
		case WindowVisualState::WindowVisualState_Normal:
			return PyUnicode_FromString("Normal");
		default:
			return PyUnicode_FromString("Unknown");
		}
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Element doesn't support the window pattern.");
		return NULL;
	}
}
PyObject* uiautomation_setwindowvisualstate(PyObject* selfObj, PyObject* args)
{
	PyObject* value = NULL;
	if (!PyArg_ParseTuple(args, "U", &value))
		return NULL;

	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pWindow)
	{
		wchar_t* valueChars = PyUnicode_AsWideCharString(value, NULL);

		HRESULT hr = S_OK;
		if (_wcsicmp(valueChars, L"Maximized") == 0)
		{
			hr = self->pWindow->SetWindowVisualState(WindowVisualState::WindowVisualState_Maximized);
		}
		else if (_wcsicmp(valueChars, L"Minimized") == 0)
		{
			hr = self->pWindow->SetWindowVisualState(WindowVisualState::WindowVisualState_Minimized);
		}
		else if (_wcsicmp(valueChars, L"Normal") == 0)
		{
			hr = self->pWindow->SetWindowVisualState(WindowVisualState::WindowVisualState_Normal);
		}
		else
		{
			PyErr_SetString(PyExc_ValueError, "Unknown Window State");
		}

		PyMem_FREE(valueChars);

		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Element doesn't support the window pattern.");
		return NULL;
	}
}
PyObject* uiautomation_getwindowinteractionstate(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pWindow)
	{
		WindowInteractionState state;
		HRESULT hr = self->pWindow->get_CurrentWindowInteractionState(&state);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		switch (state)
		{
		case WindowInteractionState::WindowInteractionState_Running:
			return PyUnicode_FromString("Running");
		case WindowInteractionState::WindowInteractionState_Closing:
			return PyUnicode_FromString("Closing");
		case WindowInteractionState::WindowInteractionState_ReadyForUserInteraction:
			return PyUnicode_FromString("ReadyForUserInteraction");
		case WindowInteractionState::WindowInteractionState_BlockedByModalWindow:
			return PyUnicode_FromString("BlockedByModalWindow");
		case WindowInteractionState::WindowInteractionState_NotResponding:
			return PyUnicode_FromString("NotResponding");
		default:
			return PyUnicode_FromString("Unknown");
		}

	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Element doesn't support the window pattern.");
		return NULL;
	}
}
PyObject* uiautomation_close(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pWindow)
	{
		HRESULT hr = self->pWindow->Close();
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Element doesn't support the window pattern.");
		return NULL;
	}
}
PyObject* uiautomation_waitforinputidle(PyObject* selfObj, PyObject* args)
{
	int timeout = 0;
	if (!PyArg_ParseTuple(args, "|i", &timeout))
		return NULL;

	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pWindow)
	{
		BOOL success = FALSE;
		HRESULT hr = self->pWindow->WaitForInputIdle(timeout, &success);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		if (success == TRUE)
		{
			Py_RETURN_TRUE;
		}
		else
		{
			Py_RETURN_FALSE;
		}
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Element doesn't support the window pattern.");
		return NULL;
	}
}

#pragma endregion

#pragma region Toggle Pattern

PyObject* uiautomation_supportstoggle(PyObject* selfObj)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pToggle)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}
PyObject* uiautomation_gettogglestate(uiautomation_object* self)
{
	if (self->pToggle)
	{
		ToggleState state = ToggleState_Indeterminate;
		HRESULT hr = self->pToggle->get_CurrentToggleState(&state);
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		switch (state)
		{
		case ToggleState_Indeterminate:
			Py_RETURN_NONE;
		case ToggleState_On:
			Py_RETURN_TRUE;
		case ToggleState_Off:
			Py_RETURN_FALSE;
		default:
			Py_RETURN_NONE;
		}
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the toggle pattern.");
		return NULL;
	}
}
PyObject* uiautomation_toggle(uiautomation_object* self)
{
	if (self->pToggle)
	{
		HRESULT hr = self->pToggle->Toggle();
		if (FAILED(hr))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
			return NULL;
		}

		return uiautomation_gettogglestate(self);
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Item doesn't support the toggle pattern.");
		return NULL;
	}
}

#pragma endregion

PyObject* uiautomation_test(PyObject* selfObj, PyObject* args, PyObject* kwargs)
{
	uiautomation_object* self = (uiautomation_object*)selfObj;
	if (self->pElement)
	{

	}
	Py_RETURN_NONE;
}
PyMethodDef uiautomation_methods[] = {
	// Base automation element
	{ "get_root_node", (PyCFunction)uiautomation_getrootnode, METH_NOARGS | METH_CLASS, "Gets the root node of the automation tree." },
	{ "find_child", (PyCFunction)uiautomation_findchild, METH_VARARGS, "Finds the first child matching the dictionary of properties." },
	{ "find_children", (PyCFunction)uiautomation_findchildren, METH_VARARGS, "Finds all child nodes matching the dictionary of properties." },
	{ "find_descendant", (PyCFunction)uiautomation_finddescendant, METH_VARARGS, "Finds the first descendant matching the dictionary of properties." },
	{ "find_descendants", (PyCFunction)uiautomation_finddescendants, METH_VARARGS, "Finds all descendant nodes matching the dictionary of properties." },
	{ "get_properties", (PyCFunction)uiautomation_getpropertylist, METH_NOARGS, "Gets the list of supported properties." },
	{ "get_property", (PyCFunction)uiautomation_getproperty, METH_VARARGS, "Gets the value of the specified property." },
	{ "set_focus", (PyCFunction)uiautomation_setfocus, METH_NOARGS, "Gives focus to this automation element." },

	// Expand Collapse Pattern
	{ "supports_expandcollapse", (PyCFunction)uiautomation_supportsexpandcollapse, METH_NOARGS, "Gets whether the element supports the expand/collapse pattern." },
	{ "expand",  (PyCFunction)uiautomation_expand, METH_NOARGS, "Expands the element." },
	{ "collapse",  (PyCFunction)uiautomation_collapse, METH_NOARGS, "Collapses the element." },
	{ "is_expanded", (PyCFunction)uiautomation_isexpanded, METH_NOARGS, "Gets whether the element is expanded." },
	{ "is_collapsed",  (PyCFunction)uiautomation_iscollapsed, METH_NOARGS, "Gets whether the element is collapsed." },
	{ "is_leafnode",  (PyCFunction)uiautomation_isleafnode, METH_NOARGS, "Gets whether the element is a leaf node." },
	{ "is_partial_expanded",  (PyCFunction)uiautomation_ispartialexpanded, METH_NOARGS, "Gets whether the element is partially expanded." },
	{ "get_expand_state",  (PyCFunction)uiautomation_getexpandstate, METH_NOARGS, "Gets the current expansion state of the item." },

	// Invoke Pattern
	{ "supports_invoke", (PyCFunction)uiautomation_supportsinvoke, METH_NOARGS, "Supports invoke pattern methods." },
	{ "invoke", (PyCFunction)uiautomation_invoke, METH_NOARGS, "Invoke element" },

	// Legacy IAccessible Pattern
	{ "supports_legacy", (PyCFunction)uiautomation_supportslegacy, METH_NOARGS, "Gets whether the element supports the legacy IAccessible pattern." },
	{ "legacy_get_childid", (PyCFunction)uiautomation_legacychildid, METH_NOARGS, "Gets the legacy child id." },
	{ "legacy_get_defaultaction", (PyCFunction)uiautomation_legacygetdefaultaction, METH_NOARGS, "Gets the legacy default action name." },
	{ "legacy_get_description", (PyCFunction)uiautomation_legacygetdescription, METH_NOARGS, "Gets the legacy description." },
	{ "legacy_get_help", (PyCFunction)uiautomation_legacygethelp, METH_NOARGS, "Gets the legacy help string." },
	{ "legacy_get_keyboardshortcut", (PyCFunction)uiautomation_legacygetkeyboardshortcut, METH_NOARGS, "Gets the legacy keyboard shortcut string." },
	{ "legacy_get_name", (PyCFunction)uiautomation_legacygetname, METH_NOARGS, "Gets the legacy name." },
	{ "legacy_get_role", (PyCFunction)uiautomation_legacygetrole, METH_NOARGS, "Gets the legacy role(s)." },
	{ "legacy_get_state", (PyCFunction)uiautomation_legacygetstate, METH_NOARGS, "Gets the legacy state(s)." },
	{ "legacy_get_value", (PyCFunction)uiautomation_legacygetvalue, METH_NOARGS, "Gets the legacy value." },
	{ "legacy_do_default_action", (PyCFunction)uiautomation_legacydodefaultaction, METH_NOARGS, "Executes the legacy default action." },
	{ "legacy_get_current_selection", (PyCFunction)uiautomation_legacygetcurrentselection, METH_NOARGS, "Gets the elements that are selected." },
	{ "legacy_select", (PyCFunction)uiautomation_legacyselect, METH_VARARGS, "Selects the element." },
	{ "legacy_set_value", (PyCFunction)uiautomation_legacysetvalue, METH_VARARGS, "Sets the elements value." },

	// Selection Pattern
	{ "supports_selection", (PyCFunction)uiautomation_supportsselection, METH_NOARGS, "Supports the selection pattern." },
	{ "can_select_multiple", (PyCFunction)uiautomation_canselectmultiple, METH_NOARGS, "Multiple selections are allowed." },
	{ "is_selection_required", (PyCFunction)uiautomation_isselectionrequired, METH_NOARGS, "Gets whether a selection is required." },
	{ "get_selections", (PyCFunction)uiautomation_getselections, METH_NOARGS, "Gets the AutomationElements that are selected." },

	// Selection Item Pattern
	{ "supports_selection_item", (PyCFunction)uiautomation_supportsselectionitem, METH_NOARGS, "Supports the selection item pattern." },
	{ "is_selected", (PyCFunction)uiautomation_isselected, METH_NOARGS, "Gets whether the item is selected." },
	{ "get_selection_container", (PyCFunction)uiautomation_getselectioncontainer, METH_NOARGS, "Gets the AutomationElement that owns this selection item." },
	{ "add_to_selection", (PyCFunction)uiautomation_addtoselection, METH_NOARGS, "Adds this item to the current selection." },
	{ "remove_from_selection", (PyCFunction)uiautomation_removefromselection, METH_NOARGS, "Removes this item from the current selection" },
	{ "select", (PyCFunction)uiautomation_select, METH_NOARGS, "Selects this item. (Deselects any current selections)" },

	// Toggle Pattern
	{ "supports_toggle", (PyCFunction)uiautomation_supportstoggle, METH_NOARGS, "Supports the toggle pattern." },
	{ "toggle", (PyCFunction)uiautomation_toggle, METH_NOARGS, "Toggle the control. Returns the current state. See get_toggle_state." },
	{ "get_toggle_state", (PyCFunction)uiautomation_gettogglestate, METH_NOARGS, "Gets the current toggle state. True = On; False = Off; None = Indeterminate" },

	// Value Pattern
	{ "supports_value", (PyCFunction)uiautomation_supportsvalue, METH_NOARGS, "Supports the Value pattern." },
	{ "value_is_readonly", (PyCFunction)uiautomation_valuereadonly, METH_NOARGS, "Gets whether the value is read-only." },
	{ "get_value", (PyCFunction)uiautomation_getvalue, METH_NOARGS, "Gets the value." },
	{ "set_value", (PyCFunction)uiautomation_setvalue, METH_VARARGS, "Sets the value." },

	// Window Pattern
	{ "supports_window", (PyCFunction)uiautomation_supportswindow, METH_NOARGS, "Supports window pattern methods." },
	{ "can_maximize", (PyCFunction)uiautomation_canmaximize, METH_NOARGS, "Window can be maximized" },
	{ "can_minimize", (PyCFunction)uiautomation_canminimize, METH_NOARGS, "Window can be minimized" },
	{ "is_modal", (PyCFunction)uiautomation_ismodal, METH_NOARGS, "Window is modal" },
	{ "is_topmost", (PyCFunction)uiautomation_istopmost, METH_NOARGS, "Window is topmost" },
	{ "get_window_visual_state", (PyCFunction)uiautomation_getwindowvisualstate, METH_NOARGS, "Gets the current window visual state" },
	{ "set_window_visual_state", (PyCFunction)uiautomation_setwindowvisualstate, METH_VARARGS, "Gets the current window visual state" },
	{ "get_window_interaction_state", (PyCFunction)uiautomation_getwindowinteractionstate, METH_NOARGS, "Gets the current window interaction state" },
	{ "close", (PyCFunction)uiautomation_close, METH_NOARGS, "Closes the window" },
	{ "wait_for_input_idle", (PyCFunction)uiautomation_waitforinputidle, METH_VARARGS, "Waits for the window to become idle." },



	{ "test", (PyCFunction)uiautomation_test, METH_VARARGS | METH_KEYWORDS, "test function" },
	{ NULL, NULL, 0, NULL }
};
PyMemberDef uiautomation_members[] = {
	{ NULL, 0, 0, 0, NULL }
};
PyTypeObject uiautomation_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.AutomationElement",			/* tp_name */
	sizeof(uiautomation_object),			/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)uiautomation_dealloc,		/* tp_dealloc */
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
	uiautomation_str,						/* tp_str */
	uiautomation_getattro,					/* tp_getattro */
	0,										/* tp_setattro */
	0,										/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |
	Py_TPFLAGS_BASETYPE,					/* tp_flags */
	"Windows API UI Automation",			/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	uiautomation_methods,					/* tp_methods */
	uiautomation_members,					/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)uiautomation_init,			/* tp_init */
	0,										/* tp_alloc */
	uiautomation_new,						/* tp_new */
};

#pragma endregion