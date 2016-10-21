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

#include "DeserializerBase.hpp"

#include <unistd.h>


class StringDeserializer: public Deserializer {
public:
    StringDeserializer(const std::vector<char>& data);
    StringDeserializer(const std::string& data);
    StringDeserializer(const char* data, size_t size);

    virtual ~StringDeserializer()
        {
        }

    virtual bool finished() {
        return mIndex >= mData.size();
        }

    virtual char readByte();
    virtual int32_t readInt32();
    virtual int64_t readInt64();
    virtual double readFloat64();
    virtual void readInt64s(std::vector<int64_t>& ioInts);
    virtual std::string readString();

private:
    std::vector<char> mData;
    uint64_t mIndex;
    };
