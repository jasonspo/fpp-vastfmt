#!/bin/bash

#DEBUG=1
DEBUG_LOG=${DEBUG_LOG:-${LOGDIR}/vastfmt.log}

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

if [ -n "$DEBUG" ]; then
	echo "Full args: $*" >> $DEBUG_LOG
fi

# Die if they fat finger arguments, this program will be run as root
[ $? = 0 ] || die "Error parsing arguments. Try $PROGRAM_NAME --help"

eval set -- "$OPTS"
while true; do
	case $1 in
		-l|--list)
			echo "media,playlist"; exit 0;
		;;
		-t|--type)
			shift
			case $1 in
				media|playlist)
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
			DATA=$(echo $1 | tr -dc '[:alnum:][:space:][:punct:]')
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

killall rds
vast_args="$(dirname $0)/bin/rds"

if [ ! -e ${CFGDIR}/plugin.vastfmt ]; then
	echo "Error finding plugin settings, please configure FMT!" >&2
	exit 1
fi

transmit_setting=$(grep -i "^TurnOff\s*=.*" ${CFGDIR}/plugin.vastfmt | sed -e "s/.*=\s*//")
power=$(grep -i "^Power\s*=.*" ${CFGDIR}/plugin.vastfmt | sed -e "s/.*=\s*//")
rds_setting=$(grep -i "^RdsType\s*=.*" ${CFGDIR}/plugin.vastfmt | sed -e "s/.*=\s*\"\(.*\)\"/\1/")
set_frequency=$(grep -i "^SetFreq\s*=.*" ${CFGDIR}/plugin.vastfmt | sed -e "s/.*=\s*//")
frequency=$(grep -i "^Frequency\s*=.*" ${CFGDIR}/plugin.vastfmt | sed -e "s/.*=\s*//")
station=$(grep -i "^Station\s*=.*" ${CFGDIR}/plugin.vastfmt | sed -e "s/.*=\s*\"\(.*\)\"/\1/")

if [ -n "$DEBUG" ]; then

	echo "$DATA" >&2

	echo "set transmit: $transmit_setting" >&2
	echo "power: $power" >&2
	echo "rds type: $rds_setting" >&2
	echo "set freq: $set_frequency" >&2
	echo "frequency: $frequency" >&2
	echo "station: $station" >&2

fi


case $operation in
	media)
		# Without this sleep, sometimes our playlist start and
		# media callback are stacked too close to each other and
		# seem to upset the VastFMT
		sleep 1


		if [ "x$rds_setting" != "xdisabled" ]; then
			vast_args="$vast_args --rds"
		fi

		# Use jq to parse our JSON data
		artist=$(echo $DATA | $(dirname $0)/jq/jq -r ".artist")
		title=$(echo $DATA | $(dirname $0)/jq/jq -r ".title")

		# Try to grab artist if Media exists.  This assumes the filename
		# is "Artist - Title.extension"
		if [ -z "$artist" ] && [ -n "$Media" ]; then
			artist="$(echo $Media | sed -n '/-/p' | cut -d- -f1 | sed 's/^\s*//g;s/\s*$//g')"
		fi
		# Try to grab title if Media exists.  This assumes the filename
		# is "Artist - Title.extension"
		if [ -z "$title" ] && [ -n "$Media" ]; then
			title="$(echo $Media | sed -n '/-/p' | cut -d- -f2 | sed 's/^\s*//g;s/\s*\.\(ogg\|mp4\|mkv\|mp3\)\s*$//g;')"
		fi

		if [ -z "$rds_setting" ]; then
			rds_setting=rtp
		fi

		case $rds_setting in
			rtp)
				if [ -n "$artist" ] && [ -n "$title" ]; then
					vast_args="$vast_args --artist \"$artist\" --title \"$title\""
				elif [ -n "$artist" ]; then
					vast_args="$vast_args --artist \"$artist\""
				elif [ -n "$title" ]; then
					vast_args="$vast_args --title \"$title\""
				elif [ -n "$Media" ]; then
					vast_args="$vast_args --title \"${Media%.*}\""
				elif [ -n "$Sequence" ]; then
					vast_args="$vast_args --title \"${Sequence%.*}\""
				fi
			;;
			rt)
				if [ -n "$artist" ] && [ -n "$title" ]; then
					vast_args="$vast_args -R \"$artist - $title\""
				elif [ -n "$Media" ]; then
					vast_args="$vast_args -R \"${Media%.*}\""
				elif [ -n "$artist" ]; then
					vast_args="$vast_args -R \"$artist\""
				elif [ -n "$title" ]; then
					vast_args="$vast_args -R \"$title\""
				elif [ -n "$Sequence" ]; then
					vast_args="$vast_args -R \"${Sequence%.*}\""
				fi
			;;
			ps|disabled|*)
				if [ -n "$DEBUG" ]; then
					echo "RDS Setting $rds_setting doesn't need to write out individual songs, do nothing" >&2
				fi
			;;
		esac

		if [ -n "$DEBUG" ]; then
			echo $vast_args >&2

			# Log to a file as well
			date >> ${DEBUG_LOG}
			echo $vast_args >> ${DEBUG_LOG}
			echo >> ${DEBUG_LOG}
		fi

		if [ -n "$DEBUG" ]; then
			eval timeout 5 $vast_args >> ${DEBUG_LOG} 2>&1
		else
			eval timeout 5 $vast_args
		fi
		;;
	playlist)
		# Without this sleep, sometimes our playlist start and
		# media callback are stacked too close to each other and
		# seem to upset the VastFMT
		sleep 1

		if echo $DATA | grep "Action" | grep -qc "start"; then
			echo "turn things on..."
			if [ "x$transmit_setting" == "x1" ]; then
				vast_args="$vast_args -t"
				if [ -n "$power" ]; then
					vast_args="$vast_args -p $power"
				fi
			fi
			if [ "x$set_frequency" == "x1" ]; then
				vast_args="$vast_args -f $frequency"
			fi
			if [ "x$rds_setting" != "xdisabled" ]; then
				if [ ${#station} -gt 0 ]; then
					vast_args="$vast_args --rds -s \"$station\""
				else
					vast_args="$vast_args --rds"
				fi
			fi
		else
			echo "turn things off..."
			if [ "x$transmit_setting" == "x1" ]; then
				vast_args="$vast_args -n"
			fi
		fi

		if [ -n "$DEBUG" ]; then
			echo $vast_args >&2

			# Log to a file as well
			date >> ${DEBUG_LOG}
			echo $vast_args >> ${DEBUG_LOG}
			echo >> ${DEBUG_LOG}
		fi

		if [ -n "$DEBUG" ]; then
			eval timeout 5 $vast_args >> ${DEBUG_LOG} 2>&1
		else
			eval timeout 5 $vast_args
		fi
		;;
	*)
		die "You must specify a callback type!"
		;;
esac
