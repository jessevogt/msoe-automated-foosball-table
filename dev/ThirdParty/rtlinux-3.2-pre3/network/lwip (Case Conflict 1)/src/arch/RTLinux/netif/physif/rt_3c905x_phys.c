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

	    The RTL-lwIP project has been supported by the Spanish Government Research    
	    Office (CICYT) under grant TIC2002-04123-C03-03 
	    SISTEMAS DE TIEMPO REAL EMPOTRADOS, FIABLES Y DISTRIBUIDOS BASADOS EN 
	    COMPONENTES

          Valencia (Spain)                      

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
	     - ....

	     The porting is just for the 3Com905C-X card. So no other cards are supported
	     (as far as I know...). 
	     
	     Just take a look at it.
=====================================================================================

    - see Documentation/networking/vortex.txt
*/

#include "rt_3c905x_phys.h"
#include "rt_pci.h"
#include "netif/net_policy/FIFO_policy.h"

static struct net_policy_operations rt_3c905x_policy = {
  FIFO_add_frame_to_buffer,
  FIFO_extract_frame_of_buffer,
  FIFO_initialize_rx_buffer,
  FIFO_dealloc_rx_buffer,
};

typedef struct rtl_ethernetif_thread{
  pthread_t pthread;
  void (* function)(void);
  unsigned char registered;
} com3_thread_t;

#define COM3_905C_MAJOR 204
#define COM3_905C_NAME "eth"
#define COM3_SIGNAL RTL_SIGUSR2

static int inside_the_interrupt_handler = 0, rt_3c905c_trying_to_close = 0;
static int interrupted = 0, writting = 0;

static struct rtl_file_operations rtl_3COM905C_fops = {
       	NULL, 
	rtl_3COM905C_read,
	rtl_3COM905C_write,
	rtl_3COM905C_ioctl,
	NULL,
	rtl_3COM905C_open,
	rtl_3COM905C_release
};

struct pci_dev *com3dev;
static struct vortex_private vp;
static unsigned char rt_3c905c_registered = 0x00, rt_3c905c_opened = 0x00;
com3_thread_t *threads_vector[MAX_THREADS];
static int rt_3c905c_current_thread = 0;
unsigned char rt_3c905c_ip_addr[2][4]={{0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00}};
static int rt_3c905c_n_filters = 0;

/*************************************************************************************/
/* This function is used to set an IP filter to the incoming packets. It is accessed */
/* by means of ioctl. Only two IP filters are allowed.                               */  
/*************************************************************************************/
int rt_3c905c_set_ip_filter(unsigned long ipaddr){
  if(rt_3c905c_n_filters<=1){
    rt_3c905c_ip_addr[rt_3c905c_n_filters][0]=ipaddr & 0x000000ff; 
    rt_3c905c_ip_addr[rt_3c905c_n_filters][1]= (ipaddr >> 8) & 0x000000ff; 
    rt_3c905c_ip_addr[rt_3c905c_n_filters][2]= (ipaddr >> 16) & 0x000000ff; 
    rt_3c905c_ip_addr[rt_3c905c_n_filters][3]= (ipaddr >> 24) & 0x000000ff; 
    rt_3c905c_n_filters++;
    return 0;
  } 

  rtl_printf("You cannot set more than 2 IP filters !!");
  return -1;
}

/***************************************************************************************/
/* This function is used to register a thread that wants to be signaled when an        */
/* incoming packet arrives. It registers the thread and a function that the thread     */
/* wants to be executed just after being signaled. That function should be an          */
/* atomic_increment function. That is necessary because the thread could be signaled   */
/* while it is executing the signal hander. Inside the signal handler, the signals are */
/* masked, so no signal would be received. The atomic_increment function should be     */
/* used to increment a pendent_signals variable.                                       */
/* It is accessed by means of ioctl.                                                   */
/***************************************************************************************/
int rt_3c905c_register_thread(com3_thread_t *thread)
{
  if(( (pthread_t) thread->pthread != NULL) && (rt_3c905c_current_thread < MAX_THREADS)){
    threads_vector[rt_3c905c_current_thread] = thread;
    thread->registered = 0x01;
    rt_3c905c_current_thread++;
    return 0;
  }else
    return -1;
}

/***************************************************************************************/
/* This function is used to unregister a thread previously registered. It is important */
/* to unregister the thread, otherwise the system could chrash. Think in this context: */
/* the module containing the thread and the function that it registers in this driver  */
/* is removed (rmmod thread_module); then, an incoming packet arrives; if the thread   */
/* has not been unregistered, that thread will be signaled and the function that it    */
/* registered called. Since that function has been removed, the call would point to a  */
/* memory area where there's nothing, and that would crash the system.                 */
/* It is accessed by means of ioctl.                                                   */ 
/***************************************************************************************/
void rt_3c905c_unregister_thread(com3_thread_t *thread)
{
  int i;
  rtl_irqstate_t state;

  rtl_no_interrupts(state);

  for(i=0; i< MAX_THREADS; i++)
    if(threads_vector[i]->pthread == thread->pthread){
      threads_vector[i]->registered = 0x00;
      break;
    }
  
  rtl_restore_interrupts(state);
}

/***************************************************************************************/
/* This function is used to send a signal to those threads registered in the driver.   */
/* After signaling each thread, it calls to the functions registered by means of       */
/* rt_3c905c_register_thread. This function is called each time an incoming packet     */
/* for us arrives. It is called inside the interrupt handler.                          */
/***************************************************************************************/
void rt_3c905c_send_signal(void){
  int i;
  for(i=0; i<rt_3c905c_current_thread; i++){
    if(threads_vector[i]->registered == 0x01){
      pthread_kill(threads_vector[i]->pthread,COM3_SIGNAL);
      threads_vector[i]->function();
    }else
      rtl_printf("Not sending signal cause thread has deregistered\n");
  }
}

/***************************************************************************************/
/* This function is used to get the MAC address of the ethernet card. It is accessed   */
/* by means of ioctl.                                                                  */
/***************************************************************************************/
int rt_3c905c_obtain_mac_address(unsigned char *mac){
  int i;
  for(i=0; i<6; i++)
    mac[i]=vp.dev_addr[i];
  return 0;
}

