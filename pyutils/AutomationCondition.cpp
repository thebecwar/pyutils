#include "Stdafx.h"
#include "AutomationCondition.h"

#include <Python.h>
#include <structmember.h>
#include <UIAutomation.h>
#include <atlcoll.h>

PROPERTYID GetPropertyIdForName(const wchar_t* val)
{
	if (_wcsicmp(L"RuntimeId", val) == 0)
		return UIA_RuntimeIdPropertyId;
	if (_wcsicmp(L"BoundingRectangle", val) == 0)
		return UIA_BoundingRectanglePropertyId;
	if (_wcsicmp(L"ProcessId", val) == 0)
		return UIA_ProcessIdPropertyId;
	if (_wcsicmp(L"ControlType", val) == 0)
		return UIA_ControlTypePropertyId;
	if (_wcsicmp(L"LocalizedControlType", val) == 0)
		return UIA_LocalizedControlTypePropertyId;
	if (_wcsicmp(L"Name", val) == 0)
		return UIA_NamePropertyId;
	if (_wcsicmp(L"AcceleratorKey", val) == 0)
		return UIA_AcceleratorKeyPropertyId;
	if (_wcsicmp(L"AccessKey", val) == 0)
		return UIA_AccessKeyPropertyId;
	if (_wcsicmp(L"HasKeyboardFocus", val) == 0)
		return UIA_HasKeyboardFocusPropertyId;
	if (_wcsicmp(L"IsKeyboardFocusable", val) == 0)
		return UIA_IsKeyboardFocusablePropertyId;
	if (_wcsicmp(L"IsEnabled", val) == 0)
		return UIA_IsEnabledPropertyId;
	if (_wcsicmp(L"AutomationId", val) == 0)
		return UIA_AutomationIdPropertyId;
	if (_wcsicmp(L"ClassName", val) == 0)
		return UIA_ClassNamePropertyId;
	if (_wcsicmp(L"HelpText", val) == 0)
		return UIA_HelpTextPropertyId;
	if (_wcsicmp(L"ClickablePoint", val) == 0)
		return UIA_ClickablePointPropertyId;
	if (_wcsicmp(L"Culture", val) == 0)
		return UIA_CulturePropertyId;
	if (_wcsicmp(L"IsControlElement", val) == 0)
		return UIA_IsControlElementPropertyId;
	if (_wcsicmp(L"IsContentElement", val) == 0)
		return UIA_IsContentElementPropertyId;
	if (_wcsicmp(L"LabeledBy", val) == 0)
		return UIA_LabeledByPropertyId;
	if (_wcsicmp(L"IsPassword", val) == 0)
		return UIA_IsPasswordPropertyId;
	if (_wcsicmp(L"NativeWindowHandle", val) == 0)
		return UIA_NativeWindowHandlePropertyId;
	if (_wcsicmp(L"ItemType", val) == 0)
		return UIA_ItemTypePropertyId;
	if (_wcsicmp(L"IsOffscreen", val) == 0)
		return UIA_IsOffscreenPropertyId;
	if (_wcsicmp(L"Orientation", val) == 0)
		return UIA_OrientationPropertyId;
	if (_wcsicmp(L"FrameworkId", val) == 0)
		return UIA_FrameworkIdPropertyId;
	if (_wcsicmp(L"IsRequiredForForm", val) == 0)
		return UIA_IsRequiredForFormPropertyId;
	if (_wcsicmp(L"ItemStatus", val) == 0)
		return UIA_ItemStatusPropertyId;
	if (_wcsicmp(L"IsDockPatternAvailable", val) == 0)
		return UIA_IsDockPatternAvailablePropertyId;
	if (_wcsicmp(L"IsExpandCollapsePatternAvailable", val) == 0)
		return UIA_IsExpandCollapsePatternAvailablePropertyId;
	if (_wcsicmp(L"IsGridItemPatternAvailable", val) == 0)
		return UIA_IsGridItemPatternAvailablePropertyId;
	if (_wcsicmp(L"IsGridPatternAvailable", val) == 0)
		return UIA_IsGridPatternAvailablePropertyId;
	if (_wcsicmp(L"IsInvokePatternAvailable", val) == 0)
		return UIA_IsInvokePatternAvailablePropertyId;
	if (_wcsicmp(L"IsMultipleViewPatternAvailable", val) == 0)
		return UIA_IsMultipleViewPatternAvailablePropertyId;
	if (_wcsicmp(L"IsRangeValuePatternAvailable", val) == 0)
		return UIA_IsRangeValuePatternAvailablePropertyId;
	if (_wcsicmp(L"IsScrollPatternAvailable", val) == 0)
		return UIA_IsScrollPatternAvailablePropertyId;
	if (_wcsicmp(L"IsScrollItemPatternAvailable", val) == 0)
		return UIA_IsScrollItemPatternAvailablePropertyId;
	if (_wcsicmp(L"IsSelectionItemPatternAvailable", val) == 0)
		return UIA_IsSelectionItemPatternAvailablePropertyId;
	if (_wcsicmp(L"IsSelectionPatternAvailable", val) == 0)
		return UIA_IsSelectionPatternAvailablePropertyId;
	if (_wcsicmp(L"IsTablePatternAvailable", val) == 0)
		return UIA_IsTablePatternAvailablePropertyId;
	if (_wcsicmp(L"IsTableItemPatternAvailable", val) == 0)
		return UIA_IsTableItemPatternAvailablePropertyId;
	if (_wcsicmp(L"IsTextPatternAvailable", val) == 0)
		return UIA_IsTextPatternAvailablePropertyId;
	if (_wcsicmp(L"IsTogglePatternAvailable", val) == 0)
		return UIA_IsTogglePatternAvailablePropertyId;
	if (_wcsicmp(L"IsTransformPatternAvailable", val) == 0)
		return UIA_IsTransformPatternAvailablePropertyId;
	if (_wcsicmp(L"IsValuePatternAvailable", val) == 0)
		return UIA_IsValuePatternAvailablePropertyId;
	if (_wcsicmp(L"IsWindowPatternAvailable", val) == 0)
		return UIA_IsWindowPatternAvailablePropertyId;
	if (_wcsicmp(L"ValueValue", val) == 0)
		return UIA_ValueValuePropertyId;
	if (_wcsicmp(L"ValueIsReadOnly", val) == 0)
		return UIA_ValueIsReadOnlyPropertyId;
	if (_wcsicmp(L"RangeValueValue", val) == 0)
		return UIA_RangeValueValuePropertyId;
	if (_wcsicmp(L"RangeValueIsReadOnly", val) == 0)
		return UIA_RangeValueIsReadOnlyPropertyId;
	if (_wcsicmp(L"RangeValueMinimum", val) == 0)
		return UIA_RangeValueMinimumPropertyId;
	if (_wcsicmp(L"RangeValueMaximum", val) == 0)
		return UIA_RangeValueMaximumPropertyId;
	if (_wcsicmp(L"RangeValueLargeChange", val) == 0)
		return UIA_RangeValueLargeChangePropertyId;
	if (_wcsicmp(L"RangeValueSmallChange", val) == 0)
		return UIA_RangeValueSmallChangePropertyId;
	if (_wcsicmp(L"ScrollHorizontalScrollPercent", val) == 0)
		return UIA_ScrollHorizontalScrollPercentPropertyId;
	if (_wcsicmp(L"ScrollHorizontalViewSize", val) == 0)
		return UIA_ScrollHorizontalViewSizePropertyId;
	if (_wcsicmp(L"ScrollVerticalScrollPercent", val) == 0)
		return UIA_ScrollVerticalScrollPercentPropertyId;
	if (_wcsicmp(L"ScrollVerticalViewSize", val) == 0)
		return UIA_ScrollVerticalViewSizePropertyId;
	if (_wcsicmp(L"ScrollHorizontallyScrollable", val) == 0)
		return UIA_ScrollHorizontallyScrollablePropertyId;
	if (_wcsicmp(L"ScrollVerticallyScrollable", val) == 0)
		return UIA_ScrollVerticallyScrollablePropertyId;
	if (_wcsicmp(L"SelectionSelection", val) == 0)
		return UIA_SelectionSelectionPropertyId;
	if (_wcsicmp(L"SelectionCanSelectMultiple", val) == 0)
		return UIA_SelectionCanSelectMultiplePropertyId;
	if (_wcsicmp(L"SelectionIsSelectionRequired", val) == 0)
		return UIA_SelectionIsSelectionRequiredPropertyId;
	if (_wcsicmp(L"GridRowCount", val) == 0)
		return UIA_GridRowCountPropertyId;
	if (_wcsicmp(L"GridColumnCount", val) == 0)
		return UIA_GridColumnCountPropertyId;
	if (_wcsicmp(L"GridItemRow", val) == 0)
		return UIA_GridItemRowPropertyId;
	if (_wcsicmp(L"GridItemColumn", val) == 0)
		return UIA_GridItemColumnPropertyId;
	if (_wcsicmp(L"GridItemRowSpan", val) == 0)
		return UIA_GridItemRowSpanPropertyId;
	if (_wcsicmp(L"GridItemColumnSpan", val) == 0)
		return UIA_GridItemColumnSpanPropertyId;
	if (_wcsicmp(L"GridItemContainingGrid", val) == 0)
		return UIA_GridItemContainingGridPropertyId;
	if (_wcsicmp(L"DockDockPosition", val) == 0)
		return UIA_DockDockPositionPropertyId;
	if (_wcsicmp(L"ExpandCollapseExpandCollapseState", val) == 0)
		return UIA_ExpandCollapseExpandCollapseStatePropertyId;
	if (_wcsicmp(L"MultipleViewCurrentView", val) == 0)
		return UIA_MultipleViewCurrentViewPropertyId;
	if (_wcsicmp(L"MultipleViewSupportedViews", val) == 0)
		return UIA_MultipleViewSupportedViewsPropertyId;
	if (_wcsicmp(L"WindowCanMaximize", val) == 0)
		return UIA_WindowCanMaximizePropertyId;
	if (_wcsicmp(L"WindowCanMinimize", val) == 0)
		return UIA_WindowCanMinimizePropertyId;
	if (_wcsicmp(L"WindowWindowVisualState", val) == 0)
		return UIA_WindowWindowVisualStatePropertyId;
	if (_wcsicmp(L"WindowWindowInteractionState", val) == 0)
		return UIA_WindowWindowInteractionStatePropertyId;
	if (_wcsicmp(L"WindowIsModal", val) == 0)
		return UIA_WindowIsModalPropertyId;
	if (_wcsicmp(L"WindowIsTopmost", val) == 0)
		return UIA_WindowIsTopmostPropertyId;
	if (_wcsicmp(L"SelectionItemIsSelected", val) == 0)
		return UIA_SelectionItemIsSelectedPropertyId;
	if (_wcsicmp(L"SelectionItemSelectionContainer", val) == 0)
		return UIA_SelectionItemSelectionContainerPropertyId;
	if (_wcsicmp(L"TableRowHeaders", val) == 0)
		return UIA_TableRowHeadersPropertyId;
	if (_wcsicmp(L"TableColumnHeaders", val) == 0)
		return UIA_TableColumnHeadersPropertyId;
	if (_wcsicmp(L"TableRowOrColumnMajor", val) == 0)
		return UIA_TableRowOrColumnMajorPropertyId;
	if (_wcsicmp(L"TableItemRowHeaderItems", val) == 0)
		return UIA_TableItemRowHeaderItemsPropertyId;
	if (_wcsicmp(L"TableItemColumnHeaderItems", val) == 0)
		return UIA_TableItemColumnHeaderItemsPropertyId;
	if (_wcsicmp(L"ToggleToggleState", val) == 0)
		return UIA_ToggleToggleStatePropertyId;
	if (_wcsicmp(L"TransformCanMove", val) == 0)
		return UIA_TransformCanMovePropertyId;
	if (_wcsicmp(L"TransformCanResize", val) == 0)
		return UIA_TransformCanResizePropertyId;
	if (_wcsicmp(L"TransformCanRotate", val) == 0)
		return UIA_TransformCanRotatePropertyId;
	if (_wcsicmp(L"IsLegacyIAccessiblePatternAvailable", val) == 0)
		return UIA_IsLegacyIAccessiblePatternAvailablePropertyId;
	if (_wcsicmp(L"LegacyIAccessibleChildId", val) == 0)
		return UIA_LegacyIAccessibleChildIdPropertyId;
	if (_wcsicmp(L"LegacyIAccessibleName", val) == 0)
		return UIA_LegacyIAccessibleNamePropertyId;
	if (_wcsicmp(L"LegacyIAccessibleValue", val) == 0)
		return UIA_LegacyIAccessibleValuePropertyId;
	if (_wcsicmp(L"LegacyIAccessibleDescription", val) == 0)
		return UIA_LegacyIAccessibleDescriptionPropertyId;
	if (_wcsicmp(L"LegacyIAccessibleRole", val) == 0)
		return UIA_LegacyIAccessibleRolePropertyId;
	if (_wcsicmp(L"LegacyIAccessibleState", val) == 0)
		return UIA_LegacyIAccessibleStatePropertyId;
	if (_wcsicmp(L"LegacyIAccessibleHelp", val) == 0)
		return UIA_LegacyIAccessibleHelpPropertyId;
	if (_wcsicmp(L"LegacyIAccessibleKeyboardShortcut", val) == 0)
		return UIA_LegacyIAccessibleKeyboardShortcutPropertyId;
	if (_wcsicmp(L"LegacyIAccessibleSelection", val) == 0)
		return UIA_LegacyIAccessibleSelectionPropertyId;
	if (_wcsicmp(L"LegacyIAccessibleDefaultAction", val) == 0)
		return UIA_LegacyIAccessibleDefaultActionPropertyId;
	if (_wcsicmp(L"AriaRole", val) == 0)
		return UIA_AriaRolePropertyId;
	if (_wcsicmp(L"AriaProperties", val) == 0)
		return UIA_AriaPropertiesPropertyId;
	if (_wcsicmp(L"IsDataValidForForm", val) == 0)
		return UIA_IsDataValidForFormPropertyId;
	if (_wcsicmp(L"ControllerFor", val) == 0)
		return UIA_ControllerForPropertyId;
	if (_wcsicmp(L"DescribedBy", val) == 0)
		return UIA_DescribedByPropertyId;
	if (_wcsicmp(L"FlowsTo", val) == 0)
		return UIA_FlowsToPropertyId;
	if (_wcsicmp(L"ProviderDescription", val) == 0)
		return UIA_ProviderDescriptionPropertyId;
	if (_wcsicmp(L"IsItemContainerPatternAvailable", val) == 0)
		return UIA_IsItemContainerPatternAvailablePropertyId;
	if (_wcsicmp(L"IsVirtualizedItemPatternAvailable", val) == 0)
		return UIA_IsVirtualizedItemPatternAvailablePropertyId;
	if (_wcsicmp(L"IsSynchronizedInputPatternAvailable", val) == 0)
		return UIA_IsSynchronizedInputPatternAvailablePropertyId;
	if (_wcsicmp(L"OptimizeForVisualContent", val) == 0)
		return UIA_OptimizeForVisualContentPropertyId;
	if (_wcsicmp(L"IsObjectModelPatternAvailable", val) == 0)
		return UIA_IsObjectModelPatternAvailablePropertyId;
	if (_wcsicmp(L"AnnotationAnnotationTypeId", val) == 0)
		return UIA_AnnotationAnnotationTypeIdPropertyId;
	if (_wcsicmp(L"AnnotationAnnotationTypeName", val) == 0)
		return UIA_AnnotationAnnotationTypeNamePropertyId;
	if (_wcsicmp(L"AnnotationAuthor", val) == 0)
		return UIA_AnnotationAuthorPropertyId;
	if (_wcsicmp(L"AnnotationDateTime", val) == 0)
		return UIA_AnnotationDateTimePropertyId;
	if (_wcsicmp(L"AnnotationTarget", val) == 0)
		return UIA_AnnotationTargetPropertyId;
	if (_wcsicmp(L"IsAnnotationPatternAvailable", val) == 0)
		return UIA_IsAnnotationPatternAvailablePropertyId;
	if (_wcsicmp(L"IsTextPattern2Available", val) == 0)
		return UIA_IsTextPattern2AvailablePropertyId;
	if (_wcsicmp(L"StylesStyleId", val) == 0)
		return UIA_StylesStyleIdPropertyId;
	if (_wcsicmp(L"StylesStyleName", val) == 0)
		return UIA_StylesStyleNamePropertyId;
	if (_wcsicmp(L"StylesFillColor", val) == 0)
		return UIA_StylesFillColorPropertyId;
	if (_wcsicmp(L"StylesFillPatternStyle", val) == 0)
		return UIA_StylesFillPatternStylePropertyId;
	if (_wcsicmp(L"StylesShape", val) == 0)
		return UIA_StylesShapePropertyId;
	if (_wcsicmp(L"StylesFillPatternColor", val) == 0)
		return UIA_StylesFillPatternColorPropertyId;
	if (_wcsicmp(L"StylesExtendedProperties", val) == 0)
		return UIA_StylesExtendedPropertiesPropertyId;
	if (_wcsicmp(L"IsStylesPatternAvailable", val) == 0)
		return UIA_IsStylesPatternAvailablePropertyId;
	if (_wcsicmp(L"IsSpreadsheetPatternAvailable", val) == 0)
		return UIA_IsSpreadsheetPatternAvailablePropertyId;
	if (_wcsicmp(L"SpreadsheetItemFormula", val) == 0)
		return UIA_SpreadsheetItemFormulaPropertyId;
	if (_wcsicmp(L"SpreadsheetItemAnnotationObjects", val) == 0)
		return UIA_SpreadsheetItemAnnotationObjectsPropertyId;
	if (_wcsicmp(L"SpreadsheetItemAnnotationTypes", val) == 0)
		return UIA_SpreadsheetItemAnnotationTypesPropertyId;
	if (_wcsicmp(L"IsSpreadsheetItemPatternAvailable", val) == 0)
		return UIA_IsSpreadsheetItemPatternAvailablePropertyId;
	if (_wcsicmp(L"Transform2CanZoom", val) == 0)
		return UIA_Transform2CanZoomPropertyId;
	if (_wcsicmp(L"IsTransformPattern2Available", val) == 0)
		return UIA_IsTransformPattern2AvailablePropertyId;
	if (_wcsicmp(L"LiveSetting", val) == 0)
		return UIA_LiveSettingPropertyId;
	if (_wcsicmp(L"IsTextChildPatternAvailable", val) == 0)
		return UIA_IsTextChildPatternAvailablePropertyId;
	if (_wcsicmp(L"IsDragPatternAvailable", val) == 0)
		return UIA_IsDragPatternAvailablePropertyId;
	if (_wcsicmp(L"DragIsGrabbed", val) == 0)
		return UIA_DragIsGrabbedPropertyId;
	if (_wcsicmp(L"DragDropEffect", val) == 0)
		return UIA_DragDropEffectPropertyId;
	if (_wcsicmp(L"DragDropEffects", val) == 0)
		return UIA_DragDropEffectsPropertyId;
	if (_wcsicmp(L"IsDropTargetPatternAvailable", val) == 0)
		return UIA_IsDropTargetPatternAvailablePropertyId;
	if (_wcsicmp(L"DropTargetDropTargetEffect", val) == 0)
		return UIA_DropTargetDropTargetEffectPropertyId;
	if (_wcsicmp(L"DropTargetDropTargetEffects", val) == 0)
		return UIA_DropTargetDropTargetEffectsPropertyId;
	if (_wcsicmp(L"DragGrabbedItems", val) == 0)
		return UIA_DragGrabbedItemsPropertyId;
	if (_wcsicmp(L"Transform2ZoomLevel", val) == 0)
		return UIA_Transform2ZoomLevelPropertyId;
	if (_wcsicmp(L"Transform2ZoomMinimum", val) == 0)
		return UIA_Transform2ZoomMinimumPropertyId;
	if (_wcsicmp(L"Transform2ZoomMaximum", val) == 0)
		return UIA_Transform2ZoomMaximumPropertyId;
	if (_wcsicmp(L"FlowsFrom", val) == 0)
		return UIA_FlowsFromPropertyId;
	if (_wcsicmp(L"IsTextEditPatternAvailable", val) == 0)
		return UIA_IsTextEditPatternAvailablePropertyId;
	if (_wcsicmp(L"IsPeripheral", val) == 0)
		return UIA_IsPeripheralPropertyId;

	// default
	return 0;
}
const wchar_t* GetNameForPropertyId(PROPERTYID val)
{
	switch (val)
	{
	case UIA_RuntimeIdPropertyId:
		return L"RuntimeId";
	case UIA_BoundingRectanglePropertyId:
		return L"BoundingRectangle";
	case UIA_ProcessIdPropertyId:
		return L"ProcessId";
	case UIA_ControlTypePropertyId:
		return L"ControlType";
	case UIA_LocalizedControlTypePropertyId:
		return L"LocalizedControlType";
	case UIA_NamePropertyId:
		return L"Name";
	case UIA_AcceleratorKeyPropertyId:
		return L"AcceleratorKey";
	case UIA_AccessKeyPropertyId:
		return L"AccessKey";
	case UIA_HasKeyboardFocusPropertyId:
		return L"HasKeyboardFocus";
	case UIA_IsKeyboardFocusablePropertyId:
		return L"IsKeyboardFocusable";
	case UIA_IsEnabledPropertyId:
		return L"IsEnabled";
	case UIA_AutomationIdPropertyId:
		return L"AutomationId";
	case UIA_ClassNamePropertyId:
		return L"ClassName";
	case UIA_HelpTextPropertyId:
		return L"HelpText";
	case UIA_ClickablePointPropertyId:
		return L"ClickablePoint";
	case UIA_CulturePropertyId:
		return L"Culture";
	case UIA_IsControlElementPropertyId:
		return L"IsControlElement";
	case UIA_IsContentElementPropertyId:
		return L"IsContentElement";
	case UIA_LabeledByPropertyId:
		return L"LabeledBy";
	case UIA_IsPasswordPropertyId:
		return L"IsPassword";
	case UIA_NativeWindowHandlePropertyId:
		return L"NativeWindowHandle";
	case UIA_ItemTypePropertyId:
		return L"ItemType";
	case UIA_IsOffscreenPropertyId:
		return L"IsOffscreen";
	case UIA_OrientationPropertyId:
		return L"Orientation";
	case UIA_FrameworkIdPropertyId:
		return L"FrameworkId";
	case UIA_IsRequiredForFormPropertyId:
		return L"IsRequiredForForm";
	case UIA_ItemStatusPropertyId:
		return L"ItemStatus";
	case UIA_IsDockPatternAvailablePropertyId:
		return L"IsDockPatternAvailable";
	case UIA_IsExpandCollapsePatternAvailablePropertyId:
		return L"IsExpandCollapsePatternAvailable";
	case UIA_IsGridItemPatternAvailablePropertyId:
		return L"IsGridItemPatternAvailable";
	case UIA_IsGridPatternAvailablePropertyId:
		return L"IsGridPatternAvailable";
	case UIA_IsInvokePatternAvailablePropertyId:
		return L"IsInvokePatternAvailable";
	case UIA_IsMultipleViewPatternAvailablePropertyId:
		return L"IsMultipleViewPatternAvailable";
	case UIA_IsRangeValuePatternAvailablePropertyId:
		return L"IsRangeValuePatternAvailable";
	case UIA_IsScrollPatternAvailablePropertyId:
		return L"IsScrollPatternAvailable";
	case UIA_IsScrollItemPatternAvailablePropertyId:
		return L"IsScrollItemPatternAvailable";
	case UIA_IsSelectionItemPatternAvailablePropertyId:
		return L"IsSelectionItemPatternAvailable";
	case UIA_IsSelectionPatternAvailablePropertyId:
		return L"IsSelectionPatternAvailable";
	case UIA_IsTablePatternAvailablePropertyId:
		return L"IsTablePatternAvailable";
	case UIA_IsTableItemPatternAvailablePropertyId:
		return L"IsTableItemPatternAvailable";
	case UIA_IsTextPatternAvailablePropertyId:
		return L"IsTextPatternAvailable";
	case UIA_IsTogglePatternAvailablePropertyId:
		return L"IsTogglePatternAvailable";
	case UIA_IsTransformPatternAvailablePropertyId:
		return L"IsTransformPatternAvailable";
	case UIA_IsValuePatternAvailablePropertyId:
		return L"IsValuePatternAvailable";
	case UIA_IsWindowPatternAvailablePropertyId:
		return L"IsWindowPatternAvailable";
	case UIA_ValueValuePropertyId:
		return L"ValueValue";
	case UIA_ValueIsReadOnlyPropertyId:
		return L"ValueIsReadOnly";
	case UIA_RangeValueValuePropertyId:
		return L"RangeValueValue";
	case UIA_RangeValueIsReadOnlyPropertyId:
		return L"RangeValueIsReadOnly";
	case UIA_RangeValueMinimumPropertyId:
		return L"RangeValueMinimum";
	case UIA_RangeValueMaximumPropertyId:
		return L"RangeValueMaximum";
	case UIA_RangeValueLargeChangePropertyId:
		return L"RangeValueLargeChange";
	case UIA_RangeValueSmallChangePropertyId:
		return L"RangeValueSmallChange";
	case UIA_ScrollHorizontalScrollPercentPropertyId:
		return L"ScrollHorizontalScrollPercent";
	case UIA_ScrollHorizontalViewSizePropertyId:
		return L"ScrollHorizontalViewSize";
	case UIA_ScrollVerticalScrollPercentPropertyId:
		return L"ScrollVerticalScrollPercent";
	case UIA_ScrollVerticalViewSizePropertyId:
		return L"ScrollVerticalViewSize";
	case UIA_ScrollHorizontallyScrollablePropertyId:
		return L"ScrollHorizontallyScrollable";
	case UIA_ScrollVerticallyScrollablePropertyId:
		return L"ScrollVerticallyScrollable";
	case UIA_SelectionSelectionPropertyId:
		return L"SelectionSelection";
	case UIA_SelectionCanSelectMultiplePropertyId:
		return L"SelectionCanSelectMultiple";
	case UIA_SelectionIsSelectionRequiredPropertyId:
		return L"SelectionIsSelectionRequired";
	case UIA_GridRowCountPropertyId:
		return L"GridRowCount";
	case UIA_GridColumnCountPropertyId:
		return L"GridColumnCount";
	case UIA_GridItemRowPropertyId:
		return L"GridItemRow";
	case UIA_GridItemColumnPropertyId:
		return L"GridItemColumn";
	case UIA_GridItemRowSpanPropertyId:
		return L"GridItemRowSpan";
	case UIA_GridItemColumnSpanPropertyId:
		return L"GridItemColumnSpan";
	case UIA_GridItemContainingGridPropertyId:
		return L"GridItemContainingGrid";
	case UIA_DockDockPositionPropertyId:
		return L"DockDockPosition";
	case UIA_ExpandCollapseExpandCollapseStatePropertyId:
		return L"ExpandCollapseExpandCollapseState";
	case UIA_MultipleViewCurrentViewPropertyId:
		return L"MultipleViewCurrentView";
	case UIA_MultipleViewSupportedViewsPropertyId:
		return L"MultipleViewSupportedViews";
	case UIA_WindowCanMaximizePropertyId:
		return L"WindowCanMaximize";
	case UIA_WindowCanMinimizePropertyId:
		return L"WindowCanMinimize";
	case UIA_WindowWindowVisualStatePropertyId:
		return L"WindowWindowVisualState";
	case UIA_WindowWindowInteractionStatePropertyId:
		return L"WindowWindowInteractionState";
	case UIA_WindowIsModalPropertyId:
		return L"WindowIsModal";
	case UIA_WindowIsTopmostPropertyId:
		return L"WindowIsTopmost";
	case UIA_SelectionItemIsSelectedPropertyId:
		return L"SelectionItemIsSelected";
	case UIA_SelectionItemSelectionContainerPropertyId:
		return L"SelectionItemSelectionContainer";
	case UIA_TableRowHeadersPropertyId:
		return L"TableRowHeaders";
	case UIA_TableColumnHeadersPropertyId:
		return L"TableColumnHeaders";
	case UIA_TableRowOrColumnMajorPropertyId:
		return L"TableRowOrColumnMajor";
	case UIA_TableItemRowHeaderItemsPropertyId:
		return L"TableItemRowHeaderItems";
	case UIA_TableItemColumnHeaderItemsPropertyId:
		return L"TableItemColumnHeaderItems";
	case UIA_ToggleToggleStatePropertyId:
		return L"ToggleToggleState";
	case UIA_TransformCanMovePropertyId:
		return L"TransformCanMove";
	case UIA_TransformCanResizePropertyId:
		return L"TransformCanResize";
	case UIA_TransformCanRotatePropertyId:
		return L"TransformCanRotate";
	case UIA_IsLegacyIAccessiblePatternAvailablePropertyId:
		return L"IsLegacyIAccessiblePatternAvailable";
	case UIA_LegacyIAccessibleChildIdPropertyId:
		return L"LegacyIAccessibleChildId";
	case UIA_LegacyIAccessibleNamePropertyId:
		return L"LegacyIAccessibleName";
	case UIA_LegacyIAccessibleValuePropertyId:
		return L"LegacyIAccessibleValue";
	case UIA_LegacyIAccessibleDescriptionPropertyId:
		return L"LegacyIAccessibleDescription";
	case UIA_LegacyIAccessibleRolePropertyId:
		return L"LegacyIAccessibleRole";
	case UIA_LegacyIAccessibleStatePropertyId:
		return L"LegacyIAccessibleState";
	case UIA_LegacyIAccessibleHelpPropertyId:
		return L"LegacyIAccessibleHelp";
	case UIA_LegacyIAccessibleKeyboardShortcutPropertyId:
		return L"LegacyIAccessibleKeyboardShortcut";
	case UIA_LegacyIAccessibleSelectionPropertyId:
		return L"LegacyIAccessibleSelection";
	case UIA_LegacyIAccessibleDefaultActionPropertyId:
		return L"LegacyIAccessibleDefaultAction";
	case UIA_AriaRolePropertyId:
		return L"AriaRole";
	case UIA_AriaPropertiesPropertyId:
		return L"AriaProperties";
	case UIA_IsDataValidForFormPropertyId:
		return L"IsDataValidForForm";
	case UIA_ControllerForPropertyId:
		return L"ControllerFor";
	case UIA_DescribedByPropertyId:
		return L"DescribedBy";
	case UIA_FlowsToPropertyId:
		return L"FlowsTo";
	case UIA_ProviderDescriptionPropertyId:
		return L"ProviderDescription";
	case UIA_IsItemContainerPatternAvailablePropertyId:
		return L"IsItemContainerPatternAvailable";
	case UIA_IsVirtualizedItemPatternAvailablePropertyId:
		return L"IsVirtualizedItemPatternAvailable";
	case UIA_IsSynchronizedInputPatternAvailablePropertyId:
		return L"IsSynchronizedInputPatternAvailable";
	case UIA_OptimizeForVisualContentPropertyId:
		return L"OptimizeForVisualContent";
	case UIA_IsObjectModelPatternAvailablePropertyId:
		return L"IsObjectModelPatternAvailable";
	case UIA_AnnotationAnnotationTypeIdPropertyId:
		return L"AnnotationAnnotationTypeId";
	case UIA_AnnotationAnnotationTypeNamePropertyId:
		return L"AnnotationAnnotationTypeName";
	case UIA_AnnotationAuthorPropertyId:
		return L"AnnotationAuthor";
	case UIA_AnnotationDateTimePropertyId:
		return L"AnnotationDateTime";
	case UIA_AnnotationTargetPropertyId:
		return L"AnnotationTarget";
	case UIA_IsAnnotationPatternAvailablePropertyId:
		return L"IsAnnotationPatternAvailable";
	case UIA_IsTextPattern2AvailablePropertyId:
		return L"IsTextPattern2Available";
	case UIA_StylesStyleIdPropertyId:
		return L"StylesStyleId";
	case UIA_StylesStyleNamePropertyId:
		return L"StylesStyleName";
	case UIA_StylesFillColorPropertyId:
		return L"StylesFillColor";
	case UIA_StylesFillPatternStylePropertyId:
		return L"StylesFillPatternStyle";
	case UIA_StylesShapePropertyId:
		return L"StylesShape";
	case UIA_StylesFillPatternColorPropertyId:
		return L"StylesFillPatternColor";
	case UIA_StylesExtendedPropertiesPropertyId:
		return L"StylesExtendedProperties";
	case UIA_IsStylesPatternAvailablePropertyId:
		return L"IsStylesPatternAvailable";
	case UIA_IsSpreadsheetPatternAvailablePropertyId:
		return L"IsSpreadsheetPatternAvailable";
	case UIA_SpreadsheetItemFormulaPropertyId:
		return L"SpreadsheetItemFormula";
	case UIA_SpreadsheetItemAnnotationObjectsPropertyId:
		return L"SpreadsheetItemAnnotationObjects";
	case UIA_SpreadsheetItemAnnotationTypesPropertyId:
		return L"SpreadsheetItemAnnotationTypes";
	case UIA_IsSpreadsheetItemPatternAvailablePropertyId:
		return L"IsSpreadsheetItemPatternAvailable";
	case UIA_Transform2CanZoomPropertyId:
		return L"Transform2CanZoom";
	case UIA_IsTransformPattern2AvailablePropertyId:
		return L"IsTransformPattern2Available";
	case UIA_LiveSettingPropertyId:
		return L"LiveSetting";
	case UIA_IsTextChildPatternAvailablePropertyId:
		return L"IsTextChildPatternAvailable";
	case UIA_IsDragPatternAvailablePropertyId:
		return L"IsDragPatternAvailable";
	case UIA_DragIsGrabbedPropertyId:
		return L"DragIsGrabbed";
	case UIA_DragDropEffectPropertyId:
		return L"DragDropEffect";
	case UIA_DragDropEffectsPropertyId:
		return L"DragDropEffects";
	case UIA_IsDropTargetPatternAvailablePropertyId:
		return L"IsDropTargetPatternAvailable";
	case UIA_DropTargetDropTargetEffectPropertyId:
		return L"DropTargetDropTargetEffect";
	case UIA_DropTargetDropTargetEffectsPropertyId:
		return L"DropTargetDropTargetEffects";
	case UIA_DragGrabbedItemsPropertyId:
		return L"DragGrabbedItems";
	case UIA_Transform2ZoomLevelPropertyId:
		return L"Transform2ZoomLevel";
	case UIA_Transform2ZoomMinimumPropertyId:
		return L"Transform2ZoomMinimum";
	case UIA_Transform2ZoomMaximumPropertyId:
		return L"Transform2ZoomMaximum";
	case UIA_FlowsFromPropertyId:
		return L"FlowsFrom";
	case UIA_IsTextEditPatternAvailablePropertyId:
		return L"IsTextEditPatternAvailable";
	case UIA_IsPeripheralPropertyId:
		return L"IsPeripheral";
	default:
		return L"";
	}
}
VARTYPE GetPropertyType(PROPERTYID id)
{
	switch (id)
	{
	case UIA_AcceleratorKeyPropertyId: return VT_BSTR;
	case UIA_AccessKeyPropertyId: return VT_BSTR;
		//case UIA_AnnotationObjectsPropertyId: return VT_I4;
		//case UIA_AnnotationTypesPropertyId: return VT_I4;
	case UIA_AriaPropertiesPropertyId: return VT_BSTR;
	case UIA_AriaRolePropertyId: return VT_BSTR;
	case UIA_AutomationIdPropertyId: return VT_BSTR;
	case UIA_BoundingRectanglePropertyId: return VT_R8;
		//case UIA_CenterPointPropertyId: return VT_R8;
	case UIA_ClassNamePropertyId: return VT_BSTR;
	case UIA_ClickablePointPropertyId: return VT_R8;
	case UIA_ControllerForPropertyId: return VT_UNKNOWN;
	case UIA_ControlTypePropertyId: return VT_I4;
	case UIA_CulturePropertyId: return VT_I4;
	case UIA_DescribedByPropertyId: return VT_UNKNOWN;
		//case UIA_FillColorPropertyId: return VT_I4;
		//case UIA_FillTypePropertyId: return VT_I4;
	case UIA_FlowsFromPropertyId: return VT_UNKNOWN;
	case UIA_FlowsToPropertyId: return VT_UNKNOWN;
	case UIA_FrameworkIdPropertyId: return VT_BSTR;
		//case UIA_FullDescriptionPropertyId: return VT_BSTR;
	case UIA_HasKeyboardFocusPropertyId: return VT_BOOL;
	case UIA_HelpTextPropertyId: return VT_BSTR;
	case UIA_IsContentElementPropertyId: return VT_BOOL;
	case UIA_IsControlElementPropertyId: return VT_BOOL;
	case UIA_IsDataValidForFormPropertyId: return VT_BOOL;
	case UIA_IsEnabledPropertyId: return VT_BOOL;
	case UIA_IsKeyboardFocusablePropertyId: return VT_BOOL;
	case UIA_IsOffscreenPropertyId: return VT_BOOL;
	case UIA_IsPasswordPropertyId: return VT_BOOL;
	case UIA_IsPeripheralPropertyId: return VT_BOOL;
	case UIA_IsRequiredForFormPropertyId: return VT_BOOL;
	case UIA_ItemStatusPropertyId: return VT_BSTR;
	case UIA_ItemTypePropertyId: return VT_BSTR;
	case UIA_LabeledByPropertyId: return VT_UNKNOWN;
		//case UIA_LandmarkTypePropertyId: return VT_I4;
		//case UIA_LevelPropertyId: return VT_I4;
	case UIA_LiveSettingPropertyId: return VT_I4;
	case UIA_LocalizedControlTypePropertyId: return VT_BSTR;
		//case UIA_LocalizedLandmarkTypePropertyId: return VT_BSTR;
	case UIA_NamePropertyId: return VT_BSTR;
	case UIA_NativeWindowHandlePropertyId: return VT_I4;
	case UIA_OptimizeForVisualContentPropertyId: return VT_BOOL;
	case UIA_OrientationPropertyId: return VT_I4;
		//case UIA_OutlineColorPropertyId: return VT_I4;
		//case UIA_OutlineThicknessPropertyId: return VT_R8;
		//case UIA_PositionInSetPropertyId: return VT_I4;
	case UIA_ProcessIdPropertyId: return VT_I4;
	case UIA_ProviderDescriptionPropertyId: return VT_BSTR;
		//case UIA_RotationPropertyId: return VT_R8;
	case UIA_RuntimeIdPropertyId: return VT_I4;
		//case UIA_SizePropertyId: return VT_R8;
		//case UIA_SizeOfSetPropertyId: return VT_I4;
		//case UIA_VisualEffectsPropertyId: return VT_I4;
	case UIA_AnnotationAnnotationTypeIdPropertyId: return VT_I4;
	case UIA_AnnotationAnnotationTypeNamePropertyId: return VT_BSTR;
	case UIA_AnnotationAuthorPropertyId: return VT_BSTR;
	case UIA_AnnotationDateTimePropertyId: return VT_BSTR;
	case UIA_AnnotationTargetPropertyId: return VT_UNKNOWN;
	case UIA_DockDockPositionPropertyId: return VT_I4;
	case UIA_DragDropEffectPropertyId: return VT_BSTR;
	case UIA_DragDropEffectsPropertyId: return VT_BSTR;
	case UIA_DragIsGrabbedPropertyId: return VT_BOOL;
	case UIA_DragGrabbedItemsPropertyId: return VT_UNKNOWN;
	case UIA_DropTargetDropTargetEffectPropertyId: return VT_BSTR;
	case UIA_DropTargetDropTargetEffectsPropertyId: return VT_BSTR;
	case UIA_ExpandCollapseExpandCollapseStatePropertyId: return VT_I4;
	case UIA_GridColumnCountPropertyId: return VT_I4;
	case UIA_GridItemColumnPropertyId: return VT_I4;
	case UIA_GridItemColumnSpanPropertyId: return VT_I4;
	case UIA_GridItemContainingGridPropertyId: return VT_UNKNOWN;
	case UIA_GridItemRowPropertyId: return VT_I4;
	case UIA_GridItemRowSpanPropertyId: return VT_I4;
	case UIA_GridRowCountPropertyId: return VT_I4;
	case UIA_LegacyIAccessibleChildIdPropertyId: return VT_I4;
	case UIA_LegacyIAccessibleDefaultActionPropertyId: return VT_BSTR;
	case UIA_LegacyIAccessibleDescriptionPropertyId: return VT_BSTR;
	case UIA_LegacyIAccessibleHelpPropertyId: return VT_BSTR;
	case UIA_LegacyIAccessibleKeyboardShortcutPropertyId: return VT_BSTR;
	case UIA_LegacyIAccessibleNamePropertyId: return VT_BSTR;
	case UIA_LegacyIAccessibleRolePropertyId: return VT_I4;
	case UIA_LegacyIAccessibleSelectionPropertyId: return VT_UNKNOWN;
	case UIA_LegacyIAccessibleStatePropertyId: return VT_I4;
	case UIA_LegacyIAccessibleValuePropertyId: return VT_BSTR;
	case UIA_MultipleViewCurrentViewPropertyId: return VT_I4;
	case UIA_MultipleViewSupportedViewsPropertyId: return VT_I4;
	case UIA_RangeValueIsReadOnlyPropertyId: return VT_BOOL;
	case UIA_RangeValueLargeChangePropertyId: return VT_R8;
	case UIA_RangeValueMaximumPropertyId: return VT_R8;
	case UIA_RangeValueMinimumPropertyId: return VT_R8;
	case UIA_RangeValueSmallChangePropertyId: return VT_R8;
	case UIA_RangeValueValuePropertyId: return VT_R8;
	case UIA_ScrollHorizontallyScrollablePropertyId: return VT_BOOL;
	case UIA_ScrollHorizontalScrollPercentPropertyId: return VT_R8;
	case UIA_ScrollHorizontalViewSizePropertyId: return VT_R8;
	case UIA_ScrollVerticallyScrollablePropertyId: return VT_BOOL;
	case UIA_ScrollVerticalScrollPercentPropertyId: return VT_R8;
	case UIA_ScrollVerticalViewSizePropertyId: return VT_R8;
	case UIA_SelectionCanSelectMultiplePropertyId: return VT_BOOL;
	case UIA_SelectionIsSelectionRequiredPropertyId: return VT_BOOL;
	case UIA_SelectionSelectionPropertyId: return VT_UNKNOWN;
	case UIA_SelectionItemIsSelectedPropertyId: return VT_BOOL;
	case UIA_SelectionItemSelectionContainerPropertyId: return VT_UNKNOWN;
	case UIA_SpreadsheetItemFormulaPropertyId: return VT_BSTR;
	case UIA_SpreadsheetItemAnnotationObjectsPropertyId: return VT_UNKNOWN;
	case UIA_SpreadsheetItemAnnotationTypesPropertyId: return VT_I4;
	case UIA_StylesExtendedPropertiesPropertyId: return VT_BSTR;
	case UIA_StylesFillColorPropertyId: return VT_I4;
	case UIA_StylesFillPatternColorPropertyId: return VT_I4;
	case UIA_StylesFillPatternStylePropertyId: return VT_BSTR;
	case UIA_StylesShapePropertyId: return VT_BSTR;
	case UIA_StylesStyleIdPropertyId: return VT_I4;
	case UIA_StylesStyleNamePropertyId: return VT_BSTR;
	case UIA_TableColumnHeadersPropertyId: return VT_UNKNOWN;
	case UIA_TableItemColumnHeaderItemsPropertyId: return VT_UNKNOWN;
	case UIA_TableRowHeadersPropertyId: return VT_UNKNOWN;
	case UIA_TableRowOrColumnMajorPropertyId: return VT_I4;
	case UIA_TableItemRowHeaderItemsPropertyId: return VT_UNKNOWN;
	case UIA_ToggleToggleStatePropertyId: return VT_I4;
	case UIA_TransformCanMovePropertyId: return VT_BOOL;
	case UIA_TransformCanResizePropertyId: return VT_BOOL;
	case UIA_TransformCanRotatePropertyId: return VT_BOOL;
	case UIA_Transform2CanZoomPropertyId: return VT_BOOL;
	case UIA_Transform2ZoomLevelPropertyId: return VT_R8;
	case UIA_Transform2ZoomMaximumPropertyId: return VT_R8;
	case UIA_Transform2ZoomMinimumPropertyId: return VT_R8;
	case UIA_ValueIsReadOnlyPropertyId: return VT_BOOL;
	case UIA_ValueValuePropertyId: return VT_BSTR;
	case UIA_WindowCanMaximizePropertyId: return VT_BOOL;
	case UIA_WindowCanMinimizePropertyId: return VT_BOOL;
	case UIA_WindowIsModalPropertyId: return VT_BOOL;
	case UIA_WindowIsTopmostPropertyId: return VT_BOOL;
	case UIA_WindowWindowInteractionStatePropertyId: return VT_I4;
	case UIA_WindowWindowVisualStatePropertyId: return VT_I4;
	case UIA_IsAnnotationPatternAvailablePropertyId: return VT_BOOL;
		//case UIA_IsCustomNavigationPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsDockPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsDragPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsDropTargetPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsExpandCollapsePatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsGridItemPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsGridPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsInvokePatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsItemContainerPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsLegacyIAccessiblePatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsMultipleViewPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsObjectModelPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsRangeValuePatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsScrollItemPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsScrollPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsSelectionItemPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsSelectionPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsSpreadsheetPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsSpreadsheetItemPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsStylesPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsSynchronizedInputPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsTableItemPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsTablePatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsTextChildPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsTextEditPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsTextPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsTextPattern2AvailablePropertyId: return VT_BOOL;
	case UIA_IsTogglePatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsTransformPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsTransformPattern2AvailablePropertyId: return VT_BOOL;
	case UIA_IsValuePatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsVirtualizedItemPatternAvailablePropertyId: return VT_BOOL;
	case UIA_IsWindowPatternAvailablePropertyId: return VT_BOOL;
	default:
		return VT_ILLEGAL;
	}
}

