/*

   comedi/drivers/adl_pci9111.c
 
   Hardware driver for PCI9111 ADLink cards:
 
     PCI-9111HR
            
   Copyright (C) 2002 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
Driver: adl_pci9111.o
Description: Driver for the Adlink PCI-9111HR card.
Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
Devices: [ADLink] PCI-9111HR (adl_pci9111)
Status: experimental

Supports:

  - ai_insn read
  - ao_insn read/write
  - di_insn read
  - do_insn read/write
  - ai_do_cmd mode with the following sources:
  
    - start_src 		TRIG_NOW
    - scan_begin_src 		TRIG_FOLLOW 	TRIG_TIMER	TRIG_EXT
    - convert_src				TRIG_TIMER	TRIG_EXT
    - scan_end_src		TRIG_COUNT
    - stop_src			TRIG_COUNT	TRIG_NONE
    
    The scanned channels must be consecutive and start from 0. They must
    all have the same range and aref.
    
Configuration options:
  
    [0] - PCI bus number (optional)
    [1] - PCI slot number (optional)
    
    If bus/slot is not specified, the first available PCI
    device will be used.
  
*/

/*
CHANGELOG:

  2002/02/19 Fixed the two's complement conversion in pci9111_(hr_)ai_get_data.
  2002/02/18 Added external trigger support for analog input.

TODO: 

  - Really test implemented functionality.
  - Add support for the PCI-9111DG with a probe routine to identify the card type 
    (perhaps with the help of the channel number readback of the A/D Data register). 
  - Add analog input mode 4 suppport.
  - Add external multiplexer support.
  - Add analog output cmd support (use irq_on_timer_tick to update analog output).
  - Add kernel < 2.4 support.

*/

#include <linux/comedidev.h>

#include <linux/delay.h>
#include <linux/pci.h>

#include "8253.h"

typedef enum
{
  false,
  true
} bool;

#define PCI9111_DRIVER_NAME 	"adl_pci9111"
#define PCI9111_HR_DEVICE_ID 	0x9111

// TODO: Add other pci9111 board id

#define PCI9111_IO_RANGE 	0x0100

#define PCI9111_FIFO_HALF_SIZE	512

#define PCI9111_AI_CHANNEL_NBR			16

#define PCI9111_AI_RESOLUTION			12
#define PCI9111_AI_RESOLUTION_MASK		0x0FFF
#define PCI9111_AI_RESOLUTION_2_CMP_BIT		0x0800

#define PCI9111_HR_AI_RESOLUTION		16
#define PCI9111_HR_AI_RESOLUTION_MASK		0xFFFF
#define PCI9111_HR_AI_RESOLUTION_2_CMP_BIT	0x8000

#define PCI9111_AI_ACQUISITION_PERIOD_MIN_NS	10000
#define PCI9111_AO_CHANNEL_NBR			1
#define	PCI9111_AO_RESOLUTION			12
#define PCI9111_AO_RESOLUTION_MASK		0x0FFF
#define PCI9111_DI_CHANNEL_NBR			16
#define	PCI9111_DO_CHANNEL_NBR			16
#define PCI9111_DO_MASK				0xFFFF

#define PCI9111_RANGE_SETTING_DELAY		10
#define PCI9111_AI_INSTANT_READ_UDELAY_US	2			
#define PCI9111_AI_INSTANT_READ_TIMEOUT		100

#define PCI9111_8254_CLOCK_PERIOD_NS		500

#define PCI9111_8254_COUNTER_0			0x00
#define PCI9111_8254_COUNTER_1			0x40
#define PCI9111_8254_COUNTER_2			0x80
#define PCI9111_8254_COUNTER_LATCH		0x00
#define PCI9111_8254_READ_LOAD_LSB_ONLY		0x10
#define PCI9111_8254_READ_LOAD_MSB_ONLY		0x20
#define PCI9111_8254_READ_LOAD_LSB_MSB		0x30
#define PCI9111_8254_MODE_0			0x00
#define PCI9111_8254_MODE_1			0x02
#define PCI9111_8254_MODE_2			0x04
#define PCI9111_8254_MODE_3			0x06
#define PCI9111_8254_MODE_4			0x08
#define PCI9111_8254_MODE_5			0x0A
#define PCI9111_8254_BINARY_COUNTER		0x00
#define PCI9111_8254_BCD_COUNTER		0x01

/* IO address map */

#define PCI9111_REGISTER_AD_FIFO_VALUE 			0x00 // AD Data stored in FIFO 
#define PCI9111_REGISTER_DA_OUTPUT 			0x00
#define PCI9111_REGISTER_DIGITAL_IO 			0x02
#define PCI9111_REGISTER_EXTENDED_IO_PORTS 		0x04
#define PCI9111_REGISTER_AD_CHANNEL_CONTROL 		0x06 // Channel selection 
#define PCI9111_REGISTER_AD_CHANNEL_READBACK 		0x06
#define PCI9111_REGISTER_INPUT_SIGNAL_RANGE 		0x08
#define PCI9111_REGISTER_RANGE_STATUS_READBACK 		0x08
#define PCI9111_REGISTER_TRIGGER_MODE_CONTROL 		0x0A
#define PCI9111_REGISTER_AD_MODE_INTERRUPT_READBACK 	0x0A
#define PCI9111_REGISTER_SOFTWARE_TRIGGER 		0x0E
#define PCI9111_REGISTER_INTERRUPT_CONTROL 		0x0C
#define PCI9111_REGISTER_8254_COUNTER_0			0x40
#define PCI9111_REGISTER_8254_COUNTER_1			0x42
#define PCI9111_REGISTER_8254_COUNTER_2 		0X44
#define PCI9111_REGISTER_8254_CONTROL			0x46
#define PCI9111_REGISTER_INTERRUPT_CLEAR 		0x48

