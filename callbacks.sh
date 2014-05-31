#!/bin/bash

#DEBUG=1

die()
{
	echo $1 >&2
	exit 1
}

usage()
{
	echo "TODO: Fill usage in!"
}

OPTS=$(getopt -n $0 --options lt:d: --longoptions list,type:,data: -- "$@")

# Die if they fat finger arguments, this program will be run as root
[ $? = 0 ] || die "Error parsing arguments. Try $PROGRAM_NAME --help"

eval set -- "$OPTS"
while true; do
	case $1 in
		-l|--list)
			echo "media"; exit 0;
		;;
		-t|--type)
			shift
			case $1 in
				media)
					operation=$1
					;;
				*)
					die "Error: Unsupported type $1!"
					;;
			esac
			shift; continue
		;;
		-h|--help)
			usage
			exit 0
		;;
		-v|--version)
			printf "%s, version %s\n" "$PROGRAM_NAME" "$PROGRAM_VERSION"
			exit 0
		;;
		-d|--data)
			shift
			DATA=$1
			shift; continue
		;;
		--)
			# no more arguments to parse
			break
		;;
		*)
			printf "Unknown option %s\n" "$1"
			exit 1
		;;
	esac
done

handle_media()
{
	vast_args="$(dirname $0)/bin/rds -t --rds"

	eval $(echo $1 | sed 's/^{//;s/}$//;s/,\s*/\n/g;s/"\([^"]*\)"\:\s*"\([^"]*\)"/\1="\2"/g')

	artist="$(echo $Media | sed -n '/-/p' | cut -d- -f1 | sed 's/^\s*//g;s/\s*$//g')"
	title="$(echo $Media | sed -n '/-/p' | cut -d- -f2 | sed 's/^\s*//g;s/\s*\.ogg\s*$//g;')"

	if [ -n "$artist" ] && [ -n "$title" ]; then
		vast_args="$vast_args --artist \"$artist\" --title \"$title\""
	elif [ -n "$Media" ]; then
		vast_args="$vast_args --rds-text \"${Media%.ogg}\""
	fi

	echo $vast_args
}

case $operation in
	media)
		value=$(handle_media "$DATA")
		if [ -n "$DEBUG" ]; then
			echo $value

			# Log to a file as well
			date >> callback_debug.log
			echo $value >> callback_debug.log
			echo >> callback_debug.log
		fi

		volume=$(amixer | grep 'Front Left:' | awk '{print $4}')
		amixer set PCM -- 0 mute
		sleep 1
		eval $value &
		sleep 2
		amixer set PCM -- $volume unmute
		;;
	*)
		die "You must specify a callback type!"
		;;
esac
