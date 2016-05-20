#   Copyright 2016 Ufora Inc.
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


import ufora.FORA.python.PurePython.InMemorySimulationExecutorFactory as \
    InMemorySimulationExecutorFactory
import ufora.test.PerformanceTestReporter as PerformanceTestReporter
import ufora.FORA.python.FORA as FORA

import numpy as np
import pyfora
import pyfora.random.mtrand
import scipy.special
import time
import unittest


class Obligor(object):
    def __init__(self, probability_of_default, value, factor_loads):
        self.probability_of_default = probability_of_default
        self.x = (2.0 ** 0.5) * scipy.special.erfinv(
            2.0 * probability_of_default - 1.0)
        self.value = value
        self.factor_loads = factor_loads

    def __str__(self):
        return "Obligor(p=%s, x=%s, v=%s, a=%s)" % (
            self.probability_of_default,
            self.x,
            self.value,
            self.factor_loads
            )
        

class Portfolio(object):
    def __init__(self, obligors):
        self.obligors = obligors
        self.x_list = np.array([obligor.x for obligor in obligors])
        self.value_list = np.array([obligor.value for obligor in obligors])
        self.n_factors = len(obligors[0].factor_loads)

        assert all([len(obligor.factor_loads) == self.n_factors \
                    for obligor in obligors])

        factor_loads = [
            obligor.factor_loads.tolist() for obligor in obligors
            ]
        
        self.factor_loads = np.array(factor_loads)

    def simulate_losses(self, n_samples, random_state=None, seed=None):
        if random_state is None:
            random_state = pyfora.random.mtrand.RandomState(seed)

        tr = []
        for _ in xrange(n_samples):
            loss, random_state = self.simulate_loss(random_state)
            tr = tr + [loss]

        return np.array(tr), random_state
 
    def simulate_loss(self, random_state):
        Z, random_state = random_state.randn(self.n_factors)

        X = np.dot(self.factor_loads, Z)
        loss = 0.0
        for ix in xrange(len(X)):
            if X[ix] > self.x_list[ix]:
                loss = loss + self.value_list[ix]
        return loss, random_state


class TestCopulaModelPerf(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.executor = None

    @classmethod
    def tearDownClass(cls):
        if cls.executor is not None:
            cls.executor.close()

    def getS3Interface(self, executor):
        return executor.s3Interface

    @classmethod
    def create_executor(cls, allowCached=True):
        if not allowCached:
            return InMemorySimulationExecutorFactory.create_executor()
        if cls.executor is None:
            cls.executor = InMemorySimulationExecutorFactory.create_executor()
            cls.executor.stayOpenOnExit = True

        return cls.executor

    def loopScalabilityTest(self, n_threads, testName):
        n_factors = 40
        n_obligors = 5000

        seed = 54326

        with self.create_executor() as executor:
            with executor.remotely:
                def get_obligors(n_obligors):
                    rng = pyfora.random.mtrand.RandomState(seed=seed)

                    obligors = []
                    for _ in xrange(n_obligors):
                        probability_of_default, rng = rng.rand()
                        value, rng = rng.uniform(low=0, high=100)
                        factor_loads, rng = rng.rand(n_factors)

                        obligors = obligors + [
                            Obligor(
                                probability_of_default=probability_of_default,
                                value=value,
                                factor_loads=factor_loads
                                )]

                    return obligors

                obligors = get_obligors(n_obligors)

            def f(seed):
                with executor.remotely:
                    portfolio = Portfolio(obligors=obligors)

                    total_portfolio_value = sum([obligor.value for obligor in obligors])

                    n_samples = 20000 * n_threads

                    def batch_size(batch_ix):
                        if batch_ix < n_threads - 1:
                            return n_samples / n_threads
                        return n_samples / n_threads + n_samples % n_threads

                    batches_of_simulated_losses = [
                        portfolio.simulate_losses(
                            n_samples=batch_size(batch_ix), seed=seed + batch_ix)[0] \
                        for batch_ix in xrange(n_threads)
                        ]

                    simulated_losses = [
                        elt for batch in batches_of_simulated_losses for elt in batch
                        ]

                    assert len(simulated_losses) == n_samples

                    thresh = total_portfolio_value * 0.95

                    p = 0
                    for jx in xrange(len(simulated_losses)):
                        if simulated_losses[jx] > thresh:
                            p = p + 1
                    p = p / float(len(simulated_losses))

                return p, thresh

            # burn in run
            f(seed)

            t0 = time.time()
            p, thresh = f(seed + 1)
            print "estimated P(L > x=%s) = %s" % (
                thresh.toLocal().result(), p.toLocal().result())
            t_elapsed = time.time() - t0

            PerformanceTestReporter.recordTest(testName, t_elapsed, None)

    def test_loopScalability_1(self):
        self.loopScalabilityTest(1, "pyfora.BigBox.LoopScalabilityTest.01Threads")

    def test_loopScalability_2(self):
        self.loopScalabilityTest(2, "pyfora.BigBox.LoopScalabilityTest.02Threads")

    def test_loopScalability_4(self):
        self.loopScalabilityTest(4, "pyfora.BigBox.LoopScalabilityTest.04Threads")

    def test_loopScalability_8(self):
        self.loopScalabilityTest(8, "pyfora.BigBox.LoopScalabilityTest.08Threads")

    def test_loopScalability_16(self):
        self.loopScalabilityTest(16, "pyfora.BigBox.LoopScalabilityTest.16Threads")

    def test_loopScalability_32(self):
        self.loopScalabilityTest(32, "pyfora.BigBox.LoopScalabilityTest.32Threads")


if __name__ == "__main__":
    import ufora.config.Mainline as Mainline
    Mainline.UnitTestMainline([FORA])