/***************************************************************************************/
/* This function implements the read call. Makes buf to point to an internal buffer    */
/* and returns the lenght of that buffer. The packet returned depends on the policy    */
/* selected.                                                                           */
/***************************************************************************************/
static ssize_t rtl_3COM905C_read(struct rtl_file *filp, char *buf, size_t count, loff_t* ppos){
  return rt_3c905x_policy.extract_frame_of_buffer(buf);
}

/***************************************************************************************/
/* This function implements the ioctl call.                                            */
/***************************************************************************************/
static int rtl_3COM905C_ioctl(struct rtl_file * filp, unsigned int request, unsigned long other){

  switch(request) {
  case 1:           /* set_ip_filter */
    rt_3c905c_set_ip_filter(other);
    break;
  case 2:           /* obtain_mac_address */
    rt_3c905c_obtain_mac_address((unsigned char *)other);
    break;
  case 3:           /* register thread */
    rt_3c905c_register_thread((com3_thread_t *)other);
    break;
  case 4:           /* unregister thread */
    rt_3c905c_unregister_thread((com3_thread_t *)other);
    break;
  }
  return 0;
}

/***************************************************************************************/
/* This function implements the close call. It stalls the card and frees resources.    */
/***************************************************************************************/
static int rtl_3COM905C_release (struct rtl_file *filp){

    rtl_irqstate_t state;
    rtl_no_interrupts(state);

    vortex_close(com3dev);

    vortex_remove_one(com3dev);

    if(rt_3c905c_opened == 0x01){
      rt_3c905x_policy.dealloc_rx_buffer();
    }

    rt_3c905c_opened = 0x00;
    rtl_restore_interrupts(state);
    return 0;
}

/***************************************************************************************/
/* This function implements the write call. It is not a blocking function, that means  */
/* that writting returns inmediatly, it doesn't waits till the packet has been sent.   */
/* The return value is always the size of the buffer being sent.                       */
/***************************************************************************************/
static ssize_t rtl_3COM905C_write(struct rtl_file *filp, const char *buf, size_t count, loff_t* ppos)
{
  ssize_t tmp; 
  rtl_irqstate_t state;

  rtl_no_interrupts(state);
  tmp=rt_3c905c_send_packet(buf,count);
  rtl_restore_interrupts(state);
  return tmp;
}

/***************************************************************************************/
/* This function implements the open call. It initialises the card. The card starts    */
/* receiving packets.                                                                  */
/***************************************************************************************/
static int rtl_3COM905C_open (struct rtl_file *filp){

  rtl_3COM905C_release (filp);  
  
  if(rt_3c905c_opened == 0x00){
    if((com3dev = rtl_3COM905C_init_device())!=NULL){
      rtl_3COM905C_start_up_device(com3dev);
      rt_3c905c_opened = 0x01;
      return COM3_905C_MAJOR;
    }else{
      rtl_printf("ERROR: Couldn't initialize device\n");
      return -1;
    }
  }else{
    rtl_printf("Device is already opened\n");
    return -1;
  }
}

/***************************************************************************************/
/* This function is used to initialise the pci and the buffering subsystem.            */
/***************************************************************************************/
struct pci_dev *rtl_3COM905C_init_device(void){
  struct pci_dev *dev;

  /* First of all, we must get a pointer to the pci_dev structure */
  if((dev = rt_pci_find_device(COM3_VENDOR_ID, COM3_DEVICE_ID, NULL))== NULL)
    return NULL;

  rt_3c905x_policy.initialize_rx_buffer();

  /* Let's enable the device */
  if (rt_pci_enable_device(dev)){
    rtl_printf("PCI ERROR: Can't enable device\n");
    return NULL;
  }

  return dev;
}

/***************************************************************************************/
/* This function is called when the driver module is inserted. It registers the device */
/* , letting other modules to use it by means of the calls read, write, close, open    */
/* and ioctl.                                                                          */
/***************************************************************************************/
int init_module(void){
  printk("\n\n\nRT-Linux driver for the Ethernet Card 3Com905c-x being loaded\n\n\n");

  if (rtl_register_rtldev (COM3_905C_MAJOR, COM3_905C_NAME, &rtl_3COM905C_fops)) {
    printk ("RTLinux /dev/%s: unable to get RTLinux major %d\n", COM3_905C_NAME, COM3_905C_MAJOR);
    return -EIO;
  }else{
    printk("Registered device /dev/%s major number %d\n",COM3_905C_NAME, COM3_905C_MAJOR);
    rt_3c905c_registered = 0x01;
    return 0;
  }
}

/***************************************************************************************/
/* This function is called when removing the driver module. It makes sure that there   */
/* are no pending executions of the interrupt handler. It releases the ethernet card   */
/* and frees all resources. The driver is unregistered.                                */
/***************************************************************************************/
void cleanup_module(void){
  int inside = 1;
  rtl_irqstate_t state;

  rt_3c905c_trying_to_close = 1;

  /* Since inside the interrupt handler there's a call to the scheduler, there may  */
  /* be an execution of the interrupt handler that hasn't been completely executed. */
  /* The card cannot be released in that case, so we must be sure that there is no  */
  /* interrupt handler execution pending. Otherwise, that may crash the system.     */
  while(inside){
    rtl_no_interrupts(state);      

    if(inside_the_interrupt_handler){
      rtl_restore_interrupts(state);
      usleep(10);
    }else{
      rtl_hard_disable_irq(vp.pdev->irq);        
      rtl_restore_interrupts(state);
      inside = 0;
    }
  }

  rtl_3COM905C_issue_and_wait(UpStall);
  rtl_3COM905C_issue_and_wait(DownStall);

  if(rt_3c905c_registered == 0x01){
    printk("Unregistering device /dev/%s\n",COM3_905C_NAME);
    rtl_unregister_rtldev(COM3_905C_MAJOR,COM3_905C_NAME);
  }

  printk("\n\n\nRT-Linux driver for the Ethernet Card 3Com905c-x being removed\n\n\n");
}

