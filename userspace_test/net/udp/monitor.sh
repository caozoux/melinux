#!/bin/bash


__ScriptVersion="version"
INTERFACE_NAME=""
cmdline=""

#===  FUNCTION  ================================================================
#         NAME:  usage
#  DESCRIPTION:  Display usage information.
#===============================================================================
function usage ()
{
	echo "Usage :  $0 [options] [--]

    Options:
    -h|help       Display this message
    -i|interface  interface name
    -c|cmdline    cmdline
    -v|version    Display script version"

}    # ----------  end of function usage  ----------

#-----------------------------------------------------------------------
#  Handle command line arguments
#-----------------------------------------------------------------------

while getopts ":hvi:c:" opt
do
  case $opt in

	h|help     )  usage; exit 0   ;;

	i|interface ) 
		INTERFACE_NAME=$OPTARG	
		;;

	c|cmdline ) 
		cmdline=$OPTARG
		;;
	v|version  )  echo "$0 -- Version $__ScriptVersion"; exit 0   ;;

	* )  echo -e "\n  Option does not exist : $OPTARG\n"
		  usage; exit 1   ;;

  esac    # --- end of case ---
done

shift $(($OPTIND-1))


INTERFACE_LIST_OLD=($(ifconfig $INTERFACE_NAME | grep dropp | awk '{print $5}'))
old_udp_rec=`netstat -s -u | sed -n '7p' | awk '{print $1}'`
old_udp_porterr=`netstat -s -u | sed -n '8p' | awk '{print $1}'`
old_udp_send=`netstat -s -u | sed -n '9p' | awk '{print $1}'`
old_udp_recbuferr=`netstat -s -u | sed -n '10p' | awk '{print $1}'`
old_udp_senbuferr=`netstat -s -u | sed -n '11p' | awk '{print $1}'`
#netstat -s -u | sed -n '5,12p'


$cmdline

new_udp_rec=`netstat -s -u | sed -n '7p' | awk '{print $1}'`
new_udp_porterr=`netstat -s -u | sed -n '8p' | awk '{print $1}'`
new_udp_send=`netstat -s -u | sed -n '9p' | awk '{print $1}'`
new_udp_recbuferr=`netstat -s -u | sed -n '10p' | awk '{print $1}'`
new_udp_senbuferr=`netstat -s -u | sed -n '11p' | awk '{print $1}'`
INTERFACE_LIST_NEW=($(ifconfig $INTERFACE_NAME | grep dropp | awk '{print $5}'))


echo "udp rec:         `expr $new_udp_rec - $old_udp_rec`"
echo "udp porterr:     `expr $new_udp_porterr - $old_udp_porterr`"
echo "udp send:        `expr $new_udp_send - $old_udp_send`"
echo "udp recbufferr:  `expr $new_udp_recbuferr - $old_udp_recbuferr`"
echo "udp sendbufferr: `expr $new_udp_senbuferr - $old_udp_senbuferr`"
echo "$INTERFACE_NAME RX drop: `expr ${INTERFACE_LIST_NEW[0]} - ${INTERFACE_LIST_OLD[0]}`"
echo "$INTERFACE_NAME TX drop: `expr ${INTERFACE_LIST_NEW[1]} - ${INTERFACE_LIST_OLD[1]}`"
