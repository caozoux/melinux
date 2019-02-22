#!/bin/bash

__ScriptVersion="1.0"

#===  FUNCTION  ================================================================
#         NAME:  usage
#  DESCRIPTION:  Display usage information.
#===============================================================================
function usage ()
{
	echo "Usage :  $0 [options] [--]

    Options:
    -h|help       Display this message
    -c|cach-size  read cache size;
    -d|anon-size  page size
    -v|version    Display script version"

}    # ----------  end of function usage  ----------

#-----------------------------------------------------------------------
#  Handle command line arguments
#-----------------------------------------------------------------------

ARGS_CACHE_SIZE=""
ARGS_ANON_SIZE=""
while getopts ":hvc:d:" opt
do
  case $opt in

	h|help     )  usage; exit 0   ;;

	c|cache-size )
		ARGS_CACHE_SIZE=$OPTARG
		;;

	d|anon-size )
		ARGS_ANON_SIZE=$OPTARG
		;;

	v|version  )  echo "$0 -- Version $__ScriptVersion"; exit 0   ;;

	* )  echo -e "\n  Option does not exist : $OPTARG\n"
		  usage; exit 1   ;;

  esac    # --- end of case ---
done
shift $(($OPTIND-1))



