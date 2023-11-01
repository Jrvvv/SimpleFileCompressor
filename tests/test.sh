#!/bin/bash

filenames=(
      "tests/files/test1.txt"
      "tests/files/test2.txt"
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
zip_unzip() {
    # executing test files in order
    for test_file in ${filenames[@]}
    do
        valgrind --log-file=${log_name} --leak-check=yes --tool=memcheck -s ./${ZIP_APP} ${test_file} ${zip_name} $1 > ${log_name}
        ZIP_RET_CODE=$?
        check_leaks ${log_name}
        
        # dont execute if zipping ended with error
        if [ "${ZIP_RET_CODE}" -eq 0 ]; then
            valgrind --log-file=${log_name} --leak-check=yes --tool=memcheck -s ./${UNZIP_APP} ${zip_name} ${unzip_name} $2 > ${log_name}
            UNZIP_RET_CODE=$?
            check_leaks ${log_name}
            if [ "${UNZIP_RET_CODE}" -eq 0 ]; then
                compare_files ${test_file} ${unzip_name}
            fi
        fi

        # if one of two is error and error expected -- SUCCESS, else -- FAIL
        if [ "${ZIP_RET_CODE}" -ne 0 ] || [ "${UNZIP_RET_CODE}" -ne 0 ]; then
            (( count++ ))
            if [ $3 -ne 0 ]; then
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
                echo PARAMS ARE $1 $2 $3
            fi
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
    fi

    # test cases: args of zip, args of unzip and expected error(1)/no_error(0)
    zip_unzip ""                                            ""    "0"
    zip_unzip "-f -c 100"                                   "-f"  "0"
    zip_unzip "-f -c 1"                                     "-f"  "0"
    zip_unzip "-f"                                          "-f"  "0"
    zip_unzip "-f -c -400"                                  "-f"  "1"
    zip_unzip "-f -c 99999999999999999999999999"            "-f"  "1"
    zip_unzip "-f -h"                                       ""    "1"
    zip_unzip "-f"                                          ""    "0"
    zip_unzip ""                                            "-f"  "0"
    zip_unzip "-fabc"                                       "-f"  "1"
    zip_unzip "-f"                                          "-fa" "1"
    zip_unzip "-f"                                          "-h"  "1"


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