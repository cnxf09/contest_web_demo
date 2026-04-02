add_library(anoncred_demo_lib
  shared/device_key.cc
  shared/files.cc
  shared/request.cc
  shared/small_demo.cc
  issuer/issue.cc
  holder/keygen.cc
  holder/prove.cc
  verifier/verify.cc)

target_link_libraries(anoncred_demo_lib mdoc)

target_include_directories(anoncred_demo_lib PUBLIC ${proofs_SOURCE_DIR})

add_executable(anoncred_issuer issuer/main.cc)
target_link_libraries(anoncred_issuer anoncred_demo_lib)

add_executable(anoncred_holder holder/main.cc)
target_link_libraries(anoncred_holder anoncred_demo_lib)

add_executable(anoncred_verifier verifier/main.cc)
target_link_libraries(anoncred_verifier anoncred_demo_lib)

add_executable(anoncred_demo_test demo_test.cc)
target_link_libraries(anoncred_demo_test anoncred_demo_lib)
proofs_add_testing_libraries(anoncred_demo_test)