/***************************************************************************************/
/* This is the interrupt handler. It is executed when the ethernet card raises an      */
/* interrupt and interrupts are enabled. In the initialisation phase we've forced the  */
/* card to only generate the UpComplete interrupt, that is, when a full incoming       */
/* packet has been processed.                                                          */
/***************************************************************************************/
unsigned int boomerang_interrupt(unsigned int irq , struct pt_regs *regs){
  long ioaddr = vp.ioaddr;
  int status = inw(ioaddr + EL3_STATUS);
  int work_done = max_interrupt_work;
  struct pci_dev *dev = com3dev;

  inside_the_interrupt_handler = 1;
  interrupted++;
  if(writting & interrupted)
    rtl_printf("I've been interrupted %d times\n",interrupted);

  if (status & UpComplete) {
    vp.rx_packets++;    
    rtl_3COM905C_issue_and_wait(UpStall);
    while(vp.rx_ring[vp.cur_rx % RX_RING_SIZE].status & 0x00008000){
      boomerang_rx(dev);
      outw(AckIntr | UpComplete, ioaddr + EL3_CMD);
      if (--work_done < 0)
	break;
    }
    outw(UpUnstall, ioaddr + EL3_CMD);
  }
  
  if (status & DownComplete){
    rtl_printf("Se acaba de enviar el paquete\n");
    outw(AckIntr | DownComplete, ioaddr + EL3_CMD);
  }
  
  /* Check for all uncommon interrupts at once. */
  if (status & (HostError | RxEarly | StatsFull | IntReq)){
    rtl_printf("VORTEX_ERROR\n");
    vortex_error(dev, status);
  }
  
  /* Acknowledge the IRQ. */
  outw(AckIntr | IntReq | IntLatch, ioaddr + EL3_CMD);
  if (vp.cb_fn_base){	
    writel(0x8000, vp.cb_fn_base + 4);
  }
  
  /* We must be sure that we're out of the interrupt handler before cleanup_modules */
  /* is executed. If cleanup_modules is being executed, we don't have to enable the */
  /* irq. If enabled, then the system could crash.                                  */
  if(!rt_3c905c_trying_to_close)
    rtl_hard_enable_irq(vp.pdev->irq);  
  
  interrupted--;

  return (inside_the_interrupt_handler = 0);
}

/***************************************************************************************/
/* This is the function called when an incoming packet has succesfuly arrived, so we   */
/* have to copy it into internal buffers.                                              */
/***************************************************************************************/
static int boomerang_rx(struct pci_dev *dev)
{
  int entry = vp.cur_rx % RX_RING_SIZE;
  int rx_status;
  int rx_work_limit = vp.dirty_rx + RX_RING_SIZE - vp.cur_rx;
  int i,j;
  unsigned char *buffer;
  int temp = vp.rx_ring[entry].status & 0x1fff;
  unsigned char mine = 0x01, multicast_packet= 0x01, arp_request_for_me = 0x01 ;

  buffer =vp.rx_skbuff[entry];

  /* A NIC receives all the packets in the LAN so we will receive an interrupt for each one of those. */
  /* As we only want those packets sent to us we must filter them. So, we will only receive packets   */
  /* directly sent us (i.e. destination address is ours) and those ARP frames which ask for our MAC.  */
  /* An ARP improvement consist on receive all ARP request packets in the LAN, so, at least, we could */
  /* know the pair IP and MAC address of those computers performing the request. As most of the       */
  /* frames in a LAN are ARP request frames the overhead produced by receiving all of them would be   */
  /* considerable, so we won't bother with this improvement.                                          */

  //Is this frame for us??
  for(i=0; i<6; i++){
    if(buffer[i] == vp.dev_addr[i])
      continue;
    else{
      mine = 0x00;
      break;
    }
  }

  if(mine == 0x01) goto accept_frame;

  // Is an ARP frame???
  if((buffer[12]==0x08) && (buffer[13]==0x06)){
    // It asks for my IP??
    for(j=0; j<rt_3c905c_n_filters; j++){
      for(i=0; i<4;i++){
	if(buffer[38+i]==rt_3c905c_ip_addr[j][i])
	  continue;
	else{
	  arp_request_for_me = 0x00;   
	  break;
	}
      }
    }
  }else
    arp_request_for_me = 0x00;   

  // Is it a multicast frame??
  for(i=0; i<6; i++){
    if(buffer[i] == 0xff)
      continue;
    else{
      multicast_packet = 0x00;
      break;
    }
  }
  
 accept_frame:

  if((mine == 0x01) || ((multicast_packet==0x01) && (arp_request_for_me==0x01))){
    vp.rx_frames_for_us++;
    if(rt_3c905x_policy.add_frame_to_buffer(buffer,temp)== 0){
      rt_3c905c_send_signal();
      rtl_schedule(); 
    }
  }

  while ((rx_status = le32_to_cpu(vp.rx_ring[entry].status)) & RxDComplete){

    if (--rx_work_limit < 0)
      break;
    if (!(rx_status & RxDError))  /* Error, update stats. */
      vp.stats.rx_packets++;
  }

  entry = (++vp.cur_rx) % RX_RING_SIZE;

  /* Refill the Rx ring buffers. */
  for (; vp.cur_rx - vp.dirty_rx > 0; vp.dirty_rx++) {
    entry = vp.dirty_rx % RX_RING_SIZE;
    vp.rx_ring[entry].status = 0;	/* Clear complete bit. */
  }
  return 0;
}

