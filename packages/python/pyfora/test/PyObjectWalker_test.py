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

from pyfora.binaryobjectregistry import BinaryObjectRegistry as BinaryObjectRegistryNative
from pyfora.BinaryObjectRegistry import BinaryObjectRegistry as BinaryObjectRegistryPure
import pyfora.PureImplementationMappings as PureImplementationMappings
import pyfora.PureImplementationMapping as PureImplementationMapping
from pyfora.PyObjectWalker import PyObjectWalker as PyObjectWalkerPure
from pyfora.pyobjectwalker import PyObjectWalker as PyObjectWalkerNative
import pyfora.NamedSingletons as NamedSingletons

import ast
import numpy
import time
import unittest


class SomeRandomInstance:
    pass


class PureUserWarning:
    pass


class PyObjectWalkerTest(unittest.TestCase):
    def setUp(self):
        self.excludeList = ["staticmethod", "property", "__inline_fora"]

        def is_pureMapping_call(node):
            return isinstance(node, ast.Call) and \
                isinstance(node.func, ast.Name) and \
                node.func.id == 'pureMapping'

        self.excludePredicateFun = is_pureMapping_call

        self.mappings = PureImplementationMappings.PureImplementationMappings()

        def terminal_value_filter(terminalValue):
            return not self.mappings.isOpaqueModule(terminalValue)

        self.terminalValueFilter = terminal_value_filter

    def test_cant_provide_mapping_for_named_singleton(self):

        #empty mappings work
        PyObjectWalkerPure(
            purePythonClassMapping=self.mappings,
            objectRegistry=None
            )

        self.mappings.addMapping(
            PureImplementationMapping.InstanceMapping(
                SomeRandomInstance(), SomeRandomInstance
                )
            )

        #an instance mapping doesn't cause an exception
        PyObjectWalkerPure(
            purePythonClassMapping=self.mappings,
            objectRegistry=None
            )

        self.assertTrue(UserWarning in NamedSingletons.pythonSingletonToName)

        self.mappings.addMapping(
            PureImplementationMapping.InstanceMapping(UserWarning, PureUserWarning)
            )

        #but this mapping doesnt
        with self.assertRaises(Exception):
            PyObjectWalkerPure(
                purePythonClassMapping=self.mappings,
                objectRegistry=None
                )

    def assertWalkersEquivalent(self, pyObject):
        mappings = PureImplementationMappings.PureImplementationMappings()

        objectRegistry1 = BinaryObjectRegistryNative()
        nativeWalker = PyObjectWalkerNative(mappings,
                                            objectRegistry1,
                                            self.excludePredicateFun,
                                            self.excludeList,
                                            self.terminalValueFilter)

        objectRegistry2 = BinaryObjectRegistryPure()
        pureWalker = PyObjectWalkerPure(mappings, objectRegistry2)

        nativeWalker.walkPyObject(pyObject)
        pureWalker.walkPyObject(pyObject)

        self.assertEqual(objectRegistry1.str(), objectRegistry2.str())

    def test_PyObjectWalkerNative_2(self):
        mappings = PureImplementationMappings.PureImplementationMappings()

        with self.assertRaises(TypeError):
            not_an_objectregistry = 2
            PyObjectWalkerNative(mappings, not_an_objectregistry, None, None, None)

        with self.assertRaises(TypeError):
            not_a_native_object_registry = BinaryObjectRegistryPure()
            PyObjectWalkerNative(mappings, not_a_native_object_registry, None, None, None)

    def test_PyObjectWalkerNative_strings_1(self):
        self.assertWalkersEquivalent("asdf")
            
    def test_PyObjectWalkerNative_lists_1(self):
        self.assertWalkersEquivalent([(1,2), 3.0, "asdf"])
            
    def test_PyObjectWalkerNative_dicts_1(self):
        obj = {'cats': 2.3, 100: ('dogs', {})}
        self.assertWalkersEquivalent(obj)

    def test_PyObjectWalkerNative_homogeneous_lists_1(self):
        obj = range(100)
        self.assertWalkersEquivalent(obj)

    def test_PyObjectWalkerNative_homogeneous_lists_2(self):
        obj = [1, 2.0,  None, "a string", False]
        self.assertWalkersEquivalent(obj)

    def test_PyObjectWalkerNative_numpy_array(self):
        obj = numpy.array(range(100))
        self.assertWalkersEquivalent(obj)

    def test_PyObjectWalker_perf(self):
        n = 1000000
        x = range(n)
        x = x + [float(elt) for elt in x]

        mappings = PureImplementationMappings.PureImplementationMappings()

        objectRegistry1 = BinaryObjectRegistryNative()
        nativeWalker = PyObjectWalkerNative(mappings, objectRegistry1, None, None, None)

        objectRegistry2 = BinaryObjectRegistryPure()
        pureWalker = PyObjectWalkerPure(mappings, objectRegistry2)

        t0 = time.time()
        nativeWalker.walkPyObject(x)
        native_time = time.time() - t0

        t0 = time.time()
        pureWalker.walkPyObject(x)
        pure_time = time.time() - t0

        self.assertEqual(objectRegistry1.str(), objectRegistry2.str())

        self.assertLessEqual(native_time * 45, pure_time)

    def test_PyObjectWalker_functions_1(self):
        def f(x):
            return x + 1

        self.assertWalkersEquivalent(f)

    def test_PyObjectWalker_functions_2(self):
        y = 2
        def f(x):
            return x + y

        self.assertWalkersEquivalent(f)

    def test_PyObjectWalker_classes_1(self):
        y = 2
        class A_4321:
            def __init__(self):
                pass
            def f(self, x):
                return x + 1

        self.assertWalkersEquivalent(A_4321)

    def test_PyObjectWalker_classes_2(self):
        y = 2
        class B_100392():
            def __init__(self):
                pass
            def f(self, x):
                return x + y

        self.assertWalkersEquivalent(B_100392)

    def test_PyObjectWalker_classes_3(self):
        class A_981723():
            def __init__(self):
                pass
            def g(self, x):
                return x + 1

        class B_43234(A_981723):
            def __init__(self):
                A_981723.__init__(self)

        self.assertWalkersEquivalent(A_981723)
        self.assertWalkersEquivalent(B_43234)

    def test_PyObjectWalker_classes_4(self):
        z = 3
        class A_34563(object):
            def __init__(self):
                pass
            def g(self, x):
                return x + z

        y = 4

        class B_77734(A_34563):
            def __init__(self):
                A_34563.__init__(self)
            def f(self, x):
                return self.g(x) + y

        self.assertWalkersEquivalent(A_34563)
        self.assertWalkersEquivalent(B_77734)

    def test_PyObjectWalker_classInstances_1(self):
        c = 5
        class A_10101(object):
            def __init__(self, x):
                self.x = x
            def f(self, y):
                return self.x + y + c

        a = A_10101(1024)

        self.assertWalkersEquivalent(a)

if __name__ == "__main__":
    unittest.main()

