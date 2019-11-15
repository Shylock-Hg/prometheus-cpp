
#add_executable(sample_server
  #sample_server.cc
#)

#target_link_libraries(sample_server
  #PRIVATE
    #${PROJECT_NAME}::pull
#)

add_executable(sample_server_proxygen
  sample_server_proxygen.cc
)

target_link_libraries(sample_server_proxygen
  PRIVATE
    ${PROJECT_NAME}::pull
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:folly>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:proxygenhttpserver>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:proxygenlib>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:glog>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:wangle>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:folly>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:gflags>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:ssl>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:crypto>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:double-conversion>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:dl>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:event>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:boost_context>
    $<$<BOOL:${USE_THIRDPARTY_LIBRARIES}>:unwind>
)
