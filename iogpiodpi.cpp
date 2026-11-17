#include <iogpiodpi.h>
#include <iostream>
#include <stdio.h>


IOGPIODPi::IOGPIODPi(int tms, int tck, int tdi, int tdo)
 : TMSPin(tms), TCKPin(tck), TDIPin(tdi), TDOPin(tdo)
{
    TMSline = TCKline = TDIline = TDOline = NULL;
    chip = gpiod_chip_open_by_name("gpiochip0"); 
    if (!chip) {
        perror("Open chip failed");
        return;
    }

    TMSline = gpiod_chip_get_line(chip, TMSPin);
    TCKline = gpiod_chip_get_line(chip, TCKPin);
    TDIline = gpiod_chip_get_line(chip, TDIPin);
    TDOline = gpiod_chip_get_line(chip, TDOPin);
    if (!TMSline || !TCKline || !TDIline || !TDOline) {
        perror("JTAG line config failed");
        goto fail;
    }
    if (gpiod_line_request_output(TMSline, "Consumer", 0) < 0) {
        perror("Request TMSline as output failed");
        goto fail;
    }
    if (gpiod_line_request_output(TCKline, "Consumer", 0) < 0) {
        perror("Request TCKline as output failed");
        goto fail;
    }
    if (gpiod_line_request_output(TDIline, "Consumer", 0) < 0) {
        perror("Request TDIline as output failed");
        goto fail;
    }
    if (gpiod_line_request_input(TDOline, "Consumer") < 0) {
        perror("Request TDOline as input failed");
        goto fail;
    }
    return;

fail:
    /* closing the chip releases every line requested from it */
    gpiod_chip_close(chip);
    chip = NULL;
    TMSline = TCKline = TDIline = TDOline = NULL;
}

/* The constructor cannot fail: report lines it could not get here, so
   xc3sprog stops instead of clocking JTAG through missing lines. */
int IOGPIODPi::Init(struct cable_t *cable, const char *devopt, unsigned int freq)
{
    if (!chip)
    {
        fprintf(stderr, "Could not get the JTAG GPIO lines\n");
        return 1;
    }
    return 0;
}

IOGPIODPi::~IOGPIODPi()
{
    if (chip)
        gpiod_chip_close(chip);
}

void IOGPIODPi::txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last)
{

  //  std::cerr << "txrx_block" << std::endl;
  int i=0;
  int j=0;
  unsigned char tdo_byte=0;
  unsigned char tdi_byte=0; /* no tdi: shift zeros (IOBase::shiftTDO) */
  if (tdi)
      tdi_byte = tdi[j];
      
  while(i<length-1){
      tdo_byte=tdo_byte+(txrx(false, (tdi_byte&1)==1)<<(i%8));
      if (tdi)
	  tdi_byte=tdi_byte>>1;
    i++;
    if((i%8)==0){ // Next byte
	if(tdo)
	    tdo[j]=tdo_byte; // Save the TDO byte
      tdo_byte=0;
      j++;
      if (tdi)
	  tdi_byte=tdi[j]; // Get the next TDI byte
    }
  };
  tdo_byte=tdo_byte+(txrx(last, (tdi_byte&1)==1)<<(i%8)); 
  if(tdo)
      tdo[j]=tdo_byte;

 gpiod_line_set_value(TCKline, 0);
  return;
}

void IOGPIODPi::tx_tms(unsigned char *pat, int length, int force)
{
    int i;
    unsigned char tms;
    for (i = 0; i < length; i++)
    {
      if ((i & 0x7) == 0)
	tms = pat[i>>3];
      tx((tms & 0x01), true);
      tms = tms >> 1;
    }
    
  gpiod_line_set_value(TCKline, 0);
}

void IOGPIODPi::tx(bool tms, bool tdi)
{
    gpiod_line_set_value(TCKline, 0);

    if(tdi)
        gpiod_line_set_value(TDIline, 1);
    else
        gpiod_line_set_value(TDIline, 0);

    if(tms)
        gpiod_line_set_value(TMSline, 1);
   else
        gpiod_line_set_value(TMSline, 0);

    gpiod_line_set_value(TCKline, 1);
}


bool IOGPIODPi::txrx(bool tms, bool tdi)
{

  // std::cerr << "txrx" << gpiod_line_get_value(TDOline) << std::endl;
    tx(tms, tdi);
    return gpiod_line_get_value(TDOline);
 // return digitalRead(TDOPin);  
}