#define PCI9111_TRIGGER_MASK 				0x0F
#define PCI9111_PTRG_OFF 				(0 << 3)
#define PCI9111_PTRG_ON 				(1 << 3)
#define PCI9111_EITS_EXTERNAL				(1 << 2)
#define PCI9111_EITS_INTERNAL				(0 << 2)
#define PCI9111_TPST_SOFTWARE_TRIGGER			(0 << 1)
#define PCI9111_TPST_TIMER_PACER			(1 << 1)
#define PCI9111_ASCAN_ON				(1 << 0)
#define PCI9111_ASCAN_OFF				(0 << 0)

#define PCI9111_ISC0_SET_IRQ_ON_ENDING_OF_AD_CONVERSION (0 << 0)
#define PCI9111_ISC0_SET_IRQ_ON_FIFO_HALF_FULL		(1 << 0)
#define PCI9111_ISC1_SET_IRQ_ON_TIMER_TICK  		(0 << 1)
#define PCI9111_ISC1_SET_IRQ_ON_EXT_TRG 		(1 << 1)
#define PCI9111_FFEN_SET_FIFO_ENABLE 			(0 << 2)
#define PCI9111_FFEN_SET_FIFO_DISABLE			(1 << 2)

#define PCI9111_CHANNEL_MASK				0x0F

#define PCI9111_RANGE_MASK				0x07
#define PCI9111_FIFO_EMPTY_MASK				0x10
#define PCI9111_FIFO_HALF_FULL_MASK			0x20
#define PCI9111_FIFO_FULL_MASK				0x40
#define PCI9111_AD_BUSY_MASK				0x80

#define PCI9111_IO_BASE dev->iobase

/* 
 * Define inlined function
 */

#define pci9111_trigger_and_autoscan_get() \
  (inb(PCI9111_IO_BASE+PCI9111_REGISTER_AD_MODE_INTERRUPT_READBACK)&0x0F)

#define pci9111_trigger_and_autoscan_set(flags) \
  outb(flags,PCI9111_IO_BASE+PCI9111_REGISTER_TRIGGER_MODE_CONTROL)

#define pci9111_interrupt_and_fifo_get() \
  ((inb(PCI9111_IO_BASE+PCI9111_REGISTER_AD_MODE_INTERRUPT_READBACK) >> 4) &0x03)

#define pci9111_interrupt_and_fifo_set(flags) \
  outb(flags,PCI9111_IO_BASE+PCI9111_REGISTER_INTERRUPT_CONTROL)

#define pci9111_interrupt_clear() \
  outb(0,PCI9111_IO_BASE+PCI9111_REGISTER_INTERRUPT_CLEAR)
  
#define pci9111_software_trigger() \
  outb(0,PCI9111_IO_BASE+PCI9111_REGISTER_SOFTWARE_TRIGGER)

#define pci9111_fifo_reset() \
  outb(PCI9111_FFEN_SET_FIFO_ENABLE,PCI9111_IO_BASE+PCI9111_REGISTER_INTERRUPT_CONTROL); \
  outb(PCI9111_FFEN_SET_FIFO_DISABLE,PCI9111_IO_BASE+PCI9111_REGISTER_INTERRUPT_CONTROL); \
  outb(PCI9111_FFEN_SET_FIFO_ENABLE,PCI9111_IO_BASE+PCI9111_REGISTER_INTERRUPT_CONTROL)

#define pci9111_is_fifo_full() \
  ((inb(PCI9111_IO_BASE+PCI9111_REGISTER_RANGE_STATUS_READBACK)& \
    PCI9111_FIFO_FULL_MASK)==0)

#define pci9111_is_fifo_half_full() \
  ((inb(PCI9111_IO_BASE+PCI9111_REGISTER_RANGE_STATUS_READBACK)& \
    PCI9111_FIFO_HALF_FULL_MASK)==0)

#define pci9111_is_fifo_empty() \
  ((inb(PCI9111_IO_BASE+PCI9111_REGISTER_RANGE_STATUS_READBACK)& \
    PCI9111_FIFO_EMPTY_MASK)==0)

#define pci9111_ai_channel_set(channel) \
  outb((channel)&PCI9111_CHANNEL_MASK,PCI9111_IO_BASE+PCI9111_REGISTER_AD_CHANNEL_CONTROL)

#define pci9111_ai_channel_get() \
  inb(PCI9111_IO_BASE+PCI9111_REGISTER_AD_CHANNEL_READBACK)&PCI9111_CHANNEL_MASK

#define pci9111_ai_range_set(range) \
  outb((range)&PCI9111_RANGE_MASK,PCI9111_IO_BASE+PCI9111_REGISTER_INPUT_SIGNAL_RANGE)

#define pci9111_ai_range_get() \
  inb(PCI9111_IO_BASE+PCI9111_REGISTER_RANGE_STATUS_READBACK)&PCI9111_RANGE_MASK

#define pci9111_ai_get_data() \
  ((inw(PCI9111_IO_BASE+PCI9111_REGISTER_AD_FIFO_VALUE)>>4)&PCI9111_AI_RESOLUTION_MASK) \
  ^ PCI9111_AI_RESOLUTION_2_CMP_BIT

#define pci9111_hr_ai_get_data() \
  (inw(PCI9111_IO_BASE+PCI9111_REGISTER_AD_FIFO_VALUE) & PCI9111_HR_AI_RESOLUTION_MASK) \
  ^ PCI9111_HR_AI_RESOLUTION_2_CMP_BIT
  
#define pci9111_ao_set_data(data) \
  outb(data&PCI9111_AO_RESOLUTION_MASK,PCI9111_IO_BASE+PCI9111_REGISTER_DA_OUTPUT)

#define pci9111_di_get_bits() \
  inw(PCI9111_IO_BASE+PCI9111_REGISTER_DIGITAL_IO)

#define pci9111_do_set_bits(bits) \
  outw(bits,PCI9111_IO_BASE+PCI9111_REGISTER_DIGITAL_IO)
  
#define pci9111_8254_control_set(flags) \
  outb(flags,PCI9111_IO_BASE+PCI9111_REGISTER_8254_CONTROL)

