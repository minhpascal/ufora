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
#include "PyAstUtil.hpp"
#include "PyObjectUtils.hpp"

#include <iostream>
#include <stdexcept>


PyAstUtil::PyAstUtil()
    : mPyAstUtilModule(NULL),
      mGetSourceFilenameAndTextFun(NULL),
      mGetSourceLinesFun(NULL),
      mPyAstFromTextFun(NULL),
      mFunctionDefOrLambdaAtLineNumberFun(NULL),
      mClassDefAtLineNumberFun(NULL)
    {
    _initPyAstUtilModule();
    _initGetSourceFilenameAndTextFun();
    _initGetSourceLinesFun();
    _initPyAstFromTextFun();
    _initFunctionDefOrLambdaAtLineNumberFun();
    _initClassDefAtLineNumberFun();
    }


void PyAstUtil::_initClassDefAtLineNumberFun()
    {
    mClassDefAtLineNumberFun =
        PyObject_GetAttrString(mPyAstUtilModule,
                               "classDefAtLineNumber");

    if (mClassDefAtLineNumberFun == NULL) {
        PyErr_Print();
        throw std::logic_error(
            "couldn't get `classDefAtLineNumber` member"
            " of PyAstUtilModule");
        }
    }


void PyAstUtil::_initFunctionDefOrLambdaAtLineNumberFun()
    {
    mFunctionDefOrLambdaAtLineNumberFun =
        PyObject_GetAttrString(mPyAstUtilModule,
                               "functionDefOrLambdaAtLineNumber");

    if (mFunctionDefOrLambdaAtLineNumberFun == NULL) {
        PyErr_Print();
        throw std::logic_error(
            "couldn't get `functionDefOrLambdaAtLineNumber` member"
            " of PyAstUtilModule");
        }
    }


void PyAstUtil::_initPyAstFromTextFun()
    {
    mPyAstFromTextFun =
        PyObject_GetAttrString(mPyAstUtilModule, "pyAstFromText");
    if (mPyAstFromTextFun == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't get `pyAstFromText` member"
                               " of PyAstUtilModule");
        }    
    }


void PyAstUtil::_initGetSourceLinesFun()
    {
    mGetSourceLinesFun =
        PyObject_GetAttrString(mPyAstUtilModule, "getSourceLines");
    if (mGetSourceLinesFun == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't get `getSourceLines` member"
                               " of PyAstUtilModule");
        }
    }


void PyAstUtil::_initGetSourceFilenameAndTextFun()
    {
    mGetSourceFilenameAndTextFun =
        PyObject_GetAttrString(mPyAstUtilModule, "getSourceFilenameAndText");
    if (mGetSourceFilenameAndTextFun == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't get `getSourceFilenameAndText` member"
                               " of PyAstUtilModule");
        }
    }


void PyAstUtil::_initPyAstUtilModule()
    {
    PyObject* pyforaModule = PyImport_ImportModule("pyfora");
    if (pyforaModule == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't import pyfora module");
        }

    PyObject* pyAstModule = PyObject_GetAttrString(pyforaModule, "pyAst");
    if (pyAstModule == NULL) {
        PyErr_Print();
        Py_DECREF(pyforaModule);
        throw std::logic_error("couldn't find pyAst member on pyfora");
        }

    mPyAstUtilModule = PyObject_GetAttrString(pyAstModule, "PyAstUtil");
    Py_DECREF(pyAstModule);
    Py_DECREF(pyforaModule);
    if (mPyAstUtilModule == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't import PyAstUtil module");
        }
    }


std::pair<std::string, std::string>
PyAstUtil::sourceFilenameAndText(PyObject* pyObject)
    {
    PyObject* res = PyObject_CallFunctionObjArgs(
        _getInstance().mGetSourceFilenameAndTextFun,
        pyObject,
        NULL
        );
    if (res == NULL) {
        PyErr_Print();
        throw std::logic_error(
            "an error occured calling getSourceFilenameAndText"
            );
        }
    if (not PyTuple_Check(res)) {
        throw std::logic_error(
            "expected a tuple in calling getSourceFilenameAndText"
            );
        }
    if (PyTuple_GET_SIZE(res) != 2) {
        throw std::logic_error(
            "we expected getSourceFilenameAndText to return a tuple of length two"
            );
        }

    // both of these are borrowed references, so don't need to be decrefed
    PyObject* sourceFileText = PyTuple_GET_ITEM(res, 0);
    PyObject* sourceFileName = PyTuple_GET_ITEM(res, 1);

    if (not PyString_Check(sourceFileName) or not PyString_Check(sourceFileText)) {
        throw std::logic_error(
            "expected getSourceFilenameAndText to return two strings"
            );
        }

    std::pair<std::string, std::string> tr = std::make_pair(
        PyObjectUtils::std_string(sourceFileName),
        PyObjectUtils::std_string(sourceFileText)
        );

    Py_DECREF(res);

    return tr;
    }


