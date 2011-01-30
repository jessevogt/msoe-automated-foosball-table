/*
        The original file was written 1996-1999 by Donald Becker.

	This software may be used and distributed according to the terms
	of the GNU General Public License, incorporated herein by reference.

	This driver is for the 3Com "Vortex" and "Boomerang" series ethercards.
	Members of the series include Fast EtherLink 3c590/3c592/3c595/3c597
	and the EtherLink XL 3c900 and 3c905 cards.

	Problem reports and questions should be directed to
	vortex@scyld.com

	The author may be reached as becker@scyld.com, or C/O
	Scyld Computing Corporation
	410 Severn Ave., Suite 210
	Annapolis MD 21403

	Linux Kernel Additions (better to see the last one first):
	
 	0.99H+lk0.9 - David S. Miller - softnet, PCI DMA updates
 	0.99H+lk1.0 - Jeff Garzik <jgarzik@mandrakesoft.com>
		Remove compatibility defines for kernel versions < 2.2.x.
		Update for new 2.3.x module interface
	LK1.1.2 (March 19, 2000)
	* New PCI interface (jgarzik)

    LK1.1.3 25 April 2000, Andrew Morton <andrewm@uow.edu.au>
    - Merged with 3c575_cb.c
    - Don't set RxComplete in boomerang interrupt enable reg
    - spinlock in vortex_timer to protect mdio functions
    - disable local interrupts around call to vortex_interrupt in
      vortex_tx_timeout() (So vortex_interrupt can use spin_lock())
    - Select window 3 in vortex_timer()'s write to Wn3_MAC_Ctrl
    - In vortex_start_xmit(), move the lock to _after_ we've altered
      vp->cur_tx and vp->tx_full.  This defeats the race between
      vortex_start_xmit() and vortex_interrupt which was identified
      by Bogdan Costescu.
    - Merged back support for six new cards from various sources
    - Set vortex_have_pci if pci_module_init returns zero (fixes cardbus
      insertion oops)
    - Tell it that 3c905C has NWAY for 100bT autoneg
    - Fix handling of SetStatusEnd in 'Too much work..' code, as
      per 2.3.99's 3c575_cb (Dave Hinds).
    - Split ISR into two for vortex & boomerang
    - Fix MOD_INC/DEC races
    - Handle resource allocation failures.
    - Fix 3CCFE575CT LED polarity
    - Make tx_interrupt_mitigation the default

    LK1.1.4 25 April 2000, Andrew Morton <andrewm@uow.edu.au>    
    - Add extra TxReset to vortex_up() to fix 575_cb hotplug initialisation probs.
    - Put vortex_info_tbl into __devinitdata
    - In the vortex_error StatsFull HACK, disable stats in vp->intr_enable as well
      as in the hardware.
    - Increased the loop counter in issue_and_wait from 2,000 to 4,000.

    LK1.1.5 28 April 2000, andrewm
    - Added powerpc defines (John Daniel <jdaniel@etresoft.com> said these work...)
    - Some extra diagnostics
    - In vortex_error(), reset the Tx on maxCollisions.  Otherwise most
      chips usually get a Tx timeout.
    - Added extra_reset module parm
    - Replaced some inline timer manip with mod_timer
      (Franois romieu <Francois.Romieu@nic.fr>)
    - In vortex_up(), don't make Wn3_config initialisation dependent upon has_nway
      (this came across from 3c575_cb).

    LK1.1.6 06 Jun 2000, andrewm
    - Backed out the PPC defines.
    - Use del_timer_sync(), mod_timer().
    - Fix wrapped ulong comparison in boomerang_rx()
    - Add IS_TORNADO, use it to suppress 3c905C checksum error msg
      (Donald Becker, I Lee Hetherington <ilh@sls.lcs.mit.edu>)
    - Replace union wn3_config with BFINS/BFEXT manipulation for
      sparc64 (Pete Zaitcev, Peter Jones)
    - In vortex_error, do_tx_reset and vortex_tx_timeout(Vortex):
      do a netif_wake_queue() to better recover from errors. (Anders Pedersen,
      Donald Becker)
    - Print a warning on out-of-memory (rate limited to 1 per 10 secs)
    - Added two more Cardbus 575 NICs: 5b57 and 6564 (Paul Wagland)

    LK1.1.7 2 Jul 2000 andrewm
    - Better handling of shared IRQs
    - Reset the transmitter on a Tx reclaim error
    - Fixed crash under OOM during vortex_open() (Mark Hemment)
    - Fix Rx cessation problem during OOM (help from Mark Hemment)
    - The spinlocks around the mdio access were blocking interrupts for 300uS.
      Fix all this to use spin_lock_bh() within mdio_read/write
    - Only write to TxFreeThreshold if it's a boomerang - other NICs don't
      have one.
    - Added 802.3x MAC-layer flow control support

   LK1.1.8 13 Aug 2000 andrewm
    - Ignore request_region() return value - already reserved if Cardbus.
    - Merged some additional Cardbus flags from Don's 0.99Qk
    - Some fixes for 3c556 (Fred Maciel)
    - Fix for EISA initialisation (Jan Rekorajski)
    - Renamed MII_XCVR_PWR and EEPROM_230 to align with 3c575_cb and D. Becker's drivers
    - Fixed MII_XCVR_PWR for 3CCFE575CT
    - Added INVERT_LED_PWR, used it.
    - Backed out the extra_reset stuff

   LK1.1.9 12 Sep 2000 andrewm
    - Backed out the tx_reset_resume flags.  It was a no-op.
    - In vortex_error, don't reset the Tx on txReclaim errors
    - In vortex_error, don't reset the Tx on maxCollisions errors.
      Hence backed out all the DownListPtr logic here.
    - In vortex_error, give Tornado cards a partial TxReset on
      maxCollisions (David Hinds).  Defined MAX_COLLISION_RESET for this.
    - Redid some driver flags and device names based on pcmcia_cs-3.1.20.
    - Fixed a bug where, if vp->tx_full is set when the interface
      is downed, it remains set when the interface is upped.  Bad
      things happen.

   LK1.1.10 17 Sep 2000 andrewm
    - Added EEPROM_8BIT for 3c555 (Fred Maciel)
    - Added experimental support for the 3c556B Laptop Hurricane (Louis Gerbarg)
    - Add HAS_NWAY to "3c900 Cyclone 10Mbps TPO"

   LK1.1.11 13 Nov 2000 andrewm
    - Dump MOD_INC/DEC_USE_COUNT, use SET_MODULE_OWNER

   LK1.1.12 1 Jan 2001 andrewm (2.4.0-pre1)
    - Call pci_enable_device before we request our IRQ (Tobias Ringstrom)
    - Add 3c590 PCI latency timer hack to vortex_probe1 (from 0.99Ra)
    - Added extended issue_and_wait for the 3c905CX.
    - Look for an MII on PHY index 24 first (3c905CX oddity).
    - Add HAS_NWAY to 3cSOHO100-TX (Brett Frankenberger)
    - Don't free skbs we don't own on oom path in vortex_open().

   LK1.1.13 27 Jan 2001
    - Added explicit `medialock' flag so we can truly
      lock the media type down with `options'.
    - "check ioremap return and some tidbits" (Arnaldo Carvalho de Melo <acme@conectiva.com.br>)
    - Added and used EEPROM_NORESET for 3c556B PM resumes.
    - Fixed leakage of vp->rx_ring.
    - Break out separate HAS_HWCKSM device capability flag.
    - Kill vp->tx_full (ANK)
    - Merge zerocopy fragment handling (ANK?)

   LK1.1.14 15 Feb 2001
    - Enable WOL.  Can be turned on with `enable_wol' module option.
    - EISA and PCI initialisation fixes (jgarzik, Manfred Spraul)
    - If a device's internalconfig register reports it has NWAY,
      use it, even if autoselect is enabled.

   LK1.1.15 6 June 2001 akpm
    - Prevent double counting of received bytes (Lars Christensen)
    - Add ethtool support (jgarzik)
    - Add module parm descriptions (Andrzej M. Krzysztofowicz)
    - Implemented alloc_etherdev() API
    - Special-case the 'Tx error 82' message.

   LK1.1.16 18 July 2001 akpm
    - Make NETIF_F_SG dependent upon nr_free_highpages(), not on CONFIG_HIGHMEM
    - Lessen verbosity of bootup messages
    - Fix WOL - use new PM API functions.
    - Use netif_running() instead of vp->open in suspend/resume.
    - Don't reset the interface logic on open/close/rmmod.  It upsets
      autonegotiation, and hence DHCP (from 0.99T).
    - Back out EEPROM_NORESET flag because of the above (we do it for all
      NICs).
    - Correct 3c982 identification string
    - Rename wait_for_completion() to issue_and_wait() to avoid completion.h
      clash.


=====================================================================================
                          PORTING TO RT-LINUX                       
=====================================================================================

   LK1.1.17 14 March 2003 
            Sergio Perez Alcañiz <serpeal@disca.upv.es>  
            Departamento de Informática de Sistemas y Computadores          
            Universidad Politécnica de Valencia                             
            Valencia (Spain)                      

	    The RTL-lwIP project has been supported by the Spanish Government Research    
	    Office (CICYT) under grant TIC2002-04123-C03-03                          
	    SISTEMAS DE TIEMPO REAL EMPOTRADOS, FIABLES Y DISTRIBUIDOS BASADOS EN 
	    COMPONENTES

	    Porting the driver to RT-Linux, that is:
	    -Giving to the driver the RT-Linux driver architecture (open, close, read,
	     write, ioctl).
	    -Using my own buffer management.
	    -No NIC stats allowed.
	    -Some calls of pci stuff have been implemented.
	    -Changing the interrupts management and the handler phylosophy.
	    -Changing the "receive" and "send" phylosophy.
	    -Introducing signaling.
	    -Registering threads that want to be notified of the arrival of a packet.
	    -IP filtering.
	    -Ethernet filtering.
	    ­Some functions added in order to ask the driver for some NIC info (such as
	     the MAC address).
	     - ....

	     The porting is just for the 3Com905C-X card. So no other cards are supported
	     (as far as I know...). 
	     
	     Just take a look at it.
=====================================================================================

    - see Documentation/networking/vortex.txt
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/in.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/mii.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/highmem.h>
#include <asm/irq.h>			/* For NR_IRQS only. */
#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <rtl_sched.h>
#include <signal.h>
#include <time.h>