#define pci9111_8254_counter_0_set(data) \
  outb(data & 0xFF, PCI9111_IO_BASE+PCI9111_REGISTER_8254_COUNTER_0); \
  outb( (data >> 8) & 0xFF, PCI9111_IO_BASE+PCI9111_REGISTER_8254_COUNTER_0)
  
#define pci9111_8254_counter_1_set(data) \
  outb(data & 0xFF, PCI9111_IO_BASE+PCI9111_REGISTER_8254_COUNTER_1); \
  outb( (data >> 8) & 0xFF, PCI9111_IO_BASE+PCI9111_REGISTER_8254_COUNTER_1)
  
#define pci9111_8254_counter_2_set(data) \
  outb(data & 0xFF, PCI9111_IO_BASE+PCI9111_REGISTER_8254_COUNTER_2); \
  outb( (data >> 8) & 0xFF, PCI9111_IO_BASE+PCI9111_REGISTER_8254_COUNTER_2)
  
// 
// Function prototypes
//
  
static int pci9111_attach (comedi_device *dev,comedi_devconfig *it);
static int pci9111_detach (comedi_device *dev);

static comedi_lrange pci9111_hr_ai_range=
{ 
  5,
  {
    BIP_RANGE(10),
    BIP_RANGE(5),
    BIP_RANGE(2.5),
    BIP_RANGE(1.25),
    BIP_RANGE(0.625)
  }
};

static struct pci_device_id pci9111_pci_table[] __devinitdata = {
	{ PCI_VENDOR_ID_ADLINK, PCI9111_HR_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	//{ PCI_VENDOR_ID_ADLINK, PCI9111_HG_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, pci9111_pci_table);

//
// Board specification structure
//

typedef struct {
  char 		*name;			// driver name
  int		device_id;
  int 		ai_channel_nbr;		// num of A/D chans
  int 		ao_channel_nbr;		// num of D/A chans
  int		ai_resolution;		// resolution of A/D
  int		ai_resolution_mask;
  int		ao_resolution;		// resolution of D/A
  int		ao_resolution_mask;
  comedi_lrange	*ai_range_list;		// rangelist for A/D
  comedi_lrange	*ao_range_list;		// rangelist for D/A
  unsigned int	ai_acquisition_period_min_ns;
} pci9111_board_struct;


static pci9111_board_struct pci9111_boards[] =
{
  {
    name: 		"pci9111_hr", 
    device_id:		PCI9111_HR_DEVICE_ID,
    ai_channel_nbr:	PCI9111_AI_CHANNEL_NBR,			
    ao_channel_nbr:	PCI9111_AO_CHANNEL_NBR,			
    ai_resolution:	PCI9111_HR_AI_RESOLUTION,
    ai_resolution_mask:	PCI9111_HR_AI_RESOLUTION_MASK,		
    ao_resolution:	PCI9111_AO_RESOLUTION,
    ao_resolution_mask:	PCI9111_AO_RESOLUTION_MASK,			
    ai_range_list:	&pci9111_hr_ai_range,  
    ao_range_list:	&range_bipolar10,
    ai_acquisition_period_min_ns:	PCI9111_AI_ACQUISITION_PERIOD_MIN_NS
  }
};

#define pci9111_board_nbr \
  (sizeof(pci9111_boards)/sizeof(pci9111_board_struct))

static comedi_driver pci9111_driver=
{
  driver_name:	PCI9111_DRIVER_NAME,
  module:	THIS_MODULE,
  attach:	pci9111_attach,
  detach:	pci9111_detach,
  num_names:	pci9111_board_nbr,
  board_name:	pci9111_boards,
  offset:	sizeof(pci9111_board_struct),
};
COMEDI_INITCLEANUP(pci9111_driver);

//
// Private data structure
//

typedef struct 
{
  struct pci_dev*	pci_device;
  int			io_range;		// PCI6503 io range

  int			lcr_io_base;		// Local configuration register base address
  int			lcr_io_range;		

  int		scan_begin_counter;
  int		scan_begin_counter_limit;

  int		stop_counter;			
  int		stop_is_none;

  int 		ao_readback;			// Last written analog output data

  int 		timer_divisor_1;		// Divisor values for the 8254 timer pacer
  int 		timer_divisor_2;

  int 		is_valid;			// Is device valid
} 
pci9111_private_data_struct;


#define dev_private 	((pci9111_private_data_struct *)dev->private)

// ------------------------------------------------------------------
// 
// PLX9050 SECTION
// 
// ------------------------------------------------------------------

#define PLX9050_REGISTER_INTERRUPT_CONTROL 0x4c

#define PLX9050_LINTI1_ENABLE		(1 << 0)
#define PLX9050_LINTI1_ACTIVE_HIGH	(1 << 1)
#define PLX9050_LINTI1_STATUS		(1 << 2)
#define PLX9050_LINTI2_ENABLE		(1 << 3)
#define PLX9050_LINTI2_ACTIVE_HIGH	(1 << 4)
#define PLX9050_LINTI2_STATUS		(1 << 5)
#define PLX9050_PCI_INTERRUPT_ENABLE	(1 << 6)
#define PLX9050_SOFTWARE_INTERRUPT	(1 << 7)

static void plx9050_interrupt_control (int io_base,
				       bool LINTi1_enable,
				       bool LINTi1_active_high,
				       bool LINTi2_enable,
				       bool LINTi2_active_high,
				       bool interrupt_enable)
{
  int flags = 0;
  
  if (LINTi1_enable) flags |= PLX9050_LINTI1_ENABLE;
  if (LINTi1_active_high) flags |= PLX9050_LINTI1_ACTIVE_HIGH;
  if (LINTi2_enable) flags |= PLX9050_LINTI2_ENABLE;
  if (LINTi2_active_high) flags |= PLX9050_LINTI2_ACTIVE_HIGH;

  if (interrupt_enable) flags |= PLX9050_PCI_INTERRUPT_ENABLE;
  
  outb (flags, io_base + PLX9050_REGISTER_INTERRUPT_CONTROL);
}
   
// ------------------------------------------------------------------
// 
// MISCELLANEOUS SECTION
// 
// ------------------------------------------------------------------

//
// 8254 timer 
//

static void pci9111_timer_set ( comedi_device * dev) 
{
  pci9111_8254_control_set ( PCI9111_8254_COUNTER_0|
			     PCI9111_8254_READ_LOAD_LSB_MSB|
			     PCI9111_8254_MODE_0|
			     PCI9111_8254_BINARY_COUNTER);

  pci9111_8254_control_set ( PCI9111_8254_COUNTER_1|
			     PCI9111_8254_READ_LOAD_LSB_MSB|
			     PCI9111_8254_MODE_2|
			     PCI9111_8254_BINARY_COUNTER);

  pci9111_8254_control_set ( PCI9111_8254_COUNTER_2|
			     PCI9111_8254_READ_LOAD_LSB_MSB|
			     PCI9111_8254_MODE_2|
			     PCI9111_8254_BINARY_COUNTER);

  comedi_udelay(1);

  pci9111_8254_counter_2_set (dev_private->timer_divisor_2);
  pci9111_8254_counter_1_set (dev_private->timer_divisor_1);
}

typedef enum 
{
  software,
  timer_pacer,
  external
} 
pci9111_trigger_sources;
  
static void pci9111_trigger_source_set (comedi_device *dev,
					pci9111_trigger_sources source)
{
  int flags;
  
  flags = pci9111_trigger_and_autoscan_get() & 0x09;

  switch (source)
  {
    case software :
      flags |= PCI9111_EITS_INTERNAL | PCI9111_TPST_SOFTWARE_TRIGGER;
      break;

    case timer_pacer :
      flags |= PCI9111_EITS_INTERNAL | PCI9111_TPST_TIMER_PACER;
      break;

    case external : 
      flags |= PCI9111_EITS_EXTERNAL;
      break;
  }

  pci9111_trigger_and_autoscan_set (flags);
}

static void pci9111_pretrigger_set (comedi_device *dev,
				    bool pretrigger)
{
  int flags;

  flags = pci9111_trigger_and_autoscan_get () & 0x07;

  if (pretrigger) flags |= PCI9111_PTRG_ON;

  pci9111_trigger_and_autoscan_set (flags);
}

static void pci9111_autoscan_set (comedi_device *dev,
				  bool autoscan)
{
  int flags;

  flags = pci9111_trigger_and_autoscan_get() & 0x0e;

  if (autoscan) flags |= PCI9111_ASCAN_ON;

  pci9111_trigger_and_autoscan_set (flags);
}

typedef enum 
{
  irq_on_eoc,
  irq_on_fifo_half_full
}
pci9111_ISC0_sources;

typedef enum
{
  irq_on_timer_tick,
  irq_on_external_trigger
} 
pci9111_ISC1_sources;

static void pci9111_interrupt_source_set (comedi_device *dev,
					  pci9111_ISC0_sources irq_0_source,
					  pci9111_ISC1_sources irq_1_source)
{
  int flags;
  
  flags = pci9111_interrupt_and_fifo_get() & 0x04;

  if (irq_0_source == irq_on_fifo_half_full) 
    flags |= PCI9111_ISC0_SET_IRQ_ON_FIFO_HALF_FULL;
  
  if (irq_1_source == irq_on_external_trigger)
    flags |= PCI9111_ISC1_SET_IRQ_ON_EXT_TRG;
  
  pci9111_interrupt_and_fifo_set (flags);
}

// ------------------------------------------------------------------
// 
// HARDWARE TRIGGERED ANALOG INPUT SECTION
// 
// ------------------------------------------------------------------

//
// Cancel analog input autoscan
//

#undef AI_DO_CMD_DEBUG

static int pci9111_ai_cancel ( comedi_device *dev,
			       comedi_subdevice *s)
{
  // Disable interrupts
  
  plx9050_interrupt_control (dev_private->lcr_io_base, true, true, true, true, false);

  pci9111_trigger_source_set (dev, software);
  
  pci9111_autoscan_set (dev, false);

  pci9111_fifo_reset();
  
#ifdef AI_DO_CMD_DEBUG
  printk (PCI9111_DRIVER_NAME ": ai_cancel\n");
#endif
  
  return 0;
}

//
// Test analog input command
// 
 
#define pci9111_check_trigger_src(src,flags) \
  tmp = src; \
  src &= flags; \
  if (!src || tmp != src) error++
  
