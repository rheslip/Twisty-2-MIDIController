
Enclosure files for the Twisty 2 MIDI Controller

There are two versions of the base - one has cutouts for the Pico USB connector and the TRS MIDI jack, and another which adds cutouts for a slide switch and USB-C for a battery charger module.

I used a 1S TP4056 5V 28x17mm module from Aliexpress. The case has a little box to hold the module - put some 2mm thick double sided tape in the bottom of the box and press the module into place. Its tight so the module may need a little sanding to fit.
Make sure you change the charging rate to a safe level for your battery! On the modules I bought R3 adjusts the charge rate - see the TP4056 datasheet.

https://www.aliexpress.com/item/1005007730039023.html?spm=a2g0o.productlist.main.33.3a48S4gVS4gVIH&algo_pvid=602ac151-a704-43e6-9ca4-8eaca6619dd5&algo_exp_id=602ac151-a704-43e6-9ca4-8eaca6619dd5-30&pdp_ext_f=%7B%22order%22%3A%2250%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&pdp_npi=6%40dis%21CAD%2113.22%214.34%21%21%2164.81%2121.27%21%402101d9ef17689602821121297e2db9%2112000042012406280%21sea%21CA%21717068514%21X%211%210%21n_tag%3A-29919%3Bd%3Abe1153aa%3Bm03_new_user%3A-29895&curPageLogUid=3bzEO6fgvTSN&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005007730039023%7C_p_origin_prod%3A


The stepup regulator is a 0.9v to 5V input, 5V output module. Solder the GND pin to Pico pin 38 GND and the 5V output pin to Pico pin 39 VSys. I added a 10k:10k divider from battery input Vin to GND with the center to pin 34 ADC3 which allows the battery voltage to be monitored.

https://www.aliexpress.com/item/1005003375824265.html?spm=a2g0o.order_list.order_list_main.369.4d9a1802hPwpIL

