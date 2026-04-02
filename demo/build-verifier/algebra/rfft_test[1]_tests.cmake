add_test([=[RFFTTest.Simple]=]  /home/cat/longfellow-zk/build-verifier/algebra/rfft_test [==[--gtest_filter=RFFTTest.Simple]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RFFTTest.Simple]=]  PROPERTIES WORKING_DIRECTORY /home/cat/longfellow-zk/build-verifier/algebra SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  rfft_test_TESTS RFFTTest.Simple)
