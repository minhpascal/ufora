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

#include "BinaryObjectRegistry.hpp"
#include "FreeVariableMemberAccessChain.hpp"
#include "PyObjectUtils.hpp"

#include <iostream>
#include <stdexcept>


BinaryObjectRegistry::BinaryObjectRegistry() : mNextObjectId(0) 
    {
    }
    

int64_t BinaryObjectRegistry::allocateObject() {
    int64_t objectId = mNextObjectId;
    mNextObjectId++;
    return objectId;
    }


void BinaryObjectRegistry::_writePrimitive(bool b) {
    mStringBuilder.addByte(CODE_BOOL);
    mStringBuilder.addByte(b ? 1 : 0);
    }


void BinaryObjectRegistry::_writePrimitive(int32_t i) {
    mStringBuilder.addByte(CODE_INT);
    mStringBuilder.addInt64(i);
    }


void BinaryObjectRegistry::_writePrimitive(int64_t i) {
    mStringBuilder.addByte(CODE_INT);
    mStringBuilder.addInt64(i);
    }


void BinaryObjectRegistry::_writePrimitive(double d) {
    mStringBuilder.addByte(CODE_FLOAT);
    mStringBuilder.addFloat64(d);
    }


void BinaryObjectRegistry::_writePrimitive(const std::string& s) {
    mStringBuilder.addByte(CODE_STR);
    mStringBuilder.addString(s);
    }


void BinaryObjectRegistry::_writePrimitive(PyObject* obj) {
    if (obj == Py_None) {
        mStringBuilder.addByte(CODE_NONE);
        }
    else if (PyBool_Check(obj))
        {
        if (PyObject_IsTrue(obj)) {
            _writePrimitive(true);
            }
        else {
            _writePrimitive(false);
            }
        }
    else if (PyInt_Check(obj)) {
        _writePrimitive(PyInt_AS_LONG(obj));
        }
    else if (PyFloat_Check(obj)) {
        _writePrimitive(PyFloat_AS_DOUBLE(obj));
        }
    else if (PyString_Check(obj)) {
        char* s = PyString_AS_STRING(obj);
        Py_ssize_t length = PyString_GET_SIZE(obj);
        
        _writePrimitive(std::string(s, length));
        }
    else if (PyList_Check(obj)) {
        Py_ssize_t length = PyList_GET_SIZE(obj);

        mStringBuilder.addByte(CODE_LIST_OF_PRIMITIVES);
        mStringBuilder.addInt64(length);
        
        for (Py_ssize_t ix = 0; ix < length; ++ix) {
            _writePrimitive(PyList_GET_ITEM(obj, ix));
            }
        }
    else {
        throw std::logic_error(
            "got an invalid type in _writePrimitive: " + 
            PyObjectUtils::repr_string(obj));
        }
    }


void BinaryObjectRegistry::defineEndOfStream() {
    mStringBuilder.addInt64(-1);
    }


void BinaryObjectRegistry::defineTuple(int64_t objectId,
                                       const std::vector<int64_t>& memberIds)
    {
    defineTuple(objectId,
                &memberIds[0],
                memberIds.size());
    }

    
void BinaryObjectRegistry::defineTuple(int64_t objectId,
                                       const int64_t* memberIds,
                                       uint64_t nMemberIds)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_TUPLE);
    mStringBuilder.addInt64s(memberIds, nMemberIds);
    }


void BinaryObjectRegistry::defineList(int64_t objectId,
                                      const std::vector<int64_t>& memberIds)
    {
    defineList(objectId,
               &memberIds[0],
               memberIds.size());
    }

    
void BinaryObjectRegistry::defineList(int64_t objectId,
                                      const int64_t* memberIds,
                                      uint64_t nMemberIds)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_LIST);
    mStringBuilder.addInt64s(memberIds, nMemberIds);
    }


void BinaryObjectRegistry::defineFile(int64_t objectId,
                                      const std::string& text,
                                      const std::string& path)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_FILE);
    mStringBuilder.addString(path);
    mStringBuilder.addString(text);
    }


void BinaryObjectRegistry::defineDict(int64_t objectId,
                                      const std::vector<int64_t>& keyIds,
                                      const std::vector<int64_t>& valueIds)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_DICT);
    mStringBuilder.addInt64s(keyIds);
    mStringBuilder.addInt64s(valueIds);
    }


void BinaryObjectRegistry::defineBuiltinExceptionInstance(
        int64_t objectId,
        const std::string& typeName,
        int64_t argsId)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_BUILTIN_EXCEPTION_INSTANCE);
    mStringBuilder.addString(typeName);
    mStringBuilder.addInt64(argsId);
    }


void BinaryObjectRegistry::defineNamedSingleton(
        int64_t objectId,
        const std::string& singletonName)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_NAMED_SINGLETON);
    mStringBuilder.addString(singletonName);
    }


