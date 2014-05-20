<script language="Javascript">

function ToggleFmTransmitter() {
	if ($('#useFmTransmitter').is(':checked')) {
		SetUseFmTransmitter(1);
	} else {
		SetUseFmTransmitter(0);
	}
}

function SetUseFmTransmitter(enabled)
{
	var xmlhttp=new XMLHttpRequest();
	var url = "plugin.php?nopage=1&plugin=vastfmt&page=ajax.php&fm=" + (enabled ? "enabled" : "disabled");
	xmlhttp.open("GET",url,false);
	xmlhttp.setRequestHeader('Content-Type', 'text/xml');
	xmlhttp.onreadystatechange = function () {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200)
			location.reload(true);
	}
	xmlhttp.send();
}

</script>
<?php
if ( isset($_POST['submit']) )
{
	echo "<html><body>We don't do anything yet, sorry!</body></html>";
	exit(0);
}

$devices=explode("\n",trim(shell_exec("ls -w 1 /dev/ttyUSB*")));

exec("if cat /proc/asound/cards | sed -n '/\s[0-9]*\s\[/p' | grep -iq vast; then echo 1; else echo 0; fi", $output, $return_val);
$fm_audio = ($output[0] == 1);
unset($output);
//TODO: check return

$asound_number = 0;
exec("sudo sed -n '/defaults.pcm.card/s/defaults.pcm.card.\([0-9]*\)/\\1/p' /usr/share/alsa/alsa.conf", $output, $return_val);
if ( $return_val == 0 )
{
	$asound_number = $output[0];
}
unset($output);

?>


<div id="usbaudio" class="settings">
<fieldset>
<legend>FM Transmitter Audio</legend>
<br />
Vast Electronics V-FMT212R: <?php echo ( $fm_audio ? "<span class='good'>Detected</span>" : "<span class='bad'>Not Detected</span>" ); ?>
<hr>
<FORM NAME="vastfmt_form" ACTION="<?php echo $_SERVER['PHP_SELF'] ?>" METHOD="POST">

<input type="checkbox" id="useFmTransmitter" onChange='ToggleFmTransmitter();'
<?php if ( $asound_number != 0 ) echo "checked"; ?> />
<label for="useFmTransmitter">Use V-FMT212R for audio output</label>
     </FORM>
</fieldset>
</div>

<br />

<div id="rds" class="settings">
<fieldset>
<legend>RDS Support Instructions</legend>

<p>Name your media file in the following format:
<br />
&lt;Artist Name&gt; - &lt;Song Title&gt;.ogg</p>

<p>The rest will happen auto-magically.</p>

<p>If the filename isn't parsed appropriately for artist and song title, the
filename will simply be passed as RT text instead of utilizing RT+.</p>

<p>To report a bug, please file it against the fpp-vastfmt plugin project here:
<a href="https://github.com/Materdaddy/fpp-vastfmt/issues/new" target="_new">fpp-vastfmt GitHub Issues</a></p>

</fieldset>
</div>
<br />

