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

#include <string>


namespace PyObjectUtils {

std::string repr_string(PyObject* obj);
std::string str_string(PyObject* obj);

// string should be a PyString. no checking is done.
std::string std_string(PyObject* string);

std::string format_exc();

long builtin_id(PyObject*);

}
