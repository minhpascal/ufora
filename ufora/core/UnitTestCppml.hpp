/***************************************************************************
   Copyright 2015 Ufora Inc.

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

#include "cppml/CPPMLPrettyPrinter.hppml"
#include "UnitTest.hpp"

#define BOOST_CHECK_EQUAL_CPPML(l,r) \
   BOOST_CHECK_MESSAGE(cppmlCmp(l,r) == 0, \
      prettyPrintString(l) << " != " << prettyPrintString(r) \
      )


#define BOOST_CHECK_EQUAL_CPPML_MESSAGE(l,r,m) \
   BOOST_CHECK_MESSAGE(cppmlCmp(l,r) == 0, \
      prettyPrintString(l) << " != " << prettyPrintString(r) << ": " << \
      prettyPrintString(m) \
      )

