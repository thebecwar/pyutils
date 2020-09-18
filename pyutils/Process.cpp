#include "Stdafx.h"
#include "Process.h"
#include "NamedTuple.h"

#include <Windows.h>
#include <Python.h>
#include <structmember.h>
#include <TlHelp32.h>

#include <wct.h>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <map>

using std::vector;
using std::map;
using std::string;
using std::stringstream;
using std::hex;
using std::uppercase;

HANDLE OpenProcessByName(LPWSTR szProcessName)
{
	PROCESSENTRY32W entry = { 0 };
	entry.dwSize = sizeof(PROCESSENTRY32W);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	HANDLE hProcess = NULL;

	if (Process32FirstW(snapshot, &entry))
	{
		if (wcsicmp(entry.szExeFile, szProcessName) == 0)
		{
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, entry.th32ProcessID);
		}
		else
		{
			while (Process32NextW(snapshot, &entry))
			{
				if (wcsicmp(entry.szExeFile, szProcessName) == 0)
				{
					hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, entry.th32ProcessID);
					break;
				}
			}
		}
	}

	CloseHandle(snapshot);
	return hProcess;
}
DWORD GetProcessIdByName(LPWSTR szProcessName)
{
	HANDLE hProcess = OpenProcessByName(szProcessName);
	if (hProcess == NULL)
		return -1;

	DWORD result = GetProcessId(hProcess);
	CloseHandle(hProcess);
	return result;
}
HANDLE ParseProcessArgs(PyObject* args)
{
	PyObject* arg = NULL;
	if (!PyArg_ParseTuple(args, "O", &arg))
		return NULL;

	HANDLE hProcess = NULL;
	if (PyLong_Check(arg))
	{
		DWORD pid = (DWORD)PyLong_AsLong(arg);
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	}
	else if (PyUnicode_Check(arg))
	{
		wchar_t* szProcess = PyUnicode_AsWideCharString(arg, NULL);
		hProcess = OpenProcessByName(szProcess);
		PyMem_FREE(szProcess);
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "Argument must be Process ID (long()) or Name (str())");
		return NULL;
	}

	if (hProcess == NULL)
	{
		PyErr_SetString(PyExc_WindowsError, "No such process");
		return NULL;
	}

	return hProcess;
}

bool EnableDebugPrivilege()
{
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES privileges = { 0 };

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		return false;
	}

	privileges.PrivilegeCount = 1;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &privileges.Privileges[0].Luid))
	{
		CloseHandle(hToken);
		return false;
	}

	privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &privileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		CloseHandle(hToken);
		return false;
	}

	CloseHandle(hToken);
	return true;

}
void EnumerateProcessThreads(DWORD pid, vector<DWORD>& threadIds)
{
	threadIds.clear();

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
	if (hSnap == NULL)
	{
		return;
	}

	THREADENTRY32 thread = { 0 };
	thread.dwSize = sizeof(THREADENTRY32);
	if (Thread32First(hSnap, &thread))
	{
		do
		{
			if (thread.th32OwnerProcessID == pid || pid == (DWORD)-1)
				threadIds.emplace_back(thread.th32ThreadID);
		} while (Thread32Next(hSnap, &thread));
	}

	CloseHandle(hSnap);
}
bool IsThreadLive(DWORD threadId)
{
	HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, threadId);
	if (hThread)
	{
		DWORD exitCode;
		GetExitCodeThread(hThread, &exitCode);
		CloseHandle(hThread);
		return (exitCode == STILL_ACTIVE);
	}
	return false;
}

