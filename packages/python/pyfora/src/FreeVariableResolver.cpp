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
#include "FreeVariableResolver.hpp"

#include <stdexcept>


FreeVariableResolver::FreeVariableResolver(
        PyObject* exclude_list,
        PyObject* terminal_value_filter
        )
    : mPureFreeVariableResolver(0),
      exclude_list(exclude_list),
      terminal_value_filter(terminal_value_filter)
    {
    Py_INCREF(exclude_list);
    Py_INCREF(terminal_value_filter);
    _initPureFreeVariableResolver();
    }


FreeVariableResolver::~FreeVariableResolver()
    {
    Py_XDECREF(mPureFreeVariableResolver);
    Py_XDECREF(terminal_value_filter);
    Py_XDECREF(exclude_list);
    }


void FreeVariableResolver::_initPureFreeVariableResolver()
    {
    PyObject* pyforaModule = PyImport_ImportModule("pyfora");
    if (pyforaModule == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't import pyforaModule");
        }
    
    PyObject* freeVariableResolverModule = PyObject_GetAttrString(
        pyforaModule,
        "FreeVariableResolver"
        );
    if (freeVariableResolverModule == NULL) {
        PyErr_Print();
        Py_DECREF(pyforaModule);
        throw std::logic_error("couldn't find FreeVariableResolver attr on pyfora");
        }

    PyObject* freeVariableResolverClass = PyObject_GetAttrString(
        freeVariableResolverModule,
        "FreeVariableResolver"
        );
    if (freeVariableResolverClass == NULL) {
        PyErr_Print();
        Py_DECREF(freeVariableResolverModule);
        Py_DECREF(pyforaModule);
        }

    mPureFreeVariableResolver = PyObject_CallFunctionObjArgs(
        freeVariableResolverClass,
        exclude_list,
        terminal_value_filter,
        NULL);

    if (mPureFreeVariableResolver == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't create a pure FreeVariableResolver instance");
        }
    }


PyObject* FreeVariableResolver::resolveFreeVariableMemberAccessChainsInAst(
        PyObject* pyObject,
        PyObject* pyAst,
        PyObject* freeMemberAccessChainsWithPositions,
        PyObject* convertedObjectCache) const
    {
    PyObject* resolveFreeVariableMemberAccessChainsInAstFun =
        PyObject_GetAttrString(mPureFreeVariableResolver,
                               "resolveFreeVariableMemberAccessChainsInAst");
    if (resolveFreeVariableMemberAccessChainsInAstFun == NULL) {
        PyErr_Print();
        throw std::logic_error("couldn't get "
            "resolveFreeVariableMemberAccessChainsInAst"
            " member on a FreeVariableResolver");
        }
    
    PyObject* res = PyObject_CallFunctionObjArgs(
        resolveFreeVariableMemberAccessChainsInAstFun,
        pyObject,
        pyAst,
        freeMemberAccessChainsWithPositions,
        convertedObjectCache,
        NULL);
    
    Py_DECREF(resolveFreeVariableMemberAccessChainsInAstFun);

    if (res == NULL) {
        PyErr_Print();
        throw std::logic_error("error calling "
            "resolveFreeVariableMemberAccessChainsInAst");
        }

    return res;
    }


