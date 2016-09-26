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
#include "PyObjectWalker.hpp"

#include "ClassOrFunctionInfo.hpp"
#include "FileDescription.hpp"
#include "FreeVariableResolver.hpp"
#include "PyAstUtil.hpp"
#include "PyAstFreeVariableAnalyses.hpp"
#include "PyforaInspect.hpp"
#include "PyObjectUtils.hpp"

#include <stdexcept>
#include <vector>


PyObjectWalker::PyObjectWalker(
        PyObject* purePythonClassMapping,
        BinaryObjectRegistry& objectRegistry,
        PyObject* excludePredicateFun,
        PyObject* excludeList,
        PyObject* terminalValueFilter) :
            mPurePythonClassMapping(purePythonClassMapping),
            mPyforaModule(NULL),
            mRemotePythonObjectClass(NULL),
            mPackedHomogenousDataClass(NULL),
            mExcludePredicateFun(excludePredicateFun),
            mExcludeList(excludeList),
            mTerminalValueFilter(terminalValueFilter),
            mObjectRegistry(objectRegistry),
            mFreeVariableResolver(excludeList, terminalValueFilter)
    {
    Py_INCREF(mPurePythonClassMapping);
    Py_INCREF(mExcludePredicateFun);
    Py_INCREF(mExcludeList);
    Py_INCREF(mTerminalValueFilter);
    _initPyforaModule();
    _initPythonSingletonToName();
    _initRemotePythonObjectClass();
    _initPackedHomogenousDataClass();
    _initFutureClass();
    }


void PyObjectWalker::_initPyforaModule()
    {
    mPyforaModule = PyImport_ImportModule("pyfora");

    if (mPyforaModule == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't import pyfora module");
        }
    }


void PyObjectWalker::_initFutureClass()
    {
    PyObject* futureModule = PyObject_GetAttrString(mPyforaModule,
                                                    "Future");

    if (futureModule == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't import Future module");
        }

    mFutureClass = PyObject_GetAttrString(futureModule,
                                          "Future");
    Py_DECREF(futureModule);
    if (mFutureClass == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't find Future.Future");
        }
    }


void PyObjectWalker::_initPackedHomogenousDataClass()
    {
    PyObject* typeDescriptionModule = PyObject_GetAttrString(mPyforaModule,
                                                             "TypeDescription");

    if (typeDescriptionModule == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't import TypeDescription module");
        }

    mPackedHomogenousDataClass = PyObject_GetAttrString(typeDescriptionModule,
                                                        "PackedHomogenousData");
    Py_DECREF(typeDescriptionModule);
    if (mPackedHomogenousDataClass == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't find TypeDescription.PackedHomogenousData");
        }
    }


void PyObjectWalker::_initRemotePythonObjectClass()
    {
    PyObject* remotePythonObjectModule = PyObject_GetAttrString(mPyforaModule,
                                                                "RemotePythonObject");

    if (remotePythonObjectModule == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't import RemotePythonObjectModule");
        }

    mRemotePythonObjectClass = PyObject_GetAttrString(remotePythonObjectModule,
                                                      "RemotePythonObject");
    Py_DECREF(remotePythonObjectModule);
    if (mRemotePythonObjectClass == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't find RemotePythonObject.RemotePythonObject");
        }
    }


void PyObjectWalker::_initPythonSingletonToName()
    {
    PyObject* namedSingletonsModule = PyObject_GetAttrString(mPyforaModule,
                                                             "NamedSingletons");
    
    if (namedSingletonsModule == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't import NamedSingleTons module");
        }

    PyObject* pythonSingletonToName = PyObject_GetAttrString(namedSingletonsModule,
                                                             "pythonSingletonToName");
    Py_DECREF(namedSingletonsModule);
    if (pythonSingletonToName == NULL) {
        PyErr_Print();
        throw std::logic_error("expected to find member pythonSingletonToName"
                               " in NamedSingletons");
        }
    if (not PyDict_Check(pythonSingletonToName)) {
        throw std::logic_error("expected pythonSingletonToName to be a dict");
        }

    PyObject * key, * value;
    Py_ssize_t pos = 0;
    char* string = NULL;
    Py_ssize_t length = 0;

    while (PyDict_Next(pythonSingletonToName, &pos, &key, &value)) {
        if (PyString_AsStringAndSize(value, &string, &length) == -1) {
            std::logic_error("expected values in pythonSingletonToName to be strings");
            }

        Py_INCREF(key);
        mPythonSingletonToName[key] = std::string(string, length);
        }

    Py_DECREF(pythonSingletonToName);
    }


