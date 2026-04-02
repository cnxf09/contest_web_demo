add_test([=[Polynomial.Eval]=]  /home/cat/longfellow-zk/build-verifier/circuits/logic/polynomial_test [==[--gtest_filter=Polynomial.Eval]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Polynomial.Eval]=]  PROPERTIES WORKING_DIRECTORY /home/cat/longfellow-zk/build-verifier/circuits/logic SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  polynomial_test_TESTS Polynomial.Eval)
