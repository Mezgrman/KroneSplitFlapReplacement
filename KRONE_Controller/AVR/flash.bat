"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe" -U lfuse:w:0xe6:m -U hfuse:w:0xd9:m -e -F -v -patmega328p -carduino -PCOM32 -b19200 -D -Uflash:w:"Fallblatt_KRONE_Modul_D\Fallblatt_KRONE_Modul_D\Debug\Fallblatt_KRONE_Modul_D.hex":i -C"C:\Program Files (x86)\Arduino\hardware\tools\avr\etc\avrdude.conf"