GWID// Driver for General-purpose WiFi Indicator Device (GWID)
// Compatible with GWID Firmware gwid-v1-00

// SPDX-License-Identifier: MIT
// Copyright (c) 2025, 2026 Timothy Sakulich


metadata {
    definition (name: "hubitat-gwid-v1-00", namespace: "tjsakulich", author: "Timothy Sakulich") {
        capability "Actuator"
        capability "Alarm"
        capability "Bulb"
        capability "Configuration"
        capability "Initialize"
        capability "Notification"
        capability "PushableButton"
        capability "Refresh"
        capability "Switch"
        capability "Tone"  
        
        /*
        // Including the "SwitchLevel" capability in this driver will make 
        // the Set Level command available on the device Commands tab and
        // to other apps and rules.  However, this command is not strictly
        // needed since GWID brightness can be controlled via 
        // deviceNotification using the LEVEL= parameter. Still, to enable
        // the Set Level command, uncomment the following line and uncomment 
        // the code for the corresponding method as indicated below. 
        
        capability "SwitchLevel"
        */
        
        /*
        // Including the "ColorControl" capability in this driver will make 
        // the setColor, setHue, and setSaturation commands available on the 
        // device Commands tab and to other apps and rules.  However, these 
        // commands are not strictly needed since GWID color can be controlled
        // via deviceNotification using the rgb= parameter. Moreover, it can 
        // be confusing to use Hubitat's standard color controls for the 
        // GWID in display modes where the 12 pixels are not all set to the 
        // same color in the first place. Still, to enable Hubitat's 
        // color controls capability, uncomment the following line and 
        // uncomment the code for the corresponding methods as indicated below.

        capability "ColorControl"
        */
        
        attribute "DeviceName", "string"
        attribute "Mode", "string"
        attribute "RGB", "string"
        attribute "Pattern", "string"
        attribute "Level", "number"
        attribute "Speed", "number"
        attribute "Tone", "string" 
    }
 
    preferences {
        section("GWID Settings") {
            input "DeviceIP", "string", title: "GWID Device IP", required: true
            input "restrictControl", "bool", title: "Restrict device control to Hubitat IP", defaultValue: true, required: false
            input "EnableDebug", "bool", title: "Enable debug logging?", defaultValue: false, required: false
            input "GWID_On", "string", title: "Display Mode for 'On'", defaultValue: DEFAULT_ON_DISPLAY, required: false
	        input "GWID_Off", "string", title: "Display Mode for 'Off'" , defaultValue: DEFAULT_OFF_DISPLAY, required: false
            
            input name: "GWID_Strobe_RGB", type: "enum", title: "Color for 'Strobe'", options: [
            ["220000": "Red"],
            ["222200": "Yellow"],
            ["002200": "Green"],
            ["002222": "Cyan"],
            ["000022": "Blue"],
            ["220022": "Purple"],
            ["222222": "White"]            
        ], defaultValue: DEFAULT_STROBE_RGB, description: "Choose one of the available options."
            
	    }   
   } 
}

// Declare static constants for use in the driver's metadata and installed(), configure(), and updated() methods
import groovy.transform.Field
@Field static final String DEFAULT_ON_DISPLAY = "pulse&RGB=ff0000&level=50&speed=2&tone=off"
@Field static final String DEFAULT_OFF_DISPLAY = "pattern=gooogooogooo&level=1&speed=0&tone=off"
@Field static final String DEFAULT_STROBE_RGB = "220000"


//====================================================
//       Methods Automatically Called by Hubitat
//====================================================

def installed() {
    // Runs when Hubitat first starts up and connects to the device
    log.debug "Executing installed()"

    // the button capability creates a way to trigger rules within Rule Machine
    // e.g., a rule that resynchronizes the GWID(s)
    sendEvent(name: "numberOfButtons", value: 1)
    
    // the following state variable is used by the updated() method 
    // as a way to determine if the user changed the value of the
    // Preference "Restrict device control to Hubitat IP"
    state.restricted = false; 
    
    configure()
}