PyObjectWalker::~PyObjectWalker()
    {
    for (std::map<long, PyObject*>::const_iterator it =
            mConvertedObjectCache.begin();
        it != mConvertedObjectCache.end();
        ++it) {
        Py_DECREF(it->second);
        }
    for (std::map<PyObject*, int64_t>::const_iterator it =
             mPyObjectToObjectId.begin();
         it != mPyObjectToObjectId.end();
         ++it) {
        Py_DECREF(it->first);
        }
    for (std::map<PyObject*, std::string>::const_iterator it =
             mPythonSingletonToName.begin();
         it != mPythonSingletonToName.end();
         ++it) {
        Py_DECREF(it->first);
        }
    Py_XDECREF(mTerminalValueFilter);
    Py_XDECREF(mExcludeList);
    Py_XDECREF(mExcludePredicateFun);
    Py_XDECREF(mFutureClass);
    Py_XDECREF(mPackedHomogenousDataClass);
    Py_XDECREF(mRemotePythonObjectClass);
    Py_XDECREF(mPyforaModule);
    Py_XDECREF(mPurePythonClassMapping);
    }


int64_t PyObjectWalker::_allocateId(PyObject* pyObject) {
    int64_t objectId = mObjectRegistry.allocateObject();

    Py_INCREF(pyObject);
    mPyObjectToObjectId[pyObject] = objectId;

    return objectId;
    }


int64_t PyObjectWalker::walkPyObject(PyObject* pyObject) 
    {
        {
        std::map<PyObject*, int64_t>::const_iterator it =
            mPyObjectToObjectId.find(pyObject);

        if (it != mPyObjectToObjectId.end()) {
            return it->second;
            }
        }
    
        {
        std::map<long, PyObject*>::const_iterator it =
            mConvertedObjectCache.find(
                PyObjectUtils::builtin_id(pyObject)
                );

        if (it != mConvertedObjectCache.end()) {
            pyObject = it->second;
            }
        }

    bool wasReplaced = false;
    if (_canMap(pyObject)) {
        pyObject = _pureInstanceReplacement(pyObject);

        wasReplaced = true;
        }

    int64_t objectId = _allocateId(pyObject);

    // TODO there's some exception logic in the py version which
    // we're not replicating here

    _walkPyObject(pyObject, objectId);

    if (wasReplaced) {
        Py_DECREF(pyObject);
        }

    return objectId;
    }


int64_t PyObjectWalker::walkFileDescription(const FileDescription& fileDescription)
    {
    std::map<std::string, int64_t>::const_iterator it =
        mConvertedFiles.find(fileDescription.filename);

    if (it != mConvertedFiles.end()) {
        return it->second;
        }

    int64_t objectId = mObjectRegistry.allocateObject();
    
    mObjectRegistry.defineFile(objectId,
                               fileDescription.filetext,
                               fileDescription.filename
                               );

    mConvertedFiles[fileDescription.filename] = objectId;

    return objectId;
    }


bool PyObjectWalker::_canMap(PyObject* pyObject) {
    PyObject* pyString = PyString_FromString("canMap");

    if (pyString == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't make a PyString from a C++ string");
        }

    PyObject* res = PyObject_CallMethodObjArgs(
        mPurePythonClassMapping,
        pyString,
        pyObject,
        NULL
        );

    Py_DECREF(pyString);

    if (res == NULL) {
        PyErr_Print();
        throw std::logic_error(
            "an error occurred trying to call purePythonClassMapping.canMap"
            );
        }

    bool tr = false;
    if (PyObject_IsTrue(res))
        {
        tr = true;
        }

    Py_DECREF(res);

    return tr;
    }


