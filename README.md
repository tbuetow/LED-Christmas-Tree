# 🎄RGB LED Christmas Trees!

Each year, I make a small holiday bauble for teachers, family, and friends. Usually my wife and I come up with an idea around Thanksgiving and then panic for three weeks to deliver something by the last day of school for the holidays.

This year we chose to revisit something we made 7 years ago, but improve on it, building on some of my more recent projects with addressable RGB LEDs and better tools like my multiple extruder 3D printer and solder reflow oven.

The concept is a clear thin 3D printed tree on top of a 3D printed base that powers LEDs inside of it. 7 years ago, this was a pair of AA batteries, a single discrete color changing LED, and a power switch. You could also run it off of a 5V supply via micro-USB. Times have changed now! I had a few simple improvements I wanted to make.

- USB-C with proper support for all USB-C power supplies. I chose to drop batteries just due to cost.
- A ring of addressable RGB LEDs instead of a single unmodifiable color changing LED.
- The ability to change the LED color effects, and provide many included options.
- Multiple color printing to make it more beautiful.
- Under $10 and very easy to manufacture, since we want to make 30-40 of these.
- Document the process to share with others and allow those who are inclined to write their one color effects.

## 3D Printed Parts
### Base
I designed the base and all other fixtures and accessories, except for the trees in Autodesk Fusion.

![Fusion screenshot](images/Fusion-base-slice.png)

You can download it from [Autodesk Fusion](https://www.autodesk.com/products/fusion-360/overview) if you want to make modifications yourself. Fusion is free for personal use.
<iframe src="https://gmail2605510.autodesk360.com/shares/public/SH90d2dQT28d5b602811dd4c983dbeae4169?mode=embed" width="800" height="600" allowfullscreen="true" webkitallowfullscreen="true" mozallowfullscreen="true"  frameborder="0"></iframe>

The most likely thing you will want to modify is the embedded text. Just edit the text sketch on the Base component.
![Fusion Browser Tree View, highlighting a sketch called text](/images/fusion-base-browser-text.png)

Printing was done on my Prusa XL and Core One.
I started by testing color combinations:

![Tree base in green with text colors for text](images/IMG_0062.jpg)


Once done, I was able to begin producing them multiples at a time:

![Tree base printing in progress](images/IMG_0077.jpg)


The light dividers and trees were printed on the Core One. Lots of time starting the next print!

![Plastic dividers printing](images/IMG_0075.jpg)

### Trees
I used tree designs that would print well in vase mode since I wanted them to be transparent. See [Trees](hw/Trees/) for more information

![Clear tree printing in vase mode](images/IMG_0078.jpg)

### PCB Fixture
I designed and printed a fixture to help with soldering as well, available in [Fixtures](/hw/Fixtures/). The fixture was printed in carbon-fiber reinforced PETG, to help the print maintain flatness. Any similar material, maybe even plain PLA, may be fine. The holes on the left are for M4 heat set inserts, used to hold the clamp in place.

<Insert Fixture Picture here>

## Circuit Boards
The circuit boards were designed in KiCAD. Thanks to the power of modern microprocessors and addressable LEDs, the circuit is quite simple.
![KiCad Electrical Schematic](images/KiCad-control-schematic.png)
![KiCad Electrical Layout](images/KiCad-control-layout.png)

The control board is **very** small, just 37mm (under 1.5") across. This is in part because I can get the new(er) Microchip ATTiny1616 in a QFN packaged. These are ordered as bare PCBs and I use solder paste and stencils to set components and then reflow them in my solder oven. I would not recommend hand soldering these unless you are very skilled. It would be better to switch to slightly larger components and change the microprocessor out for one in a SOIC package or similar.

<Insert solder oven photo here>

## Software
I used PlatformIO in VSCode as my IDE to program the microcontrollers. This works well and makes it easy to import the Arduino framework and FastLED library for quick access to easy-to-use functions and tools.

The bulk of the code came from another project I made where I had LED name plates that were edge-lit with RGB LEDs. I just added a few additional effects. [main.cpp](src/main.cpp) is available in the [src](src/) folder.


# Building your own effects
## Prerequisites
- **Programmer:** You will need a UPDI programmer. You can make one from a USB to serial adapter by just connecting USB Serial RX to the UPDI pin, and then the USB Serial TX to RX with a 1kΩ Resistor. Or you can buy something cheap like the [Adafruit Adafruit UPDI Friend](https://www.adafruit.com/product/5879) or the High Voltage version too.

- **Jumper wires:** You will need to adapt the Programmer to 3 0.1" female connectors. The Adafruit one will give you 3 male headers pins, so you will need 3 female to female jumpers. Grab [these](https://www.adafruit.com/product/1950) from Adafruit if you are already buying the UPDI Friend, or they are also in [this pack](https://www.amazon.com/Elegoo-EL-CP-004-Multicolored-Breadboard-arduino/dp/B01EV70C78) from Amazon.

- **VSCode:** You will need an Integrated Development Environment. I use [VSCode](https://code.visualstudio.com/). Within VSCode, you will need the PlatformIO. Go to the Extensions tab and search for it within VSCode or grab it from the [extensions marketplace](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide).

- **Git:** You probably want to get the [git version control system](https://git-scm.com/) to make it easy to get access to the code. This step is techincally optional, but will allow you to undo changes you make at any time.


## VSCode / PlatformIO
<!--
- Clone instructions in VSCode
- PlatformIO activation
- Navigating to main.cpp
- Programming with UPDI
-->

## Writing effects
<!--
- Explain timing
- Explain where to add new effects
-->