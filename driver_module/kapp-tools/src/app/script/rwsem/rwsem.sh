#!/bin/bash

__ScriptVersion="1.0"

order=0
#===  FUNCTION  ================================================================
#         NAME:  usage
#  DESCRIPTION:  Display usage information.
#===============================================================================
function usage ()
{
	echo "Usage :  $0 [options] [--]

    Options:
    -h|help       Display this message
    -v|version    Display script version"

}    # ----------  end of function usage  ----------

#./kapp-tool inject --rwsem --rddown --msg "rddown 2 cnt 4" &
#./kapp-tool inject --rwsem --wrdown --msg "wrdown 4 cnt 6" &

function runrwsem()
{
	./kapp-tool inject --rwsem $1 "$2 order $order" &
	order=`expr $order + 1`
}

function rwsem_multil_read_down()
{
	runrwsem --rddown "rddown 1"
	runrwsem --rddown "rddown 2"
	runrwsem --rddown "rddown 3"
}

function rwsem_multil_read_up()
{
	runrwsem --rdup "rdup 1"
	runrwsem --rdup "rdup 2"
	runrwsem --rdup "rdup 3"
}

MODE=""
#  Handle command line arguments
#-----------------------------------------------------------------------

while getopts ":hvm:" opt
do
  case $opt in

	h|help     )  usage; exit 0   ;;
	m|mode     )
		MODE=$OPTARG
		;;

	v|version  )  echo "$0 -- Version $__ScriptVersion"; exit 0   ;;

	* )  echo -e "\n  Option does not exist : $OPTARG\n"
		  usage; exit 1   ;;

  esac    # --- end of case ---
done
shift $(($OPTIND-1))

if [ $MODE == "1" ]
then
	rwsem_multil_read_down	
	read -p "input Y to exit\n"  answer
	if [ $answer == "Y" ]
	then
		rwsem_multil_read_up
	fi

elif [[ $MODE == "2" ]]; then
	::	
elif [[ $MODE == "2" ]]; then
	::
elif [[ $MODE == "2" ]]; then
	::
fi
