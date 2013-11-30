<?php

exec("if cat /proc/asound/cards | sed -n '/\s[0-9]*\s\[/p' | grep -iq vast; then echo 1; else echo 0; fi", $output, $return_val);
$fm_audio = ($output[0] == 1 ? 1 : 0);
unset($output);
//TODO: check return

if ( !empty($_GET['fm']) && $_GET['fm'] == "enabled" )
{
	exec("sudo sed -i.bak 's/defaults.ctl.card\ [0-9]/defaults.ctl.card 1/;s/defaults.pcm.card\ [0-9]/defaults.pcm.card 1/' /usr/share/alsa/alsa.conf", $output, $return_val);
	echo "enabled";
	unset($output);
	//TODO: check return
}
elseif ( !empty($_GET['fm']) && $_GET['fm'] == "disabled" )
{
	exec("sudo sed -i 's/defaults.ctl.card\ [0-9]/defaults.ctl.card 0/;s/defaults.pcm.card\ [0-9]/defaults.pcm.card 0/' /usr/share/alsa/alsa.conf", $output, $return_val);
	echo "disabled";
	unset($output);
	//TODO: check return
}

?>