PyObject* PyObjectWalker::_pureInstanceReplacement(PyObject* pyObject)
    {
    PyObject* pyString = PyString_FromString("mappableInstanceToPure");
    if (pyString == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't make a PyString from a C++ string");
        }

    PyObject* pureInstance = PyObject_CallMethodObjArgs(
        mPurePythonClassMapping,
        pyString,
        pyObject,
        NULL
        );

    Py_DECREF(pyString);

    if (pureInstance == NULL) {
        PyErr_Print();
        throw std::logic_error(
            "an error occurred trying to call "
            "purePythonClassMapping.mappableInstanceToPure"
            );
        }

    mConvertedObjectCache[PyObjectUtils::builtin_id(pyObject)] = pureInstance;

    return pureInstance;
    }


void PyObjectWalker::_walkPyObject(PyObject* pyObject, int64_t objectId) {
    /* Missing:

       RemotePythonObject.RemotePythonObject -- uses ComputedValue. going away?
       PyforaWithBlock.PyforaWithBlock
       _Unconvertible
       instancemethod
    */
    if (PyObject_IsInstance(pyObject, mPackedHomogenousDataClass))
        {
        _registerPackedHomogenousData(objectId, pyObject);
        }
    else if (PyObject_IsInstance(pyObject, mFutureClass))
        {
        _registerFuture(objectId, pyObject);
        }
    else if (PyObject_IsInstance(pyObject, PyExc_Exception)
            and _classIsNamedSingleton(pyObject))
        {
        _registerBuiltinExceptionInstance(objectId, pyObject);
        }
    else if (_isTypeOrBuiltinFunctionAndInNamedSingletons(pyObject))
        {
        _registerTypeOrBuiltinFunctionNamedSingleton(objectId, pyObject);
        }
    else if (PyTuple_Check(pyObject))
        {
        _registerTuple(objectId, pyObject);
        }
    else if (PyList_Check(pyObject))
        {
        _registerList(objectId, pyObject);
        }
    else if (PyDict_Check(pyObject))
        {
        _registerDict(objectId, pyObject);
        }
    else if (_isPrimitive(pyObject))
        {
        _registerPrimitive(objectId, pyObject);
        }
    else if (PyFunction_Check(pyObject))
        {
        _registerFunction(objectId, pyObject);
        }
    else if (PyforaInspect::isclass(pyObject))
        {
        _registerClass(objectId, pyObject);
        }
    else if (PyforaInspect::isclassinstance(pyObject))
        {
        _registerClassInstance(objectId, pyObject);
        }
    else if (PyMethod_Check(pyObject))
        {
        _registerInstanceMethod(objectId, pyObject);
        }
    else {
        throw std::logic_error("PyObjectWalker couldn't handle a PyObject: " +
            PyObjectUtils::repr_string(pyObject));
        }
    }


bool PyObjectWalker::_isPrimitive(PyObject* pyObject) {
    return Py_None == pyObject or
        PyInt_Check(pyObject) or
        PyFloat_Check(pyObject) or
        PyString_Check(pyObject) or
        PyBool_Check(pyObject);        
    }


bool PyObjectWalker::_allPrimitives(PyObject* pyList)
    {
    // precondition: the argument must be a PyList
    Py_ssize_t size = PyList_GET_SIZE(pyList);
    for (Py_ssize_t ix = 0; ix < size; ++ix)
        {
        if (not _isPrimitive(PyList_GET_ITEM(pyList, ix)))
            return false;
        }
    return true;
    }


bool PyObjectWalker::_classIsNamedSingleton(PyObject* pyObject)
    {
    PyObject* __class__attr = PyObject_GetAttrString(pyObject, "__class__");

    if (__class__attr == NULL) {
        PyErr_Print();
        throw std::logic_error(
            "arguments to this function are expected to have a ");
        }

    bool tr = (mPythonSingletonToName.find(__class__attr) != 
        mPythonSingletonToName.end());

    Py_DECREF(__class__attr);

    return tr;
    }


void PyObjectWalker::_registerPackedHomogenousData(int64_t objectId,
                                                   PyObject* pyObject)
    {
    mObjectRegistry.definePackedHomogenousData(objectId, pyObject);
    }