long PyAstUtil::startingSourceLine(PyObject* pyObject)
    {
    PyObject* res = PyObject_CallFunctionObjArgs(
        _getInstance().mGetSourceLinesFun,
        pyObject,
        NULL
        );
    if (res == NULL) {
        PyErr_Print();
        throw std::logic_error(
            "an error occured calling getSourceLines"
            );
        }

    if (not PyTuple_Check(res)) {
        throw std::logic_error(
            "expected a tuple in calling getSourceLines"
            );
        }
    if (PyTuple_GET_SIZE(res) != 2) {
        throw std::logic_error(
            "we expected getSourceLines to return a tuple of length two"
            );
        }

    // borrowed reference -- don't need to decref
    PyObject* startingSourceLine = PyTuple_GET_ITEM(res, 1);
    if (startingSourceLine == NULL) {
        PyErr_Print();
        throw std::logic_error("hit an error calling PyforaInspect.getSourceLines");
        }

    if (not PyInt_Check(startingSourceLine)) {
        throw std::logic_error(
            "expected PyforaInspect.getSourceLines to return an int");
        }

    long tr = PyInt_AS_LONG(startingSourceLine);

    Py_DECREF(res);

    return tr;
    }


PyObject* PyAstUtil::pyAstFromText(const std::string& fileText)
    {
    PyObject* pyString = PyString_FromStringAndSize(fileText.data(),
                                                    fileText.size());
    if (pyString == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't create a PyString out of a C++ string");
        }

    PyObject* res = PyObject_CallFunctionObjArgs(
        _getInstance().mPyAstFromTextFun,
        pyString,
        NULL
        );

    Py_DECREF(pyString);

    return res;
    }


PyObject*
PyAstUtil::functionDefOrLambdaAtLineNumber(PyObject* pyObject,
                                           long sourceLine)
    {
    PyObject* pySourceLine = PyInt_FromLong(sourceLine);
    if (pySourceLine == NULL) {
        PyErr_Print();
        throw std::logic_error("error creating a python int out of a C++ long");
        }

    PyObject* tr = PyObject_CallFunctionObjArgs(
        _getInstance().mFunctionDefOrLambdaAtLineNumberFun,
        pyObject,
        pySourceLine,
        NULL
        );

    if (tr == NULL) {
        PyErr_Print();
        throw std::logic_error("error calling functionDefOrLambdaAtLineNumber");
        }

    Py_DECREF(pySourceLine);

    return tr;
    }


PyObject* 
PyAstUtil::classDefAtLineNumber(PyObject* pyObject,
                                long sourceLine)
    {
    PyObject* pySourceLine = PyInt_FromLong(sourceLine);
    if (pySourceLine == NULL) {
        PyErr_Print();
        throw std::logic_error("error creating a python int out of a C++ long");
        }

    PyObject* tr = PyObject_CallFunctionObjArgs(
        _getInstance().mClassDefAtLineNumberFun,
        pyObject,
        pySourceLine,
        NULL
        );

    if (tr == NULL) {
        PyErr_Print();
        throw std::logic_error("error calling classDefAtLineNumber");
        }

    Py_DECREF(pySourceLine);

    return tr;
    }


PyObject* PyAstUtil::collectDataMembersSetInInit(PyObject* pyObject)
    {
    PyObject* collectDataMembersSetInInitFun = PyObject_GetAttrString(
        _getInstance().mPyAstUtilModule,
        "collectDataMembersSetInInit");
    if (collectDataMembersSetInInitFun == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't get collectDataMembersSetInInit function");
        }

    PyObject* res = PyObject_CallFunctionObjArgs(
        collectDataMembersSetInInitFun,
        pyObject);

    Py_DECREF(collectDataMembersSetInInitFun);

    return res;
    }
