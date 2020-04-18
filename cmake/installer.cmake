set(CPACK_PACKAGE_VENDOR "lokinet.org")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://lokinet.org/")
set(CPACK_PACKAGE_README_FILE "${CMAKE_SOURCE_DIR}/readme.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")

if(WIN32)
  include(cmake/win32_installer_deps.cmake)
endif()

if(APPLE)
  include(cmake/macos_installer_deps.cmake)
endif()
  

# This must always be last!
include(CPack)
