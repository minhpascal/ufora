/***************************************************************************
   Copyright 2016 Ufora Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
****************************************************************************/
#include <Python.h>
#include <structmember.h>

#include <stdexcept>

#include "PyObjectWalker.hpp"
#include "PyBinaryObjectRegistry.hpp"

/*********************************
Defining a Python C-extension for the C++ class PyObjectWalker,
from PyObjectWalker.hpp

cribbed off https://docs.python.org/2.7/extending/newtypes.html
**********************************/

typedef struct {
    PyObject_HEAD
    PyObject* purePythonClassMapping;
    PyObject* binaryObjectRegistry;
    PyObject* excludePredicateFun;
    PyObject* excludeList;
    PyObject* terminalValueFilter;
    PyObjectWalker* nativePyObjectWalker;
} PyObjectWalkerStruct;


extern "C" {

static PyObject*
PyObjectWalkerStruct_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
    {
    PyObjectWalkerStruct* self;

    self = (PyObjectWalkerStruct*)type->tp_alloc(type, 0);
    self->purePythonClassMapping = 0;
    self->binaryObjectRegistry = 0;
    self->excludePredicateFun = 0;
    self->excludeList = 0;
    self->terminalValueFilter = 0;
    self->nativePyObjectWalker = 0;

    return (PyObject*) self;
    }


static void
PyObjectWalkerStruct_dealloc(PyObjectWalkerStruct* self)
    {
    Py_XDECREF(self->terminalValueFilter);
    Py_XDECREF(self->excludeList);
    Py_XDECREF(self->excludePredicateFun);
    Py_XDECREF(self->binaryObjectRegistry);
    Py_XDECREF(self->purePythonClassMapping);
    delete self->nativePyObjectWalker;
    self->ob_type->tp_free((PyObject*)self);
    }


static int
PyObjectWalkerStruct_init(PyObjectWalkerStruct* self, PyObject* args, PyObject* kwds)
    {
    // check that arg[0] is a BinaryObjectRegistry

    PyObject* binaryObjectRegistryModule = 
        PyImport_ImportModule("pyfora.binaryobjectregistry");

    if (binaryObjectRegistryModule == NULL) {
        throw std::logic_error("couldn't import pyfora.binaryobjectregistry");
        }

    PyObject* binaryObjectRegistryClass = 
        PyObject_GetAttrString(binaryObjectRegistryModule,
                               "BinaryObjectRegistry");
    
    if (binaryObjectRegistryClass == NULL) {
        throw std::logic_error(
            "couldn't find pyfora.binaryobjectregistry.BinaryObjectRegistry"
            );
        }

    if (!PyArg_ParseTuple(args, "OO!OOO",
                          &self->purePythonClassMapping,
                          binaryObjectRegistryClass,
                          &self->binaryObjectRegistry,
                          &self->excludePredicateFun,
                          &self->excludeList,
                          &self->terminalValueFilter)) {
        return -1;
        }
    
    Py_INCREF(self->purePythonClassMapping);
    Py_INCREF(self->binaryObjectRegistry);
    Py_INCREF(self->excludePredicateFun);
    Py_INCREF(self->excludeList);
    Py_INCREF(self->terminalValueFilter);

    self->nativePyObjectWalker = new PyObjectWalker(
        self->purePythonClassMapping,
        *(((PyBinaryObjectRegistry*)(self->binaryObjectRegistry))->nativeBinaryObjectRegistry),
        self->excludePredicateFun,
        self->excludeList,
        self->terminalValueFilter
        );

    return 0;
    }


static PyObject*
PyObjectWalkerStruct_walkPyObject(PyObjectWalkerStruct* self, PyObject* args)
    {
    PyObject* objToWalk = 0;
    if (!PyArg_ParseTuple(args, "O", &objToWalk)) {
        return NULL;
        }
    
    return PyInt_FromLong(self->nativePyObjectWalker->walkPyObject(objToWalk));
    }


} // extern "C"

static PyMethodDef PyObjectWalkerStruct_methods[] = {
    {"walkPyObject", (PyCFunction)PyObjectWalkerStruct_walkPyObject, METH_VARARGS,
     "walk a python object"},
    {NULL}
    };


static PyMemberDef PyObjectWalkerStruct_members[] = {
    {"objectRegistry", T_OBJECT_EX,
     offsetof(PyObjectWalkerStruct, binaryObjectRegistry), 0,
     "object registry attribute"},
    {NULL}
    };


static PyTypeObject PyObjectWalkerStructType = {
    PyObject_HEAD_INIT(NULL)
    0,                                          /* ob_size */
    "pyobjectwalker.PyObjectWalker",            /* tp_name */
    sizeof(PyObjectWalkerStruct),               /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)PyObjectWalkerStruct_dealloc,   /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_compare */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "PyObjectWalker objects",                   /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    PyObjectWalkerStruct_methods,               /* tp_methods */
    PyObjectWalkerStruct_members,               /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)PyObjectWalkerStruct_init,        /* tp_init */
    0,                                          /* tp_alloc */
    PyObjectWalkerStruct_new,                   /* tp_new */
    };


static PyMethodDef module_methods[] = {
    {NULL}
    };


#ifndef PyMODINIT_FUNC/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif


extern "C" {

PyMODINIT_FUNC
initpyobjectwalker(void)
    {
    PyObject* m;

    if (PyType_Ready(&PyObjectWalkerStructType) < 0)
        return;

    m = Py_InitModule3("pyobjectwalker",
                      module_methods,
                      "expose PyObjectWalker C++ class");

    if (m == NULL)
        return;

    Py_INCREF(&PyObjectWalkerStructType);
    PyModule_AddObject(
        m,
        "PyObjectWalker",
        (PyObject*)&PyObjectWalkerStructType);
    }

} // extern "C"