/***************************************************************************************/
/* This function does internal initialisations of the card. It writes into internal    */
/* registers and checks the card capabilities.                                         */
/***************************************************************************************/
int rtl_3COM905C_start_up_device(struct pci_dev *dev){
  struct vortex_chip_info * const vci = &vortex_info;
  unsigned int eeprom[0x40], checksum = 0;		/* EEPROM contents */
  char *print_name;
  int retval;
  long ioaddr;
  int i,step;

  print_name = dev ? dev->slot_name : "3c59x";

  ioaddr = rt_pci_resource_start(dev, 0);

  vp.drv_flags = vci->drv_flags;
  vp.has_nway = (vci->drv_flags & HAS_NWAY) ? 1 : 0;
  vp.io_size = vci->io_size;
  vp.card_idx = 0;
  vp.ioaddr = ioaddr;
  vp.media_override = 7;
  vp.mtu = mtu;

  print_name = dev ? dev->slot_name : "3c59x";

  /* PCI-only startup logic */
  if (dev) {
    if (request_region(ioaddr, vci->io_size, print_name) != NULL){
      vp.must_free_region = 1;
     }

    /* enable bus-mastering if necessary */		
    if (vci->flags & PCI_USES_MASTER)
      pci_set_master (dev);

    vp.pdev = dev;

    /* Makes sure rings are at least 16 byte aligned. */
    vp.rx_ring = pci_alloc_consistent(dev, sizeof(struct boom_rx_desc) * RX_RING_SIZE, &vp.rx_ring_dma);
    vp.tx_ring = pci_alloc_consistent(dev, sizeof(struct boom_tx_desc) * TX_RING_SIZE, &vp.tx_ring_dma);

    retval = -ENOMEM;
    if ((vp.rx_ring == 0) || (vp.tx_ring == 0))
      goto free_region;
        
    EL3WINDOW(0);
    {
      int base;

      if (vci->drv_flags & EEPROM_8BIT)
	base = 0x230;
      else if (vci->drv_flags & EEPROM_OFFSET)
	base = EEPROM_Read + 0x30;
      else
	base = EEPROM_Read;

      for (i = 0; i < 0x40; i++) {
	int timer;
	
	/* This means that we want to read EepromCommand Register and disable writting */
	/* Issuing ReadRegister & WriteDisable                                         */
	outw(base + i, ioaddr + Wn0EepromCmd);

	for (timer = 10; timer >= 0; timer--) {

	  /* The read data is available through the EepromData register 162us after  */
	  /* the ReadRegister command has been issued                                */
	  rtl_delay(162000);

	  /* Bit 15th (eepromBusy) of EepromCommand register is a read-only bit asserted   */
	  /* during the execution of EEProm commands. Further commans should not be issued */
	  /* to the EepromCommand register, nor should data be read from the EepromData    */
	  /* register while this bit is true                                               */
	  if ((inw(ioaddr + Wn0EepromCmd) & 0x8000) == 0)
	    break;
	}

	/* Save the contents of the 3C90xC NIC's EEPROM     */ 
	eeprom[i] = inw(ioaddr + Wn0EepromData);
      }

    }//EL3WINDOW(0) configuration finished

    /* EEPROM can be checksummed in order to assure that reading was OK */
    for (i = 0; i < 0x18; i++)
      checksum ^= eeprom[i];
    checksum = (checksum ^ (checksum >> 8)) & 0xff;
    if (checksum != 0x00) {		/* Grrr, needless incompatible change 3Com. */
      while (i < 0x21)
	checksum ^= eeprom[i++];
      checksum = (checksum ^ (checksum >> 8)) & 0xff;
    }
    if ((checksum != 0x00) && !(vci->drv_flags & IS_TORNADO))
      printk(" ***INVALID CHECKSUM %4.4x*** ", checksum);
    
    /* Save HW address into dev_addr (MAC address in format 00:04:75:bd:ea:e7) */
    for (i = 0; i < 3; i++)
      ((u16 *)vp.dev_addr)[i] = htons(eeprom[i + 10]);
    
    /* This writes into the StationAddress register the NIC's HW address in order */
    /* to define the individual destination address that the NIC responds to when */
    /* receiving packets                                                          */
    EL3WINDOW(2);
    for (i = 0; i < 6; i++)
      outb(vp.dev_addr[i], ioaddr + i);

    EL3WINDOW(4);
    step = (inb(ioaddr + Wn4_NetDiag) & 0x1e) >> 1;

    if (dev && vci->drv_flags & HAS_CB_FNS) {
      unsigned long fn_st_addr;			/* Cardbus function status space */
      unsigned short n;
      
      fn_st_addr = pci_resource_start (dev, 2);
      if (fn_st_addr) {
	vp.cb_fn_base = ioremap(fn_st_addr, 128);
	retval = -ENOMEM;
	if (!vp.cb_fn_base)
	  goto free_ring;
      }

      EL3WINDOW(2);
      
      n = inw(ioaddr + Wn2_ResetOptions) & ~0x4010;
      if (vp.drv_flags & INVERT_LED_PWR)
	n |= 0x10;
      if (vp.drv_flags & INVERT_MII_PWR)
	n |= 0x4000;
      outw(n, ioaddr + Wn2_ResetOptions);
    }

    /* Extract our information from the EEPROM data. */
    vp.info1 = eeprom[13];
    vp.info2 = eeprom[15];
    vp.capabilities = eeprom[16];
 
    if (vp.info1 & 0x8000){
      vp.full_duplex = 1;
      printk(KERN_INFO "Full duplex capable\n");
    }

    {
      unsigned int config;
      EL3WINDOW(3);

      /* This reads the MediaOptions register which shows what physical media */
      /* connections are available in the NIC                                 */
      vp.available_media = inw(ioaddr + Wn3_Options); //Wn3_Options = 8 vp.available_media = 0xa

      if ((vp.available_media & 0xff) == 0)		/* Broken 3c916 */
	vp.available_media = 0x40;

      /* This reads the InternalConfig register which provides a way to set */
      /* NIC-specific, non-host-related configuration settings              */ 
      config = inl(ioaddr + Wn3_Config); //Wn3_Config = 0

      vp.default_media = XCVR(config);
      if (vp.default_media == XCVR_NWAY)
	vp.has_nway = 1;
      vp.autoselect = AUTOSELECT(config);
    }

    if (vp.media_override != 7) {
      vp.if_port = vp.media_override;
    } else
      vp.if_port = vp.default_media;

    if (vp.if_port == XCVR_MII || vp.if_port == XCVR_NWAY) {
      int phy, phy_idx = 0;
      EL3WINDOW(4);
      mii_preamble_required++;
      mii_preamble_required++;
      rtl_3COM905C_mdio_read(24, 1);
      for (phy = 0; phy < 32 && phy_idx < 1; phy++) {
	int mii_status, phyx;
	
	/*
	 * For the 3c905CX we look at index 24 first, because it bogusly
	 * reports an external PHY at all indices
	 */
	if (phy == 0)
	  phyx = 24;
	else if (phy <= 24)
	  phyx = phy - 1;
	else
	  phyx = phy;
	mii_status = rtl_3COM905C_mdio_read(phyx, 1);
	if (mii_status  &&  mii_status != 0xffff) {
	  vp.phys[phy_idx++] = phyx;
	  
	  if ((mii_status & 0x0040) == 0)
	    mii_preamble_required++;
	}
      }		
      mii_preamble_required--;
      if (phy_idx == 0) {
	vp.phys[0] = 24;
      } else {
	vp.advertising = rtl_3COM905C_mdio_read(vp.phys[0], 4);
	if (vp.full_duplex) {
	  /* Only advertise the FD media types. */
	  vp.advertising &= ~0x02A0;
	  rtl_3COM905C_mdio_write(vp.phys[0], 4, vp.advertising);
	}
      }
    }
    if (vp.capabilities & CapBusMaster) {
      vp.full_bus_master_tx = 1;
      
      vp.full_bus_master_rx = (vp.info2 & 1) ? 1 : 2;
      vp.bus_master = 0;		/* AKPM: vortex only */
    }
    if (vp.pdev && vp.enable_wol) {
      vp.pm_state_valid = 1;
      rt_pci_save_state(dev, vp.power_state);
      rtl_3COM905C_acpi_set_WOL();
    }
    vortex_open(dev);

  }// if(dev)
    
  return 0;

 free_ring:

  pci_free_consistent(dev, sizeof(struct boom_rx_desc) * RX_RING_SIZE, vp.rx_ring, vp.rx_ring_dma);
  pci_free_consistent(dev, sizeof(struct boom_tx_desc) * TX_RING_SIZE, vp.tx_ring, vp.tx_ring_dma);

  return -1;

 free_region:
  if (vp.must_free_region){
    release_region(ioaddr, vci->io_size);
    vp.must_free_region = 0;
  }
  printk("vortex_probe1 fails.  Returns %d\n", retval);

  return -1;
}

