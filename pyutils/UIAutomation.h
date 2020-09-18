#pragma once

#include <Python.h>
#include <UIAutomation.h>
#include <vector>
#include "AutomationCondition.h"

PyObject* UIAutomation_New(IUIAutomationElement* element);
extern PyTypeObject uiautomation_type;

typedef struct
{
	PyObject_HEAD;
	IUIAutomationElement* pElement;
	IUIAutomationInvokePattern* pInvoke;
	IUIAutomationWindowPattern* pWindow;
	IUIAutomationExpandCollapsePattern* pExpandCollapse;
	IUIAutomationValuePattern* pValue;
	IUIAutomationSelectionPattern* pSelection;
	IUIAutomationSelectionItemPattern* pSelectionItem;
	IUIAutomationLegacyIAccessiblePattern* pLegacy;
	IUIAutomationTogglePattern* pToggle;
} uiautomation_object;

#pragma region Managed Utility Methods

namespace pyutils
{
	namespace Native
	{
	}
	using namespace System;
	using System::Collections::Generic::List;

	public ref class AutomationManager
	{
	private:
		static IUIAutomation* pAutomation;
		static PyObject* cachedPropertyList;

		static AutomationManager();
		static void GetConditionForPyObject(PyObject* source, IUIAutomationCondition** ppCondition, std::vector<IUIAutomationCondition*>& children);
		
	public:

		static IUIAutomationElement* GetRootNode();
		static IUIAutomationElement* GetElementsFromRoot(PyObject* searchTuple);
		static IUIAutomationElement* GetProcessRoot(PyObject* process);
		static PyObject* GetPropertyList();
		static void BuildVariantFromPyObject(PROPERTYID pid, VARIANT* pVariant, PyObject* source);
		static PyObject* BuildPyObjectFromVariant(VARIANT* pVariant);
		static HRESULT GetAutomationProperty(IUIAutomationElement* fromElement, PROPERTYID propertyId, VARIANT* pResult);
		static PyObject* GetProperty(IUIAutomationElement* fromElement, PROPERTYID propertyId);
		static PyObject* GetProperty(IUIAutomationElement* fromElement, const wchar_t* name);
		static PyObject* GetProperty(IUIAutomationElement* fromElement, PyObject* name);
		static IUIAutomationElement* FindFirst(IUIAutomationElement* fromElement, TreeScope scope, automationcondition_object* conditions);
		static IUIAutomationElement* FindFirst(IUIAutomationElement* fromElement, TreeScope scope, PyObject* paramDict);
		static IUIAutomationElementArray* FindAll(IUIAutomationElement* fromElement, TreeScope scope, automationcondition_object* conditions);
		static IUIAutomationElementArray* FindAll(IUIAutomationElement* fromElement, TreeScope scope, PyObject* paramDict);
	};
}


#pragma endregion