void PyObjectWalker::_registerFuture(int64_t objectId, PyObject* pyObject)
    {
    PyObject* result_attr = PyObject_GetAttrString(pyObject, "result");

    if (result_attr == NULL) {
        PyErr_Print();
        throw std::logic_error("expected a result member on Future.Future instances");
        }

    PyObject* res = PyObject_CallMethodObjArgs(result_attr, NULL);
    Py_DECREF(result_attr);
    if (res == NULL) {
        PyErr_Print();
        Py_DECREF(result_attr);
        throw std::logic_error("an error occurred when calling future.result()");
        }

    walkPyObject(res);

    Py_DECREF(res);
    }


void PyObjectWalker::_registerBuiltinExceptionInstance(int64_t objectId,
                                                       PyObject* pyException)
    {
    PyObject* __class__attr = PyObject_GetAttrString(pyException, "__class__");

    if (__class__attr == NULL) {
        PyErr_Print();
        throw std::logic_error(
            "expected this PyObject to have a `__class__` attr");
        }

    std::map<PyObject*, std::string>::const_iterator it =
        mPythonSingletonToName.find(__class__attr);
    Py_DECREF(__class__attr);
    if (it == mPythonSingletonToName.end()) {
        throw std::logic_error(
            "it's supposed to be a precondition to this function that this not happen");
        }

    PyObject* args_attr = PyObject_GetAttrString(pyException, "args");
    if (args_attr == NULL) {
        PyErr_Print();
        throw std::logic_error(
            "expected this PyObject to have an `args` attr");
        }

    int64_t argsId = walkPyObject(args_attr);

    Py_DECREF(args_attr);

    mObjectRegistry.defineBuiltinExceptionInstance(objectId,
                                                   it->second,
                                                   argsId);
    }


bool PyObjectWalker::_isTypeOrBuiltinFunctionAndInNamedSingletons(PyObject* pyObject)
    {
    if (not PyType_Check(pyObject) and not PyCFunction_Check(pyObject)) {
        return false;
        }

    return mPythonSingletonToName.find(pyObject) != mPythonSingletonToName.end();
    }


std::string PyObjectWalker::_fileText(const std::string& filename)
    {
    PyObject* fileNamePyObj = PyString_FromStringAndSize(filename.data(),
                                                         filename.size());

    if (fileNamePyObj == NULL) {
        throw std::logic_error("couldn't create a python string out of a std::string");
        }

    PyObject* lines = PyforaInspect::getlines(fileNamePyObj);
    Py_DECREF(fileNamePyObj);

    if (lines == NULL) {
        throw std::logic_error(
            "error calling getlines func on string `" + filename + "`");
        }
    if (not PyList_Check(lines)) {
        Py_DECREF(lines);
        throw std::logic_error("expected a list");
        }

    std::ostringstream oss;
    for (Py_ssize_t ix = 0; ix < PyList_GET_SIZE(lines); ++ix)
        {
        PyObject* item = PyList_GET_ITEM(lines, ix);
        if (not PyString_Check(item)) {
            Py_DECREF(lines);
            throw std::logic_error("all elements in lines should be strings");
            }

        oss << std::string(PyString_AS_STRING(item), PyString_GET_SIZE(item));
        }
    
    Py_DECREF(lines);

    return oss.str();
    }


void PyObjectWalker::_registerTypeOrBuiltinFunctionNamedSingleton(int64_t objectId,
                                                                  PyObject* pyObject)
    {
    std::map<PyObject*, std::string>::const_iterator it =
        mPythonSingletonToName.find(pyObject);

    if (it == mPythonSingletonToName.end()) {
        throw std::logic_error(
            "this shouldn't happen if _isTypeOrBuiltinFunctionAndInNamedSingletons "
            "returned true"
            );
        }

    mObjectRegistry.defineNamedSingleton(objectId, it->second);
    }


void PyObjectWalker::_registerTuple(int64_t objectId, PyObject* pyTuple)
    {
    std::vector<int64_t> memberIds;
    Py_ssize_t size = PyTuple_GET_SIZE(pyTuple);
    for (Py_ssize_t ix = 0; ix < size; ++ix)
        {
        memberIds.push_back(
            walkPyObject(PyTuple_GET_ITEM(pyTuple, ix))
            );
        }

    mObjectRegistry.defineTuple(objectId, memberIds);
    }
    

void PyObjectWalker::_registerList(int64_t objectId, PyObject* pyList)
    {
    if (_allPrimitives(pyList))
        {
        _registerListOfPrimitives(objectId, pyList);
        }
    else {
        _registerListGeneric(objectId, pyList);
        }
    }


