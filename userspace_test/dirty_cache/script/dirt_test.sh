#!/bin/bash


WR_SIZE=""
SYNC=""
#===  FUNCTION  ================================================================
#         NAME:  usage
#  DESCRIPTION:  Display usage information.
#===============================================================================
function usage ()
{
	echo "Usage :  $0 [options] [--]

    Options:
    -h|help       Display this message
    -s|sync       sync write
    -n|size       wiret n*MB size file
    -v|version    Display script version"

}    # ----------  end of function usage  ----------

#-----------------------------------------------------------------------
#  Handle command line arguments
#-----------------------------------------------------------------------

while getopts ":hvsn:" opt
do
  case $opt in

	h|help     )  usage; exit 0   ;;

	s|sync     )  
		SYNC="sync"
		;;

	n|size     )  
		WR_SIZE=$OPTARG
		;;

	v|version  )  echo "$0 -- Version $__ScriptVersion"; exit 0   ;;

	* )  echo -e "\n  Option does not exist : $OPTARG\n"
		  usage; exit 1   ;;

  esac    # --- end of case ---
done
shift $(($OPTIND-1))

echo $WR_SIZE
file_name=`date +%H_%M_%S`
dd if=/dev/zero of=date_${file_name} bs=1M count=${WR_SIZE} ${SYNC}
