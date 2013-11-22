<?php

$path = trim(`sudo bash -c "cd && pwd"`);
exec("if cat /proc/asound/cards | sed -n '/\s[0-9]*\s\[/p' | grep -iq vast; then echo 1; else echo 0; fi", $output, $return_val);
$fm_audio = ($output[0] == 1 ? 1 : 0);
unset($output);
//TODO: check return

if ( !empty($_GET['fm']) && $_GET['fm'] == "enabled" )
{
	error_log("checking $path/.asoundrc");
	exec("sudo bash -c \"echo -e 'pcm.!default {\n\ttype hw\n\tcard $fm_audio\n\tdevice 0\n}\n' > ~/.asoundrc\"", $output, $return_val);
	echo "enabled";
	//TODO: check return
}
elseif ( !empty($_GET['fm']) && $_GET['fm'] == "disabled" )
{
	exec("sudo bash -c \"sed -i 's/card \([0-9]*\)/card 0/' ~/.asoundrc\"", $output, $return_val);
	echo "disabled";
	unset($output);
	//TODO: check return
}

?>