void PyObjectWalker::_registerListOfPrimitives(int64_t objectId, PyObject* pyList)
    {
    mObjectRegistry.definePrimitive(objectId, pyList);
    }


void PyObjectWalker::_registerListGeneric(int64_t objectId ,PyObject* pyList)
    {
    std::vector<int64_t> memberIds;
    Py_ssize_t size = PyList_GET_SIZE(pyList);
    for (Py_ssize_t ix = 0; ix < size; ++ix)
        {
        memberIds.push_back(
            walkPyObject(PyList_GET_ITEM(pyList, ix))
            );
        }
    mObjectRegistry.defineList(objectId, memberIds);
    }
    

void PyObjectWalker::_registerDict(int64_t objectId, PyObject* pyDict)
    {
    std::vector<int64_t> keyIds;
    std::vector<int64_t> valueIds;
    PyObject* key = NULL;
    PyObject* value = NULL;
    Py_ssize_t pos = 0;
    
    while (PyDict_Next(pyDict, &pos, &key, &value)) {
        keyIds.push_back(walkPyObject(key));
        valueIds.push_back(walkPyObject(value));
        }

    mObjectRegistry.defineDict(objectId, keyIds, valueIds);
    }
    

void PyObjectWalker::_registerFunction(int64_t objectId, PyObject* pyObject)
    {
    ClassOrFunctionInfo info = _classOrFunctionInfo(pyObject, true);
    
    mObjectRegistry.defineFunction(
        objectId,
        info.sourceFileId(),
        info.lineNumber(),
        info.freeVariableMemberAccessChainsToId()
        );
    }
    

ClassOrFunctionInfo
PyObjectWalker::_classOrFunctionInfo(PyObject* obj, bool isFunction)
    {
    // old PyObjectWalker checks for __inline_fora here

    // should probably make these just return PyStrings, as 
    // we only repackage these into PyStrings anyway
    std::pair<std::string, std::string> filenameAndText =
        PyAstUtil::sourceFilenameAndText(obj);
    long startingSourceLine = PyAstUtil::startingSourceLine(obj);
    PyObject* sourceAst = PyAstUtil::pyAstFromText(filenameAndText.second);
    if (sourceAst == NULL) {
        PyErr_Print();
        throw std::logic_error(
            "an error occured calling pyAstFromText"
            );
        }

    PyObject* pyAst = NULL;
    if (isFunction) {
        pyAst = PyAstUtil::functionDefOrLambdaAtLineNumber(
            sourceAst,
            startingSourceLine);
        }
    else {
        pyAst = PyAstUtil::classDefAtLineNumber(sourceAst,
                                                startingSourceLine);
        }

    Py_DECREF(sourceAst);

    if (pyAst == NULL)
        {
        PyErr_Print();
        throw std::logic_error("an error occured getting the sub-ast.");
        }

    PyObject* resolutions =
        _computeAndResolveFreeVariableMemberAccessChainsInAst(obj,
                                                              pyAst);

    Py_DECREF(pyAst);

    std::map<FreeVariableMemberAccessChain, int64_t> processedResolutions =
        _processFreeVariableMemberAccessChainResolutions(resolutions);

    Py_DECREF(resolutions);

    int64_t fileId = walkFileDescription(
        FileDescription::cachedFromArgs(filenameAndText));

    return ClassOrFunctionInfo(fileId,
                               startingSourceLine,
                               processedResolutions);
    }


std::map<FreeVariableMemberAccessChain, int64_t>
PyObjectWalker::_processFreeVariableMemberAccessChainResolutions(
        PyObject* resolutions
        )
    {
    if (not PyDict_Check(resolutions)) {
        throw std::logic_error("expected a dict argument");
        }

    PyObject * key, * value;
    Py_ssize_t pos = 0;
    std::map<FreeVariableMemberAccessChain, int64_t> tr;

    while (PyDict_Next(resolutions, &pos, &key, &value)) {
        /*
          Values should be length-two tuples: (resolution, location)
         */
        if (not PyTuple_Check(value)) {
            throw std::logic_error("expected tuple values");
            }
        if (PyTuple_GET_SIZE(value) != 2) {
            throw std::logic_error("expected values to be tuples of length 2");
            }
        PyObject* resolution = PyTuple_GET_ITEM(value, 0);
        Py_INCREF(resolution);
        Py_INCREF(key);
        tr[_toChain(key)] = walkPyObject(resolution);
        Py_DECREF(key);
        Py_DECREF(resolution);
        }

    return tr;
    }


