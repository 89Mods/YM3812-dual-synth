# YM3812-dual-synth
MIDI synthesizer built from two YM3812s for 18-voice mono or 9-voice stereo. More info: https://tholin.dev/ym3812d/

# Build instructions

## PCB

The gerber file for the synthesizer can be found [here](https://github.com/89Mods/YM3812-dual-synth/tree/main/KiCad/production) and you should be able to use this with all the major board houses. Just make sure they correctly recognize it as a 4-layer PCB. The dimensions of the PCB are low enough to where it should be cheap despite it being 4-layers (8€ on JLCPCB).

## Parts

The YM3812s and their DACs are the most annoying parts to obtain. They can be easily found on eBay and AliExpress by simply using "YM3812" and "Y3014" keywords, but the condition of the parts can vary. The AliExpress listings I bought from are [here](https://de.aliexpress.com/item/1005001723547967.html) and [here.](https://de.aliexpress.com/item/1005002637379806.html)

The nanoCH32V003 mini dev board you can maybe even find at your favorite maker shop, but also [on AliExpress](https://de.aliexpress.com/item/1005005221751705.html). If there is an option to buy the board plus a "WCHLinkE" you may use that (recommended), but I will also show you how to use an Arduino Uno to flash the synthesizer firmware later on (though this requires a Linux install or WSL).

All the other integrated circuits can be found at various electronics retailers:

LM358: [TME](https://www.tme.eu/de/details/lm358ap/tht-operationsverstarker/texas-instruments/) - [Mouser](https://www.mouser.de/ProductDetail/Texas-Instruments/LM358N-NOPB?qs=QbsRYf82W3GXbWt%2FvwDTcQ%3D%3D) - [LCSC](https://www.lcsc.com/product-detail/Operational-Amplifier_HGSEMI-LM358N_C434570.html)

6N139 (or 6N138): [TME](https://www.tme.eu/de/details/6n139-eve/optokoppler-analogausgang/everlight/6n139/) - [Mouser](https://www.mouser.de/ProductDetail/Vishay-Semiconductors/6N139?qs=WHRYDAs2thAk3qh0MF5UBQ%3D%3D) - [LCSC](https://www.lcsc.com/product-detail/Optocouplers-Logic-Output_CT-Micro-International-6N139_C191869.html)

SN74LV1T34DBVRG4: [TME](https://www.tme.eu/de/details/sn74lv1t34dbvrg4/logische-wandler/texas-instruments/) - [Mouser](https://www.mouser.de/ProductDetail/Texas-Instruments/SN74LV1T34DBVRG4?qs=vdi0iO8H4N10R%252Bt%252BjmxZrg%3D%3D) - [LCSC](https://www.lcsc.com/product-detail/Buffer-Driver-Transceiver_Texas-Instruments-SN74LV1T34DBVR_C100024.html)

MX25L1005 (or MX25L2006 or 25Q32 or any spiflash that supports the 25xx command set and is made by Winbond or Macronix): [TME](https://www.tme.eu/de/details/mx25l2006em1i-12g/serielle-flash-speicher/macronix-international/mx25l2006em1i-12g-tube/) - [LCSC](https://www.lcsc.com/product-detail/NOR-FLASH_MXIC-Macronix-MX25L2006EM1I-12G_C135940.html) - I also recommend looking for "MX25L1005" on AliExpress, you can get them really cheap there.

74HCT164 (or 74LS164, NOT 74HC164): [TME](https://www.tme.eu/de/details/cd74hct164e/schieberegister/texas-instruments/) - [Mouser](https://www.mouser.de/ProductDetail/Texas-Instruments/SN74LS164NE4?qs=SL3LIuy2dWzMustM9SNFlQ%3D%3D) - [LCSC](https://www.lcsc.com/product-detail/Shifting-Register_Texas-Instruments-SN74LS164N_C129807.html)

## Build notes

The BOM for assembling the board is available [here](https://example.com/). Otherwise, there is little special to keep in mind, just solder the SMD components first, and use pin sockets for the nanoCH. It needs to be removed from the synthesizer to be re-flashed.

The 5-pin header at the bottom of the board is where MIDI input is. You will need to wire in a broken-out MIDI connector here. Only two need to actually be wired up. The 2nd pin from the left wires to the Current Sink on the MIDI connector and the 4th pin from the left to Current Source.

## Flashing Firmware

This step changes depending on if you’re running Windows or Linux, and if you have bought the WCHLinkE. In any case, you will need the minichlink utility.

If you are running Linux, it is part of a repo [here](https://github.com/cnlohr/ch32v003fun/tree/master/minichlink) and can be built using the makefile there. Also read the readme there for instructions on installing the required udev rules.

If you are running windows, download the pre-built executable files [here](https://github.com/cnlohr/ch32v003fun/blob/master/minichlink/minichlink.exe) and [here](https://github.com/cnlohr/ch32v003fun/blob/master/minichlink/libusb-1.0.dll) (you need both files).

A pre-built binary of the synthesizer’s firmware is [here](https://example.com/). To flash it using a WCHLinkE, the command-line `minichlink(.exe) -w synth.bin flash -b` should work on either OS, and you are already done.

If you did not buy the WCHLinkE, but have an Arduino Uno laying around, you can use Ardulink. It is available on its own repo [here](https://gitlab.com/BlueSyncLine/arduino-ch32v003-swio). Follow the instructions there to set it up. Note that it requires a Linux install or WSL to build, but can be *used* from either OS.

On Windows: `minichlink.exe -c COMx -w synth.bin flash -b` where "COMx" needs to be substituted with whichever COM port your Arduino uses.

On Linux: `minichlink -c /dev/ttyACMx -w synth.bin flash -b` where "ttyACMx" needs to be substituted with whichever tty port your Arduino uses.

## Uploading patches

A set of patches for all standard MIDI instruments must be uploaded to the synthesizer at least once. A public-domain file containing patches is included in this repo: [GENMIDI.op2](https://github.com/89Mods/YM3812-dual-synth/blob/main/PatchUploader/GENMIDI.op2). You can edit this to your liking using the [OPL3 Bank Editor](https://github.com/Wohlstand/OPL3BankEditor) (scroll down and use one of the "Fresh dev builds", the latest GitHub release is years old and has issues).

A custom tool I made can then be used to send this data to the synthesizer over the MIDI port. [PatchUploader.jar](https://github.com/89Mods/YM3812-dual-synth/blob/main/PatchUploader/PatchUploader.jar) (requires Java 11 or higher). If your system does not already associate .jar files with Java, you may need to run it through a command line with `java -jar PatchUploader.jar`

The UI should be straight forward. After selecting the the .op2 file and the MIDI port of the synthesizer, click the upload button and you should see activity on the synthesizer in the form of a flashing blue LED on the nanoCH. It should flash multiple times and the synth should then reset and play a jingle. If nothing happens, check if the MIDI port is wired up correctly and if any software or hardware in your setup is potentially blocking the sysex messages used by the uploader.

The uploader software releases the MIDI port when it is done, meaning you can keep it open and use it multiple times by just clicking the button again. The file is also re-loaded each time and does not need to be selected again.

## Switching Modes