PyStructSequence_Field HeapBlockInfo_Fields[] = {
	{ "block_handle", "Handle to the heap block."},
	{ "address", "The starting address of the block." },
	{ "block_size", "The size of the heap block, in bytes." },
	{ "flags", "FIXED (the block is fixed), FREE (the block is unused), or MOVABLE (the block is movable)" },
	{ NULL, NULL }
};
PyStructSequence_Desc HeapBlockInfo_Desc = {
	"HeapBlockInfo",
	"Information about a heap block",
	HeapBlockInfo_Fields,
	_countof(HeapBlockInfo_Fields) - 1
};
PyStructSequence_Field HeapInfo_Fields[] = {
	{ "blocks", "List of heap blocks" },
	{ NULL, NULL }
};
PyStructSequence_Desc HeapInfo_Desc = {
	"HeapInfo",
	"Information about a process' heap",
	HeapInfo_Fields,
	_countof(HeapInfo_Fields) - 1
};
PyStructSequence_Field ModuleInfo_Fields[] = {
	{ "name", "Module name." },
	{ "filename", "Module path." },
	{ "process_id", "Module process id." },
	{ "global_use_count", "Reference Count (global)." },
	{ "process_use_count", "Reference Count (process)." },
	{ "base_address", "Module Base Address." },
	{ "base_size", "Module Base Size." },
	{ NULL, NULL }
};
PyStructSequence_Desc ModuleInfo_Desc = {
	"ModuleInfo",
	"Information about a loaded module",
	ModuleInfo_Fields,
	_countof(ModuleInfo_Fields) - 1,
};
PyStructSequence_Field WaitChainNode_Fields[] = {
	{ "type", "Object Type" },
	{ "status", "Object Status" },
	{ "name", "Object Name" },
	{ "timeout", "Wait timeout" },
	{ "alertable", "Alertable" },
	{ "process_id", "Process ID" },
	{ "thread_id", "Thread ID" },
	{ "wait_time", "Wait Time" },
	{ "context_switches", "Context Switches" },
	{ NULL, NULL }
};
PyStructSequence_Desc WaitChainNode_Desc = {
	"WaitChainNode",
	"A node in the thread wait chain",
	WaitChainNode_Fields,
	_countof(WaitChainNode_Fields) - 1,
};
PyStructSequence_Field WaitChainInfo_Fields[] = {
	{ "cycle_detected", "Whether there is a cycle." },
	{ "chain", "List of wait chain nodes" },
	{ NULL, NULL }
};
PyStructSequence_Desc WaitChainInfo_Desc = {
	"WaitChainInfo",
	"Wait chain information for a thread.",
	WaitChainInfo_Fields,
	_countof(WaitChainInfo_Fields) - 1,
};
PyStructSequence_Field GdiHandleInfo_Fields[] = {
	{ "objects", "GDI Objects." },
	{ "objects_peak", "Peak number of GDI objects." },
	{ "user", "GDI USER Objects." },
	{ "user_peak", "Peak number of GDI USER objects." },
	{ NULL, NULL }
};
PyStructSequence_Desc GdiHandleInfo_Desc = {
	"GdiHandleInfo",
	"Information about a process' GDI objects.",
	GdiHandleInfo_Fields,
	_countof(GdiHandleInfo_Fields) - 1,
};

PyTypeObject* HeapBlockInfo_Type = NULL;
PyTypeObject* HeapInfo_Type = NULL;
PyTypeObject* ModuleInfo_Type = NULL;
PyTypeObject* WaitChainNode_Type = NULL;
PyTypeObject* WaitChainInfo_Type = NULL;
PyTypeObject* GdiHandleInfo_Type = NULL;

bool Process_InitTypes(PyObject* module)
{
	HeapBlockInfo_Type = NamedTuple_NewType(&HeapBlockInfo_Desc);
	HeapInfo_Type = NamedTuple_NewType(&HeapInfo_Desc);
	ModuleInfo_Type = NamedTuple_NewType(&ModuleInfo_Desc);
	WaitChainNode_Type = NamedTuple_NewType(&WaitChainNode_Desc);
	WaitChainInfo_Type = NamedTuple_NewType(&WaitChainInfo_Desc);
	GdiHandleInfo_Type = NamedTuple_NewType(&GdiHandleInfo_Desc);
	return true;
}