#define DRV_NAME	"3c59x"
#define DRV_VERSION	"0.1"
#define DRV_RELDATE	"27 december 2002"

#define MAX_THREADS 10

#define COM3_VENDOR_ID 0x10B7
#define COM3_DEVICE_ID 0X9200

#define BFEXT(value, offset, bitcount)  \
    ((((unsigned long)(value)) >> (offset)) & ((1 << (bitcount)) - 1))

#define BFINS(lhs, rhs, offset, bitcount)					\
	(((lhs) & ~((((1 << (bitcount)) - 1)) << (offset))) |	\
	(((rhs) & ((1 << (bitcount)) - 1)) << (offset)))

#define RAM_SIZE(v)		BFEXT(v, 0, 3)
#define RAM_WIDTH(v)	BFEXT(v, 3, 1)
#define RAM_SPEED(v)	BFEXT(v, 4, 2)
#define ROM_SIZE(v)		BFEXT(v, 6, 2)
#define RAM_SPLIT(v)	BFEXT(v, 16, 2)
#define XCVR(v)			BFEXT(v, 20, 4)
#define AUTOSELECT(v)	BFEXT(v, 24, 1)

/* Operational definitions.
   These are not used by other compilation units and thus are not
   exported in a ".h" file.

   First the windows.  There are eight register windows, with the command
   and status registers available in each.
   */
