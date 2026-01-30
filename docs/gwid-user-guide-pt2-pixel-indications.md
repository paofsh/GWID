# GWID USER GUIDE PART 2:<br>PIXEL INDICATIONS DURING BOOT UP 

|Pixel Display|Description|
|:----------------------------------:|:----------------------------------------|
|![Image of Neopixel with red pixel at top](images/gwid-1red.png)|Red pixel @ position 0 <br>GWID is in WiFi Configuration Mode.<br><br>The GWID has opened the GWID_AP access portal @ IP Address 192.168.1.4 and is waiting on user to supply network credentials through the portal.|
|![Image of Neopixel with red pixels at top and bottom](images/gwid-2red.png)|Red pixels @ position 0 and 6 (flashing) <br>GWID is in WiFi Configuration Mode. <br><br>The GWID failed to connected to the network using the credentials supplied through the GWID_AP access portal, and will reboot into the Access Portal again. |
|![Image of Neopixel with green pixels at top and bottom](images/gwid-2green.png)|Green pixels @ positions 0 and 6 (flashing) <br>GWID is in WiFi Configuration Mode. <br><br>The GWID successfully connected to the network for the first time, and will reboot for normal operations.|
| | |
|![Image of Neopixel with yellow pixel at top](images/gwid-1yellow.png)|Yellow pixel @ position 0 <br>Part of normal boot up sequence. <br><br>The GWID is initializing and attempting to connect to the network.|
|![Image of Neopixel with yellow pixels at top and bottom](images/gwid-2yellow.png)|Yellow pixels @ positions 0 and 6 (flashing) <br>Part of normal boot up sequence. <br><br>The GWID failed to connect using the saved network credentials, and will reboot. If this happens repeatedly, the user should manually reset the device, and supply correct network credentials through the configuration mode.  |
|![Image of Neopixel with green pixel at top](images/gwid-1green.png)|Green pixel @ position 0 <br>Part of normal boot up sequence. <br><br>The GWID succefully connected to WiFi.|
|![Image of Neopixel with blue pixel at top](images/gwid-1blue.png)|Blue pixel @ position 0 <br>Part of normal boot up sequence. <br><br>The GWID is ready to accept commands. If the values of SavedIP and SavedPort are **not** set, the GWID will pause in this state until it receives an external command.|
| | |
|![Image of Neopixel with two blue pixels](images/gwid-2blue.png)<br><br>![Image of Neopixel with one blue and one red pixel](images/gwid-1blue1red.png)|Blue pixels @ positions 0 and 6 <br>Part of normal boot up sequence.<br>The GWID is configured with savedIP and savedPort information but the **restricted flag is NOT set**.<br><br>/OR/<br><br>Blue pixel @ position 0 and Red pixel @ position 6 <br>Part of normal boot up sequence. <br>The GWID is configured with savedIP and savedPort information and the **restricted flag is set**.|
| | |
|![Image of Neopixel with four blue pixels](images/gwid-4blue.png)<br><br>![Image of Neopixel with three blue and one red pixels](images/gwid-3blue1red.png)|Blue pixels @ position 0, 3, 6, and 9<br>Part of normal boot up sequence. <br>The GWID has sent a `sync` request to the URL defined by savedIP and savedPort and is awaiting a reply, but the **restricted flag is NOT set**. The GWID will pause in this state until it receives an external command. <br><br>/OR/<br><br>Blue pixels @ positions 0, 3, and 9 and Red pixel @ postiion 6<br>Part of normal boot up sequence. <br>The GWID has sent a `sync` request to the URL defined by savedIP and savedPort and is awaiting a reply, and the **restricted flag is set**. The GWID will pause in this state until it receives an external command **from the savedIP**.|
|Any other pixel colors/patterns|The GWID pixels and the piezo buzzer change in response to properly formatted messages.|

---

&copy; 2025, 2026 Tim Sakulich. GWID documentation is licensed under Creative Commons Attribution-ShareAlike 4.0 International. <br>
See: [`LICENSE-DOCS`](/LICENSE-DOCS)
