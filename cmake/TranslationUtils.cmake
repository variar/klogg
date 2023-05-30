#=======================================================================
# Copyright © 2019-2022 Pedro López-Cabanillas <plcl@users.sf.net>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=======================================================================

function(ADD_APP_TRANSLATIONS_RESOURCE res_file)
    set(_qm_files ${ARGN})
    set(_res_file ${CMAKE_CURRENT_BINARY_DIR}/app_translations.qrc)

    file(WRITE ${_res_file} "<!DOCTYPE RCC><RCC version=\"1.0\">\n <qresource prefix=\"/i18n/\">\n")
    foreach(_lang ${_qm_files})
        get_filename_component(_filename ${_lang} NAME)
        file(APPEND ${_res_file} "  <file>${_filename}</file>\n")
    endforeach()
    file(APPEND ${_res_file} " </qresource>\n</RCC>\n")

    set(${res_file} ${_res_file} PARENT_SCOPE)
endfunction()

function(ADD_QT_TRANSLATIONS_RESOURCE res_file)
    set(_languages ${ARGN})
    set(_res_file ${CMAKE_CURRENT_BINARY_DIR}/qt_translations.qrc)
    set(_patterns qtbase qtmultimedia qtscript qtxmlpatterns qt)
    # get qt translation dir
    get_target_property (QT_QMAKE_EXECUTABLE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    execute_process(COMMAND ${QT_QMAKE_EXECUTABLE} -query "QT_INSTALL_TRANSLATIONS"
            TIMEOUT 3
            OUTPUT_VARIABLE TRANSLATION_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
    message(WARNING "FINDTRANS, dir:" ${TRANSLATION_DIR})

    get_filename_component(_srcdir "${TRANSLATION_DIR}" ABSOLUTE)
    set(_outfiles)
    foreach(_lang ${_languages})
        set(_infiles)
        set(_out qt_${_lang}.qm)
        foreach(_pat ${_patterns})
            set(_file "${_srcdir}/${_pat}_${_lang}.qm")
            message(WARNING "FINDTRANS, file:" ${_file})

            if (EXISTS ${_file})
                message(WARNING "FINDTRANS, exist:" ${_file})
                list(APPEND _infiles ${_file})
            endif()
        endforeach()
        if(_infiles)
            add_custom_command(OUTPUT ${_out}
                COMMAND ${Qt_LCONVERT_EXECUTABLE}
                ARGS -i ${_infiles} -o ${_out}
                COMMAND_EXPAND_LISTS VERBATIM)
            list(APPEND _outfiles ${_out})
        endif()
    endforeach()
    file(WRITE ${_res_file} "<!DOCTYPE RCC><RCC version=\"1.0\">\n <qresource prefix=\"/i18n/\">\n")
    foreach(_file ${_outfiles})
        get_filename_component(_filename ${_file} NAME)
        message(WARNING "FINDTRANS, write:" ${_filename})
        file(APPEND ${_res_file} "  <file>${_filename}</file>\n")
    endforeach()
    file(APPEND ${_res_file} " </qresource>\n</RCC>\n")
    set(${res_file} ${_res_file} PARENT_SCOPE)
endfunction()
