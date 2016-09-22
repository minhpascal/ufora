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


BinaryObjectRegistry::BinaryObjectRegistry() : mNextObjectId(0) 
    {
    }
    

uint64_t BinaryObjectRegistry::allocateObject() {
    uint64_t objectId = mNextObjectId;
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


void BinaryObjectRegistry::defineEndOfStream() {
    mStringBuilder.addInt64(-1);
    }


void BinaryObjectRegistry::defineTuple(uint64_t objectId,
                                       const std::vector<uint64_t>& memberIds)
    {
    defineTuple(objectId,
                &memberIds[0],
                memberIds.size());
    }

    
void BinaryObjectRegistry::defineTuple(uint64_t objectId,
                                       const uint64_t* memberIds,
                                       uint64_t nMemberIds)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_TUPLE);
    mStringBuilder.addInt64s(memberIds, nMemberIds);
    }


void BinaryObjectRegistry::defineList(uint64_t objectId,
                                      const std::vector<uint64_t>& memberIds)
    {
    defineList(objectId,
               &memberIds[0],
               memberIds.size());
    }

    
void BinaryObjectRegistry::defineList(uint64_t objectId,
                                      const uint64_t* memberIds,
                                      uint64_t nMemberIds)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_LIST);
    mStringBuilder.addInt64s(memberIds, nMemberIds);
    }


void BinaryObjectRegistry::defineFile(uint64_t objectId,
                                      const std::string& text,
                                      const std::string& path)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_FILE);
    mStringBuilder.addString(path);
    mStringBuilder.addString(text);
    }


void BinaryObjectRegistry::defineDict(uint64_t objectId,
                                      const std::vector<uint64_t>& keyIds,
                                      const std::vector<uint64_t>& valueIds)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_DICT);
    mStringBuilder.addInt64s(keyIds);
    mStringBuilder.addInt64s(valueIds);
    }


void BinaryObjectRegistry::defineBuiltinExceptionInstance(
        uint64_t objectId,
        const std::string& typeName,
        int64_t argsId)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_BUILTIN_EXCEPTION_INSTANCE);
    mStringBuilder.addString(typeName);
    mStringBuilder.addInt64(argsId);
    }


void BinaryObjectRegistry::defineNamedSingleton(
        uint64_t objectId,
        const std::string& singletonName)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_NAMED_SINGLETON);
    mStringBuilder.addString(singletonName);
    }


void BinaryObjectRegistry::defineUnconvertible(
        uint64_t objectId,
        const std::string& modulePath)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_UNCONVERTIBLE);

    // modulePath non None case. not sure what to do with Nones ...
    mStringBuilder.addByte(1);
    mStringBuilder.addStringTuple(modulePath);
    mUnconvertibleIndices.insert(objectId);
    }


void BinaryObjectRegistry::defineClassInstance(
        uint64_t objectId,
        uint64_t classId,
        const std::map<std::string, uint64_t>& classMemberNameToClassMemberId)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_CLASS_INSTANCE);
    mStringBuilder.addInt64(classId);
    mStringBuilder.addInt32(classMemberNameToClassMemberId.size());
    for (std::map<std::string, uint64_t>::const_iterator it = 
             classMemberNameToClassMemberId.begin();
         it != classMemberNameToClassMemberId.end();
         ++it)
        {
        mStringBuilder.addString(it->first);
        mStringBuilder.addInt64(it->second);
        }
    }


void BinaryObjectRegistry::defineInstanceMethod(uint64_t objectId,
                                                uint64_t instanceId,
                                                const std::string& methodName)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_INSTANCE_METHOD);
    mStringBuilder.addInt64(instanceId);
    mStringBuilder.addString(methodName);
    }


void BinaryObjectRegistry::definePyAbortException(uint64_t objectId,
                                                  const std::string& typeName,
                                                  uint64_t argsId)
    {
    mStringBuilder.addInt64(objectId);
    mStringBuilder.addByte(CODE_PY_ABORT_EXCEPTION);
    mStringBuilder.addString(typeName);
    mStringBuilder.addInt64(argsId);
    }
