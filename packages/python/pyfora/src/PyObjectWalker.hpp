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
    
    void _registerUnconvertible(int64_t objectId, const PyObject* PyObject) const;
    void _registerRemotePythonObject(int64_t objectId, PyObject* pyObject) const;
    void _registerPackedHomogenousData(int64_t objectId, PyObject* pyObject) const;
    void _registerFuture(int64_t objectId, PyObject* pyObject);
    void _registerBuiltinExceptionInstance(int64_t objectId, PyObject* pyException);
    void _registerTypeOrBuiltinFunctionNamedSingleton(int64_t objectId,
                                                      PyObject* pyObject) const;
    void _registerWithBlock(int64_t objectId, PyObject* pyObject);
    void _registerTuple(int64_t objectId, PyObject* pyTuple);
    void _registerList(int64_t objectId, PyObject* pyList);
    void _registerListOfPrimitives(int64_t objectId, PyObject* pyList) const;
    void _registerListGeneric(int64_t objectId, const PyObject* pyList);
    void _registerDict(int64_t objectId, PyObject* pyObject);
    void _registerFunction(int64_t objectId, PyObject* pyFunction);
    void _registerClass(int64_t objectId, PyObject* pyClass);
    void _registerClassInstance(int64_t objectId, PyObject* pyClass);   
    void _registerInstanceMethod(int64_t objectId, PyObject* pyObject);

    template<typename T>
    void _registerPrimitive(int64_t objectId, const T& t) {
        mObjectRegistry.definePrimitive(objectId, t);
        }

    bool _canMap(PyObject* pyObject) const;
    PyObject* _pureInstanceReplacement(PyObject* pyObject);

    /*
      Basically just calls FreeVariableResolver::resolveFreeVariableMemberAccessChainsInAst, which means this:
      returns a new reference to a dict: FVMAC -> (resolution, location)
     FVMAC here is a tuple of strings
     */
    PyObject* _computeAndResolveFreeVariableMemberAccessChainsInAst(
        const PyObject* pyObject,
        const PyObject* pyAst
        ) const;
    
    PyObject* _freeMemberAccessChainsWithPositions(
        const PyObject* pyAst
        ) const;

    PyObject* _getPyConvertedObjectCache() const;
    PyObject* _getDataMemberNames(PyObject* classInstance, PyObject* classObject) const;
    PyObject* _withBlockFun(PyObject* withBlockAst, int64_t lineno) const;
    PyObject* _defaultAstArgs() const;

    void _augmentChainsWithBoundValuesInScope(
        PyObject* pyObject,
        PyObject* withBlockFun,
        PyObject* boundVariables,
        PyObject* chainsWithPositions) const;

    // checks: pyObject.__class__ in NamedSingletons.pythonSingletonToName
    // (expects that pyObject has a __class__ attr
    bool _classIsNamedSingleton(PyObject* pyObject) const;
    bool _isTypeOrBuiltinFunctionAndInNamedSingletons(PyObject* pyObject) const;

    ClassOrFunctionInfo _classOrFunctionInfo(PyObject*, bool isFunction);

    std::map<FreeVariableMemberAccessChain, int64_t>
    _processFreeVariableMemberAccessChainResolutions(PyObject* resolutions);

    FreeVariableMemberAccessChain _toChain(const PyObject*) const;

    std::string _fileText(const std::string& filename) const;
    std::string _fileText(const PyObject* filename) const;

    std::string _getWithBlockSourceFileName(PyObject* pyforaWithBlock) const;

    /*
      Returns a valid PyObject, either None or a String,
      or throws a std::runtime_error
     */
    PyObject* _getModulePathForObject(const PyObject* pyObject) const;

    // init functions called from ctor
    void _initPyforaModule();
    void _initPythonSingletonToName();
    void _initRemotePythonObjectClass();
    void _initPackedHomogenousDataClass();
    void _initFutureClass();
    void _initWithBlockClass();
    void _initGetPathToObjectFun();
    void _initUnconvertibleClass();
    void _initPyforaConnectHack();

    static bool _isPrimitive(const PyObject* pyObject);
    static bool _allPrimitives(const PyObject* pyList);

    PyObject* mPurePythonClassMapping;
    PyObject* mPyforaModule;
    PyObject* mRemotePythonObjectClass;
    PyObject* mPackedHomogenousDataClass;
    PyObject* mFutureClass;
    PyObject* mExcludePredicateFun;
    PyObject* mExcludeList;
    PyObject* mTerminalValueFilter;
    PyObject* mWithBlockClass;
    PyObject* mGetPathToObjectFun;
    PyObject* mUnconvertibleClass;
    PyObject* mPyforaConnectHack;

    std::map<long, PyObject*> mConvertedObjectCache;
    std::map<PyObject*, int64_t> mPyObjectToObjectId;
    std::map<PyObject*, std::string> mPythonSingletonToName;
    std::map<std::string, int64_t> mConvertedFiles;
    BinaryObjectRegistry& mObjectRegistry;
    FreeVariableResolver mFreeVariableResolver;
    };
