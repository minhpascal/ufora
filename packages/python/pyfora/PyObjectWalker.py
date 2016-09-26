#   Copyright 2015 Ufora Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
import pyfora
import pyfora.Exceptions as Exceptions
import pyfora.pyAst.PyAstFreeVariableAnalyses as PyAstFreeVariableAnalyses
import pyfora.RemotePythonObject as RemotePythonObject
import pyfora.Future as Future
import pyfora.NamedSingletons as NamedSingletons
import pyfora.PyforaWithBlock as PyforaWithBlock
import pyfora.TypeDescription as TypeDescription
import pyfora.PyforaInspect as PyforaInspect
import pyfora.pyAst.PyAstUtil as PyAstUtil
import pyfora.ModuleLevelObjectIndex as ModuleLevelObjectIndex
from pyfora.FreeVariableResolver import FreeVariableResolver
from pyfora.TypeDescription import isPrimitive
from pyfora.PyforaInspect import PyforaInspectError
from pyfora.Unconvertible import Unconvertible
from pyfora.UnresolvedFreeVariableExceptions import UnresolvedFreeVariableException, \
    UnresolvedFreeVariableExceptionWithTrace, \
    _convertUnresolvedFreeVariableExceptionAndRaise

import __builtin__
import ast
import sys

def pythonTracebackToJson(stacktrace):
    if stacktrace is None:
        return None

    res = []

    while stacktrace is not None:
        filename = stacktrace.tb_frame.f_code.co_filename
        lineno = stacktrace.tb_lineno

        res.append({'path':[filename],'range':{'start':{'line':lineno,'col':1}, 'stop':{'line':lineno,'col':1}}})
        stacktrace = stacktrace.tb_next

    #stacktraces are innermost to outermost
    return list(reversed(res))

def get_traceback_type():
    try:
        raise UserWarning()
    except:
        return type(sys.exc_info()[2])

traceback_type = get_traceback_type()



def isClassInstance(pyObject):
    return hasattr(pyObject, "__class__")


class _AClassWithAMethod:
    def f(self):
        pass


instancemethod = type(_AClassWithAMethod().f)


class _FunctionDefinition(object):
    def __init__(self, sourceFileId, lineNumber, freeVariableMemberAccessChainsToId):
        self.sourceFileId = sourceFileId
        self.lineNumber = lineNumber
        self.freeVariableMemberAccessChainsToId = \
            freeVariableMemberAccessChainsToId


class _ClassDefinition(object):
    def __init__(self, sourceFileId, lineNumber, freeVariableMemberAccessChainsToId):
        self.sourceFileId = sourceFileId
        self.lineNumber = lineNumber
        self.freeVariableMemberAccessChainsToId = \
            freeVariableMemberAccessChainsToId


class _FileDescription(object):
    _fileTextCache = {}

    def __init__(self, fileName, fileText):
        self.fileName = fileName
        self.fileText = fileText

    @classmethod
    def cachedFromArgs(cls, fileName, fileText=None):
        if fileName in cls._fileTextCache:
            return cls._fileTextCache[fileName]

        if fileText is None:
            fileText = "".join(PyforaInspect.getlines(fileName))

        if isinstance(fileText, unicode):
            fileText = fileText.encode("utf8")

        tr = cls(fileName, fileText)
        cls._fileTextCache[fileName] = tr
        return tr


