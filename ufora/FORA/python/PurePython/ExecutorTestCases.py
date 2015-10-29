#   Copyright 2015 Ufora Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import pyfora.Exceptions
import pyfora.PyAstUtil as PyAstUtil
import numpy
import time
import ufora.FORA.python.PurePython.EquivalentEvaluationTestCases as EquivalentEvaluationTestCases
import ufora.FORA.python.PurePython.ExceptionTestCases as ExceptionTestCases

class ExecutorTestCases(
            EquivalentEvaluationTestCases.EquivalentEvaluationTestCases,
            ExceptionTestCases.ExceptionTestCases
            ):
    """ExecutorTestCases - mixin to define test cases for the pyfora Executor cass."""

    def create_executor(self, allowCached=True):
        """Subclasses of the test harness should implement"""
        raise NotImplementedError()

    def evaluateWithExecutor(self, func, *args, **kwds):
        shouldClose = True
        if 'executor' in kwds:
            executor = kwds['executor']
            shouldClose = False
        else: 
            executor = self.create_executor()

        try: 
            func_proxy = executor.define(func).result()
            args_proxy = [executor.define(a).result() for a in args]
            res_proxy = func_proxy(*args_proxy).result()

            result = res_proxy.toLocal().result()
            return result
        except Exception as ex:
            raise ex
        finally:
            if shouldClose:
                executor.__exit__(None, None, None)


    @staticmethod
    def defaultComparison(x, y):
        if isinstance(x, basestring) and isinstance(y, basestring):
            return x == y

        if hasattr(x, '__len__') and hasattr(y, '__len__'):
            l1 = len(x)
            l2 = len(y)
            if l1 != l2:
                return False
            for idx in range(l1):
                if not ExecutorTestCases.defaultComparison(x[idx], y[idx]):
                    return False
            return True
        else:
            return x == y and type(x) is type(y)

    def equivalentEvaluationTest(self, func, *args, **kwds):
        comparisonFunction = ExecutorTestCases.defaultComparison
        if 'comparisonFunction' in kwds:
            comparisonFunction = kwds['comparisonFunction']

        with self.create_executor() as executor:
            t0 = time.time()
            func_proxy = executor.define(func).result()
            args_proxy = [executor.define(a).result() for a in args]
            res_proxy = func_proxy(*args_proxy).result()

            pyforaResult = res_proxy.toLocal().result()
            t1 = time.time()
            pythonResult = func(*args)
            t2 = time.time()

            self.assertTrue(
                comparisonFunction(pyforaResult, pythonResult), 
                "Pyfora and python returned different results: %s != %s for arguments %s" % (pyforaResult, pythonResult, args)
                )

            if t2 - t0 > 5.0:
                print "Pyfora took ", t1 - t0, ". python took ", t2 - t1

        return pythonResult

    def test_string_indexing(self):
        def f():
            a = "abc"
            return (a[0], a[1], a[2], a[-1], a[-2])
        self.equivalentEvaluationTest(f)

    def test_string_indexing_2(self):
        def f(idx):
            x = "asdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdf"
            return x[idx]

        self.equivalentEvaluationTest(f, -1)
        self.equivalentEvaluationTest(f, -2)
        self.equivalentEvaluationTest(f, 0)
        self.equivalentEvaluationTest(f, 1)

    def test_class_pass(self):
        def f():
            class X:
                pass

        self.equivalentEvaluationTest(f)

    def test_string_comparison(self):
        def f():
            a = "a"
            b = "b"
            r1 = a < b
            r2 = a > b
            return (r1, r2)

        self.equivalentEvaluationTest(f)

    def test_string_duplication(self):
        def f():
            a = "asdf"
            r1 = a * 20
            r2 = 20 * a
            return (r1, r2)

        self.equivalentEvaluationTest(f)

    def test_range_builtin_simple(self):
        def f(x):
            return range(x)

        self.equivalentEvaluationTest(f, 10)

    def test_xrange_builtin_simple(self):
        def f(x):
            toReturn = 0
            for ix in xrange(x):
                toReturn = ix + toReturn
            return toReturn
        self.equivalentEvaluationTest(f, 10)

    def test_range_builtin_overloads(self):
        def f(start, stop, incr=1):
            return range(start, stop, incr)

        self.equivalentEvaluationTest(f, 1, 10)
        self.equivalentEvaluationTest(f, 10, 5)
        self.equivalentEvaluationTest(f, 5, 10)
        self.equivalentEvaluationTest(f, 10, 1, 2)
        self.equivalentEvaluationTest(f, 10, 5, 5)
        self.equivalentEvaluationTest(f, 10, 10, 10)

    def equivalentEvaluationTestThatHandlesExceptions(self, func, *args):
        with self.create_executor() as executor:
            try:
                r1 = func(*args)
                pythonSucceeded = True
            except Exception as ex:
                pythonSucceeded = False
                
            try:
                r2 = self.evaluateWithExecutor(func, *args, executor=executor)
                pyforaSucceeded = True
            except pyfora.Exceptions.ComputationError as ex:
                pyforaSucceeded = False

            self.assertEqual(pythonSucceeded, pyforaSucceeded)
            if pythonSucceeded:
                self.assertEqual(r1, r2)
                return r1

    def test_slicing_operations_1(self):
        a = "testing"
        l = len(a)
        with self.create_executor() as fora:
            pythonResults = []
            futures = []
            for idx1 in range(-l - 1, l + 1):
                for idx2 in range(-l - 1, l + 1):
                    def f():
                        return a[idx1:idx2]
                    futures.append(
                        fora.submit(f)
                        )
                    try:
                        result = f()
                        pythonResults.append(result)
                    except:
                        pass
        foraResults = self.resolvedFutures(futures)
        self.assertTrue(len(foraResults) > (l * 2 + 1) ** 2)
        self.assertEqual(pythonResults, foraResults)


    def resolvedFutures(self, futures):
        results = [f.result() for f in futures]
        localResults = [r.toLocal() for r in results]
        foraResults = []
        for f in localResults:
            try:
                foraResults.append(f.result())
            except Exception as e:
                pass
        return foraResults        

    def test_slicing_operations_2(self):
        a = "abcd"
        l = len(a) + 1
        with self.create_executor() as fora:
            pythonResults = []
            futures = []
            for idx1 in range(-l, l):
                for idx2 in range(-l, l):
                    for idx3 in range(-l, l):
                        def f():
                            return a[idx1:idx2:idx3]
                        futures.append(
                            fora.submit(f)
                            )
                        try:
                            result = f()
                            pythonResults.append(result)
                        except:
                            pass

            foraResults = self.resolvedFutures(futures)
            # we are asserting that we got a lot of results
            # we don't expect to have (l * 2 -1)^3, because we expect some of the 
            # operations to fail
            self.assertTrue(len(foraResults) > 500)
            self.assertEqual(pythonResults, foraResults)

    def test_ord_chr_builtins(self):
        def f():
            chars = [chr(val) for val in range(40, 125)]
            vals = [ord(val) for val in chars]
            return (chars, vals)

        r = self.equivalentEvaluationTest(f)

    def test_python_if_int(self):
        def f():
            if 1:
                return True
            else:
                return False
        self.equivalentEvaluationTest(f)

    def test_python_if_int_2(self):
        def f2():
            if 0:
                return True
            else:
                return False
        self.equivalentEvaluationTest(f2)

    def test_python_and_or(self):
        def f():
            return (
                0 or 1,
                1 or 2,
                1 or 0,
                0 or False, 
                1 or 2 or 3, 
                0 or 1,
                0 or 1 or 2,
                1 and 2,
                0 and 1,
                1 and 0,
                0 and False,
                1 and 2 and 3,
                0 and 1 and 2,
                1 and 2 and 0,
                1 and 0 and 2,
                0 and False and 2
                )

        self.equivalentEvaluationTest(f)

    def test_slicing_operations_3(self):
        a = "abcd"
        l = len(a)
        l2 = -l + 1
        def f():
            toReturn = []
            for idx1 in range(l2, l + 1):
                for idx2 in range(l2, l + 1):
                    for idx3 in range(l2, l + 1):
                        if(idx3 != 0):
                            r = a[idx1:idx2:idx3]
                            toReturn = toReturn + [r]
            return toReturn
        self.equivalentEvaluationTestThatHandlesExceptions(f)

    def test_string_equality_methods(self):
        def f():
            a = "val1"
            b = "val1"
            r1 = a == b
            r2 = a != b
            a = "val2"
            r3 = a == b
            r4 = a != b
            r5 = a.__eq__(b)
            r6 = a.__ne__(b)
            return (r1, r2, r3, r4, r5, r6)

        self.equivalentEvaluationTest(f)

    def test_nested_function_arguments(self):
        def c(v1, v2):
            return v1 + v2
        def b(v, f):
            return f(v, 8)
        def a(v):
            return b(v, c)

        self.equivalentEvaluationTest(a, 10)

    def test_default_arguments_1(self):
        def f(a=None):
            return a

        self.equivalentEvaluationTest(f, 0)
        self.equivalentEvaluationTest(f, None)
        self.equivalentEvaluationTest(f, -3.3)
        self.equivalentEvaluationTest(f, "testing")

    def test_default_arguments_2(self):
        def f(a, b=1, c=None):
            return (a, b, c)

        self.equivalentEvaluationTest(f, 0, None)
        self.equivalentEvaluationTest(f, None, 2)
        self.equivalentEvaluationTest(f, None, "test")
        self.equivalentEvaluationTest(f, -3.3)
        self.equivalentEvaluationTest(f, "test", "test")

    def test_handle_empty_list(self):
        def f():
            return []
        self.equivalentEvaluationTest(f)

    def test_zero_division_should_throw(self):
        def f1():
            return 4 / 0

        with self.assertRaises(pyfora.Exceptions.ComputationError):
            self.evaluateWithExecutor(f1)

        def f2():
            return 4.0 / 0

        with self.assertRaises(pyfora.Exceptions.ComputationError):
            self.evaluateWithExecutor(f2)

        def f3():
            return 4 / 0.0

        with self.assertRaises(pyfora.Exceptions.ComputationError):
            self.evaluateWithExecutor(f3)

        def f4():
            return 4.0 / 0.0

        with self.assertRaises(pyfora.Exceptions.ComputationError):
            self.evaluateWithExecutor(f4)

    def test_builtins_abs(self):
        def f(x):
            return abs(x)
        for x in range(-10, 10):
            self.equivalentEvaluationTest(f, x)
            
        self.equivalentEvaluationTest(f, True)
        self.equivalentEvaluationTest(f, False)
        with self.assertRaises(pyfora.Exceptions.ComputationError):
            self.evaluateWithExecutor(f, [])
        with self.assertRaises(pyfora.Exceptions.ComputationError):
            self.evaluateWithExecutor(f, ["test"])
        with self.assertRaises(pyfora.Exceptions.ComputationError):
            self.evaluateWithExecutor(f, "test")

    def test_builtins_all(self):
        def f(x):
            return all(x)
        self.equivalentEvaluationTest(f, [])
        self.equivalentEvaluationTest(f, [True])
        self.equivalentEvaluationTest(f, [True, True])
        self.equivalentEvaluationTest(f, [True, False])
        self.equivalentEvaluationTest(f, [False, True])
        self.equivalentEvaluationTest(f, [False, False])

    def test_large_strings(self):
        def f():
            a = "val1"

            while len(a) < 1000000:
                a = a + a

            return a

        self.equivalentEvaluationTest(f)

    def test_numpy(self):
        n = numpy.zeros(10)
        def f():
            return n.shape
        self.equivalentEvaluationTest(f)

    def test_return_numpy(self):
        n = numpy.zeros(10)
        def f():
            return n
        res = self.evaluateWithExecutor(f)

        self.assertTrue(isinstance(res, numpy.ndarray), res)

    def test_return_list(self):
        def f():
            return [1,2,3,4,5]

        self.equivalentEvaluationTest(f)

    def test_list_in_loop(self):
        def f(ct):
            ix = 0
            l = []
            while ix < ct:
                l = l + [ix]
                ix = ix + 1

            res = 0
            for e in l:
                res = res + e
            return res

        ct = 1000000
        res = self.evaluateWithExecutor(f, ct)
        self.assertEqual(res, ct * (ct-1)/2)

    def test_list_getitem(self):
        def f():
            l = [1,2,3]

            return l[0]

        self.equivalentEvaluationTest(f)

    def test_list_len(self):
        def f():
            l = [1,2,3]

            return (len(l), len(l) == 3, len(l) is 3)

        self.equivalentEvaluationTest(f)

    def test_tuple_conversion(self):
        def f(x):
            return (x,x+1)

        self.equivalentEvaluationTest(f, 10)

    def test_len(self):
        def f(x):
            return len(x)

        self.equivalentEvaluationTest(f, "asdf")

    def test_TrueFalseNone(self):
        def f():
            return (True, False, None)

        self.equivalentEvaluationTest(f)

    def test_returns_len(self):
        def f():
            return len

        res = self.evaluateWithExecutor(f)
        self.assertIs(res, f())

    def test_returns_str(self):
        def f():
            return str

        res = self.evaluateWithExecutor(f)
        self.assertIs(str, f())

    def test_str_on_class(self):
        def f():
            class StrOnClass:
                def __str__(self):
                    return "special"
            return str(StrOnClass())

        self.equivalentEvaluationTest(f)

    def test_define_constant(self):
        x = 4
        with self.create_executor() as executor:
            define_x = executor.define(x)
            fora_x = define_x.result()
            self.assertIsNotNone(fora_x)

    def test_define_calculation_on_prior_value(self):
        x = 4
        with self.create_executor() as executor:
            fora_x = executor.define(x).result()

            def f(x):
                return x + 1

            fora_f = executor.define(f).result()

            fora_f_of_x = fora_f(fora_x).result()

            fora_f_of_f_of_x = fora_f(fora_f_of_x).result()

            self.assertEqual(fora_f_of_f_of_x.toLocal().result(), 6)

    def test_define_calculation_on_prior_defined_value_in_closure(self):
        x = 4
        with self.create_executor() as executor:
            fora_x = executor.define(x).result()

            def f():
                return fora_x + 1

            fora_f = executor.define(f).result()

            self.assertEqual(fora_f().result().toLocal().result(), 5)

    def test_define_calculation_on_prior_calculated_value_in_closure(self):
        x = 4
        with self.create_executor() as executor:
            fora_x = executor.define(x).result()

            def f(x):
                return fora_x + 1

            fora_f = executor.define(f).result()

            fora_f_x = fora_f(fora_x).result()

            def f2():
                return fora_f_x + 1

            fora_f2 = executor.define(f2).result()

            self.assertEqual(fora_f2().result().toLocal().result(), 6)

    def test_define_constant_string(self):
        x = "a string"
        with self.create_executor() as executor:
            define_x = executor.define(x)
            fora_x = define_x.result()
            self.assertIsNotNone(fora_x)

    def test_compute_string(self):
        def f():
            return "a string"

        remote = self.evaluateWithExecutor(f)
        self.assertEqual(f(), remote)
        self.assertTrue(isinstance(remote, str))

    def test_compute_float(self):
        def f():
            return 1.5

        remote = self.evaluateWithExecutor(f)
        self.assertEqual(f(), remote)
        self.assertTrue(isinstance(remote, float))

    def test_compute_nothing(self):
        def f():
            return None

        remote = self.evaluateWithExecutor(f)
        self.assertIs(f(), remote)

    def test_define_function(self):
        def f(x):
            return x+1
        arg = 4

        with self.create_executor() as executor:
            f_proxy = executor.define(f).result()
            arg_proxy = executor.define(arg).result()

            res_proxy = f_proxy(arg_proxy).result()
            self.assertEqual(res_proxy.toLocal().result(), 5)

    def test_class_member_functions(self):
        class ClassTest0:
            def __init__(self,x):
                self.x = x

            def f(self):
                return 10

        def testFun():
            c = ClassTest0(10)
            return c.x

        self.equivalentEvaluationTest(testFun)

    def test_primitives_know_they_are_pyfora(self):
        def testFun():
            x = 10
            return x.__is_pyfora__

        self.assertTrue(self.evaluateWithExecutor(testFun))

    def test_classes_know_they_are_pyfora(self):
        class ClassTest2:
            def __init__(self):
                pass

        def testFun():
            c = ClassTest2()
            return c.__is_pyfora__

        self.assertTrue(self.evaluateWithExecutor(testFun))

    def test_class_member_semantics(self):
        def f():
            return 'free f'

        y = 'free y'

        class ClassTest1:
            def __init__(self, y):
                self.y = y

            def f(self):
                return ('member f', y, self.y)

            def g(self):
                return (f(), self.f())

        def testFun():
            c = ClassTest1('class y')
            return c.g()

        self.equivalentEvaluationTest(testFun)

    def test_repeatedEvaluation(self):
        def f(x):
            return x+1
        arg = 4

        for _ in range(10):
            with self.create_executor() as executor:
                f_proxy = executor.define(f).result()
                arg_proxy = executor.define(arg).result()

                res_proxy = f_proxy(arg_proxy).result()
                self.assertEqual(res_proxy.toLocal().result(), f(arg))

    def test_returnClasses(self):
        class ReturnedClass:
            def __init__(self, x):
                self.x = x

        def f(x):
            return ReturnedClass(x)

        shouldBeReturnedClass = self.evaluateWithExecutor(f, 10)

        self.assertEqual(shouldBeReturnedClass.x, 10)
        self.assertEqual(str(shouldBeReturnedClass.__class__), str(ReturnedClass))
        

    def test_returnFunctions(self):
        y = 2
        def toReturn(x):
            return x * y

        def f():
            return toReturn

        shouldBeToReturn = self.evaluateWithExecutor(f)

        self.assertEqual(shouldBeToReturn(10), toReturn(10))
        self.assertEqual(str(shouldBeToReturn.__name__), str(toReturn.__name__))
        self.assertEqual(PyAstUtil.getSourceText(shouldBeToReturn), PyAstUtil.getSourceText(toReturn))

    def test_returnClassObject(self):
        class ReturnedClass2:
            @staticmethod
            def f():
                return 10

        def f():
            return ReturnedClass2

        def comparisonFunction(pyforaVal, pythonVal):
            return pyforaVal.f() == pythonVal.f()

        self.equivalentEvaluationTest(f, comparisonFunction=comparisonFunction)

    def test_returnDict(self):
        x = { 1: 2, 3: 4, 5: 6, 7: 8, 9: 10, 11: 12 }

        def f():
            return x

        self.equivalentEvaluationTest(f, comparisonFunction=lambda x, y: x == y)

    def test_returnClassObjectWithClosure(self):
        x = 10
        class ReturnedClass3:
            def f(self, y):
                return x + y

        def f():
            return ReturnedClass3

        def comparisonFunction(pyforaVal, pythonVal):
            return pyforaVal().f(10) == pythonVal().f(10)

        self.equivalentEvaluationTest(f, comparisonFunction=comparisonFunction)

    def test_define_complicated_function(self):
        with self.create_executor() as executor:
            y = 1
            z = 2
            w = 3
            def h(x):
                return w + 2 * x
            def f(x):
                if x < 0:
                    return x
                return y + g(x - 1) + h(x)
            def g(x):
                if x < 0:
                    return x
                return z * f(x - 1) + h(x - 1)

            arg = 4
            res_proxy = executor.submit(f, arg).result()
            self.assertEqual(res_proxy.toLocal().result(), f(arg))


    def test_cancellation(self):
        with self.create_executor() as executor:
            def f(x):
                i = 0
                while i < x:
                    i = i + 1
                return i
            arg = 100000000000

            future = executor.submit(f, arg)
            self.assertFalse(future.done())
            self.assertTrue(future.cancel())
            self.assertTrue(future.cancelled())
            with self.assertRaises(pyfora.Exceptions.CancelledError):
                future.result()


    def test_divide_by_zero(self):
        with self.create_executor() as executor:
            def f(x):
                return 1/x
            arg = 0

            future = executor.submit(f, arg)
            with self.assertRaises(pyfora.Exceptions.PyforaError):
                future.result().toLocal().result()
                

    def test_invalid_apply(self):
        with self.create_executor() as executor:
            def f(x):
                return x[0]
            arg = 0

            future = executor.submit(f, arg)
            with self.assertRaises(pyfora.Exceptions.ComputationError):
                try:
                    print "result=",future.result()
                    print future.result().toLocal().result()
                except Exception as e:
                    print e
                    raise

    def test_conversion_error(self):
        with self.create_executor() as executor:
            def f(x):
                y = [1, 2, 3, 4]
                y[1] = x
                return y

            future = executor.define(f)
            with self.assertRaises(pyfora.Exceptions.PythonToForaConversionError):
                future.result()

    def test_pass_returns_None(self):
        with self.create_executor() as executor:
            def f():
                pass

            self.assertIs(self.evaluateWithExecutor(f), None)

    def test_run_off_end_of_function_returns_None(self):
        with self.create_executor() as executor:
            def f():
                x = 10

            self.assertIs(self.evaluateWithExecutor(f), None)

    def test_run_off_end_of_class_member_function_returns_None(self):
        with self.create_executor() as executor:
            def f():
                class X2:
                    def f(self):
                        x = 10

                return X2().f()

            self.assertIs(self.evaluateWithExecutor(f), None)

    def test_class_member_function_return_correct(self):
        with self.create_executor() as executor:
            def f():
                class X2:
                    def f(self):
                        return 10

                return X2().f()

            self.assertIs(self.evaluateWithExecutor(f), 10)

    def test_run_off_end_of_class_member_function_returns_None_2(self):
        with self.create_executor() as executor:
            class X3:
                def f(self):
                    x = 10

            def f():
                return X3().f()

            self.assertIs(self.evaluateWithExecutor(f), None)

    def test_empty_return_returns_None(self):
        with self.create_executor() as executor:
            def f():
                return

            self.assertIs(self.evaluateWithExecutor(f), None)

    def test_negate_int(self):
        with self.create_executor() as executor:
            def f(): return -10
            self.equivalentEvaluationTest(f)

    def test_sum_xrange(self):
        with self.create_executor() as executor:
            arg = 1000000000
            def f():
                return sum(xrange(arg))

            self.assertEqual(self.evaluateWithExecutor(f), arg*(arg-1)/2)

    def test_xrange(self):
        with self.create_executor() as executor:
            low = 0
            for high in [None] + range(-7,7):
                for step in [-3,-2,-1,None,1,2,3]:
                    print low, high, step
                    if high is None:
                        self.equivalentEvaluationTest(range, low)
                    elif step is None:
                        self.equivalentEvaluationTest(range, low, high)
                    else:
                        self.equivalentEvaluationTest(range, low, high, step)


    def test_jsonConversionError(self):
        with self.create_executor(allowCached=False) as executor:
            def f():
                pass

            errorMsg = "I always throw!"
                
            class MyException(Exception):
                pass

            def alwaysThrows(*args):
                raise MyException(errorMsg)

            # to make our a toLocal call throw an expected exception
            executor.objectRehydrator.convertJsonResultToPythonObject = alwaysThrows

            func_proxy = executor.define(f).result()
            res_proxy = func_proxy().result()

            try:
                res_proxy.toLocal().result()
            except Exception as e:
                self.assertIsInstance(e, pyfora.Exceptions.PyforaError)
                self.assertIsInstance(e.message, MyException)
                self.assertEqual(e.message.message, errorMsg)

    def test_iterable_is_pyfora_object(self):
        def it(x):
            while x > 0:
                yield x
                x = x - 1

        def f():
            return it(10).__is_pyfora__
        
        self.assertIs(self.evaluateWithExecutor(f), True)

    def test_free_function_is_pyfora_object(self):
        def g():
            return 10
        def f():
            return g.__is_pyfora__
        
        self.assertIs(self.evaluateWithExecutor(f), True)

    def test_local_function_is_pyfora_object(self):
        def f():
            def g():
                pass
            return g.__is_pyfora__
        
        self.assertIs(self.evaluateWithExecutor(f), True)

    def test_list_on_iterable(self):
        def it(x):
            while x > 0:
                yield x
                x = x - 1

        def f1():
            return list(xrange(10))
        
        def f2():
            return list(it(10))

        self.equivalentEvaluationTest(f1)
        self.equivalentEvaluationTest(f2)

    def test_member_access(self):
        def g():
            return 10
        def f():
            return g().__str__()
        
        self.equivalentEvaluationTest(f)

    def test_convert_lambda_external(self):
        g = lambda: 10
        def f():
            return g()
        
        self.equivalentEvaluationTest(f)

    def test_convert_lambda_internal(self):
        def f():
            g = lambda: 10
            return g()
        
        self.equivalentEvaluationTest(f)

    def test_evaluate_lambda_directly(self):
        self.equivalentEvaluationTest(lambda x,y: x+y, 10, 20)

    def test_return_lambda(self):
        def f():
            return lambda: 10
        
        self.assertEqual(self.evaluateWithExecutor(f)(), 10)

    def test_GeneratorExp_works(self):
        self.equivalentEvaluationTest(lambda: list(x for x in xrange(10)))

    def test_is_returns_true(self):
        self.equivalentEvaluationTest(lambda x: x is 10, 10)
        self.equivalentEvaluationTest(lambda x: x is 10, 11)

    def test_sum_on_generator(self):
        class Generator1:
            def __pyfora_generator__(self):
                return self

            def __init__(self, x, y, func):
                self.x = x
                self.y = y
                self.func = func

            def __iter__(self):
                yield self.func(self.x)

            def canSplit(self):
                return self.x + 1 < self.y

            def split(self):
                if not self.canSplit():
                    return None

                return (
                    Generator1(self.x, (self.x+self.y)/2, self.func),
                    Generator1((self.x+self.y)/2, self.y, self.func)
                    )

            def map(self, mapFun):
                newMapFun = lambda x: mapFun(self.func(x))
                return Generator1(self.x, self.y, newMapFun)

        def f():
            return sum(Generator1(0, 100, lambda x:x))

        self.assertEqual(f(), 0)
        self.assertEqual(self.evaluateWithExecutor(f), sum(xrange(100)))
    
    def test_tuples_are_pyfora_objects(self):
        def f():
            return (1,2,3).__is_pyfora__
        
        self.assertTrue(self.evaluateWithExecutor(f))

    def test_list_generators_splittable(self):
        def f():
            return [1,2,3].__pyfora_generator__().canSplit()
        
        self.assertTrue(self.evaluateWithExecutor(f))

    def test_list_generators_splittable(self):
        def f():
            return [1,2,3].__pyfora_generator__().canSplit()
        
        self.assertTrue(self.evaluateWithExecutor(f))

    def test_list_generators_mappable(self):
        def f():
            return list([1,2,3].__pyfora_generator__().map(lambda z:z*2)) == [2,4,6]
        
        self.assertTrue(self.evaluateWithExecutor(f))

    def test_iterate_split_xrange(self):
        def f():
            g = xrange(100).__pyfora_generator__().split()[0]
            res = []
            for x in g:
                res = res + [x]

            return res == range(50)

        self.assertTrue(self.evaluateWithExecutor(f))

    def test_iterate_split_mapped_xrange(self):
        def f():
            g = xrange(100).__pyfora_generator__().map(lambda x:x).split()[0]
            res = []
            for x in g:
                res = res + [x]

            return res == range(50)

        self.assertTrue(self.evaluateWithExecutor(f))

    def test_iterate_map_split_xrange(self):
        def f():
            g = xrange(100).__pyfora_generator__().split()[0].map(lambda x:x)
            res = []
            for x in g:
                res = res + [x]

            return res == range(50)

        self.assertTrue(self.evaluateWithExecutor(f))

    def test_iterate_xrange(self):
        def f():
            res = []

            for x in xrange(50):
                res = res + [x]

            return res == range(50)

        self.assertTrue(self.evaluateWithExecutor(f))

    def test_iterate_xrange_generator(self):
        def f():
            res = []

            for x in xrange(50).__pyfora_generator__().map(lambda x:x):
                res = res + [x]

            return res == range(50)

        self.assertTrue(self.evaluateWithExecutor(f))

    def test_iterate_xrange_empty(self):
        def f():
            res = []

            for x in xrange(0):
                res = res + [x]

            return res == []

        self.assertTrue(self.evaluateWithExecutor(f))

    def test_list_on_xrange(self):
        for ct in [0,1,2,4,8,16,32,64,100,101,102,103]:
            self.equivalentEvaluationTest(lambda: sum(x for x in xrange(ct)))
            self.equivalentEvaluationTest(lambda: list(x for x in xrange(ct)))
            self.equivalentEvaluationTest(lambda: [x for x in xrange(ct)])

    def test_sum_isPrime(self):
        def isPrime(p):
            x = 2
            while x*x <= p:
                if p%x == 0:
                    return 0
                x = x + 1
            return x

        self.equivalentEvaluationTest(lambda: sum(isPrime(x) for x in xrange(1000000)))