PyObject* PyObjectWalker::_getPyConvertedObjectCache() const
    {
    PyObject* tr = PyDict_New();
    if (tr == NULL) {
        return NULL;
        }

    for (std::map<long, PyObject*>::const_iterator it =
             mConvertedObjectCache.begin();
         it != mConvertedObjectCache.end();
         ++it)
        {
        PyObject* pyLong = PyLong_FromLong(it->first);
        if (pyLong == NULL) {
            PyErr_Print();
            throw std::logic_error("error getting python long from C long");
            }

        if (PyDict_SetItem(tr, pyLong, it->second) != 0)
            {
            return NULL;
            }

        Py_DECREF(pyLong);
        }

    return tr;
    }


PyObject* PyObjectWalker::_computeAndResolveFreeVariableMemberAccessChainsInAst(
        PyObject* pyObject,
        PyObject* pyAst
        ) const
    {
    PyObject* chainsWithPositions = _freeMemberAccessChainsWithPositions(pyAst);
    if (chainsWithPositions == NULL) {
        PyErr_Print();
        throw std::logic_error("error calling _freeMemberAccessChainsWithPositions");
        }

    PyObject* pyConvertedObjectCache = _getPyConvertedObjectCache();
    if (pyConvertedObjectCache == NULL) {
        PyErr_Print();
        Py_DECREF(chainsWithPositions);
        throw std::logic_error("error getting convertedObjectCache");
        }

    PyObject* resolutions = 
        mFreeVariableResolver.resolveFreeVariableMemberAccessChainsInAst(
            pyObject,
            pyAst,
            chainsWithPositions,
            pyConvertedObjectCache);

    Py_DECREF(pyConvertedObjectCache);
    Py_DECREF(chainsWithPositions);

    return resolutions;
    }


PyObject* PyObjectWalker::_freeMemberAccessChainsWithPositions(
        PyObject* pyAst
        ) const
    {
    return PyAstFreeVariableAnalyses::getFreeMemberAccessChainsWithPositions(
            pyAst,
            false,
            true,
            mExcludePredicateFun
            );
    }


void PyObjectWalker::_registerClass(int64_t objectId, PyObject* pyObject)
    {
    ClassOrFunctionInfo info = _classOrFunctionInfo(pyObject, false);

    PyObject* bases = PyObject_GetAttrString(pyObject,
                                             "__bases__");

    if (bases == NULL) {
        throw std::logic_error(
            "couldn't get __bases__ member of an object we expected to be a class"
            );
        }
    if (not PyTuple_Check(bases)) {
        Py_DECREF(bases);
        throw std::logic_error("expected bases to be a list");
        }

    std::vector<int64_t> baseClassIds;
    for (Py_ssize_t ix = 0; ix < PyTuple_GET_SIZE(bases); ++ix)
        {
        PyObject* item = PyTuple_GET_ITEM(bases, ix);

        std::map<PyObject*, int64_t>::const_iterator it =
            mPyObjectToObjectId.find(item);
        
        if (it == mPyObjectToObjectId.end()) {
            Py_DECREF(bases);
            throw std::logic_error(
                "expected each base class to have a registered id");
            }
        
        baseClassIds.push_back(it->second);
        }

    Py_DECREF(bases);

    mObjectRegistry.defineClass(
        objectId,
        info.sourceFileId(),
        info.lineNumber(),
        info.freeVariableMemberAccessChainsToId(),
        baseClassIds);

    }


