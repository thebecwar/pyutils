#include "Stdafx.h"
#include "ExeInfo.h"

#include <Python.h>
#include <structmember.h>
#include <Windows.h>
#include <ImageHlp.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

using std::wstring;
using std::wstringstream;
using std::vector;
using std::setw;
using std::setfill;

typedef struct {
	PyObject_HEAD;

	// GetFileVersionInfo members
	PyObject* comments;
	PyObject* internalName;
	PyObject* productName;
	PyObject* companyName;
	PyObject* legalCopyright;
	PyObject* productVersion;
	PyObject* fileDescription;
	PyObject* legalTrademarks;
	PyObject* privateBuild;
	PyObject* fileVersion;
	PyObject* originalFilename;
	PyObject* specialBuild;

	// IMAGE_FILE_HEADER information
	PyObject* moduleName;
	PyObject* machine;
	PyObject* timestamp;

} exefile_object;

static void exefile_gettranslations(void* infoBlock, vector<wstring>& translations)
{
	struct LANGANDCODEPAGE
	{
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;

	UINT cbTranslate;
	VerQueryValueW(infoBlock, L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate);

	wchar_t buffer[10];
	int count = cbTranslate / sizeof(struct LANGANDCODEPAGE);
	for (int i = 0; i < count; i++)
	{
		_swprintf(buffer, L"%04x%04x", lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
		translations.emplace_back(wstring(buffer));
	}
}

static PyObject* exefile_fetch(void* infoBlock, const wchar_t* translation, const wchar_t* block)
{

	wstring infoPath = L"\\StringFileInfo\\";
	infoPath += translation;
	infoPath += L"\\";
	infoPath += block;

	wchar_t* str;
	UINT len;
	BOOL ok = VerQueryValueW(infoBlock, infoPath.c_str(), (LPVOID*)&str, &len);
	if (ok)
	{
		return PyUnicode_FromWideChar(str, -1);
	}
	else
	{
		return PyUnicode_FromString("");
	}
}

static void exefile_dealloc(exefile_object* self)
{
	Py_XDECREF(self->comments);
	Py_XDECREF(self->internalName);
	Py_XDECREF(self->productName);
	Py_XDECREF(self->companyName);
	Py_XDECREF(self->legalCopyright);
	Py_XDECREF(self->productVersion);
	Py_XDECREF(self->fileDescription);
	Py_XDECREF(self->legalTrademarks);
	Py_XDECREF(self->privateBuild);
	Py_XDECREF(self->fileVersion);
	Py_XDECREF(self->originalFilename);
	Py_XDECREF(self->specialBuild);

	Py_XDECREF(self->moduleName);
	Py_XDECREF(self->machine);
	Py_XDECREF(self->timestamp);

	Py_TYPE(self)->tp_free((PyObject*)self);
}
static PyObject* exefile_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyObject* path = NULL;
	if (!PyArg_ParseTuple(args, "O", &path))
		return NULL;


	exefile_object* self = (exefile_object*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		wchar_t* szPathUni = PyUnicode_AsWideCharString(path, NULL);
		char* szPath = PyUnicode_AsUTF8AndSize(path, NULL); // DONT FREE! THIS IS A COPY

															// GetFileVersionInfo
		DWORD dwHandle = 0;
		DWORD size = GetFileVersionInfoSizeW(szPathUni, &dwHandle);
		if (size == 0)
		{
			PyErr_SetString(PyExc_WindowsError, "Unable to obtain buffer size for GetFileVersionInfo");
			PyMem_FREE(szPathUni);
			Py_DECREF(self);
			return NULL;
		}

		void* buffer = calloc(size, 1);
		BOOL ok = GetFileVersionInfoW(szPathUni, 0, size, buffer);
		if (!ok)
		{
			PyErr_SetExcFromWindowsErr(PyExc_WindowsError, GetLastError());
			PyMem_FREE(szPathUni);
			Py_DECREF(self);
			return NULL;
		}

		vector<wstring> translations;
		exefile_gettranslations(buffer, translations);

		if (translations.size() > 1)
		{
			self->comments = PyDict_New();
			self->internalName = PyDict_New();
			self->productName = PyDict_New();
			self->companyName = PyDict_New();
			self->legalCopyright = PyDict_New();
			self->productVersion = PyDict_New();
			self->fileDescription = PyDict_New();
			self->legalTrademarks = PyDict_New();
			self->privateBuild = PyDict_New();
			self->fileVersion = PyDict_New();
			self->originalFilename = PyDict_New();
			self->specialBuild = PyDict_New();

			for (auto iter = translations.begin(); iter != translations.end(); iter++)
			{
				const wchar_t* tx = iter->c_str();
				PyObject* key = PyUnicode_FromWideChar(tx, -1);
				PyDict_SetItem(self->comments, key, exefile_fetch(buffer, tx, L"Comments"));
				PyDict_SetItem(self->internalName, key, exefile_fetch(buffer, tx, L"InternalName"));
				PyDict_SetItem(self->productName, key, exefile_fetch(buffer, tx, L"ProductName"));
				PyDict_SetItem(self->companyName, key, exefile_fetch(buffer, tx, L"CompanyName"));
				PyDict_SetItem(self->legalCopyright, key, exefile_fetch(buffer, tx, L"LegalCopyright"));
				PyDict_SetItem(self->productVersion, key, exefile_fetch(buffer, tx, L"ProductVersion"));
				PyDict_SetItem(self->fileDescription, key, exefile_fetch(buffer, tx, L"FileDescription"));
				PyDict_SetItem(self->legalTrademarks, key, exefile_fetch(buffer, tx, L"LegalTrademarks"));
				PyDict_SetItem(self->privateBuild, key, exefile_fetch(buffer, tx, L"PrivateBuild"));
				PyDict_SetItem(self->fileVersion, key, exefile_fetch(buffer, tx, L"FileVersion"));
				PyDict_SetItem(self->originalFilename, key, exefile_fetch(buffer, tx, L"OriginalFilename"));
				PyDict_SetItem(self->specialBuild, key, exefile_fetch(buffer, tx, L"SpecialBuild"));
				Py_DECREF(key);
			}
		}
		else if (translations.size() == 1)
		{
			const wchar_t* tx = translations.at(0).c_str();
			self->comments = exefile_fetch(buffer, tx, L"Comments");
			self->internalName = exefile_fetch(buffer, tx, L"InternalName");
			self->productName = exefile_fetch(buffer, tx, L"ProductName");
			self->companyName = exefile_fetch(buffer, tx, L"CompanyName");
			self->legalCopyright = exefile_fetch(buffer, tx, L"LegalCopyright");
			self->productVersion = exefile_fetch(buffer, tx, L"ProductVersion");
			self->fileDescription = exefile_fetch(buffer, tx, L"FileDescription");
			self->legalTrademarks = exefile_fetch(buffer, tx, L"LegalTrademarks");
			self->privateBuild = exefile_fetch(buffer, tx, L"PrivateBuild");
			self->fileVersion = exefile_fetch(buffer, tx, L"FileVersion");
			self->originalFilename = exefile_fetch(buffer, tx, L"OriginalFilename");
			self->specialBuild = exefile_fetch(buffer, tx, L"SpecialBuild");
		}
		else
		{
			self->comments = Py_None; Py_INCREF(Py_None);
			self->internalName = Py_None; Py_INCREF(Py_None);
			self->productName = Py_None; Py_INCREF(Py_None);
			self->companyName = Py_None; Py_INCREF(Py_None);
			self->legalCopyright = Py_None; Py_INCREF(Py_None);
			self->productVersion = Py_None; Py_INCREF(Py_None);
			self->fileDescription = Py_None; Py_INCREF(Py_None);
			self->legalTrademarks = Py_None; Py_INCREF(Py_None);
			self->privateBuild = Py_None; Py_INCREF(Py_None);
			self->fileVersion = Py_None; Py_INCREF(Py_None);
			self->originalFilename = Py_None; Py_INCREF(Py_None);
			self->specialBuild = Py_None; Py_INCREF(Py_None);
		}

		PLOADED_IMAGE img = ImageLoad(szPath, NULL);
		if (img != NULL)
		{
			self->moduleName = PyUnicode_FromString(img->ModuleName);

			switch (img->FileHeader->FileHeader.Machine)
			{
			case IMAGE_FILE_MACHINE_I386:
				self->machine = PyUnicode_FromString("x86");
				break;
			case IMAGE_FILE_MACHINE_IA64:
				self->machine = PyUnicode_FromString("Itanium");
				break;
			case IMAGE_FILE_MACHINE_AMD64:
				self->machine = PyUnicode_FromString("x64");
				break;
			default:
				self->machine = Py_None;
				Py_INCREF(Py_None);
				break;
			}

			SYSTEMTIME epoch = { 1970, 1, 1, 1,  0, 0, 0, 0 };
			FILETIME epochFt = { 0 };
			SystemTimeToFileTime(&epoch, &epochFt);
			ULARGE_INTEGER u = { 0 };
			u.HighPart = epochFt.dwHighDateTime;
			u.LowPart = epochFt.dwLowDateTime;
			const double secondsPer100ns = 100.0 * 1e-9;
			u.QuadPart += (ULONGLONG)((img->FileHeader->FileHeader.TimeDateStamp) / secondsPer100ns);
			epochFt.dwHighDateTime = u.HighPart;
			epochFt.dwLowDateTime = u.LowPart;
			FileTimeToSystemTime(&epochFt, &epoch);

			std::stringstream time;
			time << epoch.wYear << "-" << setw(2) << setfill('0') << epoch.wMonth << "-"
				<< setw(2) << setfill('0') << epoch.wDay << " " << setw(2) << setfill('0') << epoch.wHour << ":"
				<< setw(2) << setfill('0') << epoch.wMinute << ":" << setw(2) << setfill('0') << epoch.wSecond;
			self->timestamp = PyUnicode_FromString(time.str().c_str());

			ImageUnload(img);
		}

		// PSAPI

		PyMem_FREE(szPathUni);
	}
	return (PyObject*)self;
}

static int exefile_init(exefile_object* self, PyObject* args, PyObject* kwargs)
{
	return 0;
}


static PyMethodDef exefile_methods[] =
{
	{ NULL, NULL, 0, NULL },
};

static PyMemberDef exefile_members[] =
{
	// GetFileVersionInfo members
	{ "comments", T_OBJECT, offsetof(exefile_object, comments), READONLY, "" },
	{ "internal_name", T_OBJECT, offsetof(exefile_object, internalName), READONLY, "" },
	{ "product_name", T_OBJECT, offsetof(exefile_object, productName), READONLY, "" },
	{ "company_name", T_OBJECT, offsetof(exefile_object, companyName), READONLY, "" },
	{ "legal_copyright", T_OBJECT, offsetof(exefile_object, legalCopyright), READONLY, "" },
	{ "product_version", T_OBJECT, offsetof(exefile_object, productVersion), READONLY, "" },
	{ "file_description", T_OBJECT, offsetof(exefile_object, fileDescription), READONLY, "" },
	{ "legal_trademarks", T_OBJECT, offsetof(exefile_object, legalTrademarks), READONLY, "" },
	{ "private_build", T_OBJECT, offsetof(exefile_object, privateBuild), READONLY, "" },
	{ "file_version", T_OBJECT, offsetof(exefile_object, fileVersion), READONLY, "" },
	{ "original_filename", T_OBJECT, offsetof(exefile_object, originalFilename), READONLY, "" },
	{ "special_build", T_OBJECT, offsetof(exefile_object, specialBuild), READONLY, "" },

	// IMAGE_FILE_HEADER information
	{ "module_name", T_OBJECT, offsetof(exefile_object, moduleName), READONLY, "" },
	{ "machine", T_OBJECT, offsetof(exefile_object, machine), READONLY, "" },
	{ "timestamp", T_OBJECT, offsetof(exefile_object, timestamp), READONLY, "" },

	{ NULL }
};

PyTypeObject exeinfo_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyutils.ExeFile",						/* tp_name */
	sizeof(exefile_object),					/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)exefile_dealloc,			/* tp_dealloc */
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
	"Executable File Information",			/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	exefile_methods,						/* tp_methods */
	exefile_members,						/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)exefile_init,					/* tp_init */
	0,										/* tp_alloc */
	exefile_new,							/* tp_new */
};