
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

<ol>
<li>Copy/paste the sample script into your favorite text editor.</li>
<li>Edit the artist/song/station information near the top</li>
<li>Save it as something you'll remember with a ".sh" extension.</li>
(If you're using Windows/Notepad, you may need to look up how to
make sure the file extension isn't ".txt")

<li>Upload to the "Scripts" tab on the <a href="uploadfile.php" target="_blank">upload page</a>.</li>
</ol>

<pre>
#!/bin/sh

##### EDIT HERE #####

FREQUENCY=87.9

ARTIST="Artist Name"
TITLE="Song Title"

# Station cannot be more than 8 characters
STATION="VAST"

##### DO NOT EDIT PAST THIS #####

sudo /opt/fpp/plugins/vastfmt/bin/rds -vvvvv -t \
     -f $FREQUENCY \
     --artist "$ARTIST" \
     --title "$TITLE" \
     --rds-station "$STATION" \ &> /home/pi/media/logs/vastfmt_rds.log
</pre>

</fieldset>
</div>
<br />

