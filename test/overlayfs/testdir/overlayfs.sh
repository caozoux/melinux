#!/bin/bash


__ScriptVersion="version"

#===  FUNCTION  ================================================================
#         NAME:  usage
#  DESCRIPTION:  Display usage information.
#===============================================================================
function usage ()
{
	echo "Usage :  $0 [options] [--]

    Options:
    -h|help       Display this message
    -i|install    mount overlayfs 
    -u|uinstall   unmount overlayfs 
    -t|test       test overlayfs 
    -v|version    Display script version"

}    # ----------  end of function usage  ----------

#-----------------------------------------------------------------------
#  Handle command line arguments
#-----------------------------------------------------------------------

curdir=`pwd`

while getopts ":hviut" opt
do
  case $opt in

	h|help     )  usage; exit 0   ;;

	i|install  )
		echo "zz"
		sudo sudo mount -t overlay overlay -o lowerdir=lower,upperdir=upper,workdir=work merged
		exit 0   ;;

	u|unstall  )
		rm ./upper/* -rf
		sudo umount merged
		
		exit 0   ;;

	t|test  )
		../app/test -f ./merged/file1
		exit 0   ;;

	h|help     )  usage; exit 0   ;;

	v|version  )  echo "$0 -- Version $__ScriptVersion"; exit 0   ;;

	* )  echo -e "\n  Option does not exist : $OPTARG\n"
		  usage; exit 1   ;;

  esac    # --- end of case ---
done
shift $(($OPTIND-1))