  static int pci9111_ai_do_cmd_test ( comedi_device *dev,
				      comedi_subdevice *s,
				      comedi_cmd *cmd)
{
  int tmp;
  int error=0;
  int range, reference;
  int i;
  int rounded_timer;
  pci9111_board_struct* board= (pci9111_board_struct*) dev->board_ptr;

  // Step 1 : check if trigger are trivialy valid

  pci9111_check_trigger_src (cmd->start_src, 		TRIG_NOW);
  pci9111_check_trigger_src (cmd->scan_begin_src,	TRIG_TIMER|TRIG_FOLLOW|TRIG_EXT);
  pci9111_check_trigger_src (cmd->convert_src, 		TRIG_TIMER|TRIG_EXT);
  pci9111_check_trigger_src (cmd->scan_end_src, 	TRIG_COUNT);
  pci9111_check_trigger_src (cmd->stop_src, 		TRIG_COUNT|TRIG_NONE);
  
  if (error) return 1;

  // step 2 : make sure trigger sources are unique and mutually compatible

  if (cmd->start_src != TRIG_NOW) error++;
  
  if ((cmd->scan_begin_src != TRIG_TIMER) &&
      (cmd->scan_begin_src != TRIG_FOLLOW) &&
      (cmd->scan_begin_src != TRIG_EXT)) error++;
      
  if ((cmd->convert_src != TRIG_TIMER) &&
      (cmd->convert_src != TRIG_EXT)) 
  {
    error++;
  }
  if ((cmd->convert_src == TRIG_TIMER) &&
      !((cmd->scan_begin_src == TRIG_TIMER) || 
	(cmd->scan_begin_src == TRIG_FOLLOW)))
  {
    error++;
  }
  if ((cmd->convert_src == TRIG_EXT) &&
      !((cmd->scan_begin_src == TRIG_EXT) ||
	(cmd->scan_begin_src == TRIG_FOLLOW)))
  {
    error++;
  }
    
  if (cmd->scan_end_src != TRIG_COUNT) error++;
  if ( (cmd->stop_src != TRIG_COUNT) && 
       (cmd->stop_src != TRIG_NONE)) error++;
  
  if (error) return 2;

  // Step 3 : make sure arguments are trivialy compatible
 
  if (cmd->chanlist_len<1)  
  {
    cmd->chanlist_len=1;
    error++;
  }

  if (cmd->chanlist_len>board->ai_channel_nbr)
  {
    cmd->chanlist_len=board->ai_channel_nbr;
    error++;
  }
  
  if ((cmd->start_src == TRIG_NOW) &&
      (cmd->start_arg!=0)) {
    cmd->start_arg=0;
    error++;
  }
  
  if ((cmd->convert_src == TRIG_TIMER) && 
      (cmd->convert_arg<board->ai_acquisition_period_min_ns))
  {
    cmd->convert_arg=board->ai_acquisition_period_min_ns;
    error++;
  }
  if ((cmd->convert_src == TRIG_EXT) && 
      (cmd->convert_arg != 0))
  {
    cmd->convert_arg = 0;
    error++;
  }

  if ((cmd->scan_begin_src == TRIG_TIMER) &&
      (cmd->scan_begin_arg < board->ai_acquisition_period_min_ns))
  {
    cmd->scan_begin_arg = board->ai_acquisition_period_min_ns;
    error++;
  }
  if ((cmd->scan_begin_src == TRIG_FOLLOW) &&
      (cmd->scan_begin_arg != 0))
  {
    cmd->scan_begin_arg = 0;
    error++;
  }
  if ((cmd->scan_begin_src == TRIG_EXT) &&
      (cmd->scan_begin_arg != 0))
  {
    cmd->scan_begin_arg = 0;
    error++;
  }

  if ((cmd->scan_end_src == TRIG_COUNT) &&
      (cmd->scan_end_arg != cmd->chanlist_len)) 
  {
    cmd->scan_end_arg=cmd->chanlist_len;
    error++;
  }

  if ((cmd->stop_src == TRIG_COUNT) &&
      (cmd->stop_arg < 1))
  {
    cmd->stop_arg=1;
    error++;
  }
  if ((cmd->stop_src == TRIG_NONE) &&
      (cmd->stop_arg !=0))
  {
    cmd->stop_arg=0;
    error++;
  }
  
  if (error) return 3;

  // Step 4 : fix up any arguments

  if (cmd->convert_src == TRIG_TIMER)
  {
    tmp = cmd->convert_arg;
    i8253_cascade_ns_to_timer_2div (PCI9111_8254_CLOCK_PERIOD_NS,
				    &(dev_private->timer_divisor_1),
				    &(dev_private->timer_divisor_2),
				    &(cmd->convert_arg),
				    cmd->flags & TRIG_ROUND_MASK);
    if (tmp != cmd->convert_arg) error++;
  }

  // There's only one timer on this card, so the scan_begin timer must
  // be a multiple of chanlist_len*convert_arg
  
  if (cmd->scan_begin_src == TRIG_TIMER)
  {
    rounded_timer = (cmd->scan_begin_arg / (cmd->chanlist_len * cmd->convert_arg)) *
      (cmd->chanlist_len * cmd->convert_arg);
    
    if (cmd->scan_begin_arg != rounded_timer)
    {
      cmd->scan_begin_arg = rounded_timer;
      error++;
    }

    if (cmd->scan_begin_arg < cmd->convert_arg )
    {
      cmd->scan_begin_arg = cmd->convert_arg;
      error++;
    }
  }

  if (error) return 4;

  // Step 5 : check channel list
  
  if (cmd->chanlist) {
    
    range=CR_RANGE(cmd->chanlist[0]);
    reference=CR_AREF(cmd->chanlist[0]);
    
    if (cmd->chanlist_len > 1) {
      for (i=0;i<cmd->chanlist_len;i++)
      {
	if (CR_CHAN(cmd->chanlist[i]) != i)
	{
	  comedi_error (dev,
			"entries in chanlist must be consecutive "
			"channels,counting upwards from 0\n");
	  error++;
	}
	if (CR_RANGE(cmd->chanlist[i]) != range)
	{
	  comedi_error (dev,
			"entries in chanlist must all have the same gain\n");
	  error++;
	}
	if (CR_AREF(cmd->chanlist[i]) != reference)
	{
	  comedi_error (dev,
			"entries in chanlist must all have the same reference\n");
	  error++;
	}
      }
    }
    else
    {
      if ((CR_CHAN (cmd->chanlist[0]) > (board->ai_channel_nbr -1)) ||
	  (CR_CHAN (cmd->chanlist[0]) < 0))
      {
	comedi_error (dev, 
		      "channel number is out of limits\n");
	error++;
      }
    }	
  }

  if (error) return 5;

  return 0;
  
}


//
// Analog input command
//

static int pci9111_ai_do_cmd ( comedi_device *dev,
			       comedi_subdevice *subdevice) 
{
  comedi_cmd *async_cmd=&subdevice->async->cmd;
  
  if (!dev->irq)
  {
    comedi_error (dev, "no irq assigned for PCI9111, cannot do hardware conversion");
    return -1;
  }

  // Set channel scan limit
  //
  // PCI9111 allows only scanning from channel 0 to channel n
  //
  // TODO: handle the case of an external multilexer
  // 

  if (async_cmd->chanlist_len>1)
  {
    pci9111_ai_channel_set ((async_cmd->chanlist_len)-1);
    pci9111_autoscan_set (dev, true);
  }
  else
  {
    pci9111_ai_channel_set (CR_CHAN (async_cmd->chanlist[0]));
    pci9111_autoscan_set (dev, false);
  }

  // Set gain
  //
  // This is the same gain on every channel
  // 
  
  pci9111_ai_range_set (CR_RANGE(async_cmd->chanlist[0]));

  // Set counter 

  switch (async_cmd->stop_src)
  {
    case TRIG_COUNT:
      dev_private->stop_counter = async_cmd->stop_arg * async_cmd->chanlist_len;
      dev_private->stop_is_none = 0;
      break;
      
    case TRIG_NONE:
      dev_private->stop_counter = 0;
      dev_private->stop_is_none = 1;
      break;

    default:
      comedi_error(dev, "Invalid stop trigger");
      return -1;
  }
  
  dev_private->scan_begin_counter=0;

  // Set timer pacer
  
  switch (async_cmd->convert_src)
  {
    case TRIG_TIMER:
      i8253_cascade_ns_to_timer_2div (PCI9111_8254_CLOCK_PERIOD_NS,
				      &(dev_private->timer_divisor_1),
				      &(dev_private->timer_divisor_2),
				      &(async_cmd->convert_arg),
				      async_cmd->flags & TRIG_ROUND_MASK);
#ifdef AI_DO_CMD_DEBUG
      printk (PCI9111_DRIVER_NAME ": divisors = %d, %d\n",
	      dev_private->timer_divisor_1,dev_private->timer_divisor_2);
#endif

      if (async_cmd->scan_begin_src == TRIG_TIMER)
      {
	dev_private->scan_begin_counter_limit = 
	  async_cmd->scan_begin_arg / async_cmd->convert_arg;
      } 
      else
      {
	dev_private->scan_begin_counter_limit = async_cmd->chanlist_len;
      }

      pci9111_trigger_source_set (dev,software);
      pci9111_timer_set (dev);
      pci9111_fifo_reset ();
      pci9111_interrupt_source_set (dev,irq_on_fifo_half_full,
				    irq_on_timer_tick);
      pci9111_trigger_source_set (dev,timer_pacer);
      plx9050_interrupt_control (dev_private->lcr_io_base, true, true, false, true, true); 

      break;

    case TRIG_EXT :

      dev_private->scan_begin_counter_limit = async_cmd->chanlist_len;

      pci9111_trigger_source_set (dev, external);
      pci9111_fifo_reset ();
      pci9111_interrupt_source_set (dev, irq_on_fifo_half_full,
				    irq_on_timer_tick);
      plx9050_interrupt_control (dev_private->lcr_io_base, true, true, false, true, true);

      break;

    default :
      comedi_error (dev, "Invalid convert trigger");
      return -1;
  }

#ifdef AI_DO_CMD_DEBUG
  printk (PCI9111_DRIVER_NAME ": start interruptions!\n");
  printk (PCI9111_DRIVER_NAME ": trigger source=%2x\n",
	  pci9111_trigger_and_autoscan_get());
  printk (PCI9111_DRIVER_NAME ": irq source=%2x\n",
	  pci9111_interrupt_and_fifo_get());
  printk (PCI9111_DRIVER_NAME ": ai_do_cmd\n");
  printk (PCI9111_DRIVER_NAME ": stop_counter=%d\n",
	  dev_private->stop_counter);
  printk (PCI9111_DRIVER_NAME ": scan_begin_counter_limit=%d\n", 
	  dev_private->scan_begin_counter_limit);
#endif

  return 0;
}

// ------------------------------------------------------------------
// 
// INTERRUPT SECTION
// 
// ------------------------------------------------------------------

#undef INTERRUPT_DEBUG

static void pci9111_interrupt (int irq,
			       void *p_device,
			       struct pt_regs *regs)
{
  comedi_device *dev=p_device;
  comedi_subdevice *subdevice = dev->read_subdev;
  comedi_async *async;
  unsigned long irq_flags;
  int i, data;
  int resolution = ((pci9111_board_struct *) dev->board_ptr)->ai_resolution;

