generate_for_dir () {
    DIR=$1
    OUTFILE=$2
    NAME_OF_FILE=${DIR//\//_} # format src/dir/ -> src_dir_
    NAME_OF_FILE=${NAME_OF_FILE#src_} # remove src_ prefix

    _SLASH='\\x5C'
    _NEW_LINE='\\r\\n'
    _TAB='\\t'
    _PWD='$$PWD'
    _SPACE='\x20'
    FORMAT=${_SLASH}${_NEW_LINE}${_TAB}${_PWD}/%s${_SPACE}

    SOURCES=$(ls $DIR | grep -E \.cpp$)
    HEADERS=$(ls $DIR | grep -E \.hp*$)
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
        echo "Generating and adding $DIR/$NAME_OF_FILE.pri..."
        echo -e $TEXT > $DIR/$NAME_OF_FILE.pri
        echo "include(\$\$PWD/$DIR/$NAME_OF_FILE.pri)" >> $OUTFILE
    fi
} 

main () {
    OUTFILE=$1
    if [[ -z $OUTFILE ]]; then
        echo "Using default output file (radapter_src)"
        OUTFILE="radapter_src.pri"
    fi
    DIRS=$(find src -type d | grep -v src/lib | grep -v src/build)
    echo -n "" > $OUTFILE
    for dir in $DIRS; do 
        generate_for_dir $dir $OUTFILE
    done
    echo "Adding headers.pri"
    echo "include(\$\$PWD/headers.pri)" >> $OUTFILE
    echo "Adding lib.pri"
    echo "include(\$\$PWD/src/lib/lib.pri)" >> $OUTFILE
    echo "Adding gui.pri"
    echo "include(\$\$PWD/gui.pri)" >> $OUTFILE
}
if [[ ! -z $2 ]]; then
    echo "Overriding source dir to ($1) --> ($2)"
    generate_for_dir $1 $2
else
    main $1
fi