struct PyModuleDef controltypes_def =
{
	PyModuleDef_HEAD_INIT,
	"pyutils.ControlType",
	"Control Type Enumeration",
	-1,
	0,
};
void AddControlTypeConstants(PyObject* module)
{
	PyObject* controlTypesModule = PyModule_Create(&controltypes_def);
	if (controlTypesModule == NULL) return;

	PyModule_AddIntConstant(controlTypesModule, "Button", UIA_ButtonControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Calendar", UIA_CalendarControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "CheckBox", UIA_CheckBoxControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "ComboBox", UIA_ComboBoxControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Edit", UIA_EditControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Hyperlink", UIA_HyperlinkControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Image", UIA_ImageControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "ListItem", UIA_ListItemControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "List", UIA_ListControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Menu", UIA_MenuControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "MenuBar", UIA_MenuBarControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "MenuItem", UIA_MenuItemControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "ProgressBar", UIA_ProgressBarControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "RadioButton", UIA_RadioButtonControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "ScrollBar", UIA_ScrollBarControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Slider", UIA_SliderControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Spinner", UIA_SpinnerControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "StatusBar", UIA_StatusBarControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Tab", UIA_TabControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "TabItem", UIA_TabItemControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Text", UIA_TextControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "ToolBar", UIA_ToolBarControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "ToolTip", UIA_ToolTipControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Tree", UIA_TreeControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "TreeItem", UIA_TreeItemControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Custom", UIA_CustomControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Group", UIA_GroupControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Thumb", UIA_ThumbControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "DataGrid", UIA_DataGridControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "DataItem", UIA_DataItemControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Document", UIA_DocumentControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "SplitButton", UIA_SplitButtonControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Window", UIA_WindowControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Pane", UIA_PaneControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Header", UIA_HeaderControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "HeaderItem", UIA_HeaderItemControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Table", UIA_TableControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "TitleBar", UIA_TitleBarControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "Separator", UIA_SeparatorControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "SemanticZoom", UIA_SemanticZoomControlTypeId);
	PyModule_AddIntConstant(controlTypesModule, "AppBar", UIA_AppBarControlTypeId);

	PyModule_AddObject(module, "ControlType", controlTypesModule);
}

