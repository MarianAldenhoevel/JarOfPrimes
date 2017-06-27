The Jar of Primes is a knock-off version of the Primer, a kinetic sculpture by Karl Lautman. For the original see: 

http://www.karllautman.com/primer.html
https://www.youtube.com/watch?v=8UqCyepX3AI

Mine is built around a NodeMCU board which in turn contains a ESP8266 12E chip. To that it adds a pushbutton to command the next number and a counter to display it. The code is written in the Arduino IDE. 

Via WiFi the jar serves a webinterface that allows configuration of credentials to connect to an access point and synchronize the internal number to the physical counter. Another page mirrors the counter and button in a browser and all are kept in sync through http or websockets.

<a href="https://github.com/MarianAldenhoevel/JarOfPrimes/blob/master/Screenshot.jpg" target="blank"><img src="https://github.com/MarianAldenhoevel/JarOfPrimes/blob/master/Screenshot.jpg" width="500" alt="screenshot"></img></a>

The whole thing is then put into a jar that used to contain prime olives and now contains prime primes.

<a href="https://github.com/MarianAldenhoevel/JarOfPrimes/blob/master/Preview.jpg" target="blank"><img src="https://github.com/MarianAldenhoevel/JarOfPrimes/blob/master/Preview.jpg" width="250" alt="preview"></img></a>

The project is work-in-progress, stand by for more pictures and documentation once it is completed.