/***************************************************************************************/
/* ACPI: Advanced Configuration and Power Interface.                                   */
/* Set Wake-On-LAN mode and put the board into D3 (power-down) state.                  */
/***************************************************************************************/
static void rtl_3COM905C_acpi_set_WOL(void)
{
  long ioaddr = vp.ioaddr;

  /* Power up on: 1==Downloaded Filter, 2==Magic Packets, 4==Link Status. */
  EL3WINDOW(7);
  outw(2, ioaddr + 0x0c);
  /* The RxFilter must accept the WOL frames. */
  outw(SetRxFilter|RxStation|RxBroadcast, ioaddr + EL3_CMD); //RxMulticast
  outw(RxEnable, ioaddr + EL3_CMD);

  /* Change the power state to D3; RxEnable doesn't take effect. */
  rt_pci_enable_wake(vp.pdev, 0, 1);
  rt_pci_set_power_state(vp.pdev, 3);
}

/***************************************************************************************/
/* This function registers the interrupt handler and reserves memory for buffers which */
/* only will be accessed by the card.                                                  */
/***************************************************************************************/
static int vortex_open(struct pci_dev *dev)
{
  int i;
  int retval;
  
  /* Use the now-standard shared IRQ implementation. */
  if ((retval = rtl_request_irq(dev->irq, &boomerang_interrupt))) {
    printk(KERN_ERR "%s: Could not reserve IRQ %d\n", dev->name, dev->irq);
    goto out;
  }else
    vp.must_free_irq = 1;

  if (vp.full_bus_master_rx) { /* Boomerang bus master. */

    /* RX RING INITIALIZATION */
    for (i = 0; i < RX_RING_SIZE; i++) {
      unsigned char *skb;
      vp.rx_ring[i].next = cpu_to_le32(vp.rx_ring_dma + sizeof(struct boom_rx_desc) * (i+1));
      vp.rx_ring[i].status = 0;	/* Clear complete bit. */
      vp.rx_ring[i].length = cpu_to_le32(PKT_BUF_SZ | LAST_FRAG);
      skb = rt_malloc(PKT_BUF_SZ);
      vp.rx_skbuff[i] = skb;
      if (skb == NULL)
	break;			/* Bad news!  */
      vp.rx_ring[i].addr = cpu_to_le32(pci_map_single(vp.pdev, skb, PKT_BUF_SZ, PCI_DMA_FROMDEVICE));
    }

    if (i != RX_RING_SIZE) {
      int j;
      printk(KERN_EMERG "%s: no memory for rx ring\n", dev->name);
      for (j = 0; j < i; j++) {
	if (vp.rx_skbuff[j]) {
	  rt_free(vp.rx_skbuff[j]);
	  vp.rx_skbuff[j] = 0;
	}
      }
      retval = -ENOMEM;
      goto out_free_irq;
    }

    /* Wrap the ring. */
    vp.rx_ring[i-1].next = cpu_to_le32(vp.rx_ring_dma);

    /* TX RING INITIALIZATION */
    for (i = 0; i < TX_RING_SIZE; i++) {
      unsigned char *skb;
      vp.tx_ring[i].next = 0; 
      vp.tx_ring[i].status = 0;	/* Clear complete bit. */
      vp.tx_ring[i].length = cpu_to_le32(PKT_BUF_SZ | LAST_FRAG);
      skb = rt_malloc(PKT_BUF_SZ);
      vp.tx_skbuff[i] = skb;
      if (skb == NULL)
	break;			/* Bad news!  */
      vp.tx_ring[i].addr = cpu_to_le32(pci_map_single(vp.pdev, skb, PKT_BUF_SZ, PCI_DMA_FROMDEVICE));
    }

    
    if (i != TX_RING_SIZE) {
      int j;
      printk(KERN_EMERG "%s: no memory for tx ring\n", dev->name);
      for (j = 0; j < i; j++) {
	if (vp.tx_skbuff[j]) {
	  rt_free(vp.tx_skbuff[j]);
	  vp.tx_skbuff[j] = 0;
	}
      }
      retval = -ENOMEM;
      goto out_free_irq;
    }

    /* Wrap the ring. */
    vp.tx_ring[i-1].next = cpu_to_le32(vp.tx_ring_dma);
  }

  vortex_up(dev);
  return 0;
  
 out_free_irq:
  rtl_free_irq(dev->irq); 
 out:
  return retval;
}