class PyObjectWalker(object):
    """
    `PyObjectWalker`: walk a live python object, registering its pieces with an
    `ObjectRegistry`

    The main, and only publicly viewable function on this class is `walkPyObject`

    Attributes:
        _`purePythonClassMapping`: a `PureImplementationMapping` -- used to
            "replace" python objects in an python object graph by a "Pure"
            python class. For example, treat this `np.array` as a
            `PurePython.SomePureImplementationOfNumpyArray`.
        `_convertedObjectCache`: a mapping from python id -> pure instance
        `_pyObjectIdToObjectId`: mapping from python id -> id registered in
            `self.objectRegistry`
        `_objectRegistry`: an `ObjectRegistry` which holds an image of the
            objects we visit.

    """
    def __init__(self, purePythonClassMapping, objectRegistry):
        assert purePythonClassMapping is not None

        for singleton in NamedSingletons.pythonSingletonToName:
            if purePythonClassMapping.canMap(singleton):
                raise UserWarning(
                    "You provided a mapping that applies to %s, "
                    "which already has a direct mapping" % singleton
                    )

        self._purePythonClassMapping = purePythonClassMapping
        self._convertedObjectCache = {}
        self._pyObjectIdToObjectId = {}
        self._pyObjectIdToObject = {}
        self._objectRegistry = objectRegistry
        
        def terminal_value_filter(terminalValue):
            return not self._purePythonClassMapping.isOpaqueModule(terminalValue)

        self._freeVariableResolver = FreeVariableResolver(
            exclude_list=['staticmethod', 'property', '__inline_fora'],
            terminal_value_filter=terminal_value_filter)

    def _allocateId(self, pyObject):
        objectId = self._objectRegistry.allocateObject()
        self._pyObjectIdToObjectId[id(pyObject)] = objectId
        self._pyObjectIdToObject[id(pyObject)] = pyObject

        return objectId

    def walkPyObject(self, pyObject):
        """
        `walkPyObject`: recursively traverse a live python object,
        registering its "pieces" with an `ObjectRegistry`
        (`self.objectRegistry`).

        `objectId`s are assigned to all pieces of the python object.

        Returns:
            An `int`, the `objectId` of the root python object.
        """
        if id(pyObject) in self._pyObjectIdToObjectId:
            return self._pyObjectIdToObjectId[id(pyObject)]

        if id(pyObject) in self._convertedObjectCache:
            pyObject = self._convertedObjectCache[id(pyObject)]
        elif self._purePythonClassMapping.canMap(pyObject):
            pureInstance = self._purePythonClassMapping.mappableInstanceToPure(
                pyObject
                )
            self._convertedObjectCache[id(pyObject)] = pureInstance
            pyObject = pureInstance

        objectId = self._allocateId(pyObject)

        if pyObject is pyfora.connect:
            self._registerUnconvertible(objectId, None)
            return objectId

        try:
            self._walkPyObject(pyObject, objectId)
        except Exceptions.CantGetSourceTextError:
            self._registerUnconvertible(objectId, self.getModulePathForObject(pyObject))
        except PyforaInspectError:
            self._registerUnconvertible(objectId, self.getModulePathForObject(pyObject))

        return objectId

    def getModulePathForObject(self, pyObject):
        """Return a module-centric path to the object if one exists.

        If not None, then the result is a tuple (modulename, objectname) that gives a path to the object,
        where all modules are system or package modules (which we assume are the same on client and server).
        """
        return ModuleLevelObjectIndex.ModuleLevelObjectIndex.singleton().getPathToObject(pyObject)

    def _walkPyObject(self, pyObject, objectId):
        if isinstance(pyObject, RemotePythonObject.RemotePythonObject):
            self._registerRemotePythonObject(objectId, pyObject)
        elif isinstance(pyObject, TypeDescription.PackedHomogenousData):
            #it would be better to register the future and do a second pass of walking
            self._registerPackedHomogenousData(objectId, pyObject)
        elif isinstance(pyObject, Future.Future):
            #it would be better to register the future and do a second pass of walking
            self._walkPyObject(pyObject.result(), objectId)
        elif isinstance(pyObject, _FileDescription):
            self._registerFileDescription(objectId, pyObject)
        elif isinstance(pyObject, Exception) and pyObject.__class__ in \
           NamedSingletons.pythonSingletonToName:
            self._registerBuiltinExceptionInstance(objectId, pyObject)
        elif isinstance(pyObject, (type, type(isinstance))) and \
           pyObject in NamedSingletons.pythonSingletonToName:
            self._registerNamedSingleton(
                objectId,
                NamedSingletons.pythonSingletonToName[pyObject]
                )
        elif isinstance(pyObject, traceback_type):
            self._registerStackTraceAsJson(objectId, pyObject)
        elif isinstance(pyObject, PyforaWithBlock.PyforaWithBlock):
            self._registerWithBlock(objectId, pyObject)
        elif isinstance(pyObject, Unconvertible):
            self._registerUnconvertible(objectId, self.getModulePathForObject(pyObject.objectThatsNotConvertible))
        elif isinstance(pyObject, tuple):
            self._registerTuple(objectId, pyObject)
        elif isinstance(pyObject, list):
            self._registerList(objectId, pyObject)
        elif isinstance(pyObject, dict):
            self._registerDict(objectId, pyObject)
        elif isPrimitive(pyObject):
            self._registerPrimitive(objectId, pyObject)
        elif PyforaInspect.isfunction(pyObject):
            self._registerFunction(objectId, pyObject)
        elif PyforaInspect.isclass(pyObject):
            self._registerClass(objectId, pyObject)
        elif isinstance(pyObject, instancemethod):
            self._registerInstanceMethod(objectId, pyObject)
        elif isClassInstance(pyObject):
            self._registerClassInstance(objectId, pyObject)
        else:
            assert False, "don't know what to do with %s" % pyObject

    def _registerPackedHomogenousData(self, objectId, packedHomogenousData):
        self._objectRegistry.definePackedHomogenousData(objectId, packedHomogenousData)

    def _registerRemotePythonObject(self, objectId, remotePythonObject):
        """
        `_registerRemotePythonObject`: register a remotePythonObject
        (a terminal node in a python object graph) with `self.objectRegistry`
        """
        self._objectRegistry.defineRemotePythonObject(
            objectId,
            remotePythonObject._pyforaComputedValueArg()
            )

    def _registerFileDescription(self, objectId, fileDescription):
        """
        `_registerFileDescription`: register a `_FileDescription`
        (a terminal node in a python object graph) with `self.objectRegistry`
        """
        self._objectRegistry.defineFile(
            objectId=objectId,
            path=fileDescription.fileName,
            text=fileDescription.fileText
            )

    def _registerBuiltinExceptionInstance(self, objectId, builtinExceptionInstance):
        """
        `_registerBuiltinExceptionInstance`: register a `builtinExceptionInstance`
        with `self.objectRegistry`.

        Recursively call `walkPyObject` on the args of the instance.
        """
        argsId = self.walkPyObject(builtinExceptionInstance.args)

        self._objectRegistry.defineBuiltinExceptionInstance(
            objectId,
            NamedSingletons.pythonSingletonToName[
                builtinExceptionInstance.__class__
                ],
            argsId
            )

    def _registerNamedSingleton(self, objectId, singletonName):
        """
        `_registerNamedSingleton`: register a `NamedSingleton`
        (a terminal node in a python object graph) with `self.objectRegistry`
        """
        self._objectRegistry.defineNamedSingleton(objectId, singletonName)

    def _registerTuple(self, objectId, tuple_):
        """
        `_registerTuple`: register a `tuple` instance
        with `self.objectRegistry`.

        Recursively call `walkPyObject` on the values in the tuple.
        """
        memberIds = [self.walkPyObject(val) for val in tuple_]

        self._objectRegistry.defineTuple(
            objectId=objectId,
            memberIds=memberIds
            )

    def _registerList(self, objectId, list_):
        """
        `_registerList`: register a `list` instance with `self.objectRegistry`.
        Recursively call `walkPyObject` on the values in the list.
        """
        def allPrimitives(l):
            for x in l:
                if not isPrimitive(x):
                    return False
            return True

        if allPrimitives(list_):
            self._registerPrimitive(objectId, list_)
        else:
            memberIds = [self.walkPyObject(val) for val in list_]

            self._objectRegistry.defineList(
                objectId=objectId,
                memberIds=memberIds
                )

    def _registerPrimitive(self, objectId, primitive):
        """
        `_registerPrimitive`: register a primitive (defined by `isPrimitive`)
        (a terminal node in a python object graph) with `self.objectRegistry`
        """
        self._objectRegistry.definePrimitive(
            objectId,
            primitive
            )

    def _registerStackTraceAsJson(self, objectId, traceback):
        self._objectRegistry.defineStacktrace(objectId, pythonTracebackToJson(traceback))

    def _registerDict(self, objectId, dict_):
        """
        `_registerDict`: register a `dict` instance
        with `self.objectRegistry`.

        Recursively call `walkPyObject` on the keys and values in the dict
        """
        keyIds, valueIds = [], []
        for k, v in dict_.iteritems():
            keyIds.append(self.walkPyObject(k))
            valueIds.append(self.walkPyObject(v))

        self._objectRegistry.defineDict(
            objectId=objectId,
            keyIds=keyIds,
            valueIds=valueIds
            )

    def _registerInstanceMethod(self, objectId, pyObject):
        """
        `_registerInstanceMethod`: register an `instancemethod` instance
        with `self.objectRegistry`.

        Recursively call `walkPyObject` on the object to which the instance is
        bound, and encode alongside the name of the method.
        """
        instance = pyObject.__self__
        methodName = pyObject.__name__

        instanceId = self.walkPyObject(instance)

        self._objectRegistry.defineInstanceMethod(
            objectId=objectId,
            instanceId=instanceId,
            methodName=methodName
            )


    def _registerClassInstance(self, objectId, classInstance):
        """
        `_registerClassInstance`: register a `class` instance
        with `self.objectRegistry`.

        Recursively call `walkPyObject` on the class of the `classInstance`
        and on the data members of the instance.
        """
        classObject = classInstance.__class__
        classId = self.walkPyObject(classObject)

        if self._objectRegistry.isUnconvertible(classId):
            self._objectRegistry.defineUnconvertible(objectId, self.getModulePathForObject(classInstance))
            return

        dataMemberNames = classInstance.__dict__.keys() if hasattr(classInstance, '__dict__') \
            else PyAstUtil.collectDataMembersSetInInit(classObject)
        classMemberNameToClassMemberId = {}
        for dataMemberName in dataMemberNames:
            memberId = self.walkPyObject(getattr(classInstance, dataMemberName))
            classMemberNameToClassMemberId[dataMemberName] = memberId

        self._objectRegistry.defineClassInstance(
            objectId=objectId,
            classId=classId,
            classMemberNameToClassMemberId=classMemberNameToClassMemberId
            )

    def _registerWithBlock(self, objectId, pyObject):
        """
        `_registerWithBlock`: register a `PyforaWithBlock.PyforaWithBlock`
        with `self.objectRegistry`.

        Recursively call `walkPyObject` on the resolvable free variable
        member access chains in the block and on the file object.
        """
        lineNumber = pyObject.lineNumber
        sourceTree = PyAstUtil.pyAstFromText(pyObject.sourceText)
        withBlockAst = PyAstUtil.withBlockAtLineNumber(sourceTree, lineNumber)

        withBlockFun = ast.FunctionDef(
            name="",
            args=ast.arguments(args=[], defaults=[], kwarg=None, vararg=None),
            body=withBlockAst.body,
            decorator_list=[],
            lineno=lineNumber,
            col_offset=0
            )

        if PyAstUtil.hasReturnInOuterScope(withBlockFun):
            raise Exceptions.BadWithBlockError(
                "return statement not supported in pyfora with-block (line %s)" %
                PyAstUtil.getReturnLocationsInOuterScope(withBlockFun)[0])

        if PyAstUtil.hasYieldInOuterScope(withBlockFun):
            raise Exceptions.BadWithBlockError(
                "yield expression not supported in pyfora with-block (line %s)" %
                PyAstUtil.getYieldLocationsInOuterScope(withBlockFun)[0])

        freeVariableMemberAccessChainsWithPositions = \
            self._freeMemberAccessChainsWithPositions(withBlockFun)

        boundValuesInScopeWithPositions = \
            PyAstFreeVariableAnalyses.collectBoundValuesInScope(
                withBlockFun, getPositions=True)

        for boundValueWithPosition in boundValuesInScopeWithPositions:
            val, pos = boundValueWithPosition
            if val not in pyObject.unboundLocals and val in pyObject.boundVariables:
                freeVariableMemberAccessChainsWithPositions.add(
                    PyAstFreeVariableAnalyses.VarWithPosition(var=(val,), pos=pos)
                    )

        try:
            freeVariableMemberAccessChainResolutions = \
                self._resolveFreeVariableMemberAccessChains(
                    freeVariableMemberAccessChainsWithPositions, pyObject.boundVariables
                    )
        except UnresolvedFreeVariableException as e:
            _convertUnresolvedFreeVariableExceptionAndRaise(e, pyObject.sourceFileName)

        try:
            processedFreeVariableMemberAccessChainResolutions = {}
            for chain, (resolution, position) in \
                freeVariableMemberAccessChainResolutions.iteritems():
                processedFreeVariableMemberAccessChainResolutions['.'.join(chain)] = \
                    self.walkPyObject(resolution)
        except UnresolvedFreeVariableExceptionWithTrace as e:
            e.addToTrace(
                Exceptions.makeTraceElement(
                    path=pyObject.sourceFileName,
                    lineNumber=position.lineno
                    )
                )
            raise

        sourceFileId = self.walkPyObject(
            _FileDescription.cachedFromArgs(
                fileName=pyObject.sourceFileName
                )
            )

        self._objectRegistry.defineWithBlock(
            objectId=objectId,
            freeVariableMemberAccessChainsToId=\
                processedFreeVariableMemberAccessChainResolutions,
            sourceFileId=sourceFileId,
            lineNumber=lineNumber
            )

    def _registerFunction(self, objectId, function):
        """
        `_registerFunction`: register a python function with `self.objectRegistry.

        Recursively call `walkPyObject` on the resolvable free variable member
        access chains in the function, as well as on the source file object.
        """
        functionDescription = self._classOrFunctionDefinition(
            function,
            classOrFunction=_FunctionDefinition
            )

        self._objectRegistry.defineFunction(
            objectId=objectId,
            sourceFileId=functionDescription.sourceFileId,
            lineNumber=functionDescription.lineNumber,
            scopeIds=functionDescription.freeVariableMemberAccessChainsToId
            )

    def _registerClass(self, objectId, pyObject):
        """
        `_registerClass`: register a python class with `self.objectRegistry.

        Recursively call `walkPyObject` on the resolvable free variable member
        access chains in the class, as well as on the source file object.
        """
        classDescription = self._classOrFunctionDefinition(
            pyObject,
            classOrFunction=_ClassDefinition
            )

        for base in pyObject.__bases__:
            assert id(base) in self._pyObjectIdToObjectId, (base, pyObject)

        self._objectRegistry.defineClass(
            objectId=objectId,
            sourceFileId=classDescription.sourceFileId,
            lineNumber=classDescription.lineNumber,
            scopeIds=classDescription.freeVariableMemberAccessChainsToId,
            baseClassIds=[self._pyObjectIdToObjectId[id(base)] for base in pyObject.__bases__]
            )

    def _registerUnconvertible(self, objectId, modulePath):
        self._objectRegistry.defineUnconvertible(objectId, modulePath)

    def _classOrFunctionDefinition(self, pyObject, classOrFunction):
        """
        `_classOrFunctionDefinition: create a `_FunctionDefinition` or
        `_ClassDefinition` out of a python class or function, recursively visiting
        the resolvable free variable member access chains in `pyObject` as well
        as the source file object.

        Args:
            `pyObject`: a python class or function.
            `classOrFunction`: should either be `_FunctionDefinition` or
                `_ClassDefinition`.

        Returns:
            a `_FunctionDefinition` or `_ClassDefinition`.

        """
        if pyObject.__name__ == '__inline_fora':
            raise Exceptions.PythonToForaConversionError(
                "in pyfora, '__inline_fora' is a reserved word"
                )

        sourceFileText, sourceFileName = PyAstUtil.getSourceFilenameAndText(pyObject)

        _, sourceLine = PyAstUtil.getSourceLines(pyObject)

        sourceAst = PyAstUtil.pyAstFromText(sourceFileText)

        if classOrFunction is _FunctionDefinition:
            pyAst = PyAstUtil.functionDefOrLambdaAtLineNumber(sourceAst, sourceLine)
        else:
            assert classOrFunction is _ClassDefinition
            pyAst = PyAstUtil.classDefAtLineNumber(sourceAst, sourceLine)

        assert sourceLine == pyAst.lineno

        try:
            freeVariableMemberAccessChainResolutions = \
                self._computeAndResolveFreeVariableMemberAccessChainsInAst(
                    pyObject, pyAst
                    )
        except UnresolvedFreeVariableException as e:
            _convertUnresolvedFreeVariableExceptionAndRaise(e, sourceFileName)

        try:
            processedFreeVariableMemberAccessChainResolutions = {}
            for chain, (resolution, location) in \
                freeVariableMemberAccessChainResolutions.iteritems():
                processedFreeVariableMemberAccessChainResolutions['.'.join(chain)] = \
                    self.walkPyObject(resolution)
        except UnresolvedFreeVariableExceptionWithTrace as e:
            e.addToTrace(
                Exceptions.makeTraceElement(
                    path=sourceFileName,
                    lineNumber=location[0]
                    )
                )
            raise

        sourceFileId = self.walkPyObject(
            _FileDescription.cachedFromArgs(
                fileName=sourceFileName,
                fileText=sourceFileText
                )
            )

        return classOrFunction(
            sourceFileId=sourceFileId,
            lineNumber=sourceLine,
            freeVariableMemberAccessChainsToId=\
                processedFreeVariableMemberAccessChainResolutions
            )

    @staticmethod
    def _freeMemberAccessChainsWithPositions(pyAst):
        def is_pureMapping_call(node):
            return isinstance(node, ast.Call) and \
                isinstance(node.func, ast.Name) and \
                node.func.id == 'pureMapping'

        freeVariableMemberAccessChains = \
            PyAstFreeVariableAnalyses.getFreeVariableMemberAccessChains(
                pyAst,
                isClassContext=False,
                getPositions=True,
                exclude_predicate=is_pureMapping_call
                )

        return freeVariableMemberAccessChains

    def _resolveChainByDict(self, chainWithPosition, boundVariables):
        """
        `_resolveChainByDict`: look up a free variable member access chain, `chain`,
        in a dictionary of resolutions, `boundVariables`, or in `__builtin__` and
        return a tuple (subchain, resolution, location).
        """
        freeVariable = chainWithPosition.var[0]

        if freeVariable in boundVariables:
            rootValue = boundVariables[freeVariable]

        elif hasattr(__builtin__, freeVariable):
            rootValue = getattr(__builtin__, freeVariable)

        else:
            raise UnresolvedFreeVariableException(chainWithPosition, None)

        return self._freeVariableResolver.computeSubchainAndTerminalValueAlongModules(
            rootValue, chainWithPosition)


    def _resolveFreeVariableMemberAccessChains(self,
                                               freeVariableMemberAccessChainsWithPositions,
                                               boundVariables):
        """ Return a dictionary mapping subchains to resolved ids."""
        resolutions = dict()

        for chainWithPosition in freeVariableMemberAccessChainsWithPositions:
            subchain, resolution, position = self._resolveChainByDict(
                chainWithPosition, boundVariables)

            if id(resolution) in self._convertedObjectCache:
                resolution = self._convertedObjectCache[id(resolution)][1]

            resolutions[subchain] = (resolution, position)

        return resolutions

    def _computeAndResolveFreeVariableMemberAccessChainsInAst(self, pyObject, pyAst):
        return self._freeVariableResolver.resolveFreeVariableMemberAccessChainsInAst(
            pyObject,
            pyAst,
            self._freeMemberAccessChainsWithPositions(pyAst),
            self._convertedObjectCache)

