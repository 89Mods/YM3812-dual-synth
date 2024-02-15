# YM3812-dual-synth
MIDI synthesizer built from two YM3812s for 18-voice mono or 9-voice stereo. More info: https://tholin.dev/ym3812d/

# Build instructions

## PCB

The gerber file for the synthesizer can be found [here](https://github.com/89Mods/YM3812-dual-synth/tree/main/KiCad/production) and you should be able to use this with all the major board houses. Just make sure they correctly recognize it as a 4-layer PCB. The dimensions of the PCB are low enough to where it should be cheap despite it being 4-layers (8€ on JLCPCB).

## Parts

The YM3812s and their DACs are the most annoying parts to obtain. They can be easily found on eBay and AliExpress by simply using "YM3812" and "Y3014" keywords, but the condition of the parts can vary. The AliExpress listings I bought from are [here](https://de.aliexpress.com/item/1005001723547967.html) and [here.](https://de.aliexpress.com/item/1005002637379806.html)

The nanoCH32V003 mini dev board you can maybe even find at your favorite maker shop, but also [on AliExpress](https://de.aliexpress.com/item/1005005221751705.html). If there is an option to buy the board plus a "WCHLinkE" you may use that, but I will also show you how to use an Arduino Uno to flash the synthesizer firmware later on.

All the other integrated circuits can be found at various electronics retailers:

LM358: [TME](https://www.tme.eu/de/details/lm358ap/tht-operationsverstarker/texas-instruments/) - [Mouser](https://www.mouser.de/ProductDetail/Texas-Instruments/LM358N-NOPB?qs=QbsRYf82W3GXbWt%2FvwDTcQ%3D%3D) - [LCSC](https://www.lcsc.com/product-detail/Operational-Amplifier_HGSEMI-LM358N_C434570.html)

6N139 (or 6N138): [TME](https://www.tme.eu/de/details/6n139-eve/optokoppler-analogausgang/everlight/6n139/) - [Mouser](https://www.mouser.de/ProductDetail/Vishay-Semiconductors/6N139?qs=WHRYDAs2thAk3qh0MF5UBQ%3D%3D) - [LCSC](https://www.lcsc.com/product-detail/Optocouplers-Logic-Output_CT-Micro-International-6N139_C191869.html)

SN74LV1T34DBVRG4: [TME](https://www.tme.eu/de/details/sn74lv1t34dbvrg4/logische-wandler/texas-instruments/) - [Mouser](https://www.mouser.de/ProductDetail/Texas-Instruments/SN74LV1T34DBVRG4?qs=vdi0iO8H4N10R%252Bt%252BjmxZrg%3D%3D) - [LCSC](https://www.lcsc.com/product-detail/Buffer-Driver-Transceiver_Texas-Instruments-SN74LV1T34DBVR_C100024.html)

MX25L1005 (or MX25L2006 or 25Q32 or any spiflash that supports the 25xx command set and is made by Winbond or Macronix): [TME](https://www.tme.eu/de/details/mx25l2006em1i-12g/serielle-flash-speicher/macronix-international/mx25l2006em1i-12g-tube/) - [LCSC](https://www.lcsc.com/product-detail/NOR-FLASH_MXIC-Macronix-MX25L2006EM1I-12G_C135940.html) - I also recommend looking for "MX25L1005" on AliExpress, you can get them really cheap there.

## Flashing Firmware