#pragma region AutomationCondition (base class)

void automationcondition_dealloc(automationcondition_object* self)
{
	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* automationcondition_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyErr_SetString(PyExc_TypeError, "Base type cannot be instantiated directly");
	return NULL;
}
int automationcondition_init(automationcondition_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyTypeObject AutomationConditionType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.AutomationCondition",			/* tp_name */
	sizeof(automationcondition_object),		/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)automationcondition_dealloc,	/* tp_dealloc */
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
	"Automation Condition Base Class",		/* tp_doc */
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
	(initproc)automationcondition_init,		/* tp_init */
	0,										/* tp_alloc */
	automationcondition_new,					/* tp_new */
};

#pragma endregion

#pragma region Property Condition

void propertycondition_dealloc(propertycondition_object* self)
{
	Py_XDECREF(self->key);
	Py_XDECREF(self->value);
	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* propertycondition_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyObject* key;
	PyObject* value;
	if (!PyArg_ParseTuple(args, "UO", &key, &value))
		return NULL;

	propertycondition_object* self = (propertycondition_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		self->key = key;
		self->value = value;

		// Take our own refs
		Py_INCREF(self->key);
		Py_INCREF(self->value);
	}
	return (PyObject*)self;
}
int propertycondition_init(propertycondition_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyMemberDef propertycondition_members[] = {
	{ "key", T_OBJECT, offsetof(propertycondition_object, key), READONLY, "Property name (string)" },
	{ "value", T_OBJECT, offsetof(propertycondition_object, value), READONLY, "Property value (string)" },
	{ NULL, 0, 0, 0, NULL }
};
PyTypeObject PropertyConditionType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.PropertyCondition",			/* tp_name */
	sizeof(propertycondition_object),		/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)propertycondition_dealloc,	/* tp_dealloc */
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
	"Property Condition (Key, Value)",		/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	0,										/* tp_methods */
	propertycondition_members,				/* tp_members */
	0,										/* tp_getset */
	&AutomationConditionType,				/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)propertycondition_init,		/* tp_init */
	0,										/* tp_alloc */
	propertycondition_new,					/* tp_new */
};

