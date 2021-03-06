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
#   exec_file_to_wav: takes four arguments, takes a single file as input and calls svg_towav execution for it 
#   write_screen_log: printf to both terminal and log
#

AUTHOR :    A K M Sharif Kaiser(SK)        START DATE : 27 Feb 2021

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
    printf "$1" | tee -a "$log_file"        # writes argument in both screen and in log file
}

# end: log file related code

# begin: build cpp file
CC=/usr/bin/clang++         # clang++ compiler
EXEC1=add_dim_to_points
SRC1=add_dim_to_points.cpp

EXEC2=svg_to_wav
SRC2=svg_to_wav.cpp

# build add_dim_to_points
if [[ "$SRC1" -nt $EXEC1 ]]; then                     # if source newer than executable file, then build
    write_screen_log "Rebuilding $EXEC1...\n"
    $CC -std=c++17 -stdlib=libc++ -g $SRC1 -o $EXEC1   # build, see tasks.json file for build details in vscode
fi

if [[ "$SRC2" -nt $EXEC2 ]]; then                     # if source newer than executable file, then build
    write_screen_log "Rebuilding $EXEC2...\n"
    $CC -std=c++17 -stdlib=libc++ -g $SRC2 -o $EXEC2   # build, see tasks.json file for build details in vscode
fi


# end: build cpp file

# start: validate_input_file -> check errors in an input text file
validate_input_file(){
    local file_name=$1 #arg: $1=filename 
    is_valid=true    # initialize as valid, e.g. no error found 
    if [[ -s "$file_name" ]]; then                             # check whether file exist and not empty
        if [[ -r "$file_name" ]]; then                         # check if file readable
            # read each line of the input text file
            # -r prevents backslash escapes from being interpreted
            # [ -n "$line" ] to include the last line if the last char of file is not newline 
            i=1
            while read -r line || [ -n "$line" ]; do

                # test the if first line validity
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
                write_screen_log "FAIL($file_name): The last line must contain # to denote end of file.\n"
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

exec_file_dim () {                               # single adding dimension at top of file
    local file_name=$1  # assign arg to a meaningful name file_name, it is a local var
    if [[ $file_name =~ ^./ ]]; then    # check if file name starts with ./ (same dir for mac)
        file_name=${file_name:2};   # removes first 2 characters ./ from file name
    fi
    
    write_screen_log "Executing: ./$EXEC1 $file_name ...\n"

    ./$EXEC1 $file_name                             # calls ./add_dim_to_points $file_name 
}
# end: exec_file_dim function

exec_file_to_wav () {                               # function for executing code with single file and arguments
    local file_name=$1  # assign arg to a meaningful name file_name, it is a local var
    if [[ $file_name =~ ^./ ]]; then    # check if file name starts with ./ (same dir for mac)
        file_name=${file_name:2};   # removes first 2 characters ./ from file name
    fi
    
    write_screen_log "Executing: ./$EXEC2 $file_name $2 $3 $4 ...\n"

    ./$EXEC2 $file_name $2 $3 $4                             # calls main function, args: filename seconds freq sampling_rate 
    # error checking with arguments
    if [[ "$?" != 0 ]]; then    # if main function returned non-zero
        write_screen_log "Processing FAIL: Points of $file_name was not processed. Please provide valid arguments for execution.\n\n"
    else
        write_screen_log "Processing SUCCESS: Points of $file_name has been processed successfully.\n\n"
    fi
}
# end: exec_file_to_wav function

# check if bash has at least 1 arg (filename), having argument means it will process a signle file
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
        write_screen_log "Single file test starting...\n"
        exec_file_dim $1        # add dim to the text file
        validate_input_file $1
        is_file_valid=$?            # catch return val

        if [[ $is_file_valid -eq 10 ]]; then  # false=not valid, skip processing this file
            write_screen_log "FAIL: $1 will not be processed due to errors.\n\n"
        else
            exec_file_to_wav $@         # execute with arguments
        fi
        # end: single file test
    fi

else
    # bash has no args, means batch testing
    create_log_file
    write_screen_log "Batch test starting...\n"
    
    # Add dimension (Height|Width) to the beginning of input text files if no dimensions exist
    ./$EXEC1
    
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
            exec_file_to_wav $file $duration $freq $sampling_rate    # function call for exec e.g. calls cpp main function (only input file arg is mandatory)

            # calculate second frequency
            freq=$(expr $freq*10 | bc)  # freq=freq*10
            freq=${freq%.*} # as freq contains a string with a decimal, get rid of decimal part from freq

            while [ $freq -le $max_freq ]; do       # -le means less or equal
                exec_file_to_wav $file $duration $freq $sampling_rate    # function call for exec e.g. calls cpp main function (only input file arg is mandatory)
                freq=$((freq*10))
            done

        fi

    done    #each file processing loop end
fi