#!/bin/bash
<<COMMENT
#-H-################################################################################################
# FILENAME :        test.sh
#
# DESCRIPTION :
#       Batch testing: generates test cases and calls execution of input text files (containing points 
#                       of an image) to wav signal of desired frequency
#
# PUBLIC FUNCTIONS :
#   create_wav: takes four arguments, takes a single file as input and calls svg_towav execution for it 
#   write_screen_log: printf to both terminal and log
#

AUTHOR :    A K M Sharif Kaiser(SK)        START DATE : 27 Feb 2021

CHANGES :
REF NO  VERSION DATE    WHO     DETAIL
* 02    19Mar2021       SK      OS detection and build accordingly, bug fix
* 03    23Mar2021       SK      Windows compatibility
* 04    25Mar2021       SK      Dimension addition to existing points file

#H-#
COMMENT

<<COMMENT
    ----------- some syntax helps on bash--------------------------------------
    find . : find in current dir
    -type f means only consider files, not directories
    maxdepth 1 : only the main dir, subdirectories are not considered
    name= checks if the name pattern

    // test -s means if the file exist and not empty
    if ! [[ expression && expr2 || expr3 ]]; then       #expr must produce true or false, spaces after [[ and before ]] is very important
    fi 
    In bash, there must not be any space around = sign while assigning a val
COMMENT

re='^[+-]?[0-9]+([.][0-9]+)?$'                          # regular expression for a signed number
sampling_rate=48000
max_freq=$((sampling_rate / 2)) # nyquist theorem, (bash arithmetic:no space after brackets)
duration=10 # in seconds
OS_name=""

# begin: log file related code
create_log_file(){
    currentDate=`date`
    sSysDate=`date +%Y%m%d%H%M%S`
    log_file="log_file_$sSysDate.log"           # name of output log file, string concat

    # create log file or overrite if already present
    header="Test report log generated on $currentDate:\n"
    printf "$header" > "$log_file"
}

write_screen_log () {
    printf "$1" | tee -a $log_file        # writes argument in both screen and in log file
}

# end: log file related code

detect_OS_and_build () {
    # Find OS
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
            OS_name="linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
            OS_name="macOS"
    elif [[ "$OSTYPE" == "cygwin" ]]; then
            OS_name="windows"      # POSIX compatibility layer and Linux environment emulation for Windows
    elif [[ "$OSTYPE" == "msys" ]]; then
            OS_name="windows"
    elif [[ "$OSTYPE" == "win32" ]]; then
            OS_name="windows"
    fi
    write_screen_log "OS: $OS_name ($OSTYPE)\n"

    if [[ "$OS_name" = "windows" ]]; then
        EXEC_add_dim="add_dim_to_points.exe"
        EXEC_to_wav="svg_to_wav.exe"       # windows executable file
    else
        EXEC_add_dim="add_dim_to_points"
        EXEC_to_wav="svg_to_wav"           # for linux and macOS
    fi

    SRC_to_wav="svg_to_wav.cpp"
    SRC_add_dim="add_dim_to_points.cpp"

    if [[ "$SRC_add_dim" -nt "$EXEC_add_dim" ]]; then                     # if source newer than executable file, then build
        write_screen_log "Rebuilding $SRC_add_dim...\n"

        if [[ "$OS_name" = "macOS" ]]; then
            CC=/usr/bin/clang++         # clang++ is default compiler for macOS
            $CC -std=c++17 -stdlib=libc++ -g $SRC_add_dim -o $EXEC_add_dim   # build, see tasks.json file for build details in vscode
            write_screen_log "$CC -std=c++17 -stdlib=libc++ -g $SRC_add_dim -o $EXEC_add_dim\n"
        elif [[ "$OS_name" = "linux" ]]; then
            CC=/usr/bin/g++         # g++ compiler for ubuntu
            $CC -g --std=c++17 $SRC_add_dim -o $EXEC_add_dim
            write_screen_log "$CC -g --std=c++17 $SRC_add_dim -o $EXEC_add_dim\n"
        elif [[ "$OS_name" = "windows" ]]; then
            CC=g++         # msys mingw64 compiler for windows (assuming environment path added to windows)
            $CC -g --std=c++17 $SRC_add_dim -o $EXEC_add_dim
            write_screen_log "$CC -g --std=c++17 $SRC_add_dim -o $EXEC_add_dim\n"
        fi
    fi

    if [[ "$SRC_to_wav" -nt "$EXEC_to_wav" ]]; then                     # if source newer than executable file, then build
        write_screen_log "Rebuilding $SRC_to_wav...\n"

        if [[ "$OS_name" = "macOS" ]]; then
            CC=/usr/bin/clang++         # clang++ is default compiler for macOS
            $CC -std=c++17 -stdlib=libc++ -g $SRC_to_wav -o $EXEC_to_wav   # build, see tasks.json file for build details in vscode
            write_screen_log "$CC -std=c++17 -stdlib=libc++ -g $SRC_to_wav -o $EXEC_to_wav\n"
        elif [[ "$OS_name" = "linux" ]]; then
            CC=/usr/bin/g++         # g++ compiler for ubuntu
            $CC -g --std=c++17 $SRC_to_wav -o $EXEC_to_wav
            write_screen_log "$CC -g --std=c++17 $SRC_to_wav -o $EXEC_to_wav\n"
        elif [[ "$OS_name" = "windows" ]]; then
            CC=g++         # msys mingw64 compiler for windows (assuming environment path added to windows)
            $CC -g --std=c++17 $SRC_to_wav -o $EXEC_to_wav
            write_screen_log "$CC -g --std=c++17 $SRC_to_wav -o $EXEC_to_wav\n"
        fi
    fi
}

