#!/bin/bash

function create_makefile() {
	cp ../driver_module/template/Makefile . -f
	sed -i "s/TARGET \:= medma_test/TARGET \:= $1/" Makefile
	sed -i "s/\$(TARGET)-objs \:= dmatest_entry.o/\$(TARGET)-objs \:= $1_entry.o/" Makefile
}

function create_entry() {
	cp ../driver_module/template/gpiotest.c $1_entry.c -f
	sed -i "s/gpiodriver/$1driver/" $1_entry.c
}

#===  FUNCTION  ================================================================
#         NAME:  usage
#  DESCRIPTION:  Display usage information.
#===============================================================================
function usage ()
{
	echo "Usage :  $0 [options] [--]

    Options:
    -h|help       Display this message
    -c|create     create module template
    -v|version    Display script version"

}    # ----------  end of function usage  ----------

#-----------------------------------------------------------------------
#  Handle command line arguments
#-----------------------------------------------------------------------

while getopts ":hvc:" opt
do
  case $opt in

	h|help     )  usage; exit 0   ;;
	c|create   )
		read -p "Your modules name:" MODULE_NAME
		create_makefile $MODULE_NAME
		exit 0   ;;

	* )  echo -e "\n  Option does not exist : $OPTARG\n"
		  usage; exit 1   ;;

  esac    # --- end of case ---
done
shift $(($OPTIND-1))
