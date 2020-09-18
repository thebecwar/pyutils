#include "Stdafx.h"
#include "PerformanceCounter.h"

#include <Windows.h>
#include <Pdh.h>
#include <PdhMsg.h>
#include <Python.h>
#include <structmember.h>
#include <vector>
#include <sstream>

DWORD_PTR perfcounter_user_data = 0;
HANDLE hPdhLibrary = NULL;

void SetPythonErrorFromPdhStatus(PDH_STATUS stat)
{
	if (hPdhLibrary == NULL)
	{
		hPdhLibrary = LoadLibraryW(L"pdh.dll");
		if (hPdhLibrary == NULL)
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
			return;
		}
	}

	LPWSTR pMessage = NULL;
	if (!FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		hPdhLibrary,
		stat,
		0,
		(LPWSTR)&pMessage,
		0,
		NULL))
	{
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
	}

	PyObject* msg = PyUnicode_FromWideChar(pMessage, lstrlenW(pMessage));
	PyErr_SetObject(PyExc_WindowsError, msg);
	Py_DECREF(msg);

	LocalFree(pMessage);

	return;
}

typedef struct {
	PyObject_HEAD;
	DWORD_PTR dwUserData;
	HANDLE hQuery;
	PyObject* counterList;
	std::vector<HANDLE>* counters;
	bool monitor;
	HANDLE hThread;
	DWORD dwThreadId;
	int sleepInterval;
	//PyObject* resultList;
	std::vector<std::vector<std::string>> results;
	PyObject* callback;
	int callbackBatchSize;
	FILE* logFile;
	CRITICAL_SECTION critsection;
} performancecounter_object;