void PyObjectWalker::_registerClassInstance(int64_t objectId, PyObject* pyObject)
    {
    PyObject* classObject = PyObject_GetAttrString(pyObject, "__class__");
    if (classObject == NULL) {
        PyErr_Print();
        throw std::logic_error(
            "couldn't get __class__ attr on a pyObject we thought had that attr"
            );
        }

    int64_t classId = walkPyObject(classObject);

    // Pure PyObjectWalker has a check for Unconvertibles here

    PyObject* dataMemberNames = _getDataMemberNames(pyObject, classObject);

    Py_DECREF(classObject);

    if (not PyList_Check(dataMemberNames)) {
        throw std::logic_error("expected dataMemberNames to be a list");
        }

    std::map<std::string, int64_t> classMemberNameToClassMemberId;

    for (Py_ssize_t ix = 0; ix < PyList_GET_SIZE(dataMemberNames); ++ix)
        {
        PyObject* dataMemberName = PyList_GET_ITEM(dataMemberNames, ix);
        if (not PyString_Check(dataMemberName)) {
            throw std::logic_error("expected data member names to be strings");
            }

        PyObject* dataMember = PyObject_GetAttr(pyObject, dataMemberName);
        if (dataMember == NULL) {
            PyErr_Print();
            throw std::logic_error("error getting datamember");
            }

        int64_t dataMemberId = walkPyObject(dataMember);

        classMemberNameToClassMemberId[
            std::string(
                PyString_AS_STRING(dataMemberName),
                PyString_GET_SIZE(dataMemberName)
                )
            ] = dataMemberId;

        Py_DECREF(dataMember);
        }
    
    Py_DECREF(dataMemberNames);

    mObjectRegistry.defineClassInstance(
        objectId,
        classId,
        classMemberNameToClassMemberId);
    }


PyObject*
PyObjectWalker::_getDataMemberNames(PyObject* pyObject, PyObject* classObject) const
    {
    if (PyObject_HasAttrString(pyObject, "__dict__")) {
        PyObject* __dict__attr = PyObject_GetAttrString(pyObject, "__dict__");
        if (__dict__attr == NULL) {
            PyErr_Print();
            throw std::logic_error("failed getting __dict__ attr");
            }
        if (not PyDict_Check(__dict__attr)) {
            Py_DECREF(__dict__attr);
            throw std::logic_error("expected __dict__ attr to be a dict");
            }
        PyObject* keys = PyDict_Keys(__dict__attr);
        Py_DECREF(__dict__attr);
        if (keys == NULL) {
            PyErr_Print();
            throw std::logic_error("couldn't get keys on a dict");
            }
        if (not PyList_Check(keys)) {
            throw std::logic_error("expected keys to be a list");
            }

        return keys;
        }
    else {
        return PyAstUtil::collectDataMembersSetInInit(classObject);
        }
    }


void PyObjectWalker::_registerInstanceMethod(int64_t objectId, PyObject* pyObject)
    {
    PyObject* __self__attr = PyObject_GetAttrString(pyObject, "__self__");
    if (__self__attr == NULL) {
        throw std::logic_error(
            "expected to have a __self__ attr on instancemethods"
            );
        }

    PyObject* __name__attr = PyObject_GetAttrString(pyObject, "__name__");
    if (__name__attr == NULL) {
        Py_DECREF(__self__attr);
        throw std::logic_error(
            "expected to have a __name__ attr on instancemethods"
            );
        }
    if (not PyString_Check(__name__attr)) {
        Py_DECREF(__name__attr);
        Py_DECREF(__self__attr);
        throw std::logic_error(
            "expected __name__ attr to be a string"
            );
        }

    int64_t instanceId = walkPyObject(__self__attr);

    mObjectRegistry.defineInstanceMethod(objectId,
                                         instanceId,
                                         PyObjectUtils::std_string(__name__attr)
                                         );

    Py_DECREF(__name__attr);
    Py_DECREF(__self__attr);
    }


FreeVariableMemberAccessChain PyObjectWalker::_toChain(PyObject* obj) const
    {
    if (not PyTuple_Check(obj)) {
        throw std::logic_error("expected FVMAC to be tuples ");
        }

    std::vector<std::string> variables;

    for (Py_ssize_t ix = 0; ix < PyTuple_GET_SIZE(obj); ++ix) {
        PyObject* item = PyTuple_GET_ITEM(obj, ix);
        if (not PyString_Check(item)) {
            throw std::logic_error("expected FVMAC elements to be strings");
            }
        variables.push_back(PyObjectUtils::std_string(item));
        }

    return FreeVariableMemberAccessChain(variables);
    }