typedef struct {
	PyObject_HEAD;
	HANDLE hProcess;
	DWORD pid;
} process_object;

void process_dealloc(process_object* self)
{
	if (self->hProcess != NULL)
	{
		CloseHandle(self->hProcess);
	}
	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* process_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyObject* nameOrPid = NULL;

	if (!PyArg_ParseTuple(args, "O", &nameOrPid))
		return NULL;

	process_object* self = (process_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		DWORD pid = -1;
		if (PyLong_Check(nameOrPid))
		{
			pid = PyLong_AsLong(nameOrPid);
		}
		else if (PyUnicode_Check(nameOrPid))
		{
			wchar_t* szProcName = PyUnicode_AsWideCharString(nameOrPid, NULL);
			pid = GetProcessIdByName(szProcName);
			PyMem_FREE(szProcName);
		}
		else
		{
			PyErr_SetString(PyExc_TypeError, "Argument must me process name or PID");
			Py_DECREF(self);
			return NULL;
		}

		self->hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
		if (self->hProcess == NULL)
		{
			int err = GetLastError();
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, err);
			Py_DECREF(self);
			return NULL;
		}

		self->pid = pid;
	}
	return (PyObject*)self;
}
int process_init(process_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyObject* process_wait(process_object* self, PyObject* args)
{
	int timeout = INFINITE;
	if (!PyArg_ParseTuple(args, "|i", &timeout))
		return NULL;

	if (self->hProcess == NULL)
	{
		PyErr_SetString(PyExc_OSError, "Invalid process.");
		return NULL;
	}

	DWORD result = WaitForSingleObject(self->hProcess, timeout);
	if (result == WAIT_OBJECT_0)
		Py_RETURN_NONE;
	else if (result == WAIT_ABANDONED)
	{
		PyErr_SetString(PyExc_WindowsError, "Abandoned Wait");
		return NULL;
	}
	else if (result == WAIT_TIMEOUT)
	{
		PyErr_SetString(PyExc_TimeoutError, "Timeout");
		return NULL;
	}
	else if (result == WAIT_FAILED)
	{
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
		return NULL;
	}
	else
	{
		return PyLong_FromLong(result);
	}
}
PyObject* process_kill(process_object* self)
{
	if (self->hProcess)
	{
		TerminateProcess(self->hProcess, -1);
	}
	Py_RETURN_NONE;
}
PyObject* process_heapentrytotuple(HEAPENTRY32& pEntry)
{
	PyObject* o = NamedTuple_New(HeapBlockInfo_Type);
	NamedTuple_SetItemString(o, "block_handle", PyLong_FromVoidPtr(pEntry.hHandle));
	NamedTuple_SetItemString(o, "address", PyLong_FromUnsignedLongLong(pEntry.dwAddress));
	NamedTuple_SetItemString(o, "block_size", PyLong_FromSize_t(pEntry.dwBlockSize));
	
	PyObject* flags = NULL;
	switch (pEntry.dwFlags)
	{
	case LF32_FIXED:
		flags = PyUnicode_FromString("FIXED");
		break;
	case LF32_FREE:
		flags = PyUnicode_FromString("FREE");
		break;
	case LF32_MOVEABLE:
		flags = PyUnicode_FromString("MOVEABLE");
		break;
	default:
		flags = Py_None;
		Py_INCREF(Py_None);
	}
	NamedTuple_SetItemString(o, "flags", flags);
	return o;
}
PyObject* process_heapinfototuple(HEAPLIST32* pHeap, vector<HEAPENTRY32>& entries)
{
	PyObject* heapInfo = NamedTuple_New(HeapInfo_Type);
	PyObject* blockList = PyList_New(entries.size());
	for (int i = 0; i < entries.size(); i++)
		PyList_SetItem(blockList, i, process_heapentrytotuple(entries.at(i)));
	NamedTuple_SetItemString(heapInfo, "blocks", blockList);
	return heapInfo;
}
PyObject* process_getheapinfo(process_object* self)
{
	PyObject* heapsInfo;

	HANDLE hHeapSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPHEAPLIST, self->pid);
	if (hHeapSnapshot == INVALID_HANDLE_VALUE)
	{
		heapsInfo = Py_None;
		Py_INCREF(heapsInfo);
	}
	else
	{
		HEAPLIST32 hl = { 0 };
		hl.dwSize = sizeof(HEAPLIST32);

		HEAPENTRY32 he = { 0 };
		he.dwSize = sizeof(HEAPENTRY32);

		vector<HEAPLIST32> heaps;
		map<UINT_PTR, vector<HEAPENTRY32>> entries;

		if (Heap32ListFirst(hHeapSnapshot, &hl))
		{
			do
			{
				if (Heap32First(&he, self->pid, hl.th32HeapID))
				{
					vector<HEAPENTRY32> ent;
					do
					{
						ent.emplace_back(he);
						he.dwSize = sizeof(HEAPENTRY32);
					} while (Heap32Next(&he));
					
					heaps.emplace_back(hl);
					entries.emplace(hl.th32HeapID, ent);
				}
				hl.dwSize = sizeof(HEAPLIST32);
			} while (Heap32ListNext(hHeapSnapshot, &hl));
		}
		CloseHandle(hHeapSnapshot);

		

		if (heaps.size() == 1)
		{
			heapsInfo = process_heapinfototuple(&heaps.at(0), entries[heaps.at(0).th32HeapID]);
		}
		else if (heaps.size() > 1)
		{
			heapsInfo = PyList_New(heaps.size());
			for (int i = 0; i < heaps.size(); i++)
				PyList_SetItem(heapsInfo, i, process_heapinfototuple(&heaps.at(0), entries[heaps.at(0).th32HeapID]));
		}
		else
		{
			heapsInfo = Py_None;
			Py_INCREF(Py_None);
		}
	}

	return heapsInfo;
}
PyObject* process_moduleentrytotuple(MODULEENTRY32W& pEntry)
{
	PyObject* result = NamedTuple_New(ModuleInfo_Type);
	NamedTuple_SetItemString(result, "name", PyUnicode_FromWideChar(pEntry.szModule, -1));
	NamedTuple_SetItemString(result, "filename", PyUnicode_FromWideChar(pEntry.szExePath, -1));
	NamedTuple_SetItemString(result, "process_id", PyLong_FromUnsignedLong(pEntry.th32ProcessID));
	NamedTuple_SetItemString(result, "global_use_count", PyLong_FromUnsignedLong(pEntry.GlblcntUsage));
	NamedTuple_SetItemString(result, "process_use_count", PyLong_FromUnsignedLong(pEntry.ProccntUsage));
	NamedTuple_SetItemString(result, "base_address", PyLong_FromVoidPtr(pEntry.modBaseAddr));
	NamedTuple_SetItemString(result, "base_size", PyLong_FromUnsignedLong(pEntry.modBaseSize));
	return result;
}
PyObject* process_getmoduleinfo(process_object* self)
{
	HANDLE hModSnapshot = INVALID_HANDLE_VALUE;
	MODULEENTRY32W entry = { 0 };
	entry.dwSize = sizeof(MODULEENTRY32W);

	hModSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, self->pid);
	if (hModSnapshot == INVALID_HANDLE_VALUE)
	{
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
		return NULL;
	}

	if (!Module32FirstW(hModSnapshot, &entry))
	{
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
		CloseHandle(hModSnapshot);
		return NULL;
	}
	
	vector<MODULEENTRY32W> moduleList;
	do
	{
		moduleList.push_back(entry);
	} while (Module32NextW(hModSnapshot, &entry));

	CloseHandle(hModSnapshot);

	PyObject* result;
	if (moduleList.size() == 1)
	{
		result = process_moduleentrytotuple(moduleList[0]);
	}
	else if (moduleList.size() > 1)
	{
		result = PyList_New(moduleList.size());
		for (int i = 0; i < moduleList.size(); i++)
			PyList_SetItem(result, i, process_moduleentrytotuple(moduleList[i]));
	}
	else
	{
		result = Py_None;
		Py_INCREF(Py_None);
	}

	return result;
}