# end: detect_OS_and_build


# start: validate_input_file -> check errors in an input text file
validate_input_file(){
    local file_name=$1 #arg: $1=filename

    # for windows OS, remove the crlf from text file, make txt as unix
    dos2unix $file_name
    #write_screen_log "Converting $file_name to unix format...\n"

    is_valid=true    # initialize as valid, e.g. no error found 
    if [[ -s "$file_name" ]]; then                             # check whether file exist and not empty
        if [[ -r "$file_name" ]]; then                         # check if file readable
            # read each line of the input text file
            # -r prevents backslash escapes from being interpreted
            # [ -n "$line" ] to include the last line if the last char of file is not newline 
            i=1
            while read -r line || [ -n "$line" ]; do                
                # test whether first has dimensions in h|w format
                if [[ "$i" = 1 ]] ; then
                    IFS="|"                             # set | as a delimiter, used to check height|width
                    read -a strarr <<< "$line"          # Read the split words into an array

                    if ! [[ ${#strarr[@]} = 2 ]]; then  # validation for only 2 values h and w
                        write_screen_log "FAIL($file_name): First line of file must contain dimensions in height|width format, where height and width are both rational numbers.\n" 
                        is_valid=false    # eror occured, file not valid anymore
                    fi

                    # check if height and width both are numbers
                    if ! [[ "${strarr[0]}" =~ $re && "${strarr[1]}" =~ $re ]]; then # =~ for comparing with regex 
                        write_screen_log "FAIL($file_name): First line of file must contain dimensions in height|width format, where height and width are both rational numbers.\n"
                        is_valid=false
                    fi
                
                else
                    # test all other lines
                    if [[ "$line" = "#" ]]; then    # encountered end of file
                        if [[ $i -lt 4 ]]; then    # check if at least 2 points exist
                            write_screen_log "FAIL($file_name): There are not enough points in file. Please provide at least two points for this file.\n"
                                is_valid=false
                        fi
                    
                    else
                        # did not find #, it must be a point
                        # test point coordinates x,y
                        IFS=","                             # set | as a delimiter, used to check height|width
                        read -a strarr <<< "$line"          # Read the split words into an array

                        if ! [[ ${#strarr[@]} = 2 ]]; then  # delimiter found invalid string
                                write_screen_log "FAIL($file_name): Invalid point in line $i of file. Input point format is x,y where x and y are coordinates of the point (rational numbers).\n" 
                                is_valid=false
                        else
                            # 2 items found by delimited, now check if height and width both are numbers
                            if ! [[ "${strarr[0]}" =~ $re && "${strarr[1]}" =~ $re ]]; then # =~ for comparing with regex 
                                write_screen_log "FAIL($file_name): Invalid point in line $i of file. Input point format is x,y where x and y are coordinates of the point (rational numbers).\n"
                                is_valid=false
                            fi
                        fi
                    fi
                fi

                i=$((i+1))
                #printf "$i\n"
            done < "$file_name"      # file-> each line reading end

            # check end of file exists in last line
            if ! [[ $( tail -n 1 "$file_name" ) = "#" ]]; then
                write_screen_log "FAIL($file_name): The last line must contain # to denote end of file. Please create a new line and append #\n"
                is_valid=false
            fi

        else
            write_screen_log "FAIL($file_name): file not readable.\n";
            is_valid=false
        fi

    else
        write_screen_log "FAIL($file_name): file does not exist or is empty. Please check your input file.\n"
        is_valid=false
    fi
    
    # return
    if [[ $is_valid = true ]]; then return 0; else return 10;  fi # 0 means true=valid input file, 10=false e.g. invalid
}
# end: validate_input_file function

create_wav () {                               # function for executing code with single file and arguments
    local file_name=$1  # assign arg to a meaningful name file_name, it is a local var
    if [[ $file_name =~ ^./ ]]; then    # check if file name starts with ./ (same dir for mac)
        file_name=${file_name:2};   # removes first 2 characters ./ from file name
    fi

    write_screen_log "Executing: ./$EXEC_to_wav $file_name $2 $3 $4 ...\n"
    ./$EXEC_to_wav $file_name $2 $3 $4                             # calls main function, args: filename seconds freq sampling_rate 
    # error checking with arguments
    if [[ "$?" != 0 ]]; then    # if main function returned non-zero
        write_screen_log "Processing FAIL: Points of $file_name was not processed. Please provide valid arguments for execution.\n\n"
    else
        write_screen_log "Processing SUCCESS: Points of $file_name has been processed successfully.\n\n"
    fi
}
# end: create_wav function

# check if bash has at least 1 arg (filename), having argument means it will process a single file
if [[ "$#" -ge 1 ]]; then
    
    if [ "$1" == "-h" ] ; then
        # begin: check if arg is help and add help text
        printf "Help: `basename $0` -h\n\n"
        printf "Batch Test: `basename $0`\n"
        printf "  This command takes all text files (containing image point) in the same directory and converts them to wav files with different frequencies.\n\n"
        printf "Single Input Test: `basename $0` <input file name> [duration frequency sampling_rate]\n"
        printf "  duration        : positive int \n  frequency       : positive number 0<frequency<=(sampling_rate/2)\n  sampling_rate   : 48000 (preferred) or 44100\n"
        exit 0
        #end: help text
    else
        # begin: single file test
        create_log_file
        detect_OS_and_build
        write_screen_log "Single file test starting...\n"
        
        # Add dimension to file
        write_screen_log "Executing: ./$EXEC_add_dim $1 ...\n"
        ./$EXEC_add_dim $1

        validate_input_file $1
        is_file_valid=$?            # catch return val

        if [[ $is_file_valid -eq 10 ]]; then  # false=not valid, skip processing this file
            write_screen_log "FAIL: $1 will not be processed due to errors.\n\n"
        else
            create_wav $@         # execute with arguments
        fi
        # end: single file test
    fi

else

    # bash has no args, means batch testing
    create_log_file
    detect_OS_and_build
    write_screen_log "Batch test starting...\n"

    # Add dimension to all files in the current dir
    write_screen_log "Executing: ./$EXEC_add_dim ...\n"
    ./$EXEC_add_dim

    for file in $(find . -type f -maxdepth 1 -name "*.txt")
    do
        validate_input_file $file
        is_file_valid=$?    # catch return val

        # file error check
        if [[ $is_file_valid -eq 10 ]]; then  # false=not valid, skip processing this file
            write_screen_log "FAIL: $file will not be processed due to errors.\n\n"
            continue
        
        else    # input file is correct, call main function and run tests
            write_screen_log "SUCCESS: $file is a valid input file. It will now be processed.\n"

            # create test cases for this input file
            duration=10
            # calculate first frequency test case, to fit 1 full cycle of the signal in the entire 10 sec 
            # bc for floating op arithmetic, this must be echo'ed or expr returns string
            freq=$(expr "scale=1;1/$duration" | bc)    #freq is a string, scale=1 means keep 1 places after decimal
            
            # the calculated freq is a string, call main function once while loop to avoid calculation problems due to string
            create_wav $file $duration $freq $sampling_rate    # function call for exec e.g. calls cpp main function (only input file arg is mandatory)

            # calculate second frequency
            freq=$(expr $freq*10 | bc)  # freq=freq*10
            freq=${freq%.*} # as freq contains a string with a decimal, get rid of decimal part from freq

            while [ $freq -le $max_freq ]; do       # -le means less or equal
                create_wav $file $duration $freq $sampling_rate    # function call for exec e.g. calls cpp main function (only input file arg is mandatory)
                freq=$((freq*10))
            done

        fi

    done    #each file processing loop end
fi