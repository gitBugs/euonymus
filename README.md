# euonymus
A colourful visual notifier - control a NeoPixel (WS2812) strip over WiFi / MQTT, using an ESP8266. Fades rainbow colours along the strip while idle, and listens for an MQTT message containing a RGB value and, optionally, a time in seconds (e.g. "0,128,255,30"). When it receives this message, the whole strip wipes to that colour, flashes for the appropriate time, then returns to its idle pattern. Does some crude error checking for the incoming message, but needs some cleaning up.
