# @see r2fc-file-cleanup3.md13; generated provider refuses all Git mutation.
#[[/** Refuse Git-fetch source recipes. @param ARGV Unsupported port arguments. */]]
function(vcpkg_from_git)
  message(FATAL_ERROR "Git-fetch source recipe is forbidden; select a pinned archive port")
endfunction()