//====================================================
def configure() {
    // Runs after installed() and can be called directly from the Device Commands page
    if (EnableDebug) log.debug "Executing configure()"  

    // Set default values for preferences programmatically
    if (settings.GWID_On == null) {
       device.updateSetting("GWID_On", DEFAULT_ON_DISPLAY)
    }
    if (settings.GWID_Off == null) {
       device.updateSetting("GWID_Off", DEFAULT_OFF_DISPLAY)
    }
    if (settings.GWID_Strobe_RGB == null) {
        device.updateSetting("GWID_Strobe_RGB", DEFAULT_STROBE_RGB)
    }
    if (settings.GWID_Strobe_RGB == null) {
        device.updateSetting("GWID_Strobe_RGB", DEFAULT_STROBE_RGB)
    }
    if (settings.restrictControl == null) {
        device.updateSetting("restrictControl", true)
    }

    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }
	
    // Update GWID with the hub's IP and Port values
    // (Note: The /saveurl resource automatically sets the target IP to be the same as the 
    // IP of the requesting client unless otherwise specified. So for this driver application,
    // Hubitat only needs to provide the Port value in the /saveurl request.)
    
    state.restricted = false; // this state variable is updated by the saveURLCallback method 
    
    if (restrictControl) {
        ResourceRequest = "/saveurl?port=39501&restrict"
    }
    else {
        ResourceRequest = "/saveurl?port=39501"
    }
    
    def params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: 'text/plain', 
        requestContentType: 'text/plain' 
    ]
    
    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("saveURLCallback", params)
    

    // Retrieve device version information
    ResourceRequest = "/getversion" 
    params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: "application/json"
    ]
    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("getVersionCallback", params)  

    // Retrieve device version information
    refresh();   
    }



//====================================================
def updated() {
  	if (EnableDebug) log.debug "Executing updated()"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }
	
    // check if the user changed the value of "Restrict device control to Hubitat IP" 
    // in Preferences. If so, update the GWID accordingly
 
    if (restrictControl != state.restricted) {
        if (EnableDebug) log.debug "User changed restriction setting"
        if (restrictControl) {
            ResourceRequest = "/saveurl?port=39501&restrict" 
            def params = [
            uri: "http://${ip}${ResourceRequest}",
            contentType: 'text/plain', 
            requestContentType: 'text/plain' 
            ]

            if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
            asynchttpGet("saveURLCallback", params)
        }
        else {//clear any old settings on the GWID and resend Port=39501
            ResourceRequest = "/saveurl?clear" 
            def params = [
            uri: "http://${ip}${ResourceRequest}",
            contentType: 'text/plain', 
            requestContentType: 'text/plain' 
            ]

            if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
            asynchttpGet("saveURLCallback", params)

            ResourceRequest = "/saveurl?port=39501" 
            params = [
            uri: "http://${ip}${ResourceRequest}",
            contentType: 'text/plain', 
            requestContentType: 'text/plain' 
            ]

            if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
            asynchttpGet("saveURLCallback", params)
        }  
    }
}


//====================================================
//    Methods Required to Implement Capabilities
//====================================================

def beep() {
    ResourceRequest = "/beep"
    if (EnableDebug) log.debug "Executing 'beep()' (Sending ${ResourceRequest} to ${DeviceIP})"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }
    asynchttpGet(null,[uri: "http://${DeviceIP}${ResourceRequest}"])    
}


//====================================================
def both() {
    ResourceRequest = "/?strobe&rgb=${GWID_Strobe_RGB}&level=100&speed=2&tone=on"
    
    log.info "${device.label} Executing 'both()' (${ResourceRequest})"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }
    asynchttpGet("notificationCallback",[uri: "http://${DeviceIP}${ResourceRequest}"]) 
}


//====================================================
def initialize() {
   	// Can be called from Device Commands page
  	// Also triggered when GWID reports "sync"
    if (EnableDebug) log.debug "Executing initialize()"

 	// creating the following push() event can be used to trigger a rule 
   	// in Rule Machine to resynchronize the GWID(s)
   push(1)
}


//====================================================
def off() {
    
    def isSet = settings.GWID_Off
    if (isSet) ResourceRequest = "/?" + settings.GWID_Off
    else {
        log.warn "Device preference for 'off' has null value -- using driver default."
        ResourceRequest = "/?" + DEFAULT_OFF_DISPLAY
    }
    
    log.info "${device.label} Executing 'off()' (${ResourceRequest})"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }

    def params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: "application/json"
    ]

    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("notificationCallback", params)
    
    sendEvent(name: "switch", value: "off", isStateChange: false)
}


