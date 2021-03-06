/*
    module/pcmad
    hardware driver for Winsystems PCM-A/D12 and PCM-A/D16

    COMEDI - Linux Control and Measurement Device Interface
    Copyright (C) 2000,2001 David A. Schleef <ds@schleef.org>

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
Driver: pcmad.o
Description: Winsystems PCM-A/D12, PCM-A/D16
Author: ds
Devices: [Winsystems] PCM-A/D12 (pcmad12), PCM-A/D16 (pcmad16)
Status: untested

This driver was written on a bet that I couldn't write a driver
in less than 2 hours.  I won the bet, but never got paid.  =(

Configuration options:
  [0] - I/O port base
  [1] - unused
  [2] - Analog input reference
          0 = single ended
          1 = differential
  [3] - Analog input encoding (must match jumpers)
          0 = straight binary
          1 = two's complement
*/


#include <linux/comedidev.h>

#include <linux/ioport.h>


#define PCMAD_SIZE		4

#define PCMAD_STATUS		0
#define PCMAD_LSB		1
#define PCMAD_MSB		2
#define PCMAD_CONVERT		1

struct pcmad_board_struct{
	char *name;
	int n_ai_bits;
};
static struct pcmad_board_struct pcmad_boards[]={
	{
	name:		"pcmad12",
	n_ai_bits:	12,
	},
	{
	name:		"pcmad16",
	n_ai_bits:	16,
	},
};
#define this_board ((struct pcmad_board_struct *)(dev->board_ptr))
#define n_pcmad_boards (sizeof(pcmad_boards)/sizeof(pcmad_boards[0]))

struct pcmad_priv_struct{
	int differential;
	int twos_comp;
};
#define devpriv ((struct pcmad_priv_struct *)dev->private)

static int pcmad_attach(comedi_device *dev,comedi_devconfig *it);
static int pcmad_detach(comedi_device *dev);
static comedi_driver driver_pcmad={
	driver_name:	"pcmad",
	module:		THIS_MODULE,
	attach:		pcmad_attach,
	detach:		pcmad_detach,
	board_name:	pcmad_boards,
	num_names:	n_pcmad_boards,
	offset:		sizeof(pcmad_boards[0]),
};
COMEDI_INITCLEANUP(driver_pcmad);


#define TIMEOUT	100

static int pcmad_ai_insn_read(comedi_device *dev,comedi_subdevice *s,
	comedi_insn *insn, lsampl_t *data)
{
	int i;
	int chan;
	int n;

	chan=CR_CHAN(insn->chanspec);

	for(n=0;n<insn->n;n++){
		outb(chan,dev->iobase+PCMAD_CONVERT);

		for(i=0;i<TIMEOUT;i++){
			if((inb(dev->iobase+PCMAD_STATUS)&0x3) == 0x3)
				break;
		}
		data[n]=inb(dev->iobase+PCMAD_LSB);
		data[n]|=(inb(dev->iobase+PCMAD_MSB)<<8);

		if(devpriv->twos_comp){
			data[n] ^= (1<<(this_board->n_ai_bits-1));
		}
	}
	
	return n;
}

/*
 * options:
 * 0	i/o base
 * 1	unused
 * 2	0=single ended 1=differential
 * 3	0=straight binary 1=two's comp
 */
static int pcmad_attach(comedi_device *dev,comedi_devconfig *it)
{
	int ret;
	comedi_subdevice *s;
	int iobase;

	iobase=it->options[0];
	printk("comedi%d: pcmad: 0x%04x ",dev->minor,iobase);
	if(check_region(iobase,PCMAD_SIZE)<0){
		printk("I/O port conflict\n");
		return -EIO;
	}
	request_region(iobase,PCMAD_SIZE,"pcmad");
	dev->iobase=iobase;

	if((ret=alloc_subdevices(dev, 1))<0)
		return ret;
	if((ret=alloc_private(dev,sizeof(struct pcmad_priv_struct)))<0)
		return ret;

	dev->board_name = this_board->name;

	s=dev->subdevices+0;
	s->type=COMEDI_SUBD_AI;
	s->subdev_flags=SDF_READABLE|AREF_GROUND;
	s->n_chan=16;			/* XXX */
	s->len_chanlist=1;
	s->insn_read=pcmad_ai_insn_read;
	s->maxdata=(1<<this_board->n_ai_bits)-1;
	s->range_table=&range_unknown;

	return 0;
}


static int pcmad_detach(comedi_device *dev)
{
	printk("comedi%d: pcmad: remove\n",dev->minor);
	
	if(dev->irq){
		free_irq(dev->irq,dev);
	}
	if(dev->iobase)
		release_region(dev->iobase,PCMAD_SIZE);

	return 0;
}