#pragma endregion

#pragma region And Condition

void andcondition_dealloc(andcondition_object* self)
{
	Py_XDECREF(self->conditionList);
	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* andcondition_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyObject* sourceList;
	if (!PyArg_ParseTuple(args, "O", &sourceList))
		return NULL;
	if (!PySequence_Check(sourceList))
	{
		PyErr_SetString(PyExc_TypeError, "Argument must be a sequence object");
		return NULL;
	}

	andcondition_object* self = (andcondition_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		Py_ssize_t nConditions = PySequence_Length(sourceList);
		self->conditionList = PyList_New(nConditions);
		for (Py_ssize_t i = 0; i < nConditions; i++)
		{
			PyObject* current = PySequence_GetItem(sourceList, i);
			if (!PyObject_TypeCheck(current, &AutomationConditionType))
			{
				Py_DECREF(self->conditionList);
				Py_DECREF(self);
				PyErr_SetString(PyExc_TypeError, "Object must be an automation condition.");
				return NULL;
			}

			Py_INCREF(current);
			PyList_SetItem(self->conditionList, i, current);
		}
	}
	return (PyObject*)self;
}
int andcondition_init(andcondition_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyObject* andcondition_add(andcondition_object* self, PyObject* args)
{
	if (!PyObject_TypeCheck(args, &AutomationConditionType))
	{
		PyErr_SetString(PyExc_TypeError, "Argument must be an automation condition.");
		return NULL;
	}
	PyList_Append(self->conditionList, args);
	Py_RETURN_NONE;
}
PyObject* andcondition_remove(andcondition_object* self, PyObject* arg)
{
	Py_ssize_t idx = PySequence_Index(self->conditionList, arg);
	PySequence_DelItem(self->conditionList, idx);
	Py_RETURN_NONE;
}
PyMethodDef andcondition_methods[] = {
	{ "add",  (PyCFunction)andcondition_add, METH_O, "Adds a condition" },
	{ "remove", (PyCFunction)andcondition_remove, METH_O, "Removes a condition" },
	{ NULL, NULL, 0, NULL },
};
PyMemberDef andcondition_members[] = {
	{ "list", T_OBJECT, offsetof(andcondition_object, conditionList), READONLY, "List of conditions" },
	{ NULL }
};
PyTypeObject AndConditionType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.AndCondition",					/* tp_name */
	sizeof(andcondition_object),			/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)andcondition_dealloc,		/* tp_dealloc */
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
	"And Condition",						/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	andcondition_methods,					/* tp_methods */
	andcondition_members,					/* tp_members */
	0,										/* tp_getset */
	&AutomationConditionType,				/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)andcondition_init,			/* tp_init */
	0,										/* tp_alloc */
	andcondition_new,						/* tp_new */
};

