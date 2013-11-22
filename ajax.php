<?php

$path = posix_getpwuid(posix_getuid())['dir'];
exec("if cat /proc/asound/cards | sed -n '/\s[0-9]*\s\[/p' | grep -iq vast; then echo 1; else echo 0; fi", $output, $return_val);
$fm_audio = ($output[0] == 1 ? 1 : 0);
unset($output);
//TODO: check return

if ( !empty($_GET['fm']) && $_GET['fm'] == "enabled" )
{
	if (file_exists("$path/.asoundrc"))
	{
		exec("sed -i 's/card \([0-9]*\)/card $fm_audio/' ~/.asoundrc && cat ~/.asoundrc", $output, $return_val);
		echo "enabled";
		unset($output);
		//TODO: check return
	}
	else
	{
		exec("echo -e 'pcm.!default {\n\ttype hw\n\tcard $fm_audio\n\tdevice 0\n}\n' > ~/.asoundrc", $output, $return_val);
		echo "enabled";
	}
}
elseif ( !empty($_GET['fm']) && $_GET['fm'] == "disabled" )
{
	if (file_exists("$path/.asoundrc"))
	{
		exec("sed -i 's/card \([0-9]*\)/card 0/' ~/.asoundrc && cat ~/.asoundrc", $output, $return_val);
		echo "disabled";
		unset($output);
		//TODO: check return
	}
	else
		echo "disabled, no file";
}

?>
