# CONFIGURE HUBITAT TO USE GWID

>[!IMPORTANT]
>These instructions require that GWID commands are not already restricted to a saved IP address
>that is different from the Hubitat. To verify, send `http://<GWID_IPAddress>/report`
>from a browser window. If the report shows the GWID is "Configured with Saved IP Address",
>"Control Restricted to the Saved Client IP Address", and the "Saved IP" value is **not**
>the same as the hub, then this restriction must be cleared before proceeding. "Factory 
>Reset" the device and reconfigure the WiFi settings. Refer to Part 1 of the GWID Users Guide
>([`gwid-user-guide-pt1-connect-to-wifi.md`](/gwid-user-guide-pt1-connect-to-wifi.md)) in the `docs/`
>folder.


## Information Needed

- Driver for Hubitat (download `hubitat-gwid-v1-00` from `hubitat/driver/`)
- GWID's MAC Address 
- GWID Static IP address


## Configure Hubitat

- Overview: The user creates a virtual device using the GWID device
  driver and provides the static IP address and device MAC. Once
  configured, the hub can send commands to the GWID through its device
  page and through dashboards, rules, etc.

- Go to the **Drivers Code** page, click Add Driver, copy and paste in
  the text contents of the `hubitat-gwid-v1-00` file, then save. The
  driver should now be listed on the Drivers Code page.

- Go to the **Devices** page, click Add Device, select Virtual, and
  select the `hubitat-gwid-v1-00` driver

- Now go to the **Device** page for the newly-added GWID

    - Select the **Device Info** tab

      - Check/update the **Device Name** and **Device Label** as desired

      - Verify the **Type** is set to the `hubitat-gwid-v1-00` driver

      - Change the **Device Network Id** to the **MAC address** of the
        GWID (without the colons).  This is critical for the Hubitat
        to recognize incoming messages from the GWID.

      - **Save the changes**. Failure to do this will result in an incomplete set up.

    - Select the **Device Preferences** tab

      - Enter the **static IP Address** for the GWID
     
      - Verify the default settings for these Preferences:
  
        - **Restrict device control to Hubitat IP** is set (on)
        - **Display Mode for 'On'** set to `pulse&rgb=ff0000&level=50&speed=2&tone=off`
        - **Display Mode for 'Off'** set to `pattern=gooogooogooo&level=1&speed=0&tone=off`
        - **Color for 'Strobe'** set to `Red`

      - **Save the changes.**  Failure to do this will result in an incomplete set up.

    - Select the **Commands** tab

      - Click on Configure. 
      - Click on Initialize. The device state variables will update
      - To test the device from the Commands tab click On, Off, Strobe, Siren, or Both.


>[!IMPORTANT]
>During installation (and when the configure command is selected from the device Commands
>tab) Hubitat sends a `/saveurl?port=39501` resource request to the GWID. This saves the
>Hubitat IP and Port information to the GWID so that the device knows where to send its initial
>notification upon each boot up.
>
>When the **Restrict device control to Hubitat IP** is selected through the device Preferences tab,
>Hubitat includes the `restrict` argument in the resource request (i.e., `/saveurl?port=39501&restrict`).
>This tells the GWID to reject device controls that do not come from the Hubitat IP address. These settings
>persist across GWID reboots. To unrestrict the device, go to the Preferences tab and unset the
>switch. (A "factory reset" of the GWID also clears the IP/Port/restrict settings but clears WiFi settings
>as well.)

# TROUBLESHOOTING

- Verify that the GWID and Hubitat are connected to the same network

- Verify that the router is configured to use static IP addresses for
  both the Hubitat and the GWID

- From a computer, laptop or cell phone connected to the same network,
  use `http://<GWID_IPAddress>/report` to verify GWID network
  connectivity and IP address, and that it has the correct IP Address
  and port `39501` for the Hubitat hub

If all of the above checks out, then the problem is likely with the
Hubitat configuration

- Verify that the Hubitat hub is using the correct driver for the GWID,
  that the device ID is the same as the GWID MAC address (without
  colons), and that the IP address on the preferences page is set to the
  GWID's static IP

- If the GWID is still not responding to Hubitat commands, remove power
  from the GWID for a few seconds then restore power. After reboot, the
  device will attempt to communicate with the Hubitat.

- Go to the GWID's Device Preferences tab and turn on "Enable Debug Logging".
  Try a few more commands from the Device Commands tab and review the Hubitat
  log for clues.

- If the GWID is still not responding to Hubitat, the user will need to
  investigate whether there is another configuration issue between the
  Hubitat and the router (e.g., a firewell setting?) or some other
  driver or setting on the Hubitat that is interfering with the ability
  of the Hubitat to communicate with the GWID

---

&copy; 2025, 2026 Tim Sakulich. GWID documentation is licensed under Creative Commons Attribution-ShareAlike 4.0 International. <br>
See: [`LICENSE-DOCS`](/LICENSE-DOCS)
