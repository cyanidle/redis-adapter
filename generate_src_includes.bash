generate_for_dir () {
    if [[ -z $1 ]]; then
        echo "Please specify target directory!"
        exit -1
    fi
    if [[ -z $2 ]]; then
        echo "Please specify target .pri file name!"
        exit -1
    fi
    if [[ -z $3 ]]; then
        echo "Please specify output <filename>.pri!"
        exit -1
    fi
    SLASH='\\x5C'
    NEW_LINE='\\r\\n'
    TAB='\\t'
    PWD='$$PWD'
    SPACE='\x20'
    FORMAT=${SLASH}${NEW_LINE}${TAB}${PWD}/%s${SPACE}

    SOURCES=$(ls $1 | grep -E \.cpp$)
    HEADERS=$(ls $1 | grep -E \.hp*$)
    TEXT=""
    
    if [[ ! -z $SOURCES ]]; then
        SOURCES=$(echo $SOURCES | xargs printf $FORMAT)
        TEXT="${TEXT}SOURCES+= $SOURCES\n\n"
    fi
    if [[ ! -z $HEADERS ]]; then
        HEADERS=$(echo $HEADERS | xargs printf $FORMAT)
        TEXT="${TEXT}HEADERS+= $HEADERS\n"
    fi
    if [[ ! -z $TEXT ]]; then
        echo "Generating and adding $1/$2.pri..."
        echo -e $TEXT > $1/$2.pri
        echo "include(\$\$PWD/$1/$2.pri)" >> $3.pri
    fi
} 

main () {
    if [[ -z $1 ]]; then
        echo "Please specify output <filename>.pri"
        exit -1
    fi
    OUT_FILE=$1
    DIRS=$(find src -type d | grep -v src/lib)
    echo -n "" > $OUTFILE.pri
    for dir in $DIRS; do 
        NAME_OF_FILE=${dir//\//_} # format src/dir/ -> src_dir_
        NAME_OF_FILE=${NAME_OF_FILE#src_} # remove src_ prefix
        generate_for_dir $dir $NAME_OF_FILE $OUTFILE; 
    done
    echo "Adding headers.pri"
    echo "include(\$\$PWD/headers.pri)" >> $OUTFILE.pri
    echo "Adding lib.pri"
    echo "include(\$\$PWD/src/lib/lib.pri)" >> $OUTFILE.pri
    echo "Adding gui.pri"
    echo "include(\$\$PWD/gui.pri)" >> $OUTFILE.pri
}
OUTFILE=$1
if [[ -z $1 ]]; then
    echo "Using default output file (radapter_src)"
    OUTFILE=radapter_src
fi

main $OUTFILE