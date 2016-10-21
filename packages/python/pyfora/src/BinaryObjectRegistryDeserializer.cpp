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
#include "BinaryObjectRegistryDeserializer.hpp"
#include "DeserializerBase.hpp"


void BinaryObjectRegistryDeserializer::deserializeFromStream(
        Deserializer* stream,
        BinaryObjectRegistry& binaryObjectRegistry
        )
    {
    while (true) {
        int64_t objectId = stream->readInt64();

        if (objectId == -1) {
            return;
            }

        char code = stream->readByte();

        if (code == BinaryObjectRegistry::CODE_NONE or
            code == BinaryObjectRegistry::CODE_INT or
            code == BinaryObjectRegistry::CODE_LONG or
            code == BinaryObjectRegistry::CODE_FLOAT or
            code == BinaryObjectRegistry::CODE_BOOL or
            code == BinaryObjectRegistry::CODE_STR or
            code == BinaryObjectRegistry::CODE_LIST_OF_PRIMITIVES)
            {
            binaryObjectRegistry.definePrimitive(
                objectId,
                readPrimitive(code, stream)
                );
            }
        else if (code == BinaryObjectRegistry::CODE_TUPLE)
            {
            std::vector<int64_t> objectIds;
            readInt64s(stream, objectIds);
            binaryObjectRegistry.defineTuple(objectId, objectIds);
            }
        else if (code == BinaryObjectRegistry::CODE_PACKED_HOMOGENOUS_DATA)
            {
            /*
            dtype = readSimplePrimitive()
            packedBytes = stream.readString()
            objectVisitor.definePackedHomogenousData(objectId, TypeDescription.PackedHomogenousData(dtype, packedBytes))
             */

            PyObject* dtype = readSimplePrimitive(stream);
            std::string packedBytes = stream->readString();
            PyObject* packedHomogeneousData = NULL; // TypeDescription ...
            binaryObjectRegistry.definePackedHomogenousData(
                objectId,
                packedHomogeneousData);

            Py_XDECREF(packedHomogeneousData);
            Py_DECREF(dtype);
            }
        }
    }


void BinaryObjectRegistryDeserializer::readInt64s(
        Deserializer* stream,
        std::vector<int64_t>& ioInts
        )
    {
    int32_t nInts = stream->readInt32();
    for (int32_t ix = 0; ix < nInts; ++ix) {
        stream->readInt64();
        }
    }


PyObject* BinaryObjectRegistryDeserializer::readSimplePrimitive(
        Deserializer* stream
        )
    {
    char code = stream->readByte();
    
    if (code == BinaryObjectRegistry::CODE_NONE) {
        PyObject* pyNone = Py_None;
        Py_INCREF(pyNone);
        return pyNone;
        }
    if (code == BinaryObjectRegistry::CODE_INT) {
        int64_t id = stream->readInt64();
        }
    }
