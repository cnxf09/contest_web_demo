add_test([=[Counter.Fields]=]  /home/cat/longfellow-zk/build-verifier/circuits/logic/counter_test [==[--gtest_filter=Counter.Fields]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Counter.Fields]=]  PROPERTIES WORKING_DIRECTORY /home/cat/longfellow-zk/build-verifier/circuits/logic SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  counter_test_TESTS Counter.Fields)
