# GWID USER GUIDE PART 2:<br>PIXEL INDICATIONS DURING BOOT UP 

|Pixel Display|Description|
|:----------------------------------:|:----------------------------------------|
|![Image of Neopixel with red pixel at top](images/gwid-1red.png)|Red pixel @ position 0 <br><br>GWID is in WiFi Configuration Mode. It has opened the GWID_AP access portal and is waiting on user to supply network credentials through the portal.|
|![Image of Neopixel with green pixel at top](images/gwid-1green.png)|Green pixel (flashing) @ position 0 <br><br>GWID is in WiFi Configuration Mode. It has successfully connected to the network, and is about to reboot. |
|![Image of Neopixel with yellow pixel at top](images/gwid-1yellow.png)|Yellow pixel @ position 0 <br><br>Part of normal boot up sequence. The GWID is initializing and attempting to connect to the network. <br><br>Connection may take a few seconds, but if the device appears to “hang” at this stage, unplug the GWID and then power it back up. If it still fails to connect, press the GWID reset button and re-enter WiFi credentials through the access portal). NOTE: The GWID's NodeMCU radio can have issues if it's physically too close to the router.|
|![Image of Neopixel with green pixel at top](images/gwid-1green.png)|Green pixel @ position 0 <br><br>Part of normal boot up sequence. GWID has connected WiFi.|
|![Image of Neopixel with blue pixel at top](images/gwid-1blue.png)|Blue pixel @ position 0 <br><br>Part of normal boot up sequence. The GWID is ready to accept commands. If the values of SavedIP and SavedPort are **not** set, the GWID will pause in this state until it receives an external command.|
|![Image of Neopixel with two blue pixels](images/gwid-2blue.png)|Blue pixel @ positions 0 and 6 <br><br>Part of normal boot up sequence, and indicating that the GWID is configured with savedIP and savedPort information (restricted flag is NOT set). The GWID will then send a `sync` request to the URL defined by savedIP and savedPort.|
|![Image of Neopixel with one blue and one red pixel](images/gwid-1blue1red.png)|Blue pixel @ position 0 and Red pixel @ position 6 <br><br>Part of normal boot up sequence, and indicating that the GWID is configured with savedIP and savedPort information (restricted flag is set). The GWID will then send a `sync` request to the URL defined by savedIP and savedPort.|
|![Image of Neopixel with four blue pixels](images/gwid-4blue.png)|Blue pixels @ positions 0, 3, 6, and 9 <br><br>Part of normal boot up sequence. The GWID has sent a `sync` message to the URL defined by the savedIP address and savedPort, and is awaiting a reply.|
|![Image of Neopixel with two yellow pixels at top](images/gwid-2yellow.png)|Yellow pixels @ positions 0 and 6 <br><br>GWID has disconnected from the network. It will attempt to reconnect with previously saved credentials.  This can take less than a second or up to a few seconds.|
|Any other pixel colors/patterns|The GWID pixels and the piezo buzzer change in response to properly formatted messages from the Hubitat and from browser HTTP calls|

---

&copy; 2025, 2026 Tim Sakulich. GWID documentation is licensed under Creative Commons Attribution-ShareAlike 4.0 International. <br>
See: [`LICENSE-DOCS`](/LICENSE-DOCS)