  async = subdevice->async;

  comedi_spin_lock_irqsave (&dev->spinlock, irq_flags);
  
  if ((inb(dev_private->lcr_io_base+PLX9050_REGISTER_INTERRUPT_CONTROL) &
      PLX9050_LINTI1_STATUS) != 0)
  { 
    // Interrupt comes from fifo_half-full signal
  
    if (pci9111_is_fifo_full())
    {
      comedi_spin_unlock_irqrestore (&dev->spinlock, irq_flags);
      comedi_error (dev, PCI9111_DRIVER_NAME " fifo overflow");
      pci9111_interrupt_clear();
      pci9111_ai_cancel (dev, subdevice);
      async->events |= COMEDI_CB_ERROR | COMEDI_CB_EOA;
      comedi_event (dev, subdevice, async->events);

      return;
    }
 
    if (pci9111_is_fifo_half_full())
    {
#ifdef INTERRUPT_DEBUG
      printk (PCI9111_DRIVER_NAME ": fifo is half full\n");
#endif
      async->events |= COMEDI_CB_BLOCK;
      
      for (i=0;i<PCI9111_FIFO_HALF_SIZE;i++)
      {
	// FIXME: The resolution test should be done outside of the read loop
	
	if (resolution == PCI9111_HR_AI_RESOLUTION) {
	  data = pci9111_hr_ai_get_data ();
	}
	else
	{
	  data = pci9111_ai_get_data ();
	}
	
	if (((dev_private->stop_counter > 0) || (dev_private->stop_is_none)) &&
	    (dev_private->scan_begin_counter < async->cmd.chanlist_len))
	{
	  comedi_buf_put (async, data);
	  dev_private->stop_counter--;
	}
	
	dev_private->scan_begin_counter++;
	if (dev_private->scan_begin_counter >= dev_private->scan_begin_counter_limit)
	{
	  dev_private->scan_begin_counter=0;
	}
      }
    }
  }