#define EL3WINDOW(win_num) outw(SelectWindow + (win_num), ioaddr + EL3_CMD)
#define EL3_CMD 0x0e
#define EL3_STATUS 0x0e

/* A few values that may be tweaked. */
/* Keep the ring sizes a power of two for efficiency. */
#define TX_RING_SIZE	16
#define RX_RING_SIZE	32
#define PKT_BUF_SZ		1536			/* Size of each temporary Rx buffer.*/
#define MAX_ADDR_LEN	8		/* Largest hardware address length */

/* The top five bits written to EL3_CMD are a command, the lower
   11 bits are the parameter, if applicable.
   Note that 11 parameters bits was fine for ethernet, but the new chip
   can handle FDDI length frames (~4500 octets) and now parameters count
   32-bit 'Dwords' rather than octets. */

enum vortex_cmd {
	TotalReset = 0<<11, SelectWindow = 1<<11, StartCoax = 2<<11,
	RxDisable = 3<<11, RxEnable = 4<<11, RxReset = 5<<11,
	UpStall = 6<<11, UpUnstall = (6<<11)+1,
	DownStall = (6<<11)+2, DownUnstall = (6<<11)+3,
	RxDiscard = 8<<11, TxEnable = 9<<11, TxDisable = 10<<11, TxReset = 11<<11,
	FakeIntr = 12<<11, AckIntr = 13<<11, SetIntrEnb = 14<<11,
	SetStatusEnb = 15<<11, SetRxFilter = 16<<11, SetRxThreshold = 17<<11,
	SetTxThreshold = 18<<11, SetTxStart = 19<<11,
	StartDMAUp = 20<<11, StartDMADown = (20<<11)+1, StatsEnable = 21<<11,
	StatsDisable = 22<<11, StopCoax = 23<<11, SetFilterBit = 25<<11,};

