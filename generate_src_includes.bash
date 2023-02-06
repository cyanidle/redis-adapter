generate_for_dir () {
    if [[ -z $1 ]]; then
        echo "Please specify target directory!"
        exit -1
    fi
    if [[ -z $2 ]]; then
        echo "Please specify target .pri file name!"
        exit -1
    fi
    SOURCES=$(ls $1 | grep -E .*.cpp)
    HEADERS=$(ls $1 | grep -E .*.h)
    TEXT=""
    if [[ ! -z $SOURCES ]]; then
        SOURCES=$(echo $SOURCES | xargs printf "\$\$PWD/%s \\ \\\n")
        TEXT="${TEXT}SOURCES+= \ \n$SOURCES\n\n"
    fi
    if [[ ! -z $HEADERS ]]; then
        HEADERS=$(echo $HEADERS | xargs printf "\$\$PWD/%s \\ \\\n")
        TEXT="${TEXT}HEADERS+= \ \n$HEADERS\n"
    fi
    if [[ ! -z $TEXT ]]; then
        echo "Generating and adding $1/$2.pri..."
        echo -e $TEXT > $1/$2.pri
        echo "include(\$\$PWD/$1/$2.pri)" >> radapter_src.pri
    fi
} 

main () {
    DIRS=$(find src -type d | grep -v src/lib)
    echo -n "" > radapter_src.pri
    for dir in $DIRS; do 
        NAME_OF_FILE=${dir//\//_} # format src/dir/ -> src_dir_
        NAME_OF_FILE=${NAME_OF_FILE#src_} # remove src_ prefix
        generate_for_dir $dir $NAME_OF_FILE; 
    done
    echo "Adding headers.pri"
    echo "include(\$\$PWD/headers.pri)" >> radapter_src.pri
    echo "Adding lib.pri"
    echo "include(\$\$PWD/src/lib/lib.pri)" >> radapter_src.pri
}

main