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

#include <map>
#include <set>
#include <stdint.h>
#include <string>

#include "StringBuilder.hpp"


class BinaryObjectRegistry {
public:
    const static uint8_t CODE_NONE=1;
    const static uint8_t CODE_INT=2;
    const static uint8_t CODE_LONG=3;
    const static uint8_t CODE_FLOAT=4;
    const static uint8_t CODE_BOOL=5;
    const static uint8_t CODE_STR=6;
    const static uint8_t CODE_LIST_OF_PRIMITIVES=7;
    const static uint8_t CODE_TUPLE=8;
    const static uint8_t CODE_PACKED_HOMOGENOUS_DATA=9;
    const static uint8_t CODE_LIST=10;
    const static uint8_t CODE_FILE=11;
    const static uint8_t CODE_DICT=12;
    const static uint8_t CODE_REMOTE_PY_OBJECT=13;
    const static uint8_t CODE_BUILTIN_EXCEPTION_INSTANCE=14;
    const static uint8_t CODE_NAMED_SINGLETON=15;
    const static uint8_t CODE_FUNCTION=16;
    const static uint8_t CODE_CLASS=17;
    const static uint8_t CODE_UNCONVERTIBLE=18;
    const static uint8_t CODE_CLASS_INSTANCE=19;
    const static uint8_t CODE_INSTANCE_METHOD=20;
    const static uint8_t CODE_WITH_BLOCK=21;
    const static uint8_t CODE_PY_ABORT_EXCEPTION=22;
    const static uint8_t CODE_STACKTRACE_AS_JSON=23;

public:
    BinaryObjectRegistry();

    uint64_t bytecount() const {
        return mStringBuilder.bytecount();
        }

    std::string str() const {
        return mStringBuilder.str();
        }

    void clear() {
        mStringBuilder.clear();
        }
        
    inline uint64_t allocateObject();

    template<class T>
    void definePrimitive(uint64_t objectId, const T& t) {
        mStringBuilder.addInt64(objectId);
        _writePrimitive(t);
        }

    inline void defineEndOfStream();
    inline void defineTuple(uint64_t objectId,
                            const std::vector<uint64_t>& memberIds);
    inline void defineTuple(uint64_t objectId,
                            const uint64_t* memberIds,
                            uint64_t nMemberIds);
    inline void defineList(uint64_t objectId,
                           const std::vector<uint64_t>& memberIds);
    inline void defineList(uint64_t objectId,
                           const uint64_t* memberIds,
                           uint64_t nMemberIds);
    inline void defineFile(uint64_t objectId,
                           const std::string& text,
                           const std::string& path);
    void defineDict(uint64_t objectId,
                    const std::vector<uint64_t>& keyIds,
                    const std::vector<uint64_t>& valueIds);
    // TODO: DEFINE REMOTEPYTHONOBJECT???
    inline void defineBuiltinExceptionInstance(uint64_t objectId,
                                               const std::string& typeName,
                                               int64_t argsId);
    inline void defineNamedSingleton(uint64_t objectId,
                                     const std::string& singletonName);
    // TODO defineFunction
    // TODO defineClass
    void defineUnconvertible(uint64_t objectId, const std::string& modulePath);
    void defineClassInstance(
        uint64_t objectId,
        uint64_t classId,
        const std::map<std::string, uint64_t>& classMemberNameToClassMemberId);
    void defineInstanceMethod(uint64_t objectId,
                              uint64_t instanceId,
                              const std::string& methodName);
    // TODO defineWithBlock
    void definePyAbortException(uint64_t objectId,
                                const std::string& typeName,
                                uint64_t argsId);
    // TODO defineStackTrace

    template<class T>
    void definePackedHomogeneousData(uint64_t objectId,
                                     const T& dtype,
                                     const std::string& dataAsBytes)
        {
        mStringBuilder.addInt64(objectId);
        mStringBuilder.addByte(CODE_PACKED_HOMOGENOUS_DATA);

        _packedWrite(dtype);

        mStringBuilder.addString(dataAsBytes);
        }

    template<class T>
    void definePackedHomogeneousData(uint64_t objectId,
                                     const T& dtype,
                                     const char* dataAsBytes,
                                     uint64_t bytes)
        {
        mStringBuilder.addInt64(objectId);
        mStringBuilder.addByte(CODE_PACKED_HOMOGENOUS_DATA);

        _packedWrite(dtype);

        mStringBuilder.addString(dataAsBytes, bytes);
        }

    bool isUnconvertible(uint64_t objectId) {
        return mUnconvertibleIndices.find(objectId) != 
            mUnconvertibleIndices.end();
        }


private:
    StringBuilder mStringBuilder;
    uint64_t mNextObjectId;
    std::set<uint64_t> mUnconvertibleIndices;

    inline void _writePrimitive(bool b);
    inline void _writePrimitive(int32_t i);
    inline void _writePrimitive(int64_t l);
    inline void _writePrimitive(double d);
    inline void _writePrimitive(const std::string& s);

    inline void _packedWrite(int32_t i);
    inline void _packedWrite(int64_t i);
    inline void _packedWrite(const std::string& s);

    template<typename T>
    void _packedWrite(const std::vector<T>& dtype) {
        mStringBuilder.addByte(CODE_TUPLE);
        mStringBuilder.addInt32(dtype.size());
        for (typename std::vector<T>::const_iterator it = dtype.begin();
             it != dtype.end();
             ++it)
            _packedWrite(*it);
        }

    // DON'T KNOW HOW TO DEAL WITH PYTHON LONGS!

    template<typename T>
    void _writePrimitive(const std::vector<T>& primitives) {
        mStringBuilder.addByte(CODE_LIST_OF_PRIMITIVES);
        mStringBuilder.addInt64(primitives.size());
        for (typename std::vector<T>::const_iterator it = primitives.begin();
             it != primitives.end();
             ++it)
            _writePrimitive(*it);
        }

    };