/* The SetRxFilter command accepts the following classes: */
enum RxFilter {
	RxStation = 1, RxMulticast = 2, RxBroadcast = 4, RxProm = 8 };

/* Bits in the general status register. */
enum vortex_status {
	IntLatch = 0x0001, HostError = 0x0002, TxComplete = 0x0004,
	TxAvailable = 0x0008, RxComplete = 0x0010, RxEarly = 0x0020,
	IntReq = 0x0040, StatsFull = 0x0080,
	DMADone = 1<<8, DownComplete = 1<<9, UpComplete = 1<<10,
	DMAInProgress = 1<<11,			/* DMA controller is still busy.*/
	CmdInProgress = 1<<12,			/* EL3_CMD is still busy.*/
};

/* Register window 1 offsets, the window used in normal operation.
   On the Vortex this window is always mapped at offsets 0x10-0x1f. */
enum Window1 {
	TX_FIFO = 0x10,  RX_FIFO = 0x10,  RxErrors = 0x14,
	RxStatus = 0x18,  Timer=0x1A, TxStatus = 0x1B,
	TxFree = 0x1C, /* Remaining free bytes in Tx buffer. */
};
enum Window0 {
	Wn0EepromCmd = 10,		/* Window 0: EEPROM command register. */
	Wn0EepromData = 12,		/* Window 0: EEPROM results register. */
	IntrStatus=0x0E,		/* Valid in all windows. */
};
enum Win0_EEPROM_bits {
	EEPROM_Read = 0x80, EEPROM_WRITE = 0x40, EEPROM_ERASE = 0xC0,
	EEPROM_EWENB = 0x30,		/* Enable erasing/writing for 10 msec. */
	EEPROM_EWDIS = 0x00,		/* Disable EWENB before 10 msec timeout. */
};
/* EEPROM locations. */
enum eeprom_offset {
	PhysAddr01=0, PhysAddr23=1, PhysAddr45=2, ModelID=3,
	EtherLink3ID=7, IFXcvrIO=8, IRQLine=9,
	NodeAddr01=10, NodeAddr23=11, NodeAddr45=12,
	DriverTune=13, Checksum=15};

enum Window2 {			/* Window 2. */
	Wn2_ResetOptions=12,
};
enum Window3 {			/* Window 3: MAC/config bits. */
	Wn3_Config=0, Wn3_MAC_Ctrl=6, Wn3_Options=8,
};

enum Window4 {		/* Window 4: Xcvr/media bits. */
	Wn4_FIFODiag = 4, Wn4_NetDiag = 6, Wn4_PhysicalMgmt=8, Wn4_Media = 10,
};
enum Win4_Media_bits {
	Media_SQE = 0x0008,		/* Enable SQE error counting for AUI. */
	Media_10TP = 0x00C0,	/* Enable link beat and jabber for 10baseT. */
	Media_Lnk = 0x0080,		/* Enable just link beat for 100TX/100FX. */
	Media_LnkBeat = 0x0800,
};