/***************************************************************************************/
/* This function is also used to initialise the card. In this function we indicate     */
/* which interrupts do we want the card to generate.                                   */
/***************************************************************************************/
static void vortex_up(struct pci_dev *dev)
{
  long ioaddr = vp.ioaddr;
  unsigned int config;
  int i;
  
  vp.rx_packets = 0;
  vp.rx_frames_for_us = 0;

  if (vp.pdev && vp.enable_wol) {
    rt_pci_set_power_state(dev, 0);	/* Go active */
    rt_pci_restore_state(dev, vp.power_state);
  }
  
  /* Before initializing select the active media port. */
  EL3WINDOW(3);
  config = inl(ioaddr + Wn3_Config);

  if (vp.media_override != 7) {
    vp.if_port = vp.media_override;
  } else if (vp.autoselect) {
    if (vp.has_nway) {
    } else {
      /* Find first available media type, starting with 100baseTx. */
      vp.if_port = XCVR_100baseTx;
      while (! (vp.available_media & media_tbl[vp.if_port].mask))
	vp.if_port = media_tbl[vp.if_port].next;
    }
  } else {
    vp.if_port = vp.default_media;
    
  }

  vp.full_duplex = vp.force_fd;

  config = BFINS(config, vp.if_port, 20, 4);

  outl(config, ioaddr + Wn3_Config);
  
  if (vp.if_port == XCVR_MII || vp.if_port == XCVR_NWAY) {
    int mii_reg1, mii_reg5;
    EL3WINDOW(4);
    /* Read BMSR (reg1) only to clear old status. */
    mii_reg1 = rtl_3COM905C_mdio_read(vp.phys[0], 1);
    mii_reg5 = rtl_3COM905C_mdio_read(vp.phys[0], 5);
    if (mii_reg5 == 0xffff  ||  mii_reg5 == 0x0000)
      ;					/* No MII device or no link partner report */
    else if ((mii_reg5 & 0x0100) != 0	/* 100baseTx-FD */
	     || (mii_reg5 & 0x00C0) == 0x0040) /* 10T-FD, but not 100-HD */
      vp.full_duplex = 1;
    vp.partner_flow_ctrl = ((mii_reg5 & 0x0400) != 0);

    EL3WINDOW(3);
  }

  vp.full_duplex = 1;

  /* Set the full-duplex bit. */

  outw(	((vp.info1 & 0x8000) || vp.full_duplex ? 0x20 : 0) |
	(vp.mtu > 1500 ? 0x40 : 0) |
	((vp.full_duplex && vp.flow_ctrl && vp.partner_flow_ctrl) ? 0x100 : 0),
	ioaddr + Wn3_MAC_Ctrl);
  
  rtl_3COM905C_issue_and_wait(TxReset);

  /*
   * Don't reset the PHY - that upsets autonegotiation during DHCP operations.
   */
  rtl_3COM905C_issue_and_wait(RxReset|0x04);
  
  outw(SetStatusEnb | 0x00, ioaddr + EL3_CMD);

  EL3WINDOW(4);

  /* Set the station address and mask in window 2 each time opened. */
  EL3WINDOW(2);
  for (i = 0; i < 6; i++)
    outb(vp.dev_addr[i], ioaddr + i);
  for (; i < 12; i+=2)
    outw(0, ioaddr + i);
  
  if (vp.cb_fn_base) {
    unsigned short n = inw(ioaddr + Wn2_ResetOptions) & ~0x4010;
    if (vp.drv_flags & INVERT_LED_PWR)
      n |= 0x10;
    if (vp.drv_flags & INVERT_MII_PWR)
      n |= 0x4000;
    outw(n, ioaddr + Wn2_ResetOptions);
  }
  
  if (vp.if_port == XCVR_10base2)
    /* Start the thinnet transceiver. We should really wait 50ms...*/
    outw(StartCoax, ioaddr + EL3_CMD);
  if (vp.if_port != XCVR_NWAY) {
    EL3WINDOW(4);
    outw((inw(ioaddr + Wn4_Media) & ~(Media_10TP|Media_SQE)) |
	 media_tbl[vp.if_port].media_bits, ioaddr + Wn4_Media);
  }
  
  /* RTOS and statistic??? Let's disable stats. */
  outw(StatsDisable, ioaddr + EL3_CMD);
  EL3WINDOW(6);
  for (i = 0; i < 10; i++)
    inb(ioaddr + i);
  inw(ioaddr + 10);
  inw(ioaddr + 12);
  /* New: On the Vortex we must also clear the BadSSD counter. */
  EL3WINDOW(4);
  inb(ioaddr + 12);
  
  /* Switch to register set 7 for normal use. */
  EL3WINDOW(7);
  
  if (vp.full_bus_master_rx) { /* Boomerang bus master. */
    vp.cur_rx = vp.dirty_rx = vp.cur_tx = 0;

    outl(vp.rx_ring_dma, ioaddr + UpListPtr);
  }
  if (vp.full_bus_master_tx) { 		/* Boomerang bus master Tx. */
    vp.cur_tx = vp.dirty_tx = 0;
    if (vp.drv_flags & IS_BOOMERANG)

      outb(PKT_BUF_SZ>>8, ioaddr + TxFreeThreshold); /* Room for a packet. */
    /* Clear the Rx, Tx rings. */
    for (i = 0; i < RX_RING_SIZE; i++)	/* AKPM: this is done in vortex_open, too */
      vp.rx_ring[i].status = 0;
    outl(0, ioaddr + DownListPtr);
  }
  /* Set receiver mode: presumably accept b-case and phys addr only. */
  rtl_3COM905C_set_rx_mode();
  outw(StatsDisable, ioaddr + EL3_CMD);   /* Turn off statistics ASAP. */
  
  outw(RxEnable, ioaddr + EL3_CMD); /* Enable the receiver. */
  outw(TxEnable, ioaddr + EL3_CMD); /* Enable transmitter. */

  /* Allow status bits to be seen. */
  vp.status_enable = SetStatusEnb | 
    (vp.full_bus_master_rx ? UpComplete : RxComplete) | DownComplete;
  vp.intr_enable = SetIntrEnb | UpComplete | DownComplete;
  outw(vp.status_enable, ioaddr + EL3_CMD);
  /* Ack all pending events, and set active indicator mask. */
  outw(AckIntr | IntLatch | TxAvailable | RxEarly | IntReq,
       ioaddr + EL3_CMD);
  outw(vp.intr_enable, ioaddr + EL3_CMD);
  if (vp.cb_fn_base)	
    writel(0x8000, vp.cb_fn_base + 4);
}

/***************************************************************************************/
/* This function is used to issue commands to the card and wait till the command has   */
/* been successfuly carried out.                                                       */
/***************************************************************************************/
static void
rtl_3COM905C_issue_and_wait(int cmd)
{
  int i;

  outw(cmd, vp.ioaddr + EL3_CMD);
  for (i = 0; i < 2000; i++) {
    if (!(inw(vp.ioaddr + EL3_STATUS) & CmdInProgress))
      return;
  }
  
  /* OK, that didn't work.  Do it the slow way.  One second */
  for (i = 0; i < 100000; i++) {
    if (!(inw(vp.ioaddr + EL3_STATUS) & CmdInProgress)) {
      return;
    }
    usleep(10);
  }
  printk(KERN_ERR ": command 0x%04x did not complete! Status=0x%x\n",
	  cmd, inw(vp.ioaddr + EL3_STATUS));
}