#define SET_TYPE(a) case a: NamedTuple_SetItemString(result, "type", PyUnicode_FromString(#a)); break;
#define SET_STATUS(a) case a: NamedTuple_SetItemString(result, "status", PyUnicode_FromString(#a)); break;


PyObject* process_waitchainnodetotuple(WAITCHAIN_NODE_INFO& node)
{
	PyObject* result = NamedTuple_New(WaitChainNode_Type);

	switch (node.ObjectType)
	{
		SET_TYPE(WctCriticalSectionType);
		SET_TYPE(WctSendMessageType);
		SET_TYPE(WctMutexType);
		SET_TYPE(WctAlpcType);
		SET_TYPE(WctComType);
		SET_TYPE(WctThreadWaitType);
		SET_TYPE(WctProcessWaitType);
		SET_TYPE(WctThreadType);
		SET_TYPE(WctComActivationType);
		SET_TYPE(WctUnknownType);
	}
	switch (node.ObjectStatus)
	{
		SET_STATUS(WctStatusNoAccess);
		SET_STATUS(WctStatusRunning);
		SET_STATUS(WctStatusBlocked);
		SET_STATUS(WctStatusPidOnly);
		SET_STATUS(WctStatusPidOnlyRpcss);
		SET_STATUS(WctStatusOwned);
		SET_STATUS(WctStatusNotOwned);
		SET_STATUS(WctStatusAbandoned);
		SET_STATUS(WctStatusUnknown);
		SET_STATUS(WctStatusError);
	}
	
	NamedTuple_SetItemString(result, "name", CWSTRING(node.LockObject.ObjectName));
	NamedTuple_SetItemString(result, "timeout", PyLong_FromLongLong(node.LockObject.Timeout.QuadPart));
	NamedTuple_SetItemString(result, "alertable", CBOOL(node.LockObject.Alertable));
	NamedTuple_SetItemString(result, "process_id", PyLong_FromUnsignedLong(node.ThreadObject.ProcessId));
	NamedTuple_SetItemString(result, "thread_id", PyLong_FromUnsignedLong(node.ThreadObject.ThreadId));
	NamedTuple_SetItemString(result, "wait_time", PyLong_FromUnsignedLong(node.ThreadObject.WaitTime));
	NamedTuple_SetItemString(result, "context_switches", PyLong_FromUnsignedLong(node.ThreadObject.ContextSwitches));

	return result;
}
PyObject* process_waitchaintotuple(vector<WAITCHAIN_NODE_INFO>& chain, BOOL cycle)
{
	PyObject* list = PyList_New(chain.size());
	for (int i = 0; i < chain.size(); i++)
	{
		PyList_SetItem(list, i, process_waitchainnodetotuple(chain[i]));
	}

	PyObject* result = NamedTuple_New(WaitChainInfo_Type);
	NamedTuple_SetItemString(result, "cycle_detected", CBOOL(cycle));
	NamedTuple_SetItemString(result, "chain", list);
	return result;
}
PyObject* process_getwaitchain(process_object* self)
{
	HWCT hSession = OpenThreadWaitChainSession(0, NULL);
	if (hSession == NULL)
	{
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
		return NULL;
	}

	EnableDebugPrivilege();

	vector<DWORD> threads;
	EnumerateProcessThreads(self->pid, threads);

	WAITCHAIN_NODE_INFO* nodes;
	DWORD count = 0;
	BOOL isCycle = FALSE;

	map<DWORD, BOOL> cycleMap;
	map<DWORD, vector<WAITCHAIN_NODE_INFO>> nodeMap;
	for (int i = 0; i < threads.size(); i++)
	{
		nodes = new WAITCHAIN_NODE_INFO[WCT_MAX_NODE_COUNT];
		count = WCT_MAX_NODE_COUNT;
		if (!GetThreadWaitChain(hSession, NULL, WCTP_GETINFO_ALL_FLAGS, threads[i], &count, nodes, &isCycle))
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
			goto err;
		}

		if (count > WCT_MAX_NODE_COUNT)
		{
			delete[] nodes;
			nodes = new WAITCHAIN_NODE_INFO[count];
			if (!GetThreadWaitChain(hSession, NULL, WCTP_GETINFO_ALL_FLAGS, threads[i], &count, nodes, &isCycle))
			{
				PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
				goto err;
			}
		}

		cycleMap.emplace(threads[i], isCycle);

		vector<WAITCHAIN_NODE_INFO> threadNodes;
		for (DWORD idx = 0; idx < count; idx++)
		{
			threadNodes.emplace_back(nodes[idx]);
		}
		nodeMap.emplace(threads[i], threadNodes);

		delete[] nodes;
	}

	CloseThreadWaitChainSession(hSession);

	PyObject* dict = PyDict_New();
	for (auto iter = nodeMap.begin(); iter != nodeMap.end(); iter++)
	{
		stringstream ss;
		ss << "0x" << uppercase << hex << iter->first;
		PyDict_SetItemString(dict, ss.str().c_str(), process_waitchaintotuple(iter->second, cycleMap[iter->first]));
	}
	return dict;