enum Window7 {					/* Window 7: Bus Master control. */
	Wn7_MasterAddr = 0, Wn7_MasterLen = 6, Wn7_MasterStatus = 12,
};
/* Boomerang bus master control registers. */
enum MasterCtrl {
	PktStatus = 0x20, DownListPtr = 0x24, FragAddr = 0x28, FragLen = 0x2c,
	TxFreeThreshold = 0x2f, UpPktStatus = 0x30, UpListPtr = 0x38,
};

/* The Rx and Tx descriptor lists.
   Caution Alpha hackers: these types are 32 bits!  Note also the 8 byte
   alignment contraint on tx_ring[] and rx_ring[]. */
#define LAST_FRAG 	0x80000000			/* Last Addr/Len pair in descriptor. */
#define DN_COMPLETE	0x00010000			/* This packet has been downloaded */

/* Values for the Rx status entry. */
enum rx_desc_status {
	RxDComplete=0x00008000, RxDError=0x4000,
	/* See boomerang_rx() for actual error bits */
	IPChksumErr=1<<25, TCPChksumErr=1<<26, UDPChksumErr=1<<27,
	IPChksumValid=1<<29, TCPChksumValid=1<<30, UDPChksumValid=1<<31,
};

/* Values for the Tx status entry. */
enum tx_desc_status {
	CRCDisable=0x2000, TxDComplete=0x8000,
	AddIPChksum=0x02000000, AddTCPChksum=0x04000000, AddUDPChksum=0x08000000,
	TxIntrUploaded=0x80000000,		/* IRQ when in FIFO, but maybe not sent. */
};

/* The action to take with a media selection timer tick.
   Note that we deviate from the 3Com order by checking 10base2 before AUI.
 */
enum xcvr_types {
	XCVR_10baseT=0, XCVR_AUI, XCVR_10baseTOnly, XCVR_10base2, XCVR_100baseTx,
	XCVR_100baseFx, XCVR_MII=6, XCVR_NWAY=8, XCVR_ExtMII=9, XCVR_Default=10,
};

/* Chip features we care about in vp->capabilities, read from the EEPROM. */
enum ChipCaps { CapBusMaster=0x20, CapPwrMgmt=0x2000 };


/* This table drives the PCI probe routines.  It's mostly boilerplate in all
   of the drivers, and will likely be provided by some future kernel.
*/
enum pci_flags_bit {
	PCI_USES_IO=1, PCI_USES_MEM=2, PCI_USES_MASTER=4,
	PCI_ADDR0=0x10<<0, PCI_ADDR1=0x10<<1, PCI_ADDR2=0x10<<2, PCI_ADDR3=0x10<<3,
};

enum {	IS_VORTEX=1, IS_BOOMERANG=2, IS_CYCLONE=4, IS_TORNADO=8,
	EEPROM_8BIT=0x10,	/* AKPM: Uses 0x230 as the base bitmaps for EEPROM reads */
	HAS_PWR_CTRL=0x20, HAS_MII=0x40, HAS_NWAY=0x80, HAS_CB_FNS=0x100,
	INVERT_MII_PWR=0x200, INVERT_LED_PWR=0x400, MAX_COLLISION_RESET=0x800,
	EEPROM_OFFSET=0x1000, HAS_HWCKSM=0x2000 };

#define DO_ZEROCOPY 0

struct boom_rx_desc {
	u32 next;					/* Last entry points to 0.   */
	s32 status;
	u32 addr;					/* Up to 63 addr/len pairs possible. */
	s32 length;					/* Set LAST_FRAG to indicate last pair. */
};

struct boom_tx_desc {
  u32 next;					/* Last entry points to 0.   */
  s32 status;					/* bits 0:12 length, others see below.  */
  u32 addr;
  s32 length;
};

