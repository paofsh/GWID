# The GWID Project

## <ins>G</ins>eneral-purpose <ins>W</ins>iFi-connected <ins>I</ins>ndicator <ins>D</ins>evice

I began this project to solve a challenge -- create a simple visual  indicator that I could place anywhere in my house to let me know whether my garage door was open or closed. With my large family, it was not an infrequent occurrence that the main garage door would be left open all day or that I would get up in the morning to find that someone had left the garage door open all night. We live in a very safe neighborhood, but I was concerned that complacency would eventually become regret.

The obvious solution would have been to go the garage and physically look at the door before going to bed...but what's the fun in that for an electronics nerd?

Already a home automation enthusiast (using a Hubitat Elevation C-8), I decided to install a Z-wave magnetic reed switch on the garage door track and add that device to my home automation. I liked having the capability to check the door status from anywhere using the Hubitat app on my cell phone. Even so, I was still interested in having some sort of visual indicator in my kitchen (and elsewhere in the house) that immediately tell me the status of the door without having to open the app (or go look at the door). 

I thought about using a rule in my home automation to control a smart switch or smart bulb to show the door status. While simple, that approach seemed a bit clunky and did not appeal to my desire for a challenge! Instead, I envisioned building a small wireless device that would display a green LED when the door was shut and blink a red LED when the door was open--a device that could plug into any power outlet in my house. Visible but not obtrusive. After some quick research into commercial products it was clear that nothing was optimal for my application...or dirt cheap.  

That led to an electronics and software design journey which, **truth be told**, sort of got out hand as I repeatedly tinkered to add "just one more" feature. After several generations of development--not to mention more than a few eye rolls from my spouse--I have decided to declare the project complete. Well, mostly complete. At least for now.  

The final (which really means "current") device design includes an ESP8266/NodeMCU development board, NeoPixel ring, piezo buzzer, Arduino firmware, and a device driver written in Groovy for my home automation hub. **The device can be controlled over WiFi using HTTP protocols to flash, pulse, strobe, signal S.O.S., display a solid color or pixel patterns, and sound an audio alert.**

I now have three of these devices positioned strategically in our house, communicating with Hubitat hub via WiFi. There is one in our kitchen, one in the hallway outside the main bedroom, and one in our home office. They are in fact quite helpful in solving the original problem of forgetting to close the garage door, and I also use them as visual alerts for other home automation sensors (e.g., water leak sensors).

As a side note, I eventually replaced the single Z-wave magnetic contact switch on my garage door with a Zooz ZEN17 relay controller, following excellent instructions available on the Zooz website, "How to Use the ZEN17 Universal Relay as a Garage Door Opener". My ZEN17 is now connected to the garage door opener and to TWO magnetic contact switches. One contact switch is positioned so that it closes when the door is fully closed and the other is positioned so that it closes when the door is fully open. That configuration not only added actual door control to my home automation set up, but also a way to have my indicator device show whether the door is fully closed, fully open, or neither fully open nor fully closed (i.e., in motion). 

I tried to be disciplined about documenting my hardware and software design along the way, mostly to help with debugging. But once I decided to share my project with others I quickly realized that my working notes were woefully inadequate. This Github repository is the result of being more systematic in documentating the hardware build, software design, operating functionality of the device, and integration with the home automation hub. 

Also, rather than keep referring to the hardware/software solution as "the device", I cheekily decided to give it a name: the General-purpose WiFi-connected Indicator Device (GWID -- pronounced gee-wid). That formality of naming the device should not mask the point that, in the end, this was simply a hobby project. Period.

I hope that others may find my design and documentation useful for their own needs, perhaps motivate some community-developed improvements, and maybe even inspire some completely novel ideas.

# Some final thoughts...

As a hobbyist, the project's design-build-test-learn loop involved lots and lots of learning (failures). I relied quite a lot on Internet searches for basic information, and found that some of the AI tools were very useful at helping with the development and debugging of portions of the firmware and driver. Along the way I learned a few things about microcontrollers and the NodeMCU development board in particular; the NeoPixel Ring; the Arduino IDE; HTTP and JSON protocols; Hubitat Elevation device drivers and rules; writing documentation; open source licensing; and, finally, how to use GitHub!

I do not claim to have exercised best practices of coding or design, and I'm certain that others with more experience will quickly identify where I have made suboptimal choices in the hardware, firmware, and Hubitat driver. For example, my solution uses the HTTP protocol, rather than the more secure HTTPS protocol. For my application, that seemed like manageable risk since 1) the GWID is simply an indicator device that does not control other devices; 2) messages between the GWID and hub are confined to a secure local WiFi network; and 3) it's a convenience device, not used or intended to be used in any safety- or security-critical context. But encryption of the message traffic could be an area where additional development might be useful.

In the end, the GWID has proven to be an excellent solution to the problem I set out to tackle, and has been very reliable with my Hubitat set up. Mostly, the project was a fun learning experience!

---

&copy; 2025, 2026 Tim Sakulich. GWID documentation is licensed under Creative Commons Attribution-ShareAlike 4.0 International. <br>
See: [`LICENSE-DOCS`](/LICENSE-DOCS)
