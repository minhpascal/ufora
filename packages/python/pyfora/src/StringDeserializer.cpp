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
#include "StringDeserializer.hpp"


StringDeserializer::StringDeserializer(const std::vector<char>& data)
    : mData(data),
      mIndex(0)
    {
    }


StringDeserializer::StringDeserializer(const std::string& data)
    : mData(data.begin(), data.end()),
      mIndex(0)
    {
    }


StringDeserializer::StringDeserializer(const char* data, size_t size)
    : mData(data, data + size),
      mIndex(0)
    {
    }


char StringDeserializer::readByte()
    {
    char res = mData[mIndex];
    mIndex += 1;
    return res;
    }


int32_t StringDeserializer::readInt32() {
    int32_t res = *(reinterpret_cast<int32_t*>(&mData[mIndex]));
    mIndex += sizeof(int32_t);
    return res;
    }


int64_t StringDeserializer::readInt64() {
    int64_t res = *(reinterpret_cast<int64_t*>(&mData[mIndex]));
    mIndex += sizeof(int64_t);
    return res;
    }


double StringDeserializer::readFloat64() {
    double res = *(reinterpret_cast<double*>(&mData[mIndex]));
    mIndex += sizeof(double);
    return res;
    }
    

void StringDeserializer::readInt64s(std::vector<int64_t>& ioInts) {
    int64_t count = readInt64();
    for (int64_t ix = 0; ix < count; ++ix)
        ioInts.push_back(readInt64());
    }
    

std::string StringDeserializer::readString() {
    int64_t count = readInt32();
    std::string res = std::string(&mData[mIndex], count);
    mIndex += count;
    return res;
    }
