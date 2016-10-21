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

#include <stdint.h>
#include <string>
#include <vector>


class Deserializer {
public:
    virtual ~Deserializer() = 0;

    virtual bool finished() = 0;

    virtual char readByte() = 0;
    virtual int32_t readInt32() = 0;
    virtual int64_t readInt64() = 0;
    virtual double readFloat64() = 0;
    virtual void readInt64s(std::vector<int64_t>& ioInts) = 0;
    virtual std::string readString() = 0;
};