#pragma endregion

#pragma region Or Condition

void orcondition_dealloc(orcondition_object* self)
{
	Py_XDECREF(self->conditionList);
	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* orcondition_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyObject* sourceList;
	if (!PyArg_ParseTuple(args, "O", &sourceList))
		return NULL;
	if (!PySequence_Check(sourceList))
	{
		PyErr_SetString(PyExc_TypeError, "Argument must be a sequence object");
		return NULL;
	}

	orcondition_object* self = (orcondition_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		Py_ssize_t nConditions = PySequence_Length(sourceList);
		self->conditionList = PyList_New(nConditions);
		for (Py_ssize_t i = 0; i < nConditions; i++)
		{
			PyObject* current = PySequence_GetItem(sourceList, i);
			if (!PyObject_TypeCheck(current, &AutomationConditionType))
			{
				Py_DECREF(self->conditionList);
				Py_DECREF(self);
				PyErr_SetString(PyExc_TypeError, "Object must be an automation condition.");
				return NULL;
			}

			Py_INCREF(current);
			PyList_SetItem(self->conditionList, i, current);
		}
	}
	return (PyObject*)self;
}
int orcondition_init(orcondition_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyObject* orcondition_add(orcondition_object* self, PyObject* args)
{
	if (!PyObject_TypeCheck(args, &AutomationConditionType))
	{
		PyErr_SetString(PyExc_TypeError, "Argument must be an automation condition.");
		return NULL;
	}
	PyList_Append(self->conditionList, args);
	Py_RETURN_NONE;
}
PyObject* orcondition_remove(orcondition_object* self, PyObject* arg)
{
	Py_ssize_t idx = PySequence_Index(self->conditionList, arg);
	PySequence_DelItem(self->conditionList, idx);
	Py_RETURN_NONE;
}
PyMethodDef orcondition_methods[] = {
	{ "add",  (PyCFunction)orcondition_add, METH_O, "Adds a condition" },
	{ "remove", (PyCFunction)orcondition_remove, METH_O, "Removes a condition" },
	{ NULL, NULL, 0, NULL },
};
PyMemberDef orcondition_members[] = {
	{ "list", T_OBJECT, offsetof(orcondition_object, conditionList), READONLY, "List of conditions" },
	{ NULL }
};
PyTypeObject OrConditionType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.OrCondition",					/* tp_name */
	sizeof(orcondition_object),				/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)orcondition_dealloc,		/* tp_dealloc */
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
	"Or Condition",							/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	orcondition_methods,					/* tp_methods */
	orcondition_members,					/* tp_members */
	0,										/* tp_getset */
	&AutomationConditionType,				/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)orcondition_init,				/* tp_init */
	0,										/* tp_alloc */
	orcondition_new,						/* tp_new */
};

