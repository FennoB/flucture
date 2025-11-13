if(NOT EXISTS "/home/fenno/Projects/flucture/documents/pdf/podofo-master/install_manifest.txt")
  message(FATAL_ERROR "Cannot find install manifest: \"/home/fenno/Projects/flucture/documents/pdf/podofo-master/install_manifest.txt\"")
endif(NOT EXISTS "/home/fenno/Projects/flucture/documents/pdf/podofo-master/install_manifest.txt")

file(READ "/home/fenno/Projects/flucture/documents/pdf/podofo-master/install_manifest.txt" files)
string(REGEX REPLACE "\n" ";" files "${files}")
foreach(file ${files})
  message(STATUS "Uninstalling \"${file}\"")
  if(NOT EXISTS "${file}")
    message(FATAL_ERROR "File \"${file}\" does not exists.")
  endif(NOT EXISTS "${file}")
  exec_program("/usr/bin/cmake" ARGS "-E remove \"${file}\""
    OUTPUT_VARIABLE rm_out
    RETURN_VARIABLE rm_retval)
  if("${rm_retval}" GREATER 0)
    message(FATAL_ERROR "Problem when removing \"${file}\"")
  endif("${rm_retval}" GREATER 0)
endforeach(file)