/***************************************************************************************/
/* This functions sets the receive mode of the card.                                   */
/* Pre-Cyclone chips have no documented multicast filter, so the only                  */
/* multicast setting is to receive all multicast frames.  At least                     */
/* the chip has a very clean way to set the mode, unlike many others.                  */
/***************************************************************************************/
static void rtl_3COM905C_set_rx_mode(void)
{
  long ioaddr = vp.ioaddr;
  int new_mode;
  
  new_mode = SetRxFilter|RxStation|RxBroadcast;//RxMulticast|RxBroadcast;
		
  outw(new_mode, ioaddr + EL3_CMD);
}

/***************************************************************************************/
/* This function is used to release the card. It is called from the close call.        */
/***************************************************************************************/
static void vortex_down(struct pci_dev *dev)
{
  long ioaddr = vp.ioaddr;
  
  /* Turn off statistics ASAP.  We update vp->stats below. */
  outw(StatsDisable, ioaddr + EL3_CMD);
  
  /* Disable the receiver and transmitter. */
  outw(RxDisable, ioaddr + EL3_CMD);
  outw(TxDisable, ioaddr + EL3_CMD);
  
  if (vp.if_port == XCVR_10base2)
    /* Turn off thinnet power.  Green! */
    outw(StopCoax, ioaddr + EL3_CMD);
  
  outw(SetIntrEnb | 0x0000, ioaddr + EL3_CMD);
  
  if (vp.full_bus_master_rx)
    outl(0, ioaddr + UpListPtr);
  if (vp.full_bus_master_tx)
    outl(0, ioaddr + DownListPtr);
  
  if (vp.pdev && vp.enable_wol) {
    rt_pci_save_state(vp.pdev, vp.power_state);
    rtl_3COM905C_acpi_set_WOL();
  }
}

/***************************************************************************************/
/* This function is used to release the card.                                          */
/***************************************************************************************/
static int vortex_close(struct pci_dev *dev)
{
  int i;
  
  vortex_down(dev);

  if(vp.must_free_irq){
    rtl_free_irq(dev->irq);
    vp.must_free_irq = 0;
  }
  
  if (vp.full_bus_master_rx) { /* Free Boomerang bus master Rx buffers. */
    for (i = 0; i < RX_RING_SIZE; i++)
      if (vp.rx_skbuff[i]) {
	rt_free(vp.rx_skbuff[i]);
      }
  }
  if (vp.full_bus_master_tx) { /* Free Boomerang bus master Tx buffers. */
    for (i = 0; i < TX_RING_SIZE; i++)
      if (vp.tx_skbuff[i]) {
	rt_free(vp.tx_skbuff[i]);
      }
  }

  return 0;
}

/***************************************************************************************/
/* This function is never called, but it is intended to solve errors in the card.      */
/* Handle uncommon interrupt sources.  This is a separate routine to minimize          */ 
/* the cache impact.                                                                   */
/***************************************************************************************/ 
static void vortex_error(struct pci_dev *dev, int status)
{
  long ioaddr = vp.ioaddr;
  int do_tx_reset = 0, reset_mask = 0;
  
  if (status & RxEarly) {				/* Rx early is unused. */
    outw(AckIntr | RxEarly, ioaddr + EL3_CMD);
  }
  if (status & StatsFull) {			/* Empty statistics. */
    static int DoneDidThat;

    printk(KERN_DEBUG "%s: Updating stats.\n", dev->name);
    /* HACK: Disable statistics as an interrupt source. */
    /* This occurs when we have the wrong media type! */
    if (DoneDidThat == 0  &&
	inw(ioaddr + EL3_STATUS) & StatsFull) {
      printk(KERN_WARNING "%s: Updating statistics failed, disabling "
	     "stats as an interrupt source.\n", dev->name);
      EL3WINDOW(5);
      outw(SetIntrEnb | (inw(ioaddr + 10) & ~StatsFull), ioaddr + EL3_CMD);
      vp.intr_enable &= ~StatsFull;
      EL3WINDOW(7);
      DoneDidThat++;
    }
  }
  if (status & IntReq) {		/* Restore all interrupt sources.  */
    outw(vp.status_enable, ioaddr + EL3_CMD);
    outw(vp.intr_enable, ioaddr + EL3_CMD);
  }
  if (status & HostError) {
    u16 fifo_diag;
    EL3WINDOW(4);
    fifo_diag = inw(ioaddr + Wn4_FIFODiag);
    printk(KERN_ERR "%s: Host error, FIFO diagnostic register %4.4x.\n",
	   dev->name, fifo_diag);
    /* Adapter failure requires Tx/Rx reset and reinit. */
    if (vp.full_bus_master_tx) {
      int bus_status = inl(ioaddr + PktStatus);
      /* 0x80000000 PCI master abort. */
      /* 0x40000000 PCI target abort. */
      
      printk(KERN_ERR "%s: PCI bus error, bus status %8.8x\n", dev->name, bus_status);
      
      /* In this case, blow the card away */
      vortex_down(dev);
      rtl_3COM905C_issue_and_wait(TotalReset | 0xff);
      vortex_up(dev);		/* AKPM: bug.  vortex_up() assumes that the rx ring is full. It may not be. */
    } else if (fifo_diag & 0x0400)
      do_tx_reset = 1;
    if (fifo_diag & 0x3000) {
      /* Reset Rx fifo and upload logic */
      rtl_3COM905C_issue_and_wait(RxReset|0x07);
      /* Set the Rx filter to the current state. */
      rtl_3COM905C_set_rx_mode();
      outw(RxEnable, ioaddr + EL3_CMD); /* Re-enable the receiver. */
      outw(AckIntr | HostError, ioaddr + EL3_CMD);
    }
  }
  
  if (do_tx_reset) {
    rtl_3COM905C_issue_and_wait(TxReset|reset_mask);
    outw(TxEnable, ioaddr + EL3_CMD);
  }
}