struct vortex_private {
  /* The Rx and Tx rings should be quad-word-aligned. */
  struct boom_rx_desc* rx_ring;
  struct boom_tx_desc* tx_ring;
  dma_addr_t rx_ring_dma;
  dma_addr_t tx_ring_dma;
  /* The addresses of transmit- and receive-in-place skbuffs. */
  unsigned char *rx_skbuff[RX_RING_SIZE];
  unsigned char *tx_skbuff[TX_RING_SIZE];
  struct net_device *next_module;		/* NULL if PCI device */
  unsigned int cur_rx, cur_tx;		/* The next free ring entry */
  unsigned int dirty_rx, dirty_tx;	/* The ring entries to be free()ed. */
  struct net_device_stats stats;
  struct sk_buff *tx_skb;				/* Packet being eaten by bus master ctrl.  */
  dma_addr_t tx_skb_dma;				/* Allocated DMA address for bus master ctrl DMA.   */
  /* PCI configuration space information. */
  struct pci_dev *pdev;
  char *cb_fn_base;					/* CardBus function status addr space. */
  
  /* Some values here only for performance evaluation and path-coverage */
  int rx_nocopy, rx_copy, queued_packet, rx_csumhits;
  int card_idx;
  
  int options;						/* User-settable misc. driver options. */
  unsigned int media_override:4, 		/* Passed-in media type. */
    default_media:4,				/* Read from the EEPROM/Wn3_Config. */
    full_duplex:1, 
    force_fd:1, 
    autoselect:1,
    bus_master:1,					/* Vortex can only do a fragment bus-m. */
    full_bus_master_tx:1, full_bus_master_rx:2, /* Boomerang  */
    flow_ctrl:1,					/* Use 802.3x flow control (PAUSE only) */
    partner_flow_ctrl:1,			/* Partner supports flow control */
    has_nway:1,
    enable_wol:1,					/* Wake-on-LAN is enabled */
    pm_state_valid:1,				/* power_state[] has sane contents */
    open:1,
    medialock:1,
    must_free_region:1;				/* Flag: if zero, Cardbus owns the I/O region */
  int drv_flags;
  u16 status_enable;
  u16 intr_enable;
  u16 available_media;				/* From Wn3_Options. */
  u16 capabilities, info1, info2;		/* Various, from EEPROM. */
  u16 advertising;					/* NWay media advertisement */
  unsigned char phys[2];				/* MII device addresses. */
  u16 deferred;						/* Resend these interrupts when we
							 * bale from the ISR */
  u16 io_size;						/* Size of PCI region (for release_region) */
  spinlock_t lock;					/* Serialise access to device & its vortex_private */
  spinlock_t mdio_lock;				/* Serialise access to mdio hardware */
  u32 power_state[16];
  int mtu;
  unsigned char	dev_addr[MAX_ADDR_LEN];	/* hw address	*/
  long ioaddr;
  unsigned long rx_packets;
  unsigned long rx_frames_for_us;
  unsigned char if_port;
  int must_free_irq;
};

static struct vortex_chip_info {
	const char *name;
	int flags;
	int drv_flags;
	int io_size;
} vortex_info = {"3c905C Tornado",
		     PCI_USES_IO|PCI_USES_MASTER, IS_TORNADO|HAS_NWAY|HAS_HWCKSM, 128, };

enum vortex_chips {
	CH_3C590 = 0,
	CH_3C592,
	CH_3C597,
	CH_3C595_1,
	CH_3C595_2,

	CH_3C595_3,
	CH_3C900_1,
	CH_3C900_2,
	CH_3C900_3,
	CH_3C900_4,

	CH_3C900_5,
	CH_3C900B_FL,
	CH_3C905_1,
	CH_3C905_2,
	CH_3C905B_1,

	CH_3C905B_2,
	CH_3C905B_FX,
	CH_3C905C,
	CH_3C980,
	CH_3C9805,

	CH_3CSOHO100_TX,
	CH_3C555,
	CH_3C556,
	CH_3C556B,
	CH_3C575,

	CH_3C575_1,
	CH_3CCFE575,
	CH_3CCFE575CT,
	CH_3CCFE656,
	CH_3CCFEM656,

	CH_3CCFEM656_1,
	CH_3C450,
};

