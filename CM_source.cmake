# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


INCLUDE_DIRECTORIES (src-lib)
INCLUDE_DIRECTORIES (src-tool)

ADD_SUBDIRECTORY (src-lib)
ADD_SUBDIRECTORY (src-tool)
ADD_SUBDIRECTORY (src-apps)
ADD_SUBDIRECTORY (src-cam)
ADD_SUBDIRECTORY (src-doc)