/***************************************************************************************/
/* This function is called during the release phase. It is mainly used to update pci   */
/* stuff.                                                                              */
/***************************************************************************************/
static void vortex_remove_one (struct pci_dev *pdev)
{
  
  outw(TotalReset|0x14, vp.ioaddr + EL3_CMD);
  
  if (vp.pdev && vp.enable_wol) {
    rt_pci_set_power_state(vp.pdev, 0);	/* Go active */
    if (vp.pm_state_valid)
      pci_restore_state(vp.pdev, vp.power_state);
  }
  
  pci_free_consistent(pdev,
		      sizeof(struct boom_rx_desc) * RX_RING_SIZE,
		      vp.rx_ring,
		      vp.rx_ring_dma);

  pci_free_consistent(pdev,
		      sizeof(struct boom_tx_desc) * TX_RING_SIZE,
		      vp.tx_ring,
		      vp.tx_ring_dma);

  if (vp.must_free_region){
    release_region(vp.ioaddr, vp.io_size);
    vp.must_free_region = 0;
  }
}

/***************************************************************************************/
/* This function is used to send a packet. It writes into the internal buffers of the  */
/* card.                                                                               */
/***************************************************************************************/
static int rt_3c905c_send_packet(const char *buffer, size_t size)
{
  long ioaddr = vp.ioaddr;
  unsigned char *buff;
  int entry = vp.cur_tx % TX_RING_SIZE;
  int previous = (vp.cur_tx + TX_RING_SIZE - 1) % TX_RING_SIZE;
  struct boom_tx_desc *actual = &vp.tx_ring[entry];

  writting = 1;

  /* Wait for the stall to complete. */
  rtl_3COM905C_issue_and_wait(DownStall);
  
  buff = vp.tx_skbuff[entry];
  
  if(buff)
    memcpy(buff, buffer, size);
  
  actual->length = cpu_to_le32(size | LAST_FRAG);
  
  if (inl(ioaddr + DownListPtr) == 0) {
    outl(vp.tx_ring_dma + entry * sizeof(struct boom_tx_desc), ioaddr + DownListPtr);
    {
      int tmp = previous;

      while((vp.tx_ring[tmp].next != 0) && (tmp != entry)){
	vp.tx_ring[tmp].next = 0;
	tmp = (tmp + TX_RING_SIZE - 1) % TX_RING_SIZE;
      }
    }
    vp.queued_packet++;
  }else
    vp.tx_ring[previous].next = cpu_to_le32(vp.tx_ring_dma + sizeof(struct boom_tx_desc) * entry);      
  
  outw(DownUnstall, ioaddr + EL3_CMD);
  
  vp.cur_tx++;
  
  writting = 0;

  return size;
}

/* MII transceiver control section.
   Read and write the MII registers using software-generated serial
   MDIO protocol.  See the MII specifications or DP83840A data sheet
   for details. */

/* The maximum data clock rate is 2.5 Mhz.  The minimum timing is usually
   met by back-to-back PCI I/O cycles, but we insert a delay to avoid
   "overclocking" issues. */
#define rtl_3COM905C_mdio_delay() inl(mdio_addr)

#define MDIO_SHIFT_CLK	0x01
#define MDIO_DIR_WRITE	0x04
#define MDIO_DATA_WRITE0 (0x00 | MDIO_DIR_WRITE)
#define MDIO_DATA_WRITE1 (0x02 | MDIO_DIR_WRITE)
#define MDIO_DATA_READ	0x02
#define MDIO_ENB_IN		0x00

/***************************************************************************************/
/* Generate the preamble required for initial synchronization and a few older          */
/* transceivers.                                                                       */
/***************************************************************************************/
static void rtl_3COM905C_mdio_sync(long ioaddr, int bits)
{
	long mdio_addr = ioaddr + Wn4_PhysicalMgmt;

	/* Establish sync by sending at least 32 logic ones. */
	while (-- bits >= 0) {
		outw(MDIO_DATA_WRITE1, mdio_addr);
		rtl_3COM905C_mdio_delay();
		outw(MDIO_DATA_WRITE1 | MDIO_SHIFT_CLK, mdio_addr);
		rtl_3COM905C_mdio_delay();
	}
}

/***************************************************************************************/
static int rtl_3COM905C_mdio_read(int phy_id, int location)
{
	int i;
	long ioaddr = vp.ioaddr;
	int read_cmd = (0xf6 << 10) | (phy_id << 5) | location;
	unsigned int retval = 0;
	long mdio_addr = ioaddr + Wn4_PhysicalMgmt;

	if (mii_preamble_required)
		rtl_3COM905C_mdio_sync(ioaddr, 32);

	/* Shift the read command bits out. */
	for (i = 14; i >= 0; i--) {
		int dataval = (read_cmd&(1<<i)) ? MDIO_DATA_WRITE1 : MDIO_DATA_WRITE0;
		outw(dataval, mdio_addr);
		rtl_3COM905C_mdio_delay();
		outw(dataval | MDIO_SHIFT_CLK, mdio_addr);
		rtl_3COM905C_mdio_delay();
	}
	/* Read the two transition, 16 data, and wire-idle bits. */
	for (i = 19; i > 0; i--) {
		outw(MDIO_ENB_IN, mdio_addr);
		rtl_3COM905C_mdio_delay();
		retval = (retval << 1) | ((inw(mdio_addr) & MDIO_DATA_READ) ? 1 : 0);
		outw(MDIO_ENB_IN | MDIO_SHIFT_CLK, mdio_addr);
		rtl_3COM905C_mdio_delay();
	}

	return retval & 0x20000 ? 0xffff : retval>>1 & 0xffff;
}

/***************************************************************************************/
static void rtl_3COM905C_mdio_write(int phy_id, int location, int value)
{
	long ioaddr = vp.ioaddr;
	int write_cmd = 0x50020000 | (phy_id << 23) | (location << 18) | value;
	long mdio_addr = ioaddr + Wn4_PhysicalMgmt;
	int i;

	if (mii_preamble_required)
		rtl_3COM905C_mdio_sync(ioaddr, 32);

	/* Shift the command bits out. */
	for (i = 31; i >= 0; i--) {
		int dataval = (write_cmd&(1<<i)) ? MDIO_DATA_WRITE1 : MDIO_DATA_WRITE0;
		outw(dataval, mdio_addr);
		rtl_3COM905C_mdio_delay();
		outw(dataval | MDIO_SHIFT_CLK, mdio_addr);
		rtl_3COM905C_mdio_delay();
	}
	/* Leave the interface idle. */
	for (i = 1; i >= 0; i--) {
		outw(MDIO_ENB_IN, mdio_addr);
		rtl_3COM905C_mdio_delay();
		outw(MDIO_ENB_IN | MDIO_SHIFT_CLK, mdio_addr);
		rtl_3COM905C_mdio_delay();
	}
	return;
}