  if ((dev_private->stop_counter==0) && (!dev_private->stop_is_none))
  {
    async->events |= COMEDI_CB_EOA;
    pci9111_ai_cancel (dev, subdevice);
  }
  
  // Very important, otherwise another interrupt request will be inserted
  // and will cause driver hangs on processing interrupt event (and cause a
  // computer crash, and corrupt the source file of the driver you are 
  // working on, since you forgot to do a sync before test, and you cry, 
  // and ...)
  
  pci9111_interrupt_clear();

  comedi_spin_unlock_irqrestore (&dev->spinlock, irq_flags);

  comedi_event (dev, subdevice, async->events);
}
			       
// ------------------------------------------------------------------
// 
// INSTANT ANALOG INPUT OUTPUT SECTION
// 
// ------------------------------------------------------------------

//
// analog instant input
//

#undef AI_INSN_DEBUG

static int pci9111_ai_insn_read ( comedi_device *dev,
				  comedi_subdevice *subdevice, 
				  comedi_insn *insn,
				  lsampl_t *data )
{
  int resolution = ((pci9111_board_struct *) dev->board_ptr)->ai_resolution;

  int timeout,i;

#ifdef AI_INSN_DEBUG
  printk (PCI9111_DRIVER_NAME ": ai_insn set c/r/n = %2x/%2x/%2x\n",
  	  CR_CHAN((&insn->chanspec)[0]),
	  CR_RANGE((&insn->chanspec)[0]),
	  insn->n);
#endif

  pci9111_ai_channel_set(CR_CHAN((&insn->chanspec)[0]));

  if ((pci9111_ai_range_get()) != CR_RANGE((&insn->chanspec)[0]))
  {
    pci9111_ai_range_set(CR_RANGE((&insn->chanspec)[0]));
  }

  pci9111_fifo_reset();

  for (i=0;i<insn->n;i++)
  {
    pci9111_software_trigger();
    
    timeout=PCI9111_AI_INSTANT_READ_TIMEOUT;
    
    while (timeout--) {
      if (!pci9111_is_fifo_empty()) goto conversion_done;
    }

    comedi_error(dev,"A/D read timeout");
    data[i]=0;
    pci9111_fifo_reset();
    return -ETIME;

conversion_done:
 
    if (resolution==PCI9111_HR_AI_RESOLUTION)
    {
      data[i] = pci9111_hr_ai_get_data();
    } 
    else
    {
      data[i] = pci9111_ai_get_data();
    }
  }
  
#ifdef AI_INSN_DEBUG
  printk (PCI9111_DRIVER_NAME ": ai_insn get c/r/t = %2x/%2x/%2x\n",
  	  pci9111_ai_channel_get(),
	  pci9111_ai_range_get(),
	  pci9111_trigger_and_autoscan_get());
#endif

  return i;
}

//
// Analog instant output
//

static int pci9111_ao_insn_write ( comedi_device *dev,
				   comedi_subdevice *s, 
				   comedi_insn *insn,
				   lsampl_t *data )
{
  int i;

  for (i=0; i<insn->n; i++) {
    pci9111_ao_set_data(data[i]);
    dev_private->ao_readback=data[i];
  }

  return i;
}

// 
// Analog output readback
//

static int pci9111_ao_insn_read ( comedi_device * dev, 
				  comedi_subdevice * s, 
				  comedi_insn *insn, 
				  lsampl_t *data) 
{
  int i;

  for (i=0; i<insn->n; i++) { 
    data[i]=dev_private->ao_readback & PCI9111_AO_RESOLUTION_MASK;
  }

  return i;
}

// ------------------------------------------------------------------
// 
// DIGITAL INPUT OUTPUT SECTION
// 
// ------------------------------------------------------------------

//
// Digital inputs
//

static int pci9111_di_insn_bits ( comedi_device *dev, 
				  comedi_subdevice *subdevice,
				  comedi_insn *insn,
				  lsampl_t *data)
{
  lsampl_t bits;

  bits = pci9111_di_get_bits();
  data[1] = bits;
  
  return 2;
}

//
// Digital outputs
//

static int pci9111_do_insn_bits ( comedi_device *dev, 
				  comedi_subdevice *subdevice,
				  comedi_insn *insn,
				  lsampl_t *data)
{
  lsampl_t bits;

  // Only set bits that have been masked
  // data[0] = mask
  // data[1] = bit state
  
  data[0] &= PCI9111_DO_MASK;
  
  bits = subdevice->state;
  bits &= ~data[0];
  bits |= data[0] & data[1];
  subdevice->state = bits;
  
  pci9111_do_set_bits(bits);

  data[1] = bits;
  
  return 2;
}


// ------------------------------------------------------------------
// 
// INITIALISATION SECTION
// 
// ------------------------------------------------------------------

//
// Reset device
// 

static int pci9111_reset (comedi_device *dev)
{ 
  // Set trigger source to software
  
  plx9050_interrupt_control (dev_private->lcr_io_base, true, true, true, true, false);

  pci9111_trigger_source_set (dev, software);
  pci9111_pretrigger_set (dev, false);
  pci9111_autoscan_set (dev,false);

  // Reset 8254 chip
  
  dev_private->timer_divisor_1=0;
  dev_private->timer_divisor_2=0;
  
  pci9111_timer_set (dev);

  return 0;
}

//
// Attach 
// 	
// 	- Register PCI device
// 	- Declare device driver capability
//	

static int pci9111_attach(comedi_device *dev,comedi_devconfig *it)
{

  comedi_subdevice *subdevice;
  int io_base, io_range, lcr_io_base, lcr_io_range;
  struct pci_dev* pci_device;
  int error,i;
  pci9111_board_struct* board;

//
// Probe the device to determine what device in the series it is.
//
  
  printk("comedi%d: " PCI9111_DRIVER_NAME " driver\n",dev->minor);

  pci_for_each_dev(pci_device)
  {
    if (pci_device->vendor == PCI_VENDOR_ID_ADLINK)
    {
      for (i= 0; i< pci9111_board_nbr; i++)
      {
	if(pci9111_boards[i].device_id == pci_device->device)
	{
	  // was a particular bus/slot requested?
	  if((it->options[0] != 0) || (it->options[1] != 0))
	  {
	    // are we on the wrong bus/slot?
	    if(pci_device->bus->number != it->options[0] ||
	       PCI_SLOT(pci_device->devfn) != it->options[1])
	    {
	      continue;
	    }
	  }
	  
	  dev->board_ptr = pci9111_boards + i;
	  board = (pci9111_board_struct *) dev->board_ptr;
	  goto found;
	}
      }
    }
  }
  
  printk ("comedi%d: no supported board found! (req. bus/slot : %d/%d)\n",
	  dev->minor,it->options[0], it->options[1]);
  return -EIO;

found:

  printk("comedi%d: found %s (b:s:f=%d:%d:%d) , irq=%d\n", 
	 dev->minor,
	 pci9111_boards[i].name,
	 pci_device->bus->number,
	 PCI_SLOT(pci_device->devfn),
	 PCI_FUNC(pci_device->devfn),
	 pci_device->irq);

  // TODO: Warn about non-tested boards.

  switch(board->device_id)
  {
  };

  // Read local configuration register base address [PCI_BASE_ADDRESS #1].
  
  lcr_io_base = pci_resource_start (pci_device, 1);
  lcr_io_range = pci_resource_end (pci_device, 1) - lcr_io_base +1;
  
  printk ("comedi%d: local configuration registers at address 0x%4x [0x%4x]\n",
	  dev->minor,
	  lcr_io_base,
	  lcr_io_range);
  
  if (check_region (lcr_io_base, lcr_io_range) < 0) 
  {
    printk("comedi%d: I/O port conflict\n",dev->minor);
    return -EIO;
  }
  
  // Read PCI6308 register base address [PCI_BASE_ADDRESS #2].
  
  io_base = pci_resource_start (pci_device, 2);
  io_range = pci_resource_end (pci_device, 2) - io_base +1;
  
  printk ("comedi%d: 6503 registers at address 0x%4x [0x%4x]\n",
	  dev->minor,
	  io_base,
	  io_range);
  
  if (check_region (io_base, io_range) < 0) 
  {
    printk("comedi%d: I/O port conflict\n",dev->minor);
    return -EIO;
  }
  
  // Allocate IO ressources
  
  pci_request_regions(pci_device, PCI9111_DRIVER_NAME);
  
  dev->iobase=io_base;
  dev->board_name = board->name;
  
  if(alloc_private(dev,sizeof(pci9111_private_data_struct))<0)
    return -ENOMEM;
  
  dev_private->pci_device = pci_device;
  dev_private->io_range = io_range;
  dev_private->is_valid=0;
  dev_private->lcr_io_base=lcr_io_base;
  dev_private->lcr_io_range=lcr_io_range;
  
  pci9111_reset(dev);

  // Irq setup
  
  dev->irq=0;
  if (pci_device->irq>0) 
  {
    if (comedi_request_irq (pci_device->irq,
			    pci9111_interrupt, 
			    SA_SHIRQ, 
			    PCI9111_DRIVER_NAME, 
			    dev)!=0)
    {
      printk ("comedi%d: unable to allocate irq  %d\n", dev->minor, pci_device->irq);
      return -EINVAL;
    }
  }
  dev->irq = pci_device->irq;
  
//
// TODO: Add external multiplexer setup (according to option[2]).
//

  if((error=alloc_subdevices(dev, 4))<0)
    return  error;
  
  subdevice 			= dev->subdevices + 0;
  dev->read_subdev = subdevice;

  subdevice->type 		= COMEDI_SUBD_AI;
  subdevice->subdev_flags 	= SDF_READABLE|SDF_COMMON;

//
// TODO: Add external multiplexer data
// 
//    if (devpriv->usemux) { subdevice->n_chan = devpriv->usemux; }
//    else { subdevice->n_chan = this_board->n_aichan; }
//
  
  subdevice->n_chan 		= board->ai_channel_nbr;
  subdevice->maxdata 		= board->ai_resolution_mask;
  subdevice->len_chanlist 	= board->ai_channel_nbr;
  subdevice->range_table 	= board->ai_range_list;
  subdevice->cancel		= pci9111_ai_cancel;
  subdevice->insn_read		= pci9111_ai_insn_read;
  subdevice->do_cmdtest		= pci9111_ai_do_cmd_test;
  subdevice->do_cmd		= pci9111_ai_do_cmd;

  subdevice 			= dev->subdevices + 1;
  subdevice->type 		= COMEDI_SUBD_AO;
  subdevice->subdev_flags	= SDF_WRITABLE|SDF_COMMON; 
  subdevice->n_chan		= board->ao_channel_nbr;
  subdevice->maxdata		= board->ao_resolution_mask;
  subdevice->len_chanlist	= board->ao_channel_nbr;
  subdevice->range_table	= board->ao_range_list;
  subdevice->insn_write		= pci9111_ao_insn_write;
  subdevice->insn_read		= pci9111_ao_insn_read;
    
  subdevice 			= dev->subdevices + 2;
  subdevice->type 		= COMEDI_SUBD_DI;
  subdevice->subdev_flags 	= SDF_READABLE;
  subdevice->n_chan 		= PCI9111_DI_CHANNEL_NBR;
  subdevice->maxdata 		= 1;
  subdevice->range_table 	= &range_digital;
  subdevice->insn_bits		= pci9111_di_insn_bits;
	
  subdevice 			= dev->subdevices + 3;
  subdevice->type 		= COMEDI_SUBD_DO;
  subdevice->subdev_flags 	= SDF_READABLE|SDF_WRITABLE;
  subdevice->n_chan 		= PCI9111_DO_CHANNEL_NBR;
  subdevice->maxdata 		= 1;
  subdevice->range_table 	= &range_digital;
  subdevice->insn_bits		= pci9111_do_insn_bits;
  
  dev_private->is_valid			= 1;
  
  return 0;
}


//
// Detach 
//

static int pci9111_detach(comedi_device *dev)
{
  // Reset device
  
  if (dev->private!=0) 
  {
    if (dev_private->is_valid) pci9111_reset(dev);

  }

  // Release previously allocated irq
  
  if (dev->irq!=0)
  {
    comedi_free_irq(dev->irq,dev);
  }

  pci_release_regions(dev_private->pci_device);

  return 0;
}


