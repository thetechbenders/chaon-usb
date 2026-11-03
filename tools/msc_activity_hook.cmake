# Patch a build-local copy, retaining upstream notices and managed source intact.
idf_component_get_property(msc_dir espressif__esp_tinyusb COMPONENT_DIR)
idf_component_get_property(msc_lib espressif__esp_tinyusb COMPONENT_LIB)
set(msc_original "${msc_dir}/tinyusb_msc.c")
set(msc_patched "${CMAKE_BINARY_DIR}/chaon_tinyusb_msc.c")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${msc_original}" "${CMAKE_CURRENT_LIST_DIR}/patch_msc_activity.py"
    "${CMAKE_CURRENT_LIST_DIR}/patch_msc_eject.py")
execute_process(COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/patch_msc_activity.py"
    "${msc_original}" "${msc_patched}" RESULT_VARIABLE patch_result)
if(NOT patch_result EQUAL 0)
    message(FATAL_ERROR "Pinned MSC activity hook patch failed")
endif()
execute_process(COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/patch_msc_eject.py"
    "${msc_patched}" "${msc_patched}" RESULT_VARIABLE eject_patch_result)
if(NOT eject_patch_result EQUAL 0)
    message(FATAL_ERROR "Pinned MSC eject and synchronous-write patch failed")
endif()
get_target_property(msc_sources ${msc_lib} SOURCES)
set(msc_replaced 0)
set(msc_new_sources "")
foreach(source IN LISTS msc_sources)
    if(source STREQUAL "tinyusb_msc.c" OR source STREQUAL "${msc_original}")
        list(APPEND msc_new_sources "${msc_patched}")
        math(EXPR msc_replaced "${msc_replaced} + 1")
    else()
        list(APPEND msc_new_sources "${source}")
    endif()
endforeach()
if(NOT msc_replaced EQUAL 1)
    message(FATAL_ERROR "Expected exactly one pinned MSC compilation unit")
endif()
# The patched MSC backend calls these functions. Keep their definitions in
# the same component archive to avoid static archive link-order failures.
# This is a build-local integration; the managed component tree stays intact.
list(APPEND msc_new_sources "${CMAKE_SOURCE_DIR}/main/chaon_msc_eject.c")
set_property(TARGET ${msc_lib} PROPERTY SOURCES "${msc_new_sources}")

# Patch the pinned TinyUSB BOT command validator in a separate build-local
# compilation unit. The original file is checked by SHA-256 before writing.
idf_component_get_property(tusb_dir espressif__tinyusb COMPONENT_DIR)
idf_component_get_property(tusb_lib espressif__tinyusb COMPONENT_LIB)
set(tusb_original "${tusb_dir}/src/class/msc/msc_device.c")
set(tusb_patched "${CMAKE_BINARY_DIR}/chaon_tinyusb_msc_transport.c")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${tusb_original}" "${CMAKE_CURRENT_LIST_DIR}/patch_msc_transport.py")
execute_process(COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/patch_msc_transport.py"
    "${tusb_original}" "${tusb_patched}" RESULT_VARIABLE transport_patch_result)
if(NOT transport_patch_result EQUAL 0)
    message(FATAL_ERROR "Pinned TinyUSB MSC transport source check/patch failed")
endif()
get_target_property(tusb_sources ${tusb_lib} SOURCES)
set(tusb_replaced 0)
set(tusb_new_sources "")
foreach(source IN LISTS tusb_sources)
    if(source STREQUAL "src/class/msc/msc_device.c" OR source STREQUAL "${tusb_original}")
        list(APPEND tusb_new_sources "${tusb_patched}")
        math(EXPR tusb_replaced "${tusb_replaced} + 1")
    else()
        list(APPEND tusb_new_sources "${source}")
    endif()
endforeach()
if(NOT tusb_replaced EQUAL 1)
    message(FATAL_ERROR "Expected exactly one pinned TinyUSB transport compilation unit")
endif()
set_property(TARGET ${tusb_lib} PROPERTY SOURCES "${tusb_new_sources}")
