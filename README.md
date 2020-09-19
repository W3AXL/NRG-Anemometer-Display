# NRG Anemometer Display
A compact, cheap interface unit for NRG #40C 3-cup anemometers.

<p float="left">
  <img src="https://raw.githubusercontent.com/W3AXL/NRG-Anemometer-Display/master/media/pic_small_1.png" height="200" />
  <img src="https://raw.githubusercontent.com/W3AXL/NRG-Anemometer-Display/master/media/rotate.gif" height="200" /> 
  <img src="https://raw.githubusercontent.com/W3AXL/NRG-Anemometer-Display/master/media/pic_small_2.png" height="200" /> 
</p>

This project started because I found a great deal on some NRG anemometers on eBay. Two for $40 was hard to turn down, especially because each unit is calibrated from the factory and said calibration is freely available on [NRG's website](https://www.nrgsystems.com/support/product-support/calibration-reports/).

The heavy lifting of the circuit was already done for me - the technical whitepaper from NRG clearly illustrates an almost-complete schematic. The only real work to be done was selection of components, the MCU interfacing, and board design for a nice compact meter.

![NRG Block Diagram](https://raw.githubusercontent.com/W3AXL/NRG-Anemometer-Display/master/nrg-block-diagram.PNG)

I went through a lot of initial brainstorming for the interface concept - battery powered, or DC? Integrated data logging, or keep it simple? How many buttons? Should it beep at you annoyingly?

In the end, a dead-simple interface showing current and maximum wind readings was decided on. Power is provivded via a micro-USB plug, which also creates a serial port for real-time data transfer to a PC. The display is your standard run-of-the-mill eBay 1.3" OLED display, driven by I2C, and available for a few dollars all over the place. Boards were designed in Kicad and manufactured by JLCPCB. Components were sourced from Digikey, and the whole thing took a couple days to get working.

![](https://raw.githubusercontent.com/W3AXL/NRG-Anemometer-Display/master/media/video.gif)