void performancecounter_dealloc(performancecounter_object* self)
{
	if (self->hQuery != NULL)
	{
		PDH_STATUS res = PdhCloseQuery(self->hQuery);

		self->hQuery = NULL;
	}

	if (self->counters != NULL)
	{
		delete self->counters;
		self->counters = NULL;
	}

	if (self->hThread != NULL)
	{
		self->monitor = false;
		CloseHandle(self->hThread);
		self->hThread = NULL;
	}
	if (self->results.size() > 0)
	{
		self->results.clear();
	}

	DeleteCriticalSection(&self->critsection);


	Py_XDECREF(self->counterList);

	self->monitor = false;

	Py_XDECREF(self->callback);

	if (self->logFile)
	{
		fclose(self->logFile);
		self->logFile = NULL;
	}

	Py_TYPE(self)->tp_free((PyObject*)self);
}
PyObject* performancecounter_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	//static char* kwlist[] = { "name", "size", NULL };
	//if (!PyArg_ParseTupleAndKeywords(args, kwds, "Ui", &kwlist[0], &name, &size))
	//	return NULL;

	performancecounter_object* self = (performancecounter_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		InterlockedIncrement(&perfcounter_user_data);

		PDH_STATUS res = PdhOpenQueryW(NULL, self->dwUserData, &self->hQuery);
		if (res != ERROR_SUCCESS)
		{
			SetPythonErrorFromPdhStatus(res);
			Py_DECREF(self);
			return NULL;
		}

		self->counters = new std::vector<HANDLE>();
		self->sleepInterval = 1000;
		self->monitor = false;
		self->counterList = PyList_New(0);
		// self->results (RAII)
		self->callback = NULL;
		self->callbackBatchSize = INT_MAX;
		InitializeCriticalSection(&self->critsection);
	}
	return (PyObject*)self;
}
int performancecounter_init(performancecounter_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}
PyObject* performancecounter_addstatic(PyObject* selfObj, PyObject* args, PyObject* kwargs)
{
	performancecounter_object* self = (performancecounter_object*)selfObj;
	if (self->monitor)
	{
		PyErr_SetString(PyExc_AssertionError, "Can't add results while monitoring.");
		return NULL;
	}

	EnterCriticalSection(&self->critsection);
	self->results.clear();
	LeaveCriticalSection(&self->critsection);

	static char* kwlist[] = { "path", NULL };

	PyObject* path = NULL;

	if (kwargs == NULL)
	{
		if (PyUnicode_Check(args))
		{
			path = args;
		}
		else
		{
			PyErr_SetString(PyExc_TypeError, "Only string paths are supported");
			return NULL;
		}
	}
	else
	{
		if (!PyArg_ParseTupleAndKeywords(args, kwargs, "U", &kwlist[0], &path))
			return NULL;
	}
	LPCWSTR szPath = NULL;
	if (path != NULL)
	{
		szPath = PyUnicode_AsWideCharString(path, NULL);
	}
	else
	{
		PyErr_SetString(PyExc_NameError, "Path must be specified");
		return NULL;
	}


	HANDLE hCounter = NULL;
	PDH_STATUS res = PdhAddCounterW(self->hQuery, szPath, self->dwUserData, &hCounter);
	if (res != ERROR_SUCCESS)
	{
		PyMem_Free((void*)szPath);
		SetPythonErrorFromPdhStatus(res);
		return NULL;
	}

	self->counters->push_back(hCounter);
	PyList_Append(self->counterList, path);

	PyMem_Free((void*)szPath);

	Py_RETURN_NONE;
}
PyObject* performancecounter_addmulti(PyObject* selfObj, PyObject* args, PyObject* kwargs)
{
	static char* kwlist[] = { "paths", NULL };

	performancecounter_object* self = (performancecounter_object*)selfObj;
	if (self->monitor)
	{
		PyErr_SetString(PyExc_AssertionError, "Can't add results while monitoring.");
		return NULL;
	}

	EnterCriticalSection(&self->critsection);
	self->results.clear();
	LeaveCriticalSection(&self->critsection);

	PyObject* paths = NULL;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", &kwlist[0], &paths))
		return NULL;

	PyObject* iter = PyObject_GetIter(paths);
	PyObject* item = NULL;

	if (iter == NULL)
	{
		PyErr_SetString(PyExc_TypeError, "Argument must be iterable");
		return NULL;
	}

	PyObject* result;

	while (item = PyIter_Next(iter))
	{
		result = performancecounter_addstatic(selfObj, item, NULL);
		if (result == NULL) break;
		Py_XDECREF(result);
		Py_DECREF(item);
	}

	Py_DECREF(iter);

	if (PyErr_Occurred())
		return NULL;

	Py_RETURN_NONE;
}
PyObject* performancecounter_buildpath(PyObject* selfObj, PyObject* args, PyObject* kwargs)
{
	static char* kwlist[] = { "machine_name", "object_name", "instance_name", "parent_instance", "instance_index", "counter_name", NULL };

	PyObject* machine = NULL;
	PyObject* object = NULL;
	PyObject* instance = NULL;
	PyObject* parent = NULL;
	int index = 0;
	PyObject* counter = NULL;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "UUUUiU", &kwlist[0], &machine, &object, &instance, &parent, &index, &counter))
		return NULL;

	if (machine == NULL ||
		object == NULL ||
		instance == NULL ||
		parent == NULL ||
		counter == NULL)
	{
		PyErr_SetString(PyExc_Exception, "All parameters are required.");
		return NULL;
	}

	PDH_COUNTER_PATH_ELEMENTS_W elements = { 0 };
	elements.szMachineName = (LPWSTR)PyUnicode_AsWideCharString(machine, NULL);
	elements.szObjectName = (LPWSTR)PyUnicode_AsWideCharString(object, NULL);
	elements.szInstanceName = (LPWSTR)PyUnicode_AsWideCharString(instance, NULL);
	elements.szParentInstance = (LPWSTR)PyUnicode_AsWideCharString(parent, NULL);
	elements.dwInstanceIndex = index;
	elements.szCounterName = (LPWSTR)PyUnicode_AsWideCharString(counter, NULL);

	DWORD cchPath = 0;
	PDH_STATUS res = PdhMakeCounterPathW(&elements, NULL, &cchPath, 0);

	if (res != PDH_MORE_DATA)
	{
		SetPythonErrorFromPdhStatus(res);

		PyMem_FREE(elements.szMachineName);
		PyMem_FREE(elements.szObjectName);
		PyMem_FREE(elements.szInstanceName);
		PyMem_FREE(elements.szParentInstance);
		PyMem_FREE(elements.szCounterName);

		return NULL;
	}

	LPWSTR str = (LPWSTR)calloc(cchPath, sizeof(wchar_t));
	res = PdhMakeCounterPathW(&elements, str, &cchPath, 0);

	PyMem_FREE(elements.szMachineName);
	PyMem_FREE(elements.szObjectName);
	PyMem_FREE(elements.szInstanceName);
	PyMem_FREE(elements.szParentInstance);
	PyMem_FREE(elements.szCounterName);

	if (res != ERROR_SUCCESS)
	{
		SetPythonErrorFromPdhStatus(res);
		free(str);
		return NULL;
	}

	PyObject* result = PyUnicode_FromWideChar(str, -1);
	free(str);

	return result;
}
WCHAR performancecounter_pathbuffer[PDH_MAX_COUNTER_PATH];
PDH_BROWSE_DLG_CONFIG_W performancecounter_browseconfig;
PDH_STATUS __stdcall performancecounter_pathcallback(DWORD_PTR dwArg)
{
	if (dwArg == NULL || performancecounter_pathbuffer == NULL)
		return E_FAIL;

	performancecounter_object* self = (performancecounter_object*)dwArg;
	HANDLE hCounter = NULL;

	// performancecounter_pathbuffer contains a double-null terminated string array
	int len = lstrlenW(performancecounter_pathbuffer);
	int total = 0;
	while (total < PDH_MAX_COUNTER_PATH && len > 0)
	{
		PDH_STATUS res = PdhAddCounterW(self->hQuery, &performancecounter_pathbuffer[total], self->dwUserData, &hCounter);

		if (res != ERROR_SUCCESS)
			return res;

		PyObject* path = PyUnicode_FromWideChar(&performancecounter_pathbuffer[total], -1);
		self->counters->push_back(hCounter);
		PyList_Append(self->counterList, path);

		total += 1 + len;
		len = lstrlenW(&performancecounter_pathbuffer[total]);
	}

	return ERROR_SUCCESS;
}
PyObject* performancecounter_browsecounters(PyObject* selfObj)
{
	ZeroMemory(&performancecounter_browseconfig, sizeof(PDH_BROWSE_DLG_CONFIG_W));

	performancecounter_browseconfig.bIncludeInstanceIndex = TRUE;
	performancecounter_browseconfig.bSingleCounterPerAdd = FALSE; // BUG IN API - MUST BE FALSE!
	performancecounter_browseconfig.bSingleCounterPerDialog = FALSE;
	performancecounter_browseconfig.bLocalCountersOnly = FALSE;
	performancecounter_browseconfig.bWildCardInstances = TRUE;
	performancecounter_browseconfig.bHideDetailBox = FALSE;
	performancecounter_browseconfig.bInitializePath = FALSE;
	performancecounter_browseconfig.bIncludeCostlyObjects = TRUE;
	performancecounter_browseconfig.bShowObjectBrowser = FALSE;
	//config.bReserved = 0;
	performancecounter_browseconfig.hWndOwner = NULL;
	performancecounter_browseconfig.szDataSource = NULL;
	performancecounter_browseconfig.szReturnPathBuffer = performancecounter_pathbuffer;
	performancecounter_browseconfig.cchReturnPathLength = PDH_MAX_COUNTER_PATH;
	performancecounter_browseconfig.pCallBack = performancecounter_pathcallback;
	performancecounter_browseconfig.dwCallBackArg = (DWORD_PTR)selfObj;
	performancecounter_browseconfig.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;
	performancecounter_browseconfig.szDialogBoxCaption = L"Select Performance Counters";

	PDH_STATUS res = PdhBrowseCountersW(&performancecounter_browseconfig);

	if (res != ERROR_SUCCESS &&
		res != PDH_DIALOG_CANCELLED)
	{
		SetPythonErrorFromPdhStatus(res);
		return NULL;
	}

	Py_RETURN_NONE;
}
PyObject* performancecounter_fetch(PyObject* selfObj, PyObject* args)
{
	int index = 0;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;

	performancecounter_object* self = (performancecounter_object*)selfObj;
	if (index < 0 ||
		index >= self->counters->size())
	{
		PyErr_SetString(PyExc_IndexError, "Index out of range");
		return NULL;
	}

	PDH_STATUS res = PdhCollectQueryData(self->hQuery);
	if (res != ERROR_SUCCESS)
	{
		SetPythonErrorFromPdhStatus(res);
		return NULL;
	}

	HANDLE hCounter = self->counters->at(index);
	PDH_FMT_COUNTERVALUE val = { 0 };
	res = PdhGetFormattedCounterValue(hCounter, PDH_FMT_DOUBLE, NULL, &val);

	if (res == ERROR_SUCCESS)
	{
		PyObject* result = PyLong_FromDouble(val.doubleValue);
		return result;
	}
	else
	{
		SetPythonErrorFromPdhStatus(res);
		return NULL;
	}
}
PyObject* performancecounter_fetchall(PyObject* selfObj)
{
	performancecounter_object* self = (performancecounter_object*)selfObj;

	PDH_STATUS res = PdhCollectQueryData(self->hQuery);
	if (res != ERROR_SUCCESS)
	{
		SetPythonErrorFromPdhStatus(res);
		return NULL;
	}

	PyObject* result = PyList_New(0);
	PDH_FMT_COUNTERVALUE val = { 0 };
	for (auto iter = self->counters->begin(); iter != self->counters->end(); iter++)
	{
		res = PdhGetFormattedCounterValue(*iter, PDH_FMT_DOUBLE, NULL, &val);
		if (res != ERROR_SUCCESS)
		{
			SetPythonErrorFromPdhStatus(res);
			Py_DECREF(result);
			return NULL;
		}
		PyList_Append(result, PyLong_FromDouble(val.doubleValue));
	}
	return result;
}

