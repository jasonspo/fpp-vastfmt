<?php
       
    $curGpio = ReadSettingFromFile("ResetPin", "fpp-vastfmt");

    $defaultGPIO = "4";
    if ($settings['Platform'] == "BeagleBone Black") {
        $defaultGPIO = "99";
    }
    echo "<!-- " . $curGpio . "     " . $defaultGPIO . "  -->\n";
    $data = file_get_contents('http://127.0.0.1:32322/gpio');
    $gpiojson = json_decode($data, true);
    $gpioPins = Array();
    foreach($gpiojson as $gpio) {
        $pn = $gpio['pin'] . ' (GPIO: ' . $gpio['gpio'] . ')';
        $gpioPins[$pn] = $gpio['gpio'];
        
        if ($curGpio == $gpio['pin']) {
            $defaultGPIO = $gpio['gpio'];
            $pluginSettings["ResetPin"] = $defaultGPIO;
            WriteSettingToFile("ResetPin", $defaultGPIO, "fpp-vastfmt");
        }
    }
    echo "<!-- " . $curGpio . "     " . $defaultGPIO . "  -->\n";
?>

<script type="text/javascript">

function OnConnectionChanged() {
    var value = $('#Connection').val();
    if (value == "USB") {
        $('#ResetPinInfo').hide();
    } else {
        $('#ResetPinInfo').show();
    }
}
</script>


<div id="VASTFMTPluginhw" class="settings">
<fieldset>
<legend>VAST-FMT/Si4713 Hardware</legend>
<p>Connection: <?php PrintSettingSelect("Connection", "Connection", 2, 0, "USB", Array("USB"=>"USB", "I2C"=>"I2C"), "fpp-vastfmt", "OnConnectionChanged"); ?></p>
<p class="ResetPinInfo" id="ResetPinInfo">Reset GPIO: <?php PrintSettingSelect("ResetPin", "ResetPin", 2, 0, $defaultGPIO, $gpioPins, "fpp-vastfmt", ""); ?><br />
I2C connection requires a GPIO pin to reset/enable the Si4713.</p>
</fieldset>
</div>

<br />

<div id="VASTFMTPluginsettings" class="settings">
<fieldset>
<legend>VAST-FMT/Si4713 Plugin Settings</legend>
<p>Start at: <?php PrintSettingSelect("Start", "Start", 2, 0, "FPPDStart", Array("FPPD Start (default)"=>"FPPDStart", "Playlist Start"=>"PlaylistStart", "Never - RDS Only"=>"RDSOnly", "Never"=>"Never"), "fpp-vastfmt", ""); ?><br />
At Start, the hardware is reset, FM settings initialized, will broadcast any audio played, and send static RDS messages (if enabled).</p>
<p>Stop at: <?php PrintSettingSelect("Stop", "Stop", 2, 0, "Never", Array("Playlist Stop"=>"PlaylistStop", "Never (default)"=>"Never"), "fpp-vastfmt", ""); ?><br />
At Stop, the hardware is reset. Listeners will hear static.</p>
<p>Enable Volume Change Hack for Vast-FMT 212R: <?php PrintSettingCheckbox("EnableVolumeChangeHack", "EnableVolumeChangeHack", 2, 0, "1", "0", "fpp-vastfmt", ""); ?></p>
</fieldset>
</div>

<br />

<div id="VASTFMTsettings" class="settings">
<fieldset>
<legend>VAST-FMT/Si4713 FM Settings</legend>
<p>Frequency (76.00-108.00): <?php PrintSettingTextSaved("Frequency", 2, 0, 6, 6, "fpp-vastfmt", "100.10"); ?>MHz</p>
<p>Power (88-115, 116-120<sup>*</sup>): <?php PrintSettingTextSaved("Power", 2, 0, 3, 3, "fpp-vastfmt", "110"); ?>dB&mu;V
<br /><sup>*</sup>Can be set as high as 120dB&mu;V, but voltage accuracy above 115dB&mu;V is not guaranteed.</p>
<p>Preemphasis: <?php PrintSettingSelect("Preemphasis", "Preemphasis", 2, 0, "75us", Array("50&mu;s (Europe, Australia, Japan)"=>"50us", "75&mu;s (USA, default)"=>"75us"), "fpp-vastfmt", ""); ?></p>
<p>Antenna Tuning Capacitor (0=Auto, 1-191): <?php PrintSettingText("AntCap", 2, 0, 3, 3, "fpp-vastfmt", "0"); ?> * 0.25pF </p>
</fieldset>
</div>

<br />

<div id="VASTFMTRDSsettings" class="settings">
<fieldset>
<legend>VAST-FMT/Si4713 RDS Settings</legend>
<p>Enable RDS: <?php PrintSettingCheckbox("EnableRDS", "EnableRDS", 2, 0, "True", "False", "fpp-vastfmt", ""); ?></p>
<p>RDS Station - Sent 8 characters at a time.  Max of 64 characters.<br />
Station Text: <?php PrintSettingTextSaved("StationText", 2, 0, 64, 32, "fpp-vastfmt", "Merry   Christ- mas"); ?>

<br />

<p>RDS Text: <?php PrintSettingTextSaved("RDSTextText", 2, 0, 64, 32, "fpp-vastfmt", "[{Artist} - {Title}]"); ?>
<p>
Place {Artist} or {Title} where the media artist/title should be placed. Area's wrapped in brackets ( [] ) will not be output unless media is present.


<p>Program Type (PTY North America / Europe): <?php PrintSettingSelect("Pty", "Pty", 2, 0, 2,
Array(
"0 - None / None"=>0, 
"1 - News / News"=>1, 
"2 - Information / Current Affairs"=>2, 
"3 - Sport / Information"=>3, 
"4 - Talk / Sport"=>4, 
"5 - Rock / Education"=>5, 
"6 - Classic Rock / Drama"=>6, 
"7 - Adult Hits / Culture"=>7, 
"8 - Soft Rock / Science"=>8, 
"9 - Top 40 / Varied"=>9, 
"10 - Country / Pop"=>10, 
"11 - Oldies / Rock"=>11, 
"12 - Soft Music / Easy Listening"=>12, 
"13 - Nostalgia / Light Classical"=>13, 
"14 - Jazz / Serious Classical"=>14, 
"15 - Classical / Other Music"=>15, 
"16 - R&B / Weather"=>16, 
"17 - Soft R&B / Finance"=>17, 
"18 - Language / Childrens"=>18, 
"19 - Religious Music / Social Affairs"=>19, 
"20 - Religious Talk / Religion"=>20, 
"21 - Personality / Phone-In"=>21, 
"22 - Public / Travel"=>22, 
"23 - College / Leisure"=>23, 
"24 - --- / Jazz"=>24, 
"25 - --- / Country"=>25, 
"26 - --- / National Music"=>26, 
"27 - --- / Oldies"=>27, 
"28 - --- / Folk"=>28, 
"29 - Weather / Documentary"=>29), 
"fpp-vastfmt", ""); ?> - <a href="https://www.electronics-notes.com/articles/audio-video/broadcast-audio/rds-radio-data-system-pty-codes.php">Additional PTY information</a></p>
</fieldset>
</div>

<br />
<script>
OnConnectionChanged()
</script>