#pragma endregion

#pragma region Not Condition

void notcondition_dealloc(notcondition_object* self)
{
	Py_XDECREF(self->condition);
	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* notcondition_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyObject* source;
	if (!PyArg_ParseTuple(args, "O", &source))
		return NULL;

	if (!PyObject_TypeCheck(source, &AutomationConditionType))
	{
		PyErr_SetString(PyExc_TypeError, "Argument must be an automation condition.");
		return NULL;
	}

	notcondition_object* self = (notcondition_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		self->condition = source;
	}
	return (PyObject*)self;
}
int notcondition_init(notcondition_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyObject* notcondition_setcondition(notcondition_object* self, PyObject* args)
{
	if (!PyObject_TypeCheck(args, &NotConditionType))
	{
		PyErr_SetString(PyExc_TypeError, "Argument must be an automation condition.");
		return NULL;
	}

	Py_XDECREF(self->condition);
	Py_INCREF(args);
	self->condition = args;

	Py_RETURN_NONE;
}
PyMethodDef notcondition_methods[] = {
	{ "set_condition",  (PyCFunction)notcondition_setcondition, METH_O, "Sets the condition" },
	{ NULL, NULL, 0, NULL },
};
PyMemberDef notcondition_members[] = {
	{ "condition", T_OBJECT, offsetof(notcondition_object, condition), READONLY, "Condition Object" },
	{ NULL }
};
PyTypeObject NotConditionType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.NotCondition",					/* tp_name */
	sizeof(notcondition_object),			/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)notcondition_dealloc,		/* tp_dealloc */
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
	"Not Condition",						/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	notcondition_methods,					/* tp_methods */
	notcondition_members,					/* tp_members */
	0,										/* tp_getset */
	&AutomationConditionType,				/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)notcondition_init,			/* tp_init */
	0,										/* tp_alloc */
	notcondition_new,						/* tp_new */
};

#pragma endregion