// Async Performance Monitoring
DWORD CALLBACK performancecounter_ThreadProc(LPVOID selfObj)
{
	performancecounter_object* self = (performancecounter_object*)selfObj;

	int fetchCount = 0;
	bool wroteHeader = false;
	std::vector<std::string> row;

	while (self->monitor)
	{
		int timeRemaining = self->sleepInterval;
		while (timeRemaining > 0 && self->monitor)
		{
			timeRemaining -= 100;
			Sleep(100);
		}
		if (!self->monitor) return 0;

		row.clear();

		std::stringstream ss;
		SYSTEMTIME time;
		GetLocalTime(&time);

		ss << ((time.wHour < 10) ? "0" : "") << time.wHour << ":" <<
			((time.wMinute < 10) ? "0" : "") << time.wMinute << ":" <<
			((time.wSecond < 10) ? "0" : "") << time.wSecond << "." <<
			((time.wMilliseconds < 100) ? "0" : "") << ((time.wMilliseconds < 10) ? "0" : "") << time.wMilliseconds;
		row.push_back(ss.str());
		ss.str("");

		PDH_STATUS res = PdhCollectQueryData(self->hQuery);
		if (res == ERROR_SUCCESS)
		{
			PDH_FMT_COUNTERVALUE val = { 0 };
			for (auto iter = self->counters->begin(); iter != self->counters->end(); iter++)
			{
				res = PdhGetFormattedCounterValue(*iter, PDH_FMT_DOUBLE, NULL, &val);
				if (res != ERROR_SUCCESS)
				{
					row.push_back("ERROR_READING");
				}
				else
				{
					ss.str("");

					long long llval = (long long)val.doubleValue;
					if ((double)llval == val.doubleValue)
						ss << llval;
					else
						ss << val.doubleValue;
					row.push_back(ss.str());
				}
			}

			EnterCriticalSection(&self->critsection);
			self->results.push_back(row);
			LeaveCriticalSection(&self->critsection);

			fetchCount++;
			if (fetchCount > self->callbackBatchSize)
			{
				fetchCount = 0;
				if (self->callback)
				{
					PyGILState_STATE state = PyGILState_Ensure();

					PyObject* arg = PyTuple_New(0);
					PyObject* result = PyObject_Call(self->callback, arg, NULL);
					if (PyErr_Occurred())
						PyErr_Clear(); // Don't allow the monitor to generate exceptions.
					Py_XDECREF(arg);
					Py_XDECREF(result);

					PyGILState_Release(state);
				}
			}

			if (self->logFile)
			{
				std::stringstream filedata;
				if (!wroteHeader)
				{
					filedata << "Timestamp";
					Py_ssize_t cols = PySequence_Length(self->counterList);
					for (Py_ssize_t i = 0; i < cols; i++)
					{
						PyObject* obj = PySequence_GetItem(self->counterList, i);
						char* name = PyUnicode_AsUTF8(obj);
						filedata << "," << name;
						// PyMem_FREE(name); -> We don't need to release this, it belongs to the string object.
					}
					filedata << std::endl;
					wroteHeader = true;
				}
				for (auto iter = row.begin(); iter != row.end(); iter++)
				{
					filedata << (*iter) << ",";
				}
				filedata.seekp(-1, filedata.cur);
				filedata << std::endl;

				EnterCriticalSection(&self->critsection);
				fwrite(filedata.str().c_str(), sizeof(char) /* 1 */, filedata.str().size(), self->logFile);
				fflush(self->logFile);
				LeaveCriticalSection(&self->critsection);
			}
		}
	}
	return 0;
}
PyObject* performancecounter_startmonitoring(PyObject* selfObj)
{
	Py_Initialize();
	if (!PyEval_ThreadsInitialized())
	{
		PyEval_InitThreads();
	}
	performancecounter_object* self = (performancecounter_object*)selfObj;
	self->monitor = true;
	self->hThread = CreateThread(NULL, 0, performancecounter_ThreadProc, (LPVOID)self, 0, &self->dwThreadId);

	if (self->hThread == NULL)
	{
		PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
		return NULL;
	}
	Py_RETURN_NONE;
}
PyObject* performancecounter_stopmonitoring(PyObject* selfObj)
{
	performancecounter_object* self = (performancecounter_object*)selfObj;
	self->monitor = false;
	DWORD result = WaitForSingleObject(self->hThread, 1000);
	if (result != WAIT_OBJECT_0)
	{
		PyErr_SetString(PyExc_WindowsError, "Timed out waiting for thread to complete.");
		return NULL;
	}

	Py_RETURN_NONE;
}
PyObject* performancecounter_setinterval(PyObject* selfObj, PyObject* args)
{
	int timeout = 1000;
	if (!PyArg_ParseTuple(args, "i", &timeout))
		return NULL;

	((performancecounter_object*)selfObj)->sleepInterval = timeout;

	Py_RETURN_NONE;
}
PyObject* performancecounter_getlog(PyObject* selfObj, PyObject* args)
{
	bool header = false;
	if (!PyArg_ParseTuple(args, "|p", &header))
		return NULL;

	performancecounter_object* self = (performancecounter_object*)selfObj;

	PyObject* result = PyList_New(0);

	if (header)
	{
		PyObject* headerRow = PyList_New(0);
		PyList_Append(headerRow, PyUnicode_FromString("Time"));

		PyObject* iter = PyObject_GetIter(self->counterList);
		PyObject* item = NULL;

		if (iter == NULL)
		{
			PyErr_SetString(PyExc_ValueError, "No performance counters in collection.");
			return NULL;
		}

		while (item = PyIter_Next(iter))
		{
			PyList_Append(headerRow, item);
			Py_DECREF(item);
		}

		PyList_Append(result, headerRow);
	}

	EnterCriticalSection(&self->critsection);

	for (auto row = self->results.begin(); row != self->results.end(); row++)
	{
		PyObject* rowObj = PyList_New(row->size());
		Py_ssize_t idx = 0;
		for (auto col = row->begin(); col != row->end(); col++)
		{
			PyList_SetItem(rowObj, idx++, PyUnicode_FromString(col->c_str()));
		}
		PyList_Append(result, rowObj);
		Py_DECREF(rowObj);
	}
	self->results.clear();

	LeaveCriticalSection(&self->critsection);

	return result;
}
PyObject* performancecounter_setcallback(performancecounter_object* self, PyObject* args)
{
	PyObject* callback = NULL;
	int samples = 10;
	if (!PyArg_ParseTuple(args, "O|i", &callback, &samples))
		return NULL;

	if (callback == Py_None)
	{
		Py_XDECREF(self->callback);
		self->callback = NULL;
		Py_RETURN_NONE;
	}

	if (!PyCallable_Check(callback))
	{
		PyErr_SetString(PyExc_TypeError, "Callback must be callable.");
		return NULL;
	}

	Py_XDECREF(self->callback);
	self->callback = callback;
	Py_INCREF(self->callback);

	self->callbackBatchSize = samples;

	Py_RETURN_NONE;
}
PyObject* performancecounter_setlogfile(performancecounter_object* self, PyObject* arg)
{
	if (!PyUnicode_Check(arg) && arg != Py_None)
	{
		PyErr_SetString(PyExc_TypeError, "Expected a filename");
		return NULL;
	}

	if (self->logFile)
	{
		fclose(self->logFile);
		self->logFile = NULL;
	}

	if (arg == Py_None) Py_RETURN_NONE;

	wchar_t* filename = PyUnicode_AsWideCharString(arg, NULL);
	wchar_t* expandFn = NULL;
	DWORD cch = ExpandEnvironmentStringsW(filename, expandFn, 0);
	expandFn = new wchar_t[cch];
	cch = ExpandEnvironmentStringsW(filename, expandFn, cch);

	self->logFile = _wfopen(expandFn, L"a+");

	PyMem_FREE(filename);
	delete[] expandFn;

	if (self->logFile == NULL)
	{
		PyErr_SetFromErrnoWithFilenameObject(PyExc_IOError, arg);
		return NULL;
	}

	Py_RETURN_NONE;
}
PyObject* performancecounter_annotatelog(performancecounter_object* self, PyObject* arg)
{
	if (!PyUnicode_Check(arg))
	{
		PyErr_SetString(PyExc_TypeError, "Argument must be a string");
		return NULL;
	}
	if (!self->logFile)
	{
		PyErr_SetString(PyExc_FileNotFoundError, "No Log File Specified");
		return NULL;
	}

	Py_ssize_t len = 0;
	char* buf = PyUnicode_AsUTF8AndSize(arg, &len); // We aren't responsible for this buffer.

	EnterCriticalSection(&self->critsection);
	fwrite(buf, sizeof(char), len, self->logFile);
	fwrite("\r\n", 1, 2, self->logFile);
	fflush(self->logFile);
	LeaveCriticalSection(&self->critsection);

	Py_RETURN_NONE;
}
PyMethodDef performancecounter_methods[] =
{
	{ "add_static", (PyCFunction)performancecounter_addstatic, METH_KEYWORDS | METH_VARARGS, "Adds a counter to the current instance." },
	{ "add_multi", (PyCFunction)performancecounter_addmulti, METH_KEYWORDS | METH_VARARGS, "Adds multiple counters from a list" },
	{ "build_path", (PyCFunction)performancecounter_buildpath, METH_KEYWORDS | METH_VARARGS, "Builds a path given parameters." },
	{ "browse", (PyCFunction)performancecounter_browsecounters, METH_NOARGS, "Opens a browser for selecting counters." },
	{ "fetch", (PyCFunction)performancecounter_fetch, METH_VARARGS, "Gets the counter data for the specified counter." },
	{ "fetch_all", (PyCFunction)performancecounter_fetchall, METH_NOARGS, "Gets a list with all the counter data." },
	// Async Methods
	{ "start_monitoring", (PyCFunction)performancecounter_startmonitoring, METH_NOARGS, "Starts asynchronous monitoring." },
	{ "stop_monitoring", (PyCFunction)performancecounter_stopmonitoring, METH_NOARGS, "Stops asynchronous monitoring." },
	{ "set_interval", (PyCFunction)performancecounter_setinterval, METH_VARARGS, "Sets the performance logging interval." },
	{ "get_log", (PyCFunction)performancecounter_getlog, METH_VARARGS, "Gets the performance log." },
	{ "set_callback", (PyCFunction)performancecounter_setcallback, METH_VARARGS, "Sets the callback (None clears). Optional int argument specifies samples to collect between callbacks. (Default 10)" },
	{ "set_log_file", (PyCFunction)performancecounter_setlogfile, METH_O, "Sets the log file for CSV counter data to be saved. None disables this log." },
	{ "annotate_log", (PyCFunction)performancecounter_annotatelog, METH_O, "Adds a line to the log file. (Allows annotation of events)" },
	{ NULL, NULL, 0, NULL },
};

PyMemberDef performancecounter_members[] =
{
	{ "counters", T_OBJECT, offsetof(performancecounter_object, counterList), READONLY, "Paths of included counters." },
	{ NULL }
};

PyTypeObject performancecounter_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.performancecounter",				/* tp_name */
	sizeof(performancecounter_object),		/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)performancecounter_dealloc,	/* tp_dealloc */
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
	"Windows API Performance Counter",		/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	performancecounter_methods,				/* tp_methods */
	performancecounter_members,				/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)performancecounter_init,		/* tp_init */
	0,										/* tp_alloc */
	performancecounter_new,					/* tp_new */
};
