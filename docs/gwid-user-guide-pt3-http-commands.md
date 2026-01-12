# GWID USER GUIDE PART 3:<br>STANDALONE COMMANDS, DISPLAY MODES & PARAMETERS

<br>
The GWID is controlled over the local network using http protocols, where the specific URL path and query strings determine the control actions.

## BASIC GWID CONTROLS USING HTTP RESOURCE REQUESTS

Basic control is accomplished by sending HTTP resource requests to the GWID. Submit via web browser or any other device on the local network (e.g., home automation hub) using http protocols.

Basic syntax is `http://<GWID_IPAddress>/<resource>`

|`/<resource>`|Description|
|:-----|:-----|
|`/beep` |Briefly sounds the piezo buzzer. <br><br>Syntax is `http://<GWID_IPAddress>/beep`|
|`/getstate`|Returns a formatted JSON message with current device settings. (See the section titled [JSON Formatted Responses](#JSON-Formatted-Responses) for details.) <br><br>Syntax is `http://<GWID_IPAddress>/getstate`|
|`/getversion`|Returns a formatted JSON message with device hardware ID, build version, and the build date & time. <br><br>Syntax is `http://<GWID_IPAddress>/getversion`|
|`/report`|Returns an HTML formatted report of device firmware version, network connection, hub IP/port values, and device parameter values. Designed for use with a web browser to help with routine diagnostics and troubleshooting.<br><br>Syntax is `http://<GWID_IPAddress>/report`|
|`/saveurl`|Sets saved IP and port values for synchronizing communication with another device such as a home automation hub. These settings are saved to GWID flash memory and persist across reboots until changed by another `/saveurl` request or the device is "factory reset". Requesting client must be on the same local network.<br><br>Syntax is `http://<GWID_IPAddress>/saveurl?ip=<TargetIPAddress>&port=<TargetPort>`<br>The GWID then displays two blue pixels. <br><br>Omitting `ip=<TargetIPAddress>` automatically uses the ip of the requesting client.<br>Omitting `port=<TargetPort>` automatically sets the port value to 0 (ignore).<br><br>To clear the current and saved values of the IP and Port, use <br>`http://<GWID_IPAddress>/savedurl?clear` <br>The GWID then displays one blue pixel. <br><br>Including the argument `restrict` tells the GWID to reject any resource request to change a device setting unless the request comes from the Saved IP and Port. This includes `/saveurl` and all display mode/parameter controls shown in the next two tables.<br><br>Syntax is `http://<GWID_IPAddress>/saveurl?ip=<TargetIPAddress>&port=<TargetPort>&restrict`<br>The GWID then displays one blue pixel and one red pixel.<br><br>NOTE: sending `/savedurl?clear` from the Saved IP will remove the restriction. A "factory reset" will also remove the restriction but erases WiFi credentials as well.|

<br>

## USING QUERY STRINGS TO SET THE GWID DISPLAY MODE 

The GWID display mode is set by using a query string with specific arguments at the root path to the device. The query string consists of a question mark symbol followed by a `<mode>` or `<mode>=<value>` pair as shown in the following table. Display modes are mutually exclusive.

Basic syntax is `http://<GWID_IPAddress>/?<mode>`

The GWID responds with a JSON formatted message. See the section titled [JSON Formatted Responses](#JSON-Formatted-Responses).

The file [`GWID-User-Guide-Pt4-Animations.md`](GWID-User-Guide-Pt4-Animations.md) contains .gif animations of several modes.

|`/?<mode>`|Description|
|:---------|:-----|
|`/?pattern=<value>`|Sets the color of individual pixels, where \<value> is a 12-character string where each character corresponds to a pixel position, and can be one of r (red) g (green) b (blue) y (yellow) c (cyan) p (purple) w (white) or o (off). Other values and additional characters are ignored.<br><br>Example - to display a pattern of White, Red, Green, Blue, Off, Off, White, Red, Green, Blue, Off, Off, use <br>`http://<GWID_IPAddress>/?pattern=wrgboowrgboo`<br><br>NOTE: This mode should be used in combination with the `level` parameter (see below) to set overall pixel ring brightness. <br>|
|`/?flash`|Flashing effect with all pixels using the same color. <br><br>Syntax is `http://<GWID_IPAddress>/?FLASH`<br><br>NOTE: This mode should be used in combination with the `rgb`, `level`, `speed` and `tone` parameters (see below) to set the flash color, overall pixel ring brightness, the flash rate, and control the piezo buzzer.<br><br>OPTIONAL: Passing the argument `default` flashes white. <br><br>Example - `http://<GWID_IPAddress>/?flash=default`<br>|
|`/?police`|Alternates between strobing blue pixels on one half of the NeoPixel ring, and strobing red pixels on the other half. <br><br>Syntax is `http://<GWID_IPAddress>/?police`<br><br>NOTE: This mode should be used in combination with the `level`, `speed`, and `tone` parameters (see below) to set the overall pixel ring brightness, the strobing rate, and control the piezo buzzer.|
|`/?pulse`|Pulsating effect with all pixels using the same color.<br><br>Syntax is `http://<GWID_IPAddress>/?pulse`<br><br>NOTE: This mode should be used in combination with the `rgb`, `level`, and `speed` parameters (see below) to set the pulse color, overall pixel ring brightness and the pulsating rate.<br><br>OPTIONAL: Passing the argument `default` pulses in white. <br><br>Example - `http://<GWID_IPAddress>/?pulse=default`<br>|
|`/?rotor`|Three evently-spaced pixels of the same color rotating around the ring. <br><br>Syntax is `http://<GWID_IPAddress>/?rotor`<br><br>NOTE: This mode should be used in combination with the `rgb`, `level`, and `speed` parameters (see below) to set the rotor color, overall pixel ring brightness and speed of the rotation.<br><br>OPTIONAL: Passing the argument `default` rotates three white pixels. <br><br>Example - `http://<GWID_IPAddress>/?rotor=default`|
|`/?solid`|All pixels constant with the same color. <br><br>Syntax is `http://<GWID_IPAddress>/?solid`<br><br>NOTE: This mode should be used in combination with the `rgb` and `level` parameters (see below) to set the overall pixel ring color and brightness.<br><br>OPTIONAL: Passing the argument `default` displays all white pixels. <br><br>Example - `http://<GWID_IPAddress>/?solid=default`<br>|
|`/?sos`|Flashes all pixels in an S.O.S. pattern. <br><br>Syntax is `http://<GWID_IPAddress>/?sos`<br><br>NOTE: This mode should be used in combination with the `rgb`, `level`, `speed`, and `tone` parameters (see below) to set the S.O.S. color, overall pixel ring brightness, the rate, and control the piezo buzzer.<br><br>OPTIONAL: Passing the argument `default` flashes S.O.S. in white. <br><br>Example - `http://<GWID_IPAddress>/?sos=default`<br>|
|`/?strobe`|Four evenly-spaced, white pixels strobing on a single color background. <br><br>Syntax is `http://<GWID_IPAddress>/?strobe`<br><br>NOTE: This mode should be used in combination with the `rgb`, `level`, `speed`, and `tone` parameters (see below) to set the strobe color, overall pixel ring brightness, the strobing rate, and control the piezo buzzer.<br><br>OPTIONAL: Passing the argument `default` strobes white on white. <br><br>Example - `http://<GWID_IPAddress>/?strobe=default`<br>|
|`/?trail`|A single pixel moving around the ring with a trail of fading pixels behind it. The length of the trail is impacted by the intensity of the `rgb` color and the brightness `level` settings.<br><br>Syntax is `http://<GWID_IPAddress>/?trail`<br><br>NOTE: This mode should be used in combination with the `rgb`, `level`, and `speed`  parameters (see below) to set the pixel color, overall pixel ring brightness and speed of the motion.<br><br>OPTIONAL: Passing the argument `default` uses a white dot and trail. <br><br>Example - `http://<GWID_IPAddress>/?trail=default`|
|`/?rainbow`|Simply for fun! Displays a rotating color rainbow.<br><br>Syntax is `http://<GWID_IPAddress>/?rainbow`<br><br>NOTE: This mode should be used in combination with the `level` and `speed` parameters (see below) to set the overall pixel ring brightness and speed of the rotation.<br><br>OPTIONAL: Passing the argument `default` uses default parameters for this display mode.<br><br>Example - `http://<GWID_IPAddress>/?rainbow=default`|


<br>

## USING QUERY STRINGS TO MODIFY DISPLAY MODE PARAMETERS

Various display mode parameters are also set by including arguments in the query string at the root path to the device. The following table contains the complete list of parameters. 

Basic syntax is `http://<GWID_IPAddress>/?<parameter>=<value>`

The GWID responds with a JSON formatted message. See the section titled [JSON Formatted Responses](#JSON-Formatted-Responses).


|`/?<parameter>=<value>`|Description|
|:-----|:-----|
|`/?level=<value>`|Sets the overall brightness of the NeoPixel ring, where `<value>` is an integer in the range 0-100. Invalid input is ignored.<br><br>Can be used with any display mode.<br><br>Example: to change the current display mode to use brightness level of 50, use <br>`http://<GWID_IPAddress>/?level=50`|
|`/?rgb=<value>`|Sets the primary pixel color, where `<value>` is a 6-character string of hexadecimal digits representing the RGB color (omit the hashmark # prefix). Invalid input is ignored. <br><br>Can be used with display modes `flash`, `pulse`, `solid`, `strobe`, and `trail`. Ignored by other modes. <br><br>Example: to change the current display modes to use cyan, use <br>`http://<GWID_IPAddress>/?rgb=00ffff`|
|`/?speed=<value>`|Sets the speed of the <ins>visual</ins> and <ins>tone</ins> effects for the current display mode, where `<value>` is an integer in the range 0 - 4. Smaller numbers equate to “slower”, higher numbers to "faster". Invalid input is ignored. <br><br>Can be used with any display mode. <br><br>Example: to set the speed to a medium value, use `http://<GWID_IPAddress>/?speed=2`|
|`/?tone=<value>`|Determines whether the Piezo buzzer is active or not, where  `<value>` is either `on` or `off`. The pattern of beeps is determined by the current display mode. <br><br>Can be used with any display mode. <br><br>Example: to enable the buzzer with the current display mode, use <br>`http://<GWID_IPAddress>/?tone=on`|


The user can set multiple `<parameter>=<value>` pairs at the same time, using the ampersand (“&”) character acts as a separator.  Parameters can be set in combination with setting the display mode as well.  

Syntax is `http://<GWID_IPAddress>/?<mode>&<parameter1>=<value1>&<parameter2>=<value2>...`

For example, the following tells the device to flash all pixels on and off together (flash) in cyan (`rgb=00ffff`), at max brightness (`level=100`), at a modest pace (`speed=2`), while beeping the piezo buzzer (`tone=on`)

`http://<GWID_IPAddress>/?flash&rgb=00ffff&level=100&speed=2&tone=on`

<br>

## JSON FORMATTED RESPONSES

When the GWID receives a `getstate` or a valid query to set the display mode and/or display mode parameter(s) it returns a JSON formatted message confirming the device's settings.

|JSON Key | Value|
|:---|:---|
|Name|Hardware name that is factory-encoded into the NodeMCU board of the GWID (constant). For more information on this, search for "NodeMCU device name specifications" on the Internet.|
|Mode|Current display mode setting.|
|RGB|Current RGB color setting.<br><br>NOTE: When the device is in `pattern` display mode, this value is actually a calculated "average" of the RBG values across all 12 pixels.|
|Pattern|12-character string representing the current color of each pixel. <br><br>When the device is in the `pattern` display mode, each pixel can be r (red) g (green) b (blue) y (yellow) c (cyan) p (purple) w (white) or o (off). <br><br>For display modes other than `pattern`, `poll` returns the string "`************`". This is because in those other modes, the overall color setting is controlled collectively using the `rgb=<value>` parameter, which can be set to any valid RGB value (16M), not limited to the eight colors listed above.|
|Level|Current brightness level setting|
|Speed|Current speed setting|
|Tone|Current piezo buzzer setting|

A previous example showed how to command the device to flash all pixels on and off together, in cyan, at max brightness, at a modest pace, while beeping the piezo buzzer using 

`http://<GWID_IPAddress>/?flash&rgb=00ffff&level=100&speed=2&tone=on`

The resulting JSON response from the device would be

`{"Name":"ESP-######","Mode":"flash","RGB":"00ffff","Pattern":"************","Level":100,"Speed":2,"Tone":on}`

(where ESP-###### represents the factory-encoded name of the specific NodeMCU board of the GWID).

<br>

To obtain a formatted JSON response of the current device settings, use `http://<GWID_IPAddress>/getstate` 

---

&copy; 2025, 2026 Tim Sakulich. GWID documentation is licensed under Creative Commons Attribution-ShareAlike 4.0 International. <br>
See: [`LICENSE-DOCS`](/LICENSE-DOCS)
