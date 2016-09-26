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
#include "PyBinaryObjectRegistry.hpp"
#include <structmember.h>


#include "BinaryObjectRegistry.hpp"




/*********************************
Defining a Python C-extension for the C++ class BinaryObjectRegistry,
from BinaryObjectRegistry.hpp

cribbed off https://docs.python.org/2.7/extending/newtypes.html
**********************************/



extern "C" {

static PyObject*
PyBinaryObjectRegistry_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
    {
    PyBinaryObjectRegistry* self;

    self = (PyBinaryObjectRegistry*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->nativeBinaryObjectRegistry = new BinaryObjectRegistry();
        }

    return (PyObject*) self;
    }


static void
PyBinaryObjectRegistry_dealloc(PyBinaryObjectRegistry* self)
    {
    delete self->nativeBinaryObjectRegistry;
    self->ob_type->tp_free((PyObject*)self);
    }


static int
PyBinaryObjectRegistry_init(PyBinaryObjectRegistry* self, PyObject* args, PyObject* kwds)
    {
    return 0;
    }


static PyObject*
PyBinaryObjectRegistry_str(PyBinaryObjectRegistry* self)
    {
    std::string s = self->nativeBinaryObjectRegistry->str();

    return PyString_FromStringAndSize(s.data(), s.size());
    }


static PyObject*
PyBinaryObjectRegistry_defineEndOfStream(PyBinaryObjectRegistry* self,
                                         PyObject* args)
    {
    self->nativeBinaryObjectRegistry->defineEndOfStream();

    Py_RETURN_NONE;
    }


static PyObject*
PyBinaryObjectRegistry_clear(PyBinaryObjectRegistry* self,
                             PyObject* args)
    {
    self->nativeBinaryObjectRegistry->clear();

    Py_RETURN_NONE;
    }

} // extern "C"


static PyMethodDef PyBinaryObjectRegistry_methods[] = {
    {"str",
     (PyCFunction)PyBinaryObjectRegistry_str,
     METH_NOARGS,
     "return the underlying string in the buffer"},
    {"defineEndOfStream",
     (PyCFunction)PyBinaryObjectRegistry_defineEndOfStream,
     METH_NOARGS,
     "define the end of the stream"},
    {"clear",
     (PyCFunction)PyBinaryObjectRegistry_clear,
     METH_NOARGS,
     "clear the underlying string"},
    {NULL} /* Sentinel */
    };


static PyTypeObject PyBinaryObjectRegistryType = {
    PyObject_HEAD_INIT(NULL)
    0,                                          /* ob_size */
    "binaryobjectregistry.BinaryObjectRegistry",/* tp_name */
    sizeof(PyBinaryObjectRegistry),               /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)PyBinaryObjectRegistry_dealloc, /* tp_dealloc */
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
    "BinaryObjectRegistry objects",             /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    PyBinaryObjectRegistry_methods,             /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)PyBinaryObjectRegistry_init,      /* tp_init */
    0,                                          /* tp_alloc */
    PyBinaryObjectRegistry_new,                 /* tp_new */
    };


static PyMethodDef module_methods[] = {
    {NULL}
    };


#ifndef PyMODINIT_FUNC/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

extern "C" {

PyMODINIT_FUNC
initbinaryobjectregistry(void)
    {
    PyObject* m;

    if (PyType_Ready(&PyBinaryObjectRegistryType) < 0)
        return;

    m = Py_InitModule3("binaryobjectregistry",
                      module_methods,
                      "expose BinaryObjectRegistry C++ class");

    if (m == NULL)
        return;

    Py_INCREF(&PyBinaryObjectRegistryType);
    PyModule_AddObject(
        m,
        "BinaryObjectRegistry",
        (PyObject*)&PyBinaryObjectRegistryType);
    }

} // extern "C"
