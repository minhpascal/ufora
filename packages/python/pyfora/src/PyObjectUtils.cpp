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
#include "PyObjectUtils.hpp"

#include <stdexcept>


std::string PyObjectUtils::repr_string(PyObject* obj)
    {
    PyObject* obj_repr = PyObject_Repr(obj);
    if (obj_repr == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't compute repr of an object");
        }
    if (not PyString_Check(obj_repr)) {
        throw std::logic_error("repr returned a non string");
        }

    std::string tr = std::string(
        PyString_AS_STRING(obj_repr),
        PyString_GET_SIZE(obj_repr)
        );

    Py_DECREF(obj_repr);

    return tr;
    }


std::string PyObjectUtils::str_string(PyObject* obj)
    {
    PyObject* obj_str = PyObject_Str(obj);
    if (obj_str == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't compute repr of an object");
        }
    if (not PyString_Check(obj_str)) {
        throw std::logic_error("repr returned a non string");
        }

    std::string tr = std::string(
        PyString_AS_STRING(obj_str),
        PyString_GET_SIZE(obj_str)
        );

    Py_DECREF(obj_str);

    return tr;
    }


std::string PyObjectUtils::std_string(PyObject* string)
    {
    char* str = PyString_AS_STRING(string);
    if (str == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't create a C-string from a PyString");
        }

    Py_ssize_t length = PyString_GET_SIZE(string);

    return std::string(str, length);
    }


std::string PyObjectUtils::format_exc()
    {
    PyObject* traceback_module = PyImport_ImportModule("traceback");
    if (traceback_module == NULL) {
        PyErr_Print();
        throw std::logic_error("error importing traceback module");
        }

    PyObject* format_exc_fun = PyObject_GetAttrString(
        traceback_module,
        "format_exc");
    if (format_exc_fun == NULL) {
        PyErr_Print();
        throw std::logic_error("error getting traceback.format_exc function");
        }

    PyObject* tb_str = PyObject_CallFunctionObjArgs(format_exc_fun, NULL);
    if (tb_str == NULL) {
        PyErr_Print();
        throw std::logic_error("error calling traceback.format_exc");
        }

    if (not PyString_Check(tb_str)) {
        throw std::logic_error("expected a string");
        }

    std::string tr = PyObjectUtils::std_string(tb_str);

    Py_DECREF(tb_str);
    Py_DECREF(format_exc_fun);
    Py_DECREF(traceback_module);

    return tr;
    }


long PyObjectUtils::builtin_id(PyObject* pyObject)
    {
    PyObject* pyObject_builtin_id = PyLong_FromVoidPtr(pyObject);
    if (pyObject_builtin_id == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't get a builtin id");
        }

    int overflow;
    long tr = PyLong_AsLongAndOverflow(pyObject_builtin_id, &overflow);

    Py_DECREF(pyObject_builtin_id);

    if (overflow != 0) {
        throw std::logic_error("overflow in converting a python long to a C long");
        }

    return tr;
    }


bool PyObjectUtils::in(PyObject* container, PyObject* value)
    {
    if (PySet_Check(container)) {
        return PySet_Contains(container, value);
        }
    else if (PyDict_Check(container)) {
        return PyDict_Contains(container, value);
        }
    else if (PyList_Check(container)) {
        return _in_list(container, value);
        }
    else {
        throw std::logic_error("we haven't implemented all alternatives here. "
                               "should just call back into python."
            );
        }
    }


bool PyObjectUtils::_in_list(PyObject* pyList, PyObject* value)
    {
    for (Py_ssize_t ix = 0; ix < PyList_GET_SIZE(pyList); ++ix) {
        PyObject* item = PyList_GET_ITEM(pyList, ix);
        if (PyObject_Compare(value, item)) {
            return true;
            }
        }

    return false;
    }
