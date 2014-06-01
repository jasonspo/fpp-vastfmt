<?php

exec("if cat /proc/asound/cards | sed -n '/\s[0-9]*\s\[/p' | grep -iq vast; then echo 1; else echo 0; fi", $output, $return_val);
$fm_audio = ($output[0] == 1);
unset($output);
//TODO: check return

?>


<div id="usbaudio" class="settings">
<fieldset>
<legend>FM Transmitter Audio</legend>
<br />
Vast Electronics V-FMT212R: <?php echo ( $fm_audio ? "<span class='good'>Detected</span>" : "<span class='bad'>Not Detected</span>" ); ?>
</fieldset>
</div>

<br />

<div id="rds" class="settings">
<fieldset>
<legend>RDS Support Instructions</legend>

<p>You must first set up your Vast V-FMT212R using the Vast Electronics
software and save it to the EEPROM.  Once you have your VAST setup to transmit
on your frequency when booted, you can plug it into the Raspberry Pi and
reboot.  You will then go to the FPP Settings screen and select the Vast as
your sound output instead of the Pi's built-in audio.</p>

<p>Tag your MP3s/OGG files appropriate.  The tags are used to set the Artist
and Title fields for RDS's RT+ text. The rest will happen auto-magically!</p>

<p>Known Issues:
<ul>
<li>Scratchy audio when writing RDS data between songs - <a target="_new"
href="https://github.com/Materdaddy/fpp-vastfmt/issues/1">Bug 1</a></li>

<li>Possible volume gap between songs (as part of a scratchy audio
work-around) - <a target="_new"
href="https://github.com/Materdaddy/fpp-vastfmt/issues/1">Bug 1</a></li>

<li>Vast FMT will "crash" and be unable to receive RDS data if not used with
a powered USB hub.  If this happens, a cold boot is required (completely
remove power from the VAST/Pi) - <a target="_new"
href="https://github.com/Materdaddy/fpp-vastfmt/issues/2">Bug 2</a></li>

<li>Callback parsing variables incorrectly - <a target="_new"
href="https://github.com/Materdaddy/fpp-vastfmt/issues/3">Bug 3</a></li>
</ul>

Planned Features:
<ul>
<li>Setup of the Vast and writing to EEPROM through this page.
</ul>

<p>To report a bug, please file it against the fpp-vastfmt plugin project here:
<a href="https://github.com/Materdaddy/fpp-vastfmt/issues/new" target="_new">fpp-vastfmt GitHub Issues</a></p>

</fieldset>
</div>
<br />

