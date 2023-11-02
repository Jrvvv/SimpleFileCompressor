#!/bin/bash

filenames=(
      "tests/files/test1.txt"
      "tests/files/test2.txt"
)

filename_not_existing=(
      "aaaooo.txt"
      ""
)

zip_name="tests/files/test.zip"
unzip_name="tests/files/test.unzip"
log_name="tests/files/output.file"

success=0
fail=0
count=0
leaks=0

# error string, no errors if found
errStr="ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)"

compare_files() {
    (( count++ ))
    difference="$(diff -s $1 $2)"
    if [ "$difference" == "Files $1 and $2 are identical" ]; then
        (( success++ ))
        echo "-------------------"
        echo "test $count success"
        echo "-------------------"
    else
        (( fail++ ))
        echo "-------------------"
        echo "test $count fail"
        echo "-------------------"
        echo $difference
        echo
    fi
}

check_leaks() {
    if grep -Fq "$errStr" $1; then
        echo "NO LEAKS"
    else
        echo "LEAKS FOUND"
        (( leaks++ ))
    fi
}

# running with valgrind zip and unzip using as args for zip $1 and $2 for unzip, $3 is error/no_error code
# usage: zip_unzip [zip_flags] [unzip_flags] [result] [filename_list_elements]
zip_unzip() {
    local zip_flags="$1"
    local unzip_flags="$2"
    local result="$3"
    shift 3
    local current_filenames=("$@")      # list of filename array elements

    # executing test files in order
    for test_file in "${current_filenames[@]}"; do
        valgrind --log-file=${log_name} --leak-check=yes --tool=memcheck -s ./${ZIP_APP} ${test_file} ${zip_name} ${zip_flags} > ${log_name}
        ZIP_RET_CODE=$?
        check_leaks ${log_name}

        # execute unzip if zipping ended without error and called without -h
        if [ "${ZIP_RET_CODE}" -eq 0 ] && grep -vq 'h' <<< ${zip_flags}; then
            valgrind --log-file=${log_name} --leak-check=yes --tool=memcheck -s ./${UNZIP_APP} ${zip_name} ${unzip_name} ${unzip_flags} > ${log_name}
            UNZIP_RET_CODE=$?
            check_leaks ${log_name}
            # compare files if unzip ended without error and called without -h
            if [ "${UNZIP_RET_CODE}" -eq 0 ] && grep -vq 'h' <<< ${unzip_flags}; then
                compare_files ${test_file} ${unzip_name}
            fi
        fi

        # if one of two is error and error expected -- SUCCESS, else -- FAIL
        if [ "${ZIP_RET_CODE}" -ne 0 ] || [ "${UNZIP_RET_CODE}" -ne 0 ]; then
            (( count++ ))
            if [ ${result} -ne 0 ]; then
                (( success++ ))
                echo "-------------------"
                echo "test $count success"
                echo "-------------------"
                echo 
            else
                (( fail++ ))
                echo "-------------------"
                echo "test $count fail"
                echo "-------------------"
                echo PARAMS ARE ${zip_flags} ${unzip_flags} ${result}
            fi

        elif grep -q 'h' <<< ${zip_flags} || grep -q 'h' <<< ${unzip_flags} ; then
            (( count++ ))
            (( success++ ))
            echo "-------------------"
            echo "test $count success"
            echo "-------------------"
            echo 
        fi

        sudo rm -f ${zip_name} ${unzip_name} ${log_name} tests/files/*.zip tests/files/*.unzip
    done
}

ZIP_APP="build/zip"
UNZIP_APP="build/unzip"

if [ -e ${ZIP_APP} ] && [ -e ${UNZIP_APP} ]; then
    if [ -e ${zip_name} ]; then
        rm ${zip_name}
    fi
    if [ -e ${unzip_name} ]; then
        rm ${unzip_name}
    fi
    if [ -e ${log_name} ]; then
        rm ${log_name}
    fis

    # test cases: args of zip, args of unzip and expected error(1)/no_error(0)
    zip_unzip ""                                            ""    "0"   "${filenames[@]}"
    zip_unzip ""                                            ""    "1"   "${filename_not_existing[@]}"
    zip_unzip "-f -c 100"                                   "-f"  "0"   "${filenames[@]}"
    zip_unzip "-f -c 100"                                   "-f"  "1"   "${filename_not_existing[@]}"
    zip_unzip "-f -c 1"                                     "-f"  "0"   "${filenames[@]}"
    zip_unzip "-f -c 1"                                     "-f"  "1"   "${filename_not_existing[@]}"
    zip_unzip "-f"                                          "-f"  "0"   "${filenames[@]}"
    zip_unzip "-f"                                          "-f"  "1"   "${filename_not_existing[@]}"
    zip_unzip "-f -c -400"                                  "-f"  "1"   "${filenames[@]}"
    zip_unzip "-f -c 99999999999999999999999999"            "-f"  "1"   "${filenames[@]}"
    zip_unzip "-f -h"                                       ""    "0"   "${filenames[@]}"
    zip_unzip "-f"                                          ""    "0"   "${filenames[@]}"
    zip_unzip "-f"                                          ""    "1"   "${filename_not_existing[@]}"
    zip_unzip ""                                            "-f"  "0"   "${filenames[@]}"
    zip_unzip ""                                            "-f"  "1"   "${filename_not_existing[@]}"
    zip_unzip "-fabc"                                       "-f"  "1"   "${filenames[@]}"
    zip_unzip "-f"                                          "-fa" "1"   "${filenames[@]}"
    zip_unzip "-f"                                          "-h"  "0"   "${filenames[@]}"



    sudo rm -f tests/files/*.zip tests/files/*.unzip


    echo "---------------------"
    echo "success: $success"
    echo "fail: $fail"
    echo "all: $count"
    echo "leaks: $leaks"
    echo "---------------------"

    sudo rm -f ${zip_name} ${unzip_name} ${log_name} tests/files/*.zip tests/files/*.unzip
else
    echo "no files zip/unzip to execute"
fi