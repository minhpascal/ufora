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

import unittest
import pickle
import ufora.distributed.S3.InMemoryS3Interface as InMemoryS3Interface
import ufora.cumulus.test.InMemoryCumulusSimulation as InMemoryCumulusSimulation
import ufora.test.PerformanceTestReporter as PerformanceTestReporter
import math

class GpuTestCases:
    def compareCudaToCPU(self, funcExpr, vecExpr):
        s3 = InMemoryS3Interface.InMemoryS3InterfaceFactory()

        text = """
            let f = __funcExpr__;
            let i = __vecExpr__;
            let cuda = `CUDAVectorApply(f, [i])[0];
            let cpu = f(i)

            if (cuda == cpu)
                true
            else
                throw String(cuda) + " != " + String(cpu)
            """.replace("__funcExpr__", funcExpr).replace("__vecExpr__", vecExpr)

        res = InMemoryCumulusSimulation.computeUsingSeveralWorkers(text, s3, 1, timeout = 120, threadCount=4)
        self.assertIsNotNone(res)
        self.assertTrue(res.isResult(), "Failed with %s on %s: %s" % (funcExpr, vecExpr, res))

    def test_cuda_read_tuples(self):
        self.compareCudaToCPU("fun((a,b)) { (b,a) }", "(1,2)")
        self.compareCudaToCPU("fun((a,b)) { b + a }", "(1,2)")
        self.compareCudaToCPU("fun((a,b)) { b + a }", "(1s32,2s32)")
        self.compareCudaToCPU("fun(b) { b + 1s32 }", "2")

        self.compareCudaToCPU("math.log", "2")

    def test_cuda_tuple_alignment(self):
        #this test fails because the middle argument is not correctly aligned in our current model.
        self.compareCudaToCPU("fun((a,b,c)) { a+b+c }", "(1s32,2,2s32)")

    def check_log_function_on_GPU(self, input):
        s3 = InMemoryS3Interface.InMemoryS3InterfaceFactory()
        text = """
            let f = fun(x) {
                `log(x)
                }
            `CUDAVectorApply(f, [""" + str(input) + """])[0]
            """
        res = InMemoryCumulusSimulation.computeUsingSeveralWorkers(text, s3, 1, timeout = 120, threadCount=4)
        self.assertIsNotNone(res)
        self.assertTrue(res.isResult(), res)
        gpuValue = res.asResult.result.pyval
        pythonValue = math.log(input)
        print "[64bit with rounding: log({})] abs(gpuValue - pythonValue) = {}".format(input, abs(gpuValue - pythonValue))
#         self.assertTrue(abs(gpuValue - pythonValue) < 1e-10)


    @PerformanceTestReporter.PerfTest("python.InMemoryCumulus.GPU.CorrectnessOfLogarithm")
    def test_log_values(self):
        for x in [0.001, 0.01, 0.1, 0.5, 0.8, 0.9, 0.99, 0.9999, 1.0, 1.0001, 1.01, 1.1, 10, 1000, 1000000, 1000000000]:
            self.check_log_function_on_GPU(x)


    @PerformanceTestReporter.PerfTest("python.InMemoryCumulus.GPU.LotsOfLogsUsingGPU")
    def test_basic_gpu_works_1(self):
        s3 = InMemoryS3Interface.InMemoryS3InterfaceFactory()

        text = """
            let f = fun(ct) {
                let res = 0.0
                let x = 1.0
                while (x < ct)
                    {
                    x = x + 1.0
                    res = res + `log(x)
                    }
                res
                }
            `CUDAVectorApply(f, Vector.range(1024*4, {_+1000000}))
            """

        res = InMemoryCumulusSimulation.computeUsingSeveralWorkers(text, s3, 1, timeout = 120, threadCount=4)
        self.assertIsNotNone(res)
        self.assertTrue(res.isResult(), res)

class A:
    @PerformanceTestReporter.PerfTest("python.InMemoryCumulus.GPU.LotsOfLogsWithoutGPU")
    def test_basic_gpu_works_2(self):
        s3 = InMemoryS3Interface.InMemoryS3InterfaceFactory()

        text = """
            let f = fun(ct) {
                let res = 0.0
                let x = 1.0
                while (x < ct)
                    {
                    x = x + 1.0
                    res = res + `log(x)
                    }
                res
                }
            Vector.range(1024*4, {_+1000000}) ~~ f
            """

        res = InMemoryCumulusSimulation.computeUsingSeveralWorkers(text, s3, 1, timeout = 120, threadCount=4)
        self.assertIsNotNone(res)
        self.assertTrue(res.isResult(), res)