//====================================================
def on() {    
    
    def isSet = settings.GWID_On
    if (isSet) ResourceRequest = "/?" + settings.GWID_On
    else {
        log.warn "Device preference for 'on' has null value -- using driver default."
        ResourceRequest = "/?" + DEFAULT_ON_DISPLAY
    }
    
    log.info "${device.label} Executing 'on()' (${ResourceRequest})"

    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }

    def params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: "application/json"
    ]

    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("notificationCallback", params)
   
    sendEvent(name: "switch", value: "on", isStateChange: false) 
}


//====================================================
def refresh() {
    ResourceRequest = "/getstate"   
    if (EnableDebug) log.debug "Executing 'refresh()'"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }

    def params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: "application/json"
    ]

    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("notificationCallback", params)
}


//====================================================
def push(buttonNumber) {    
    log.info "${device.label} Executing 'push(${buttonNumber})'"
 
    //Generates an event for anything that is subscribed
    sendEvent(name: "pushed", value: "${buttonNumber}", isStateChange: true, type: "digital")
}



//====================================================
def siren() {
    ResourceRequest = "/?strobe&rgb=000000&level=0&speed=2&tone=on"
    
    log.info "${device.label} Executing 'siren()' (${ResourceRequest})"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }

    def params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: "application/json"
    ]

    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("notificationCallback", params)
}


//====================================================
def strobe() {
    
    def isSet = settings.GWID_Strobe_RGB
    if (isSet) ResourceRequest = "/?strobe&rgb=${GWID_Strobe_RGB}&level=100&speed=2&tone=off"
    else {
        log.warn "Device preference for 'strobe' has null value -- using driver default."
        ResourceRequest = "/?strobe&rgb=${DEFAULT_STROBE_RGB}&level=100&speed=2&tone=off"
    }
    
    log.info "${device.label} Executing 'strobe()' (${ResourceRequest})"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }

    def params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: "application/json"
    ]

    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("notificationCallback", params)
}


//====================================================
def deviceNotification(commandtext) {
    
    ResourceRequest = "/?" + commandtext
    
    log.info "${device.label} Executing Notification ${commandtext}"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }

    def params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: "application/json"
    ]

    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("notificationCallback", params)  
}



//====================================================
//            CALL BACKS AND PARSE METHODS
//        FOR HANDLING MESSAGES FROM THE GWID
//====================================================

// The notification Callback() call back method is designed to handle the response 
// to an asynchttpGet() message sent to the GWID when the expected 
// response is a JSON-formatted list of the device's current mode 
// and parameters.

private notificationCallback(response, data) {
    if (response.getStatus() == 200) {
        try {
            def jsonResponse = parseJson(response.data)
            if (EnableDebug) log.debug "Received JSON: ${jsonResponse}"

            // Update device attributes
            sendEvent(name: "DeviceName", value: jsonResponse.Name)
            sendEvent(name: "Mode",       value: jsonResponse.Mode)
            sendEvent(name: "RGB",        value: jsonResponse.RGB)
            sendEvent(name: "Pattern",    value: jsonResponse.Pattern)
            sendEvent(name: "Level",      value: jsonResponse.Level)    
            sendEvent(name: "Speed",      value: jsonResponse.Speed)
            sendEvent(name: "Tone",       value: jsonResponse.Tone)
            
            // the following code checks to see if the GWID is requesting to be sync'd
    		// and then calls initialize() 
   			if (jsonResponse.Mode == "sync") {
     		  if (EnableDebug) log.debug "${device.label} Requested sync"
       		initialize()
    		}
        } catch (e) {
            log.error "${device.label} Failed to parse JSON response: ${e}"
        }
    } 
    else {
        log.error "HTTP request failed -- status code: ${response.status} and errormessage: ${response.getErrorMessage()}"
    }
}


// The getVersionCallback() call back method is designed to handle the  
// JSON response to an asynchttpGet() message sent to the GWID resource getversion

private getVersionCallback(response, data) {
    if (response.getStatus() == 200) {
        try {
            def jsonResponse = parseJson(response.data)
            if (EnableDebug) log.debug "Received JSON: ${jsonResponse}"
            
            // Update static device info for display on the Device Info page
            device.updateDataValue("Device Hardware ID", jsonResponse.Hardware_ID)
            device.updateDataValue("Firmware Version", jsonResponse.Build_Version)
            device.updateDataValue("Version Build Date", jsonResponse.Build_Date)
            device.updateDataValue("Version Build Time", jsonResponse.Build_Time)

        } catch (e) {
            log.error "${device.label} Failed to parse JSON response: ${e}"
        }
    } 
    else {
        log.error "HTTP request failed -- status code: ${response.status} and errormessage: ${response.getErrorMessage()}"
    }
}