err:
	if (nodes) delete[] nodes;
	CloseThreadWaitChainSession(hSession);
	return NULL;
}

#undef SET_TYPE
#undef SET_STATUS

PyObject* process_getgdiobjects(process_object* self)
{
	PyObject* result = NamedTuple_New(GdiHandleInfo_Type);
	NamedTuple_SetItemString(result, "objects", PyLong_FromUnsignedLong(GetGuiResources(self->hProcess, GR_GDIOBJECTS)));
	NamedTuple_SetItemString(result, "objects_peak", PyLong_FromUnsignedLong(GetGuiResources(self->hProcess, GR_GDIOBJECTS_PEAK)));
	NamedTuple_SetItemString(result, "user", PyLong_FromUnsignedLong(GetGuiResources(self->hProcess, GR_USEROBJECTS)));
	NamedTuple_SetItemString(result, "user_peak", PyLong_FromUnsignedLong(GetGuiResources(self->hProcess, GR_USEROBJECTS_PEAK)));
	return result;
}


PyMethodDef process_methods[] = {
	{ "wait", (PyCFunction)process_wait, METH_VARARGS, "Waits for the process to terminate." },
	{ "kill", (PyCFunction)process_kill, METH_NOARGS, "Terminates the process" },
	{ "get_heap_info", (PyCFunction)process_getheapinfo, METH_NOARGS, "Gets diagnostic information about the process' heaps." },
	{ "get_module_info", (PyCFunction)process_getmoduleinfo, METH_NOARGS, "Gets diagnostic information about the process' modules." },
	{ "get_wait_chain", (PyCFunction)process_getwaitchain, METH_NOARGS, "Gets the thread wait chain for this process." },
	{ "get_gdi_info", (PyCFunction)process_getgdiobjects, METH_NOARGS, "Gets the counts of the process' GDI objects." },
	{ NULL, NULL, 0, NULL },
};
PyMemberDef process_members[] = {
	{ "pid", T_INT, offsetof(process_object, pid), READONLY, "" },
	{ NULL }
};
PyTypeObject process_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.Process",						/* tp_name */
	sizeof(process_object),					/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)process_dealloc,			/* tp_dealloc */
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
	"Windows API Process wrapper",			/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	process_methods,						/* tp_methods */
	process_members,						/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)process_init,					/* tp_init */
	0,										/* tp_alloc */
	process_new,							/* tp_new */
};