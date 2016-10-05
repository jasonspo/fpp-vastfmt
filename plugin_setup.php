<?php

exec("if cat /proc/asound/cards | sed -n '/\s[0-9]*\s\[/p' | grep -iq vast; then echo 1; else echo 0; fi", $output, $return_val);
if ( $return_val )
{
	error_log("Failed our command to check for the FM transmitter");
}
$fm_audio = ($output[0] == 1);
unset($output);

?>


<div id="transmitterinfo" class="settings">
<fieldset>
<legend>Transmitter Information</legend>
<p>Vast Electronics V-FMT212R: <?php echo ( $fm_audio ? "<span class='good'>Detected</span>" : "<span class='bad'>Not Detected</span>" ); ?></p>

<?php if ( $fm_audio ): ?>
<p>To configure FPP to use the FM transmitter for audio output, go to the <a href="/settings.php">settings page</a> and select "Transmitter" from the drop-down for "Audio Output Device".</p>
<?php else: ?>
<p>To use the VastFMT for audio output, please plug in device and reboot.</p>
<?php endif; ?>

</fieldset>
</div>

<br />

<?php if (function_exists('PrintSettingSave')): ?>

<div id="transmittersettings" class="settings">
<fieldset>
<legend>Transmitter Settings</legend>

<script>
function togglePower()
{
	if ($('#TurnOff').is(':checked'))
	{
		$('#Power').prop("disabled", false);
		$('#savePower').prop("disabled", false);
	}
	else
	{
		$('#Power').prop("disabled", true);
		$('#savePower').prop("disabled", true);
	}
}
function toggleSettings()
{
	if ($('#SetFreq').is(':checked'))
	{
		$('#Frequency').prop("disabled", false);
		$('#saveFrequency').prop("disabled", false);
	}
	else
	{
		$('#Frequency').prop("disabled", true);
		$('#saveFrequency').prop("disabled", true);
	}
}
function toggleStation()
{
	if ($('#RdsType').val() == "disabled")
	{
		$('#RDSStaticText').prop("disabled", true);
		$('#saveRDSStaticText').prop("disabled", true);
		$('#Station').prop("disabled", true);
		$('#saveStation').prop("disabled", true);
	}
	else
	{
		$('#RDSStaticText').prop("disabled", false);
		$('#saveRDSStaticText').prop("disabled", false);
		$('#Station').prop("disabled", false);
		$('#saveStation').prop("disabled", false);
	}
}

$(function(){toggleSettings();toggleStation();togglePower();});
</script>

<p>Toggle transmitter with playlist: <?php PrintSettingCheckbox("Turn off", "TurnOff", 1, 0, "1", "0", "vastfmt", "togglePower"); ?></p>
<p>Power: <?php PrintSettingText("Power", 1, 0, 3, 3, "vastfmt", "88"); ?>dB&mu;V <?php PrintSettingSave("Power", "Power", 1, 0, "vastfmt"); ?></p>
<p>Set frequency on playlist start/stop: <?php PrintSettingCheckbox("Set frequency", "SetFreq", 1, 0, "1", "0", "vastfmt", "toggleSettings"); ?></p>
<p>Frequency: <?php PrintSettingText("Frequency", 1, 0, 8, 8, "vastfmt"); ?>MHz <?php PrintSettingSave("Transmit Frequency", "Frequency", 1, 0, "vastfmt"); ?></p>
<p>RDS Type: <?php PrintSettingSelect("RDS Type", "RdsType", 1, 0, "RT+", Array("Disabled"=>"disabled", "RT+"=>"rtp", "RT"=>"rt", "PS"=>"ps"), "vastfmt", "toggleStation"); ?></p>
<p>RDS Static Text: <?php PrintSettingText("RDSStaticText", 1, 0, 50, 50, "vastfmt"); ?><?php PrintSettingSave("RDS Static Text", "RDSStaticText", 1, 0, "vastfmt"); ?></p>
<p>Station ID: <?php PrintSettingText("Station", 1, 0, 4, 4, "vastfmt"); ?><?php PrintSettingSave("Station ID", "Station", 1, 0, "vastfmt"); ?></p>

</fieldset>
</div>

<br />

<div id="plugininfo" class="settings">
<fieldset>
<legend>Plugin Information</legend>
<p>Instructions
<ul>
<li>Setup transmitter and save to EEPROM, or click the "Toggle transmitter
with playlist" option above.</li>
<li>Change audio on "FPP Settings" page.  Go to the FPP Settings screen and
select the Vast as your sound output instead of the Pi's built-in audio.</li>
<li>Tag your MP3s/OGG files appropriate.  The tags are used to set the Artist
and Title fields for RDS's RT+ text. The rest will happen auto-magically!</li>
</ul>
</p>

<?php else: ?>

<div id="rds" class="settings">
<fieldset>
<legend>RDS Support Instructions</legend>

<p style="color: red;">You're running an old version of FPP that doesn't yet contain the required
helper functions needed by this plugin. Advanced features are disabled.</p>

<p>You must first set up your Vast V-FMT212R using the Vast Electronics
software and save it to the EEPROM.  Once you have your VAST setup to transmit
on your frequency when booted, you can plug it into the Raspberry Pi and
reboot.  You will then go to the FPP Settings screen and select the Vast as
your sound output instead of the Pi's built-in audio.</p>

<?php endif; ?>

<p>Known Issues:
<ul>
<li>VastFMT will "crash" and be unable to receive RDS data if not used with
a powered USB hub.  If this happens, the transmitter must be unplugged and re-
plugged into the Pi - <a target="_new"
href="https://github.com/Materdaddy/fpp-vastfmt/issues/2">Bug 2</a></li>
</ul>

Planned Features:
<ul>
<li>Saving settings to EEPROM.</li>
</ul>

<p>To report a bug, please file it against the fpp-vastfmt plugin project here:
<a href="https://github.com/Materdaddy/fpp-vastfmt/issues/new" target="_new">fpp-vastfmt GitHub Issues</a></p>

</fieldset>
</div>
<br />