// The saveURLCallback() call back method is designed to handle the  
// JSON response to an asynchttpGet() message sent to the GWID resource saveurl


private saveURLCallback(response, data) {
    if (response.status == 200) {
        // Access the plain text data using response.data
        String rawText = response.data
        if (EnableDebug) log.debug "Received plain text response: ${rawText}"
        if (rawText.contains("(Restrict client flag is set)")) {
            state.restricted = true
        } 
        else {
        	state.restricted = false
        }       
    }
    else {
        log.error "HTTP request failed -- status code: ${response.status} and errormessage: ${response.getErrorMessage()}"
    }
}


// The parse() method handles any "unsolicited" messages from the GWID 
// (i.e., anything that is not a response to an asynchttpGet() request). 
// In particular, this method handles the situation when the GWID first powers 
// up and sends a JSON message containing "HUBSYNC" as the device display mode. 

private parse(String description) { 
    if (EnableDebug) log.debug "Executing 'parse()'"
    
    def msg = parseLanMessage(description)
	def body = msg.body
    body = parseJson(body)
    
    // the following code checks to see if the GWID is requesting to be sync'd
    // and then calls initialize() 
    if (body.Mode == "sync") {
       log.info "${device.label} Requested sync"
       initialize()
    }
    
}




/*

//===================================================================
//Additional Method Required to Implement SwitchLevel Capability
//===================================================================

def setLevel(level) {
    ResourceRequest = "/?LEVEL=${level}"   
    log.info "${device.label} Executing 'setLevel' ${level}"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }

    def params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: "application/json"
    ]

    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("notificationCallback", params)  
}
*/






