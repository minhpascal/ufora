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
#pragma once

#include <Python.h>

#include <map>
#include <stdint.h>

#include "BinaryObjectRegistry.hpp"
#include "FreeVariableResolver.hpp"


class ClassOrFunctionInfo;
class FileDescription;


class PyObjectWalker {
public:
    PyObjectWalker(
        PyObject* purePythonClassMapping,
        BinaryObjectRegistry& objectRegistry, // should we really be doing this?
        PyObject* excludePredicateFun,
        PyObject* excludeList,
        PyObject* terminalValueFilter
        );

    ~PyObjectWalker();

    int64_t walkPyObject(PyObject* pyObject);
    int64_t walkFileDescription(const FileDescription& fileDescription);

private:
    int64_t _allocateId(PyObject* pyObject);
    void _walkPyObject(PyObject* pyObject, int64_t objectId);
    
    void _registerPackedHomogenousData(int64_t objectId, PyObject* pyObject);
    void _registerFuture(int64_t objectId, PyObject* pyObject);
    void _registerBuiltinExceptionInstance(int64_t objectId, PyObject* pyException);
    void _registerTypeOrBuiltinFunctionNamedSingleton(int64_t objectId,
                                                      PyObject* pyObject);
    void _registerTuple(int64_t objectId, PyObject* pyTuple);
    void _registerList(int64_t objectId, PyObject* pyList);
    void _registerListOfPrimitives(int64_t objectId, PyObject* pyList);
    void _registerListGeneric(int64_t objectId, PyObject* pyList);
    void _registerDict(int64_t objectId, PyObject* pyObject);
    void _registerFunction(int64_t objectId, PyObject* pyFunction);
    void _registerClass(int64_t objectId, PyObject* pyClass);
    void _registerClassInstance(int64_t objectId, PyObject* pyClass);   
    void _registerInstanceMethod(int64_t objectId, PyObject* pyObject);

    template<typename T>
    void _registerPrimitive(int64_t objectId, const T& t) {
        mObjectRegistry.definePrimitive(objectId, t);
        }

    bool _canMap(PyObject* pyObject);
    PyObject* _pureInstanceReplacement(PyObject* pyObject);

    /*
      Basically just calls FreeVariableResolver::resolveFreeVariableMemberAccessChainsInAst, which means this:
      returns a new reference to a dict: FVMAC -> (resolution, location)
     FVMAC here is a tuple of strings
     */
    PyObject* _computeAndResolveFreeVariableMemberAccessChainsInAst(
        PyObject* pyObject,
        PyObject* pyAst
        ) const;
    
    PyObject* _freeMemberAccessChainsWithPositions(
        PyObject* pyAst
        ) const;

    PyObject* _getPyConvertedObjectCache() const;

    PyObject* _getDataMemberNames(PyObject* classInstance, PyObject* classObject) const;

    // checks: pyObject.__class__ in NamedSingletons.pythonSingletonToName
    // (expects that pyObject has a __class__ attr
    bool _classIsNamedSingleton(PyObject* pyObject);
    bool _isTypeOrBuiltinFunctionAndInNamedSingletons(PyObject* pyObject);

    ClassOrFunctionInfo _classOrFunctionInfo(PyObject*, bool isFunction);

    std::map<FreeVariableMemberAccessChain, int64_t>
    _processFreeVariableMemberAccessChainResolutions(PyObject* resolutions);

    FreeVariableMemberAccessChain _toChain(PyObject*) const;

    std::string _fileText(const std::string& filename);

    // init functions called from ctor
    void _initPyforaModule();
    void _initPythonSingletonToName();
    void _initRemotePythonObjectClass();
    void _initPackedHomogenousDataClass();
    void _initFutureClass();

    static bool _isPrimitive(PyObject* pyObject);
    static bool _allPrimitives(PyObject* pyList);

    PyObject* mPurePythonClassMapping;
    PyObject* mPyforaModule;
    PyObject* mRemotePythonObjectClass;
    PyObject* mPackedHomogenousDataClass;
    PyObject* mFutureClass;
    PyObject* mExcludePredicateFun;
    PyObject* mExcludeList;
    PyObject* mTerminalValueFilter;
    std::map<long, PyObject*> mConvertedObjectCache;
    std::map<PyObject*, int64_t> mPyObjectToObjectId;
    std::map<PyObject*, std::string> mPythonSingletonToName;
    std::map<std::string, int64_t> mConvertedFiles;
    BinaryObjectRegistry& mObjectRegistry;
    FreeVariableResolver mFreeVariableResolver;
    };
