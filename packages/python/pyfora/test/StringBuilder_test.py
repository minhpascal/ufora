#   Copyright 2016 Ufora Inc.
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

from pyfora.stringbuilder import StringBuilder as StringBuilderNative
from pyfora.BinaryObjectRegistry import StringBuilder as StringBuilderPure

import unittest
import numpy.random

class StringBuilderTest(unittest.TestCase):
    def assertStreamsEqual(self, s1, s2):
        self.assertEqual(s1.bytecount, s2.bytecount)
        self.assertEqual(s1.str(), s2.str())

    def test_addString_1(self):
        s = "asdf"

        stringBuilderNative = StringBuilderNative()
        stringBuilderPure = StringBuilderPure()

        stringBuilderNative.addString(s)
        stringBuilderPure.addString(s)
        
        self.assertStreamsEqual(stringBuilderNative,
                                stringBuilderPure)

    def test_addByte_1(self):
        byte_array = range(25)
        
        stringBuilderNative = StringBuilderNative()
        stringBuilderPure = StringBuilderPure()

        for b in byte_array:
            stringBuilderNative.addByte(b)
            stringBuilderPure.addByte(b)

        self.assertStreamsEqual(stringBuilderNative,
                                stringBuilderPure)

    def test_addInt32(self):
        ints = range(100) + [2147483647]

        stringBuilderNative = StringBuilderNative()
        stringBuilderPure = StringBuilderPure()

        for i in ints:
            stringBuilderNative.addInt32(i)
            stringBuilderPure.addInt32(i)

        self.assertStreamsEqual(stringBuilderNative,
                                stringBuilderPure)
        
    def test_addInt64(self):
        ints = [-9223372036854775808, 9223372036854775807, 0, 55]

        stringBuilderNative = StringBuilderNative()
        stringBuilderPure = StringBuilderPure()

        for i in ints:
            stringBuilderNative.addInt64(i)
            stringBuilderPure.addInt64(i)

        self.assertStreamsEqual(stringBuilderNative,
                                stringBuilderPure)

    def test_addFloat64(self):
        numpy.random.seed(42)
        floats = numpy.random.uniform(-1, 1, size=100)

        stringBuilderNative = StringBuilderNative()
        stringBuilderPure = StringBuilderPure()

        for f in floats:
            stringBuilderNative.addFloat64(f)
            stringBuilderPure.addFloat64(f)

        self.assertStreamsEqual(stringBuilderNative,
                                stringBuilderPure)        

    def test_addInt64s(self):
        ints = [-9223372036854775808, 9223372036854775807, 0, 55]

        stringBuilderNative = StringBuilderNative()
        stringBuilderPure = StringBuilderPure()

        stringBuilderNative.addInt64s(ints)
        stringBuilderPure.addInt64s(ints)

        s1 = stringBuilderNative.str()
        s2 = stringBuilderPure.str()

        self.assertEqual(len(s1), len(s2))
        self.assertStreamsEqual(stringBuilderNative,
                                stringBuilderPure)

    def test_addInt64s_error_1(self):
        ints = [-9223372036854775808, 9223372036854775807, 0, 55, "not_an_int"]

        stringBuilderNative = StringBuilderNative()

        with self.assertRaises(TypeError):
            stringBuilderNative.addInt64s(ints)

    def test_addInt64s_error_2(self):
        stringBuilderNative = StringBuilderNative()

        with self.assertRaises(TypeError):
            stringBuilderNative.addInt64s(1337)
   
    def test_addStringTuple(self):
        strings = ["cats", "dogs", "fish", "bats"]

        stringBuilderNative = StringBuilderNative()
        stringBuilderPure = StringBuilderPure()

        stringBuilderNative.addStringTuple(strings)
        stringBuilderPure.addStringTuple(strings)

        s1 = stringBuilderNative.str()
        s2 = stringBuilderPure.str()

        self.assertStreamsEqual(stringBuilderNative,
                                stringBuilderPure)

        