/*
// As noted above, including the "ColorControl" capability for the GWID 
// will make the setColor, setHue, and setSaturation commands available  
// on the device Commands tab and to other apps and rules.  However, these 
// commands are not strictly needed since GWID color can be controlled
// via deviceNotification using the RGB= parameter. Moreover, it can 
// be confusing to use Hubitat's standard color controls for the 
// GWID in display modes where the 12 pixels are not all set to the 
// same color in the first place.  Still, if the Hubitat's 
// color controls capability is included, the following methods will be 
// required for that functionality. 


//===================================================================
//Additional Methods Required to Implement Color Control Capabilities
//===================================================================

def setColor(value) {
    if (value.hue == null || value.saturation == null) return      
    New_RGBvalue = HSLtoRGB(value.hue, value.saturation, value.level)    
    ResourceRequest = "/?rgb=" + New_RGBvalue

    log.info "${device.label} Executing 'setColor' ${value} ==> ${New_RGBvalue}"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }

    def params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: "application/json"
    ]

    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("notificationCallback", params)       
}


//====================================================
def setHue(value) {
 
    // Set hue while keeping the current saturation and level from the device's current color value
    Current_RGBvalue = device.currentValue("RGB")
    Current_hue = RGBtoHSL(Current_RGBvalue).hue
    Current_saturation = RGBtoHSL(Current_RGBvalue).saturation
    Current_level = RGBtoHSL(Current_RGBvalue).level
    
    New_RGBvalue = HSLtoRGB(value, Current_saturation, Current_level)
    ResourceRequest = "/?rgb=" + New_RGBvalue
    
    log.info "${device.label} Executing 'setHue' ${value} ==> ${New_RGBvalue}"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }

    def params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: "application/json"
    ]

    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("notificationCallback", params)   
}


//====================================================
def setSaturation(value) {
    
    Current_RGBvalue = device.currentValue("RGB")
    Current_hue = RGBtoHSL(Current_RGBvalue).hue
    Current_saturation = RGBtoHSL(Current_RGBvalue).saturation
    Current_level = RGBtoHSL(Current_RGBvalue).level
    
    // Set saturation while keeping the current hue and level from the device's current color value
    New_RGBvalue = HSLtoRGB(Current_hue, value, Current_level)
    
    ResourceRequest = "/?rgb=" + New_RGBvalue
    
    log.info "${device.label} Executing 'setSaturation' ${value} ==> ${New_RGBvalue}"
    
    def ip = settings.DeviceIP
    if (!ip) {
        log.error "Device IP Address not configured."
        return
    }

    def params = [
        uri: "http://${ip}${ResourceRequest}",
        contentType: "application/json"
    ]

    if (EnableDebug) log.debug "Sending asynchronous GET request ${params.uri}"
    asynchttpGet("notificationCallback", params)    
}


//===================================================================
//                   Additional Utility Functions 
//           Required to Implement Color Control Capabilities
//===================================================================

private hue2rgb(p, q, t){
            if(t < 0) t += 1;
            if(t > 1) t -= 1;
            if(t < 1/6) return p + (q - p) * 6 * t;
            if(t < 1/2) return q;
            if(t < 2/3) return p + (q - p) * (2/3 - t) * 6;
            return p;
        }


private HSLtoRGB(h,s,l) {  
    //logDebug ("Executing HSLtoRGB()")
    String HEXCode = "FFFFFF"
    
    int RedValue = 0 
    int GreenValue = 0 
    int BlueValue = 0 
    
    if(s == 0) {RedValue = GreenValue = BlueValue = l * 2.55 } // achromatic
    else{
        // Adjustvinput values to convert from Hubitat HSL Color Mapping Ranges 
        h /= 100.0    // Hubitat Hue is [0,100] and needs to be [0,1]
        s /= 100.0    // Hubitat Saturation is [0,100] and needs to be [0,1]
        l /= 200.0    // Hubitat Level is [0,100] and needs to be [0,0.5]
        
        q = l < 0.5 ? l * (1 + s) : l + s - l * s
        p = 2.0 * l - q
        
        RedValue = Math.round(hue2rgb(p, q, h + 1/3.0) * 255)     
        GreenValue = Math.round(hue2rgb(p, q, h) * 255)
        BlueValue = Math.round(hue2rgb(p, q, h - 1/3.0) * 255)
    } 
    
    String RedHEXString = Integer.toHexString(RedValue)
    if (RedValue < 16) { RedHEXString = "0" + RedHEXString }
    String GreenHEXString = Integer.toHexString(GreenValue)
    if (GreenValue < 16) { GreenHEXString = "0" + GreenHEXString }
    String BlueHEXString = Integer.toHexString(BlueValue)
    if (BlueValue < 16) { BlueHEXString = "0" + BlueHEXString }

    HEXCode = RedHEXString+GreenHEXString+BlueHEXString
    //if (EnableDebug) log.debug "HSLtoRGB ============ ${HEXCode}"       
       
    return HEXCode  
}

    
private RGBtoHSL(HEXCode) {
    //logDebug ("Executing RGBtoHSL()")
    int red = Integer.valueOf(HEXCode.substring(0, 2), 16)
    int green = Integer.valueOf(HEXCode.substring(2, 4), 16)
    int blue = Integer.valueOf(HEXCode.substring(4, 6), 16)
    
    float r = red / 255f
    float g = green / 255f
    float b = blue / 255f
    
    //if (EnableDebug) log.debug "RGBtoHSL ============ red   = ${red} ==== ${r}"
    //if (EnableDebug) log.debug "RGBtoHSL ============ green = ${green} ==== ${g}"
    //if (EnableDebug) log.debug "RGBtoHSL ============ blue  = ${blue} ==== ${b}"
       
    float max = [r, g, b].max()    
    //if (EnableDebug) log.debug "RGBtoHSL ============ max = ${max}"
    
    float min = [r, g, b].min()                
    //if (EnableDebug) log.debug "RGBtoHSL ============ min = ${min}"
    
    float h = (max + min) / 2.0;
    float s = h
    float l = h

    if (max == min) {
      h = 0
      s = 0   
      l = max / 2
    }
    else {
      d = max - min;
      s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
      if (max == r) h = (g - b) / d + (g < b ? 6 : 0);
      if (max == g) h = (b - r) / d + 2;
      if (max == b) h = (r - g) / d + 4;
      h /= 6;

    }
    
    hue = (h * 100).round()        // Convert hue to range [0...100]
    saturation = (s * 100).round() // Convert saturation to range [0...100]
    level = (l * 200).round()      // Convert level to range [0...100]
    
    return [
        "red": red.asType(int),
        "green": green.asType(int),
        "blue": blue.asType(int),
        "hue": hue.asType(int),
        "saturation": saturation.asType(int),
        "level": level.asType(int),
    ]
}

*/