static struct media_table {
	char *name;
	unsigned int media_bits:16,		/* Bits to set in Wn4_Media register. */
		mask:8,				/* The transceiver-present bit in Wn3_Config.*/
		next:8;				/* The media type to try next. */
	int wait;				/* Time before we check media status. */
} media_tbl[] = {
  { "10baseT",   Media_10TP,0x08, XCVR_10base2, (14*HZ)/10},
  { "10Mbs AUI", Media_SQE, 0x20, XCVR_Default, (1*HZ)/10},
  { "undefined", 0,			0x80, XCVR_10baseT, 10000},
  { "10base2",   0,			0x10, XCVR_AUI,		(1*HZ)/10},
  { "100baseTX", Media_Lnk, 0x02, XCVR_100baseFx, (14*HZ)/10},
  { "100baseFX", Media_Lnk, 0x04, XCVR_MII,		(14*HZ)/10},
  { "MII",		 0,			0x41, XCVR_10baseT, 3*HZ },
  { "undefined", 0,			0x01, XCVR_10baseT, 10000},
  { "Autonegotiate", 0,		0x41, XCVR_10baseT, 3*HZ},
  { "MII-External",	 0,		0x41, XCVR_10baseT, 3*HZ },
  { "Default",	 0,			0xFF, XCVR_10baseT, 10000},
};

static char mii_preamble_required;
static const int mtu = 1500;
/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int max_interrupt_work = 32;

/* "Knobs" that adjust features and parameters. */
/* Set the copy breakpoint for the copy-only-tiny-frames scheme.
   Setting to > 1512 effectively disables this feature. */
#ifndef __arm__
static const int rx_copybreak = 200;
#else
/* ARM systems perform better by disregarding the bus-master
   transfer capability of these cards. -- rmk */
static const int rx_copybreak = 1513;
#endif

struct pci_dev *rtl_3COM905C_init_device(void);
int rtl_3COM905C_start_up_device(struct pci_dev *dev);
static void rtl_3COM905C_mdio_write(int phy_id, int location, int value);
static int rtl_3COM905C_mdio_read(int phy_id, int location);
static void rtl_3COM905C_mdio_sync(long ioaddr, int bits);
static int vortex_open(struct pci_dev *dev);
static void vortex_up(struct pci_dev *dev);
static void rtl_3COM905C_issue_and_wait(int cmd);
static void rtl_3COM905C_set_rx_mode(void);
static void rtl_3COM905C_acpi_set_WOL(void);
static int boomerang_rx(struct pci_dev *dev);
static void vortex_down(struct pci_dev *dev);
static void vortex_error(struct pci_dev *dev, int status);
static int vortex_close(struct pci_dev *dev);
static void vortex_remove_one (struct pci_dev *pdev);
static int rt_3c905c_send_packet(const char *buffer, size_t size);
void rt_3c905c_send_signal(void);

#define PCI_DEBUG 0
#define EEPROM_CONTENTS_DEBUG 0
#define MAC_ADDRESS_DEBUG 0
#define FUNCTION_CALL_DEBUG 0
#define INITIALIZATION_DEBUG 0
#define RECEIVE_DEBUG 0
#define PACKET_DATA_DEBUG 0
#define INTERRUPT_DEBUG 0
#define TRANSMIT_DEBUG 0
#define ERROR_DEBUG 1

#define SIGALRM2 RTL_SIGUSR1

#define DEBUG(x,y) if(x) printk y

#include <rtl_sync.h>
#include <rtl_core.h>
#include <rtl_printf.h>
#include <time.h>
#include <asm/io.h>
#include <rtl_posixio.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <rtl.h>
#include "DIDMA.h"
#include "memcopy.h"

static int rtl_3COM905C_open (struct rtl_file *filp);
static int rtl_3COM905C_release (struct rtl_file *filp);
static ssize_t rtl_3COM905C_write(struct rtl_file *filp, const char *buf, size_t count, loff_t* ppos);
static int rtl_3COM905C_ioctl(struct rtl_file * filp, unsigned int request, unsigned long other);
static ssize_t rtl_3COM905C_read(struct rtl_file *filp, char *buf, size_t count, loff_t* ppos);






