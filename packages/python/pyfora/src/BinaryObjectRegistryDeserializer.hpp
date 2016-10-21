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

#include <stdint.h>
#include <vector>


class BinaryObjectRegistry;
class Deserializer;


class BinaryObjectRegistryDeserializer {
public:
    static void deserializeFromStream(Deserializer* stream, 
                                      BinaryObjectRegistry& binaryObjectRegistry
                                      );

private:
    static void readSimplePrimitive(Deserializer* stream);
    static PyObject* readPrimitive(char code, Deserializer* stream);
    static void readInt64s(Deserializer* stream, std::vector<int64_t>& ioInts);
    static PyObject* readSimplePrimitive(Deserializer* stream);

};