void BinaryObjectRegistry::defineUnconvertible(
        int64_t objectId,
        PyObject* modulePathOrNone)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_UNCONVERTIBLE);

    if (modulePathOrNone == Py_None) {
        mStringBuilder.addByte(0);
        }
    else {
        if (not PyString_Check(modulePathOrNone)) {
            throw std::logic_error(
                "modulePathOrNone should be None or a string"
                );
            }

        char* str = PyString_AS_STRING(modulePathOrNone);
        Py_ssize_t length = PyString_GET_SIZE(modulePathOrNone);

        mStringBuilder.addByte(1);
        mStringBuilder.addStringTuple(std::string(str, length));
        mUnconvertibleIndices.insert(objectId);
        }
    }


void BinaryObjectRegistry::defineClassInstance(
        int64_t objectId,
        int64_t classId,
        const std::map<std::string, int64_t>& classMemberNameToClassMemberId)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_CLASS_INSTANCE);
    mStringBuilder.addInt64(classId);
    mStringBuilder.addInt32(classMemberNameToClassMemberId.size());
    for (std::map<std::string, int64_t>::const_iterator it = 
             classMemberNameToClassMemberId.begin();
         it != classMemberNameToClassMemberId.end();
         ++it)
        {
        mStringBuilder.addString(it->first);
        mStringBuilder.addInt64(it->second);
        }
    }


void BinaryObjectRegistry::defineInstanceMethod(int64_t objectId,
                                                int64_t instanceId,
                                                const std::string& methodName)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_INSTANCE_METHOD);
    mStringBuilder.addInt64(instanceId);
    mStringBuilder.addString(methodName);
    }


void BinaryObjectRegistry::definePyAbortException(int64_t objectId,
                                                  const std::string& typeName,
                                                  int64_t argsId)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_PY_ABORT_EXCEPTION);
    mStringBuilder.addString(typeName);
    mStringBuilder.addInt64(argsId);
    }


void BinaryObjectRegistry::defineFunction(
        int64_t objectId,
        int64_t sourceFileId,
        int64_t lineNumber,
        const std::map<FreeVariableMemberAccessChain, int64_t>& chainToId
        )
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_FUNCTION);
    mStringBuilder.addInt64(sourceFileId);
    mStringBuilder.addInt32(lineNumber);
    _writeFreeVariableResolutions(chainToId);
    }


void BinaryObjectRegistry::defineClass(
        int64_t objectId,
        int64_t sourceFileId,
        int64_t lineNumber,
        const std::map<FreeVariableMemberAccessChain, int64_t>& chainToId,
        const std::vector<int64_t> baseClassIds)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_CLASS);
    mStringBuilder.addInt64(sourceFileId);
    mStringBuilder.addInt32(lineNumber);
    _writeFreeVariableResolutions(chainToId);
    mStringBuilder.addInt64s(baseClassIds);
    }


void BinaryObjectRegistry::_writeFreeVariableResolutions(
        const std::map<FreeVariableMemberAccessChain, int64_t>& chainToId
        )
    {
    mStringBuilder.addInt32(chainToId.size());

    for (std::map<FreeVariableMemberAccessChain, int64_t>::const_iterator it =
             chainToId.begin();
         it != chainToId.end();
         ++it)
        {
        mStringBuilder.addString(it->first.str());
        mStringBuilder.addInt64(it->second);
        }
    }


void BinaryObjectRegistry::definePackedHomogenousData(int64_t objectId,
                                                      PyObject* pyObject)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_PACKED_HOMOGENOUS_DATA);
    
    PyObject* dtype = PyObject_GetAttrString(pyObject, "dtype");
    if (dtype == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't get dtype attr");
        }

    _writeDTypeElement(dtype);
    
    Py_DECREF(dtype);

    PyObject* dataAsBytes = PyObject_GetAttrString(pyObject, "dataAsBytes");
    if (dataAsBytes == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't get dataAsBytes attr");
        }
    if (not PyString_Check(dataAsBytes)) {
        Py_DECREF(dataAsBytes);
        throw std::logic_error("expected dataAsBytes attr to be a string");
        }

    mStringBuilder.addString(
        PyString_AS_STRING(dataAsBytes),
        PyString_GET_SIZE(dataAsBytes)
        );
    
    Py_DECREF(dataAsBytes);
    }


void BinaryObjectRegistry::_writeDTypeElement(PyObject* val)
    {
    if (val == Py_None) {
        mStringBuilder.addByte(CODE_NONE);
        }
    else if (PyInt_Check(val)) {
        mStringBuilder.addByte(CODE_INT);
        mStringBuilder.addInt64(PyInt_AS_LONG(val));
        }
    else if (PyString_Check(val)) {
        mStringBuilder.addByte(CODE_STR);
        mStringBuilder.addString(
            PyString_AS_STRING(val),
            PyString_GET_SIZE(val)
            );
        }
    else if (PyTuple_Check(val)) {
        mStringBuilder.addByte(CODE_TUPLE);
        Py_ssize_t len = PyTuple_GET_SIZE(val);
        mStringBuilder.addInt32(len);
        for (Py_ssize_t ix = 0; ix < len; ++ix) {
            _writeDTypeElement(PyTuple_GET_ITEM(val, ix));
            }
        }
    else {
        throw std::logic_error("unknown primitive in dtype: " +
            PyObjectUtils::str_string(val));
        }
    }
