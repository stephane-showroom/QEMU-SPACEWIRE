| **77** | **GRSPW - SpaceWire codec with AHB host Interface and RMAP target** |  |
| ---: | --- | --- |
| **77.1** | **Overview** |  |
|  | The SpaceWire core provides an interface between the AHB bus and a SpaceWire network. It imple | - |
|  | ments the SpaceWire standard (ECSS-E-ST-50-12C) with the protocol identification  | extension |

(ECSS-E-ST-50-51C). The optional Remote Memory Access Protocol (RMAP) target implements the ECSS standard (ECSS-E-ST-50-52C).

The core is configured through a set of registers accessed through an APB interface. Data is trans- ferred through DMA channels using an AHB master interface.

Currently, there is one DMA channel but the core can easily be extended to use separate DMA chan- nels for specific protocols. The core can also be configured to have either one or two ports. There can be up to four clock domains: one for the AHB interface (system clock), one for the trans- mitter and one or two for the receiver depending on the number of configured ports. The receiver clock can be twice as fast and the transmitter clock four times as fast as the system clock whose fre- quency should be at least 10 MHz.

The core only supports byte addressed 32-bit big-endian host systems. **Note:** This IP core is deprecated and should not be used for new designs. Please see the GRSPW2 IP core instead.

TXCLK D(1:0)

| TRANSMITTER |
| --- |
| TRANSMITTER |
| TRANSMITTER |

| TRANSMITTERFSM |
| --- |
| TRANSMITTERFSM |

| SENDFSM |
| --- |
| SENDFSM |

S(1:0)

|  | RMAP | TRANSMITTER |  |
| --- | --- | --- | --- |
|  | TRANSMITTER | DMA ENGINE |  |
| LINKINTERFACE |  |  | AHB |
| FSM |  |  | MASTER INTERFACE |

RECEIVER DMA ENGINE

| RECEIVER0 |  |
| --- | --- |
| RECEIVER0 |  |

D0

| RECEIVER AHB FIFO | RECEIVER AHB FIFO |
| --- | --- |

|  | RXCLK | RMAP  |  | APB |
| --- | --- | --- | --- | --- |
| S0 |  | RECEIVER | REGISTERS | INTERFACE |
| D1 |  |  |  |  |
|  | RXCLK | N-CHAR  |  |  |
| S1 |  | FIFO |  |  |

| RXCLK RXCLKRECOVERY |
| --- |
| RXCLK RXCLKRECOVERY |

| RECEIVER1 |
| --- |
| RECEIVER1 |

| RECEIVER DATAPARALLELIZATION | RECEIVER DATAPARALLELIZATION |
| --- | --- |

| RXCLK RXCLKRECOVERY |
| --- |
| RXCLK RXCLKRECOVERY |

_Figure 189._ Block diagram

**77.2** **Operation**

**77.2.1** **Overview** The main sub-blocks of the core are the link-interface, the RMAP target and the AMBA interface. A block diagram of the internal structure can be found in figure 189. 

The link interface consists of the receiver, transmitter and the link interface FSM. They handle com- munication on the SpaceWire network. The AMBA interface consists of the DMA engines, the AHB master interface and the APB interface. The link interface provides FIFO interfaces to the DMA engines. These FIFOs are used to transfer N-Chars between the AMBA and SpaceWire domains during reception and transmission.

The RMAP target is an optional part of the core which can be enabled with a VHDL generic. The RMAP target handles incoming packets which are determined to be RMAP commands instead of the

GRIP Frontgrade Gaisler AB

receiver DMA engine. The RMAP command is decoded and if it is valid, the operation is performed on the AHB bus. If a reply was requested it is automatically transmitted back to the source by the RMAP transmitter. 

The core is controlled by writing to a set of user registers through the APB interface and three signals: tick-in, rmapen and clkdiv10. The controlled parts are clock-generation, DMA engines, RMAP target and the link interface.

The link interface, DMA engines, RMAP target and AMBA interface are described in section 77.3, 77.4, 77.5 and [77.7 ](#777-amba-interface)respectively. 

**77.2.2** **Protocol support**

The core only accepts packets with a destination address corresponding to the one set in the node address register. Packets with address mismatch will be silently discarded (except in promiscuous mode which is covered in section 77.4.10). The node address register is initialized to the default address 254 during reset. It can then be changed to other values by writing to the register. The core has support for the protocol ID specified in ECSS-E-ST-50-51C. It is used for identifying RMAP commands that should be received by the RMAP target. Other packets are stored to the DMA channel. This is only applicable if the RMAP target is present and enabled. When the RMAP target is not present or disabled all the bytes after the address are treated as normal cargo. RMAP commands are identified using the protocol ID (0x01) and the instruction field. They are han- dled separately from other packets if the hardware RMAP target is enabled. When enabled, all RMAP commands are processed, executed and replied in hardware. RMAP replies received are always stored to the DMA channel. If the RMAP target is disabled, all packets are stored to the DMA channel. More information on the RMAP protocol support is found in section [77.7.](#777-amba-interface)

RMAP packets arriving with the extended protocol ID (0x000001) are stored to the DMA channel which means that the hardware RMAP target will not work if the incoming RMAP packets use the extended protocol ID. Note also that packets with the reserved extended protocol identifier (ID = 0x000000) are not ignored by the core. It is up to the client receiving the packets to ignore them. When transmitting packets, the address and protocol-ID fields must be included in the buffers from where data is fetched. They are _not_ automatically added by the core. 

Figure 190 shows the packet types supported by the core. The core also allows reception and trans- mission with extended protocol identifiers but without support for RMAP CRC calculations and the RMAP target.

| Addr | ProtID | D0 | D1 | D2 | D3 | .. | Dn-2 | Dn-1 | EOP |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

| Addr | D0 | D1 | D2 | D3 | D4 | .. | Dm-2 | Dm-1 | EOP |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

_Figure 190._ The SpaceWire packet types supported by the GRSPW.

**77.3** **Link interface**

The link interface handles the communication on the SpaceWire network and consists of a transmitter, receiver, a FSM and FIFO interfaces. An overview of the architecture is found in figure 189.

**77.3.1** **Link interface FSM**

The FSM controls the link interface (a more detailed description is found in the SpaceWire standard). The low-level protocol handling (the signal and character level of the SpaceWire standard) is handled by the transmitter and receiver while the FSM in the host domain handles the exchange level.

GRIP Frontgrade Gaisler AB

The link interface FSM is controlled through the control register. The link can be disabled through the link disable bit, which depending on the current state, either prevents the link interface from reaching the started state or forces it to the error-reset state. When the link is not disabled, the link interface FSM is allowed to enter the started state when either the link start bit is set or when a NULL character has been received and the autostart bit is set.

The current state of the link interface determines which type of characters are allowed to be transmit- ted which together with the requests made from the host interfaces determine what character will be sent.

Time-codes are sent when the FSM is in the run-state and a request is made through the time-interface (described in section 77.3.5). 

When the link interface is in the connecting- or run-state it is allowed to send FCTs. FCTs are sent automatically by the link interface when possible. This is done based on the maximum value of 56 for the outstanding credit counter and the currently free space in the receiver N-Char FIFO. FCTs are sent as long as the outstanding counter is less than or equal to 48 and there are at least 8 more empty FIFO entries than the counter value. 

N-Chars are sent in the run-state when they are available from the transmitter FIFO and there are credits available. NULLs are sent when no other character transmission is requested or the FSM is in a state where no other transmissions are allowed.

The credit counter (incoming credits) is automatically increased when FCTs are received and decreased when N-Chars are transmitted. Received N-Chars are stored to the receiver N-Char FIFO for further handling by the DMA interface. Received Time-codes are handled by the time-interface.

**77.3.2** **Transmitter**

The state of the FSM, credit counters, requests from the time-interface and requests from the DMA- interface are used to decide the next character to be transmitted. The type of character and the charac- ter itself (for N-Chars and Time-codes) to be transmitted are presented to the low-level transmitter which is located in a separate clock-domain. 

This is done because one usually wants to run the SpaceWire link on a different frequency than the host system clock. The core has a separate clock input which is used to generate the transmitter clock. More information on transmitter clock generation is found in section 77.8.2. Since the transmitter often runs on high frequency clocks (\> 100 MHz) as much logic as possible has been placed in the system clock domain to minimize power consumption and timing issues.

The transmitter logic in the host clock domain decides what character to send next and sets the proper control signal and presents any needed character to the low-level transmitter as shown in figure 191. The transmitter sends the requested characters and generates parity and control bits as needed. If no requests are made from the host domain, NULLs are sent as long as the transmitter is enabled. Most of the signal and character levels of the SpaceWire standard is handled in the transmitter. External LVDS drivers are needed for the data and strobe signals. 

| Transmitter  |
| --- |
| Transmitter  |
| Transmitter  |

D Send Time-code

Send FCT S Send NChar

Time-code\[7:0\] NChar\[8:0\]

Transmitter Clock Domain Host Clock Domain

_Figure 191._ Schematic of the link interface transmitter.

GRIP Frontgrade Gaisler AB

A transmission FSM reads N-Chars for transmission from the transmitter FIFO. It is given packet lengths from the DMA interface and appends EOPs/EEPs and RMAP CRC values if requested. When it is finished with a packet the DMA interface is notified and a new packet length value is given.

**77.3.3** **Receiver**

The receiver detects connections from other nodes and receives characters as a bit stream on the data and strobe signals. It is also located in a separate clock domain which runs on a clock generated from the received data and strobe signals. More information on the clock-generation can be found in sec- tion 77.8.2.

The receiver is activated as soon as the link interface leaves the error reset state. Then after a NULL is received it can start receiving any characters. It detects parity, escape and credit errors which causes the link interface to enter the error reset state. Disconnections are handled in the link interface part in the system clock domain because no receiver clock is available when disconnected. Received Characters are flagged to the host domain and the data is presented in parallel form. The interface to the host domain is shown in figure 192. L-Chars are the handled automatically by the host domain link interface part while all N-Chars are stored in the receiver FIFO for further handling. If two or more consecutive EOPs/EEPs are received all but the first are discarded. There are no signals going directly from the transmitter clock domain to the receiver clock domain and vice versa. All the synchronization is done to the system clock.

| D |  | Got Time-code |
| --- | --- | --- |
|  |  | Got FCT |
|  | Receiver | Got EOPGot EEP |
| S |  | Got NCharTime-code\[7:0\] |

Time-code\[7:0\]

NChar\[7:0\]

Receiver Clock Domain Host Clock Domain

_Figure 192._ Schematic of the link interface receiver.

**77.3.4** **Dual port support**

The core can be configured to include an additional SpaceWire port. With dual ports the transmitter drives an additional pair of data/strobe output signals and one extra receiver is added to handle a sec- ond pair of data/strobe input signals. 

One of the ports is set as active (how the active port is selected is explained below) and the transmitter drives the data/strobe signals of the active port with the actual output values as explained in section 77.3.2. The inactive port is driven with zero on both data and strobe. 

Both receivers will always be active but only the active port’s interface signals (see figure 192) will be propagated to the link interface FSM. Each time the active port is changed, the link will be reset so that the new link is started in a controlled manner. 

When the noportforce register is zero the portsel register bit selects the active link and when set to one it is determined by the current link activity. In the latter mode the port is changed when no activity is seen on the currently active link while there is activity on the deselected receive port. Activity is defined as a detected null. This definition is selected so that glitches (e.g. port unconnected) do not cause unwanted port switches. 

**77.3.5** **Time interface**

The time interface is used for sending Time-codes over the SpaceWire network and consists of a time- counter register, time-ctrl register, tick-in signal, tick-out signal, tick-in register field and a tick-out

GRIP Frontgrade Gaisler AB

register field. There are also two control register bits which enable the time receiver and transmitter respectively. 

Each Time-code sent from the core is a concatenation of the time-ctrl and the time-counter register. There is a timetxen bit which is used to enable Time-code transmissions. It is not possible to send time-codes if this bit is zero. 

Received Time-codes are stored to the same time-ctrl and time-counter registers which are used for transmission. The timerxen bit in the control register is used for enabling time-code reception. No time-codes will be received if this bit is zero.

The two enable bits are used for ensuring that a node will not (accidentally) both transmit and receive time-codes which violates the SpaceWire standard. It also ensures that a the master sending time- codes on a network will not have its time-counter overwritten if another (faulty) node starts sending time-codes. 

The time-counter register is set to 0 after reset and is incremented each time the tick-in signal is asserted for one clock-period and the timetxen bit is set. This also causes the link interface to send the new value on the network. Tick-in can be generated either by writing a one to the register field or by asserting the tick-in signal. A Tick-in should not be generated too often since if the time-code after the previous Tick-in has not been sent the register will not be incremented and no new value will be sent. The tick-in field is automatically cleared when the value has been sent and thus no new ticks should be generated until this field is zero. If the tick-in signal is used there should be at least 4 system-clock and 25 transmit-clock cycles between each assertion.

A tick-out is generated each time a valid time-code is received and the timerxen bit is set. When the tick-out is generated the tick-out signal will be asserted one clock-cycle and the tick-out register field is asserted until it is cleared by writing a one to it.

The current time counter value can be read from the time register. It is updated each time a Time-code is received and the timerxen bit is set. The same register is used for transmissions and can also be written directly from the APB interface.

The control bits of the Time-code are always stored to the time-ctrl register when a Time-code is received whose time-count is one more than the nodes current time-counter register. The time-ctrl reg- ister can be read through the APB interface. The same register is used during time-code transmissions. It is possible to have both the time-transmission and reception functions enabled at the same time.

**77.4** **Receiver DMA engine**

The receiver DMA engine handles reception of data from the SpaceWire network to different DMA channels. Currently there is only one receive DMA channel available but the core has been written so that additional channels can be easily added if needed. 

**77.4.1** **Basic functionality**

The receiver DMA engine reads N-Chars from the N-Char FIFO and stores them to a DMA channel. Reception is based on descriptors located in a consecutive area in memory that hold pointers to buf- fers where packets should be stored. When a packet arrives at the core it reads a descriptor from mem- ory and stores the packet to the memory area pointed to by the descriptor. Then it stores status to the same descriptor and increments the descriptor pointer to the next one.

**77.4.2** **Setting up the core for reception**

A few registers need to be initialized before reception can take place. First the link interface need to be put in the run state before any data can be sent. The DMA channel has a maximum length register which sets the maximum size of packet that can be received to this channel. Larger packets are trun- cated and the excessive part is spilled. If this happens an indication will be given in the status field of the descriptor. The minimum value for the receiver maximum length field is 4 and the value can only

GRIP Frontgrade Gaisler AB

be incremented in steps of four bytes. If the maximum length is set to zero the receiver will _not_ func- tion correctly.

The node address register needs to be set to hold the address of this SpaceWire node. Packets received with the incorrect address are discarded. Finally, the descriptor table and control register must be ini- tialized. This will be described in the two following sections.

**77.4.3** **Setting up the descriptor table address**

The core reads descriptors from an area in memory pointed to by the receiver descriptor table address register. The register consists of a base address and a descriptor selector. The base address points to the beginning of the area and must start on a 1 kbytes aligned address. It is also limited to be 1 kbytes in size which means the maximum number of descriptors is 128. 

The descriptor selector points to individual descriptors and is increased by 1 when a descriptor has been used. When the selector reaches the upper limit of the area it wraps to the beginning automati- cally. It can also be set to wrap automatically by setting a bit in the descriptors. The idea is that the selector should be initialized to 0 (start of the descriptor area) but it can also be written with another 8 bytes aligned value to start somewhere in the middle of the area. It will still wrap to the beginning of the area.

If one wants to use a new descriptor table the receiver enable bit has to be cleared first. When the rxactive bit for the channel is cleared it is safe to update the descriptor table register. When this is fin- ished and descriptors are enabled the receiver enable bit can be set again.

**77.4.4** **Enabling descriptors**

As mentioned earlier one or more descriptors must be enabled before reception can take place. Each descriptor is 8 byte in size and the layout can be found in the tables below. The descriptors should be written to the memory area pointed to by the receiver descriptor table address register. When new descriptors are added they must always be placed after the previous one written to the area. Otherwise they will not be noticed.

A descriptor is enabled by setting the address pointer to point at a location where data can be stored and then setting the enable bit. The WR bit can be set to cause the selector to be set to zero when reception has finished to this descriptor. IE should be set if an interrupt is wanted when the reception has finished. The DMA control register interrupt enable bit must also be set for this to happen. The descriptor packet address should be word aligned. All accesses on the bus are word accesses so complete words will always be overwritten regardless of whether all 32-bit contain received data. Also if the packet does not end on a word boundary the complete word containing the last data byte will be overwritten. If the _rxunaligned_ or _rmap_ VHDL generic is set to 1 this restriction is removed

GRIP Frontgrade Gaisler AB

and any number of bytes can be received to any packet address without excessive bytes being over- written.

_Table 1329._GRSPW receive descriptor word 0 (address offset 0x0)

| 31 | 30 | 29 | 28 | 27 | 26 | 25 24 | 0 |
| ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| TR | DC | HC | EP | IE | WR | EN PACKETLENGTH |  |
|  |  | 31 |  |  |  | Truncated (TR) - Packet was truncated due to maximum length violation. |  |
|  |  | 30 |  |  |  | Data CRC (DC) - 1 if a CRC error was detected for the data and 0 otherwise. |  |
|  |  | 29 |  |  |  | Header CRC (HC) - 1 if a CRC error was detected for the header and 0 otherwise. |  |
|  |  | 28 |  |  |  | EEP termination (EP) - This packet ended with an Error End of Packet character. |  |
|  |  | 27 |  |  |  | Interrupt enable (IE) - If set, an interrupt will be generated when a packet has been received if the  |  |
|  |  |  |  |  |  | receive interrupt enable bit in the DMA channel control register is set. |  |
|  |  | 26 |  |  |  | Wrap (WR) - If set, the next descriptor used by the GRSPW will be the first one in the descriptor  |  |

table (at the base address). Otherwise the descriptor pointer will be increased with 0x8 to use the descriptor at the next higher memory location. The descriptor table is limited to 1 kbytes in size and the pointer will be automatically wrap back to the base address when it reaches the 1 kbytes bound- ary.

| 25 |  | Enable (EN) - Set to one to activate this descriptor. This means that the descriptor contains valid con- |
| ---: | --- | --- |
|  |  | trol values and the memory area pointed to by the packet address field can be used to store a packet. |
| 24: | 0 | Packet length (PACKETLENGTH) - The number of bytes received to this buffer. Only valid after  |

EN has been set to 0 by the GRSPW.

_Table 1330._GRSPW receive descriptor word 1 (address offset 0x4)

| 31 |  |  | 0 |
| ---: | ---: | --- | --- |
|  |  |  | PACKETADDRESS |
|  | 31: | 0 | Packet address (PACKETADDRESS) - The address pointing at the buffer which will be used to store  |

the received packet. If the rxunaligned and rmap VHDL generics are both set to zero only bit 31 to 2 are used.

**77.4.5** **Setting up the DMA control register**

The final step to receive packets is to set the control register in the following steps: The receiver must be enabled by setting the rxen bit in the DMA control register (see section [77.9)](#779-axi-support). This can be done anytime and before this bit is set nothing will happen. The rxdescav bit in the DMA control register is then set to indicate that there are new active descriptors. This must always be done after the descrip- tors have been enabled or the core might not notice the new descriptors. More descriptors can be acti- vated when reception has already started by enabling the descriptors and writing the rxdescav bit. When these bits are set reception will start immediately when data is arriving.

**77.4.6** **The effect to the control bits during reception**

When the receiver is disabled all packets going to the DMA-channel are discarded. If the receiver is enabled the next state is entered where the rxdescav bit is checked. This bit indicates whether there are active descriptors or not and should be set by the external application using the DMA channel each time descriptors are enabled as mentioned above. If the rxdescav bit is ‘0’ and the nospill bit is ‘0’ the packets will be discarded. If nospill is one the core waits until rxdescav is set. 

When rxdescav is set the next descriptor is read and if enabled the packet is received to the buffer. If the read descriptor is not enabled, rxdescav is set to ‘0’ and the packet is spilled depending on the value of nospill.

The receiver can be disabled at any time and will cause all packets received afterwards to be dis- carded. If a packet is currently received when the receiver is disabled the reception will still be fin- ished. The rxdescav bit can also be cleared at any time. It will not affect any ongoing receptions but

GRIP Frontgrade Gaisler AB

no more descriptors will be read until it is set again. Rxdescav is also cleared by the core when it reads a disabled descriptor.

**77.4.7** **Address recognition and packet handling**

When the receiver N-Char FIFO is not empty, N-Chars are read by the receiver DMA engine. The first character is interpreted as the logical address which is compared to the node address register. If it does not match, the complete packet is discarded (up to and including the next EOP/EEP). If the address matches the next action taken depends on whether RMAP is enabled or not. If RMAP is disabled all packets are stored to the DMA channel and depending on the conditions mentioned in the previous section, the packet will be received or not. If the packet is received the complete packet including address and protocol ID but excluding EOP/EEP is stored to the address indicated in the descriptor, otherwise the complete packet is discarded.

If RMAP is enabled the protocol ID and 3rd byte in the packet is first checked before any decisions are made. If incoming packet is an RMAP packet (ID = 0x01) and the command type field is 01b the packet is processed by the RMAP command handler which is described in section 77.6. Otherwise the packet is processed by the DMA engine as when RMAP is disabled.

At least 2 non EOP/EEP N-Chars need to be received for a packet to be stored to the DMA channel. If it is an RMAP packet 3 N-Chars are needed since the command byte determines where the packet is processed. Packets smaller than the minimum size are discarded.

**77.4.8** **Status bits**

When the reception of a packet is finished the enable bit in the current descriptor is set to zero. When enable is zero, the status bits are also valid and the number of received bytes is indicated in the length field. The DMA control register contains a status bit which is set each time a packet has been received. The core can also be made to generate an interrupt for this event as mentioned in section 77.4.4. 

RMAP CRC logic is included in the implementation if the _rmapcrc_ or _rmap_ VHDL generic set to 1. The RMAP CRC calculation is always active for all received packets and all bytes except the EOP/ EEP are included. The packet is always assumed to be a RMAP packet and the length of the header is determined by checking byte 3 which should be the command field. The calculated CRC value is then checked when the header has been received (according to the calculated number of bytes) and if it is non-zero the HC bit is set indicating a header CRC error.

The CRC value is not set to zero after the header has been received, instead the calculation continues in the same way until the complete packet has been received. Then if the CRC value is non-zero the DC bit is set indicating a data CRC error. This means that the core can indicate a data CRC error even if the data field was correct when the header CRC was incorrect. However, the data should not be used when the header is corrupt and therefore the DC bit is unimportant in this case. When the header is not corrupted the CRC value will always be zero when the calculation continues with the data field and the behaviour will be as if the CRC calculation was restarted 

If the received packet is not of RMAP type the header CRC error indication bit cannot be used. It is still possible to use the DC bit if the complete packet is covered by a CRC calculated using the RMAP CRC definition. This is because the core does not restart the calculation after the header has been received but instead calculates a complete CRC over the packet. Thus any packet format with one CRC at the end of the packet calculated according to RMAP standard can be checked using the DC bit.

If the packet is neither of RMAP type nor of the type above with RMAP CRC at the end, then both the HC and DC bits should be ignored.

GRIP Frontgrade Gaisler AB

**77.4.9** **Error handling**

If a packet reception needs to be aborted because of congestion on the network, the suggested solution is to set link disable to ‘1’. Unfortunately, this will also cause the packet currently being transmitted to be truncated but this is the only safe solution since packet reception is a passive operation depending on the transmitter at the other end. A channel reset bit could be provided but is not a satisfactory solu- tion since the untransmitted characters would still be in the transmitter node. The next character (somewhere in the middle of the packet) would be interpreted as the node address which would prob- ably cause the packet to be discarded but not with 100% certainty. Usually this action is performed when a reception has stuck because of the transmitter not providing more data. The channel reset would not resolve this congestion.

If an AHB error occurs during reception the current packet is spilled up to and including the next EEP/EOP and then the currently active channel is disabled and the receiver enters the idle state. A bit in the channels control/status register is set to indicate this condition.

**77.4.10** **Promiscuous mode**

The core supports a promiscuous mode where all the data received is stored to the DMA channel regardless of the node address and possible early EOPs/EEPs. This means that all non-eop/eep N- Chars received will be stored to the DMA channel. The rxmaxlength register is still checked and packets exceeding this size will be truncated. 

RMAP commands will still be handled by the RMAP target when promiscuous mode is enabled if the rmapen bit is set. If it is cleared, RMAP commands will also be stored to the DMA channel.

**77.5** **Transmitter DMA engine**

The transmitter DMA engine handles transmission of data from the DMA channel to the SpaceWire network. There is one DMA channel available but the core has been written so that additional DMA channels can be easily added if needed. 

**77.5.1** **Basic functionality**

The transmit DMA engine reads data from the AHB bus and stores them in the transmitter FIFO for transmission on the SpaceWire network. Transmission is based on the same type of descriptors as for the receiver and the descriptor table has the same alignment and size restrictions. When there are new descriptors enabled the core reads them and transfer the amount data indicated.

**77.5.2** **Setting up the core for transmission**

Four steps need to be performed before transmissions can be done with the core. First the link inter- face must be enabled and started by writing the appropriate value to the ctrl register. Then the address to the descriptor table needs to be written to the transmitter descriptor table address register and one or more descriptors must also be enabled in the table. Finally, the txen bit in the DMA control register should be written with a one which triggers the transmission. These steps will be covered in more detail in the next sections.

**77.5.3** **Enabling descriptors**

The descriptor table address register works in the same way as the receiver’s corresponding register which was covered in section 77.4.

To transmit packets one or more descriptors have to be initialized in memory which is done in the fol- lowing way: The number of bytes to be transmitted and a pointer to the data has to be set. There are two different length and address fields in the transmit descriptors because there are separate pointers for header and data. If a length field is zero the corresponding part of a packet is skipped and if both are zero no packet is sent. The maximum header length is 255 bytes and the maximum data length is

GRIP Frontgrade Gaisler AB

16 Mbyte - 1. When the pointer and length fields have been set the enable bit should be set to enable the descriptor. This must always be done last. The other control bits must also be set before enabling the descriptor.

The transmit descriptors are 16 bytes in size so the maximum number in a single table is 64. The dif- ferent fields of the descriptor together with the memory offsets are shown in the tables below. The HC bit should be set if RMAP CRC should be calculated and inserted for the header field and correspondingly the DC bit should be set for the data field. This field is only used by the core when the CRC logic is available (_rmap_ or _rmapcrc_ VHDL generic set to 1). The header CRC will be calcu- lated from the data fetched from the header pointer and the data CRC is generated from data fetched from the data pointer. The CRCs are appended after the corresponding fields. The NON-CRC bytes field is set to the number of bytes in the beginning of the header field that should not be included in the CRC calculation. The CRCs are sent even if the corresponding length is zero.

When both header and data length are zero no packet is sent not even an EOP.

### 77.5.4 Starting transmissions

When the descriptors have been initialized, the transmit enable bit in the DMA control register has to be set to tell the core to start transmitting. New descriptors can be activated in the table on the fly (while transmission is active). Each time a set of descriptors is added the transmit enable register bit should be set. This has to be done because each time the core encounters a disabled descriptor this register bit is set to 0.

_Table 1331._GRSPW transmit descriptor word 0 (address offset 0x0)

| 31 |  |  | 18 17 16 15 14 13 12 11 8 7 | 0 |
| ---: | ---: | ---: | --- | --- |
|  |  |  | RESERVED DC HC LE IE WR EN NONCRCLEN HEADERLEN |  |
|  | 31: | 18 | RESERVED |  |
|  | 17 |  | Append data CRC (DC) - Append CRC calculated according to the RMAP specification after the  |  |

data sent from the data pointer. The CRC covers all the bytes from this pointer. A null CRC will be sent if the length of the data field is zero.

16 Append header CRC (HC) - Append CRC calculated according to the RMAP specification after the data sent from the header pointer. The CRC covers all bytes from this pointer except a number of bytes in the beginning specified by the non-crc bytes field. The CRC will not be sent if the header length field is zero.

15 Link error (LE) - A Link error occurred during the transmission of this packet. 14 Interrupt enable (IE) - If set, an interrupt will be generated when the packet has been transmitted and 

the transmitter interrupt enable bit in the DMA control register is set. 13 Wrap (WR) - If set, the descriptor pointer will wrap and the next descriptor read will be the first one 

in the table (at the base address). Otherwise the pointer is increased with 0x10 to use the descriptor at the next higher memory location.

12 Enable (EN) - Enable transmitter descriptor. When all control fields (address, length, wrap and crc) are set, this bit should be set. While the bit is set the descriptor should not be touched since this might corrupt the transmission. The GRSPW clears this bit when the transmission has finished.

11: 8 Non-CRC bytes (NONCRCLEN)- Sets the number of bytes in the beginning of the header which should not be included in the CRC calculation. This is necessary when using path addressing since one or more bytes in the beginning of the packet might be discarded before the packet reaches its destination.

7: 0 Header length (HEADERLEN) - Header Length in bytes. If set to zero, the header is skipped.

_Table 1332._GRSPW transmit descriptor word 1 (address offset 0x4)

|  | 31: | 0 | Header address (HEADERADDRESS) - Address from where the packet header is fetched. Does not  |
| --- | ---: | --- | --- |
|  |  |  | need to be word aligned. |
| GRIP |  |  | Frontgrade Gaisler AB |

_Table 1333._GRSPW transmit descriptor word 2 (address offset 0x8)

| 31 |  | 24 23 0 |
| ---: | ---: | --- |
|  | RESERVED | DATALEN |
|  | 31: 24 | RESERVED |
|  | 23: 0 | Data length (DATALEN) - Length of data part of packet. If set to zero, no data will be sent. If both  |

|  | 23: | 0 | Data length (DATALEN) - Length of data part of packet. If set to zero, no data will be sent. If both  |  |
| ---: | ---: | --- | --- | --- |
|  |  |  | data- and header-lengths are set to zero no packet will be sent. |  |
| _Table 1334._ |  |  | GRSPW transmit descriptor word 3(address offset 0xC) |  |
| 31 |  |  |  | 0 |
|  |  |  |  | DATAADDRESS |
|  | 31: | 0 | Data address (DATAADDRESS) - Address from where data is read. Does not need to be word  |  |
|  |  |  | aligned. |  |
| **77.5.5** |  |  | **The transmission process** |  |

When the txen bit is set the core starts reading descriptors immediately. The number of bytes indicated are read and transmitted. When a transmission has finished, status will be written to the first field of the descriptor and a packet sent bit is set in the DMA control register. If an interrupt was requested it will also be generated. Then a new descriptor is read and if enabled a new transmission starts, other- wise the transmit enable bit is cleared and nothing will happen until it is enabled again.

**77.5.6** **The descriptor table address register**

The internal pointer which is used to keep the current position in the descriptor table can be read and written through the APB interface. This pointer is set to zero during reset and is incremented each time a descriptor is used. It wraps automatically when the 1 kbytes limit for the descriptor table is reached or it can be set to wrap earlier by setting a bit in the current descriptor.

The descriptor table register can be updated with a new table anytime when no transmission is active. No transmission is active if the transmit enable bit is zero and the complete table has been sent or if the table is aborted (explained below). If the table is aborted one has to wait until the transmit enable bit is zero before updating the table pointer.

**77.5.7** **Error handling**

**Abort Tx** The DMA control register contains a bit called Abort TX which if set causes the current transmission to be aborted, the packet is truncated and an EEP is inserted. This is only useful if the packet needs to be aborted because of congestion on the SpaceWire network. If the congestion is on the AHB bus this will not help (This should not be a problem since AHB slaves should have a maximum of 16 wait- states). The aborted packet will have its LE bit set in the descriptor. The transmit enable register bit is also cleared and no new transmissions will be done until the transmitter is enabled again. **AHB error**

When an AHB error is encountered during transmission the currently active DMA channel is disabled and the transmitter goes to the idle mode. A bit in the DMA channel’s control/status register is set to indicate this error condition and, if enabled, an interrupt will also be generated. Further error handling depends on what state the transmitter DMA engine was in when the AHB error occurred. If the descriptor was being read the packet transmission had not been started yet and no more actions need to be taken. 

If the AHB error occurs during packet transmission the packet is truncated and an EEP is inserted. Lastly, if it occurs when status is written to the descriptor the packet has been successfully transmitted but the descriptor is not written and will continue to be enabled (this also means that no error bits are set in the descriptor for AHB errors). 

GRIP Frontgrade Gaisler AB

The client using the channel has to correct the AHB error condition and enable the channel again. No more AHB transfers are done again from the same unit (receiver or transmitter) which was active during the AHB error until the error state is cleared and the unit is enabled again. **Link error**

When a link error occurs during the transmission the remaining part of the packet is discarded up to and including the next EOP/EEP. When this is done status is immediately written (with the LE bit set) and the descriptor pointer is incremented. The link will be disconnected when the link error occurs but the core will automatically try to connect again provided that the link-start bit is asserted and the link- disabled bit is deasserted. If the LE bit in the DMA channel’s control register is not set the transmitter DMA engine will wait for the link to enter run-state and start a new transmission immediately when possible if packets are pending. Otherwise the transmitter will be disabled when a link error occurs during the transmission of the current packet and no more packets will be transmitted until it is enabled again. 

**77.6** **RMAP**

The Remote Memory Access Protocol (RMAP) is used to implement access to resources in the node via the SpaceWire Link. Some common operations are reading and writing to memory, registers and FIFOs. The core has an optional hardware RMAP target which is enabled with a VHDL generic. This section describes the basics of the RMAP protocol and the target implementation.

**77.6.1** **Fundamentals of the protocol**

RMAP is a protocol which is designed to provide remote access via a SpaceWire network to memory mapped resources on a SpaceWire node. It has been assigned protocol ID 0x01. It provides three oper- ations write, read and read-modify-write. These operations are posted operations which means that a source does not wait for an acknowledge or reply. It also implies that any number of operations can be outstanding at any time and that no timeout mechanism is implemented in the protocol. Time-outs must be implemented in the user application which sends the commands. Data payloads of up to 16 Mb - 1 is supported in the protocol. A destination can be requested to send replies and to verify data before executing an operation. A complete description of the protocol is found in the RMAP standard.

**77.6.2** **Implementation**

The core includes a taget for RMAP commands which processes all incoming packets with protocol ID = 0x01 and type field (bit 7 and 6 of the 3rd byte in the packet) equal to 01b. When such a packet is detected it is not stored to the DMA channel, instead it is passed to the RMAP receiver. The core implements all three commands defined in the standard with some restrictions. First of all the optional error code 12 is not implemented and support is only provided for 32-bit big-endian sys- tems. This means that the first byte received is the msb in a word. The command handler will not receive RMAP packets using the extended protocol ID which are always dumped to the DMA chan- nel.

The RMAP receiver processes commands. If they are correct and accepted the operation is performed on the AHB bus and a reply is formatted. If an acknowledge is requested the RMAP transmitter auto- matically send the reply. RMAP transmissions have priority over DMA channel transmissions. Packets with a mismatching destination logical address are never passed to the RMAP target. There is a user accessible destination key register which is compared to destination key field in incoming packets. If there is a mismatch and a reply has been requested the error code in the reply is set to 3. Replies are sent if and only if the ack field is set to ‘1’. 

GRIP Frontgrade Gaisler AB

Detection of all error codes except code 12 is supported. When a failure occurs during a bus access the error code is set to 1 (General Error). There is predetermined order in which error-codes are set in the case of multiple errors in the core. It is shown in table 1335. 

_Table 1335._The order of error detection in case of multiple errors in the GRSPW. The error detected first has number 1.

| **Detection Order** | **Error Code** | **Error** |
| ---: | ---: | --- |
| 1 | 2 | Unused RMAP packet type or command code |
| 2 | 3 | Invalid destination key |
| 3 | 9 | Verify buffer overrun |
| 4 | 11 | RMW data length error |
| 5 | 10 | Authorization failure |
| 6\* | 1 | General Error (AHB errors during non-verified writes) |
| 7 | 5/7 | Early EOP / EEP (if early) |
| 8 | 4 | Invalid Data CRC |
| 9 | 1 | General Error (AHB errors during verified writes or RMW) |
| 10 | 7 | EEP |
| 11 | 6 | Cargo Too Large |
| \*The AHB error is not guaranteed to be detected before Early EOP/EEP or Invalid Data CRC. For very long accesses the AHB error detection might be delayed causing the other two errors to appear first. | \*The AHB error is not guaranteed to be detected before Early EOP/EEP or Invalid Data CRC. For very long accesses the AHB error detection might be delayed causing the other two errors to appear first. | \*The AHB error is not guaranteed to be detected before Early EOP/EEP or Invalid Data CRC. For very long accesses the AHB error detection might be delayed causing the other two errors to appear first. |

Read accesses are performed on the fly, that is they are not stored in a temporary buffer before trans- mission. This means that the error code 1 will never be seen in a read reply since the header has already been sent when the data is read. If the AHB error occurs the packet will be truncated and ended with an EEP. 

Errors up to and including Invalid Data CRC (number 8) are checked before verified commands. The other errors do not prevent verified operations from being performed.

The details of the support for the different commands are now presented. All defined commands which are received but have an option set which is not supported in this specific implementation will not be executed and a possible reply is sent with error code 10.

**77.6.3** **Write commands**

The write commands are divided into two subcategories when examining their capabilities: verified writes and non-verified writes. Verified writes have a length restriction of 4 B and the address must be aligned to the size. That is 1 B writes can be done to any address, 2 B must be halfword aligned, 3 B are not allowed and 4 B writes must be word aligned. Since there will always be only one AHB oper- ation performed for each RMAP verified write command the incrementing address bit can be set to any value. 

Non-verified writes have no restrictions when the incrementing bit is set to 1. If it is set to 0 the num- ber of bytes must be a multiple of 4 and the address word aligned. There is no guarantee how many words will be written when early EOP/EEP is detected for non-verified writes.

**77.6.4** **Read commands**

Read commands are performed on the fly when the reply is sent. Thus if an AHB error occurs the packet will be truncated and ended with an EEP. There are no restrictions for incrementing reads but non-incrementing reads have the same alignment restrictions as non-verified writes. Note that the “Authorization failure” error code will be sent in the reply if a violation was detected even if the length field was zero. Also note that no data is sent in the reply if an error was detected i.e. if the status field is non-zero.

GRIP Frontgrade Gaisler AB

**77.6.5** **RMW commands**

All read-modify-write sizes are supported except 6 which would have caused 3 B being read and writ- ten on the bus. The RMW bus accesses have the same restrictions as the verified writes. As in the ver- ified write case, the incrementing bit can be set to any value since only one AHB bus operation will be performed for each RMW command. Cargo too large is detected after the bus accesses so this error will not prevent the operation from being performed. No data is sent in a reply if an error is detected i.e. the status field is non-zero.

**77.6.6** **Control**

The RMAP command handler mostly runs in the background without any external intervention, but there are a few control possibilities.

There is an enable bit in the control register of the core which can be used to completely disable the RMAP command handler. When it is set to ‘0’ no RMAP packets will be handled in hardware, instead they are all stored to the DMA channel. 

There is a possibility that RMAP commands will not be performed in the order they arrive. This can happen if a read arrives before one or more writes. Since the command handler stores replies in a buf- fer with more than one entry several commands can be processed even if no replies are sent. Data for read replies is read when the reply is sent and thus writes coming after the read might have been per- formed already if there was congestion in the transmitter. To avoid this the RMAP buffer disable bit can be set to force the command handler to only use one buffer which prevents this situation. The last control option for the command handler is the possibility to set the destination key which is found in a separate register.

GRIP Frontgrade Gaisler AB

_Table 1336._GRSPW hardware RMAP handling of different packet type and command fields.

| **Bit 7** | **Bit 6** | **Bit 5**  | **Bit 4** | **Bit 3** | **Bit 2** | **Command** | **Action** |
| ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| **Reserved** | **Command / Response** | **Write /Read** | **Verify data before write** | **Acknow-ledge** | **Increment Address** |  |  |
| 0 | 0 | - | - | - | - | Response | Stored to DMA-channel. |
| 0 | 1 | 0 | 0 | 0 | 0 | Not used | Does nothing. No reply is sent. |
| 0 | 1 | 0 | 0 | 0 | 1 | Not used | Does nothing. No reply is sent. |
| 0 | 1 | 0 | 0 | 1 | 0 | Read single address | Executed normally. Address has to be word aligned and data size a multiple of four. Reply is sent. If alignment restrictions are vio-lated error code is set to 10.  |
| 0 | 1 | 0 | 0 | 1 | 1 | Read incre-menting address. | Executed normally. No restric-tions. Reply is sent.  |
| 0 | 1 | 0 | 1 | 0 | 0 | Not used | Does nothing. No reply is sent. |
| 0 | 1 | 0 | 1 | 0 | 1 | Not used | Does nothing. No reply is sent. |
| 0 | 1 | 0 | 1 | 1 | 0 | Not used | Does nothing. Reply is sent with error code 2. |
| 0 | 1 | 0 | 1 | 1 | 1 | Read-Mod-ify-Write increment-ing address | Executed normally. If length is not one of the allowed rmw val-ues nothing is done and error code is set to 11. If the length was correct, alignment restric-tions are checked next. 1 byte can be rmw to any address. 2 bytes must be halfword aligned. 3 bytes are not allowed. 4 bytes must be word aligned. If these restrictions are violated nothing is done and error code is set to 10. If an AHB error occurs error code is set to 1. Reply is sent. |
| 0 | 1 | 1 | 0 | 0 | 0 | Write, sin-gle-address, do not verify before writ-ing, no acknowledge | Executed normally. Address has to be word aligned and data size a multiple of four. If alignment is violated nothing is done. No reply is sent. |
| 0 | 1 | 1 | 0 | 0 | 1 | Write, incre-menting address, do not verify before writ-ing, no acknowledge | Executed normally. No restric-tions. No reply is sent. |
| 0 | 1 | 1 | 0 | 1 | 0 | Write, sin-gle-address, do not verify before writ-ing, send acknowledge | Executed normally. Address has to be word aligned and data size a multiple of four. If alignment is violated nothing is done and error code is set to 10. If an AHB error occurs error code is set to 1. Reply is sent. |

GRIP Frontgrade Gaisler AB

_Table 1336._GRSPW hardware RMAP handling of different packet type and command fields.

| **Bit 7** | **Bit 6** | **Bit 5**  | **Bit 4** | **Bit 3** | **Bit 2** | **Command** | **Action** |
| ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| **Reserved** | **Command / Response** | **Write /Read** | **Verify data before write** | **Acknow-ledge** | **Increment Address** |  |  |
| 0 | 1 | 1 | 0 | 1 | 1 | Write, incre-menting address, do not verify before writ-ing, send acknowledge | Executed normally. No restric-tions. If AHB error occurs error code is set to 1. Reply is sent. |
| 0 | 1 | 1 | 1 | 0 | 0 | Write, single address, ver-ify before writing, no acknowledge | Executed normally. Length must be 4 or less. Otherwise nothing is done. Same alignment restric-tions apply as for rmw. No reply is sent. |
| 0 | 1 | 1 | 1 | 0 | 1 | Write, incre-menting address, ver-ify before writing, no acknowledge | Executed normally. Length must be 4 or less. Otherwise nothing is done. Same alignment restric-tions apply as for rmw. If they are violated nothing is done. No reply is sent. |
| 0 | 1 | 1 | 1 | 1 | 0 | Write, single address, ver-ify before writing, send acknowledge | Executed normally. Length must be 4 or less. Otherwise nothing is done and error code is set to 9. Same alignment restrictions apply as for rmw. If they are vio-lated nothing is done and error code is set to 10. If an AHB error occurs error code is set to 1. Reply is sent. |
| 0 | 1 | 1 | 1 | 1 | 1 | Write, incre-menting address, ver-ify before writing, send acknowledge | Executed normally. Length must be 4 or less. Otherwise nothing is done and error code is set to 9. Same alignment restrictions apply as for rmw. If they are vio-lated nothing is done and error code is set to 10. If an AHB error occurs error code is set to 1. Reply is sent. |
| 1 | 0 | - | - | - | - | Unused | Stored to DMA-channel. |
| 1 | 1 | - | - | - | - | Unused | Stored to DMA-channel. |

### 77.7 AMBA interface

The AMBA interface consists of an APB interface, an AHB master interface and DMA FIFOs. The APB interface provides access to the user registers which are described in section [77.9.](#779-axi-support) The DMA engines have 32-bit wide FIFOs to the AHB master interface which are used when reading and writ- ing to the bus.

The transmitter DMA engine reads data from the bus in bursts which are half the FIFO size in length. A burst is always started when the FIFO is half-empty or if it can hold the last data for the packet. The burst containing the last data might have shorter length if the packet is not an even number of bursts in size.

GRIP Frontgrade Gaisler AB

The receiver DMA works in the same way except that it checks if the FIFO is half-full and then per- forms a burst write to the bus which is half the fifo size in length. The last burst might be shorter. If the _rmap_ or _rxunaligned_ VHDL generics are set to 1 the interface also handles byte accesses. Byte accesses are used for non word-aligned buffers and/or packet lengths that are not a multiple of four bytes. There might be 1 to 3 single byte writes when writing the beginning and end of the received packets. 

**77.7.1** **APB slave interface**

As mentioned above, the APB interface provides access to the user registers which are 32-bits in width. The accesses to this interface are required to be aligned word accesses. The result is undefined if this restriction is violated.

**77.7.2** **AHB master interface**

The core contains a single master interface which is used by both the transmitter and receiver DMA engines. The arbitration algorithm between the channels is done so that if the current owner requests the interface again it will always acquire it. This will not lead to starvation problems since the DMA engines always deassert their requests between accesses.

The AHB accesses are always word accesses (HSIZE = 0x010) of type incremental burst with unspec- ified length (HBURST = 0x001) if VHDL generics rmap and rxunaligned are disabled. The AHB accesses can be of size byte, halfword and word (HSIZE = 0x000, 0x001, 0x010) otherwise. Byte and halfword accesses are always NONSEQ. Note that read accesses are always word accesses (HSIZE = 0x010), which can result in destructive read.

The burst length will be half the AHB FIFO size except for the last transfer for a packet which might be smaller. Shorter accesses are also done during descriptor reads and status writes. The AHB master also supports non-incrementing accesses where the address will be constant for sev- eral consecutive accesses. HTRANS will always be NONSEQ in this case while for incrementing accesses it is set to SEQ after the first access. This feature is included to support non-incrementing reads and writes for RMAP.

If the core does not need the bus after a burst has finished there will be one wasted cycle (HTRANS = IDLE). 

BUSY transfer types are never requested and the core provides full support for ERROR, RETRY and SPLIT responses.

**77.8** **Implementation**

**77.8.1** **Reset**

The core does not change reset behaviour depending on settings in the GRLIB configuration package (see GRLIB User’s Manual).

**77.8.2** **Clock-generation**

Figure 193 shows the clock recovery scheme for the receiver. Data and strobe are coupled directly from their pads to an xor gate which generates the clock. The output from the xor is then connected to a clock network. The specific type of clock network depends on the technology used. The xor gate is actually all that logically belongs to the Rx clock recovery module in figure 193. The clock output drives all flip-flops in the receiver module found in figure 189. The data signal which is used for generating the clock is also coupled to the data inputs of several flip-flops clocked

GRIP Frontgrade Gaisler AB

by the Rx clock as seen in figure 193. Care must be taken so that the delay from the data and strobe signals through the clock network are longer than the delay to the data input + setup time.

|  |  |  |  |  | D |
| --- | --- | --- | --- | --- | --- |
|  |  |  |  |  | D |
|  |  |  |  |  | D |
|  |  |  |  |  | D |
|  |  |  |  |  |  |
|  |  |  |  |  |  |
|  |  |  |  |  | D |
|  |  |  |  |  | D |
|  |  |  |  |  | D |
|  |  |  |  |  |  |
|  |  |  |  |  |  |

Q

D

Q

|  |  |
| --- | --- |
|  |  |

S

_Figure 193._ The clocking scheme for the receiver. The clock is 

The transmitter clock is generated from the txclk input. A separate clock input is used to allow the transmitter to be run at much higher frequencies than the system clock. The SpaceWire node contains a clock-divider which divides the txclk signal to the wanted frequency. The transmitter clock should be 10 MHz during initialization and any frequency above 2 MHz in the run-state. There is an input signal called clkdiv10 which sets the clock divisor value during initialization and the reset value for the user accessible clock divisor register. The user register value will be used in run- state. The resulting tx clock frequency will be txclk/(clock divisor value+1). So if no clock division is wanted, the clock divisor should be set to 0.

Since only integer values are allowed for the clock division and the required init-frequency is 10 Mhz the frequency of the txclk input must be a multiple of 10 MHz. The clock divisor value is 8-bits wide so the maximum txclk frequency supported is 2.56 GHz (note that there is also a restriction on the relation between the system and transmit clock frequencies).

**77.8.3** **Timers**

There are two timers in the core: one for generating the 6.4/12.8 us periods and one for disconnect timing. They run on the system (AMBA) clock and the frequency must be at least 10 MHz to guaran- tee disconnect timing limits. 

There are two user accessible registers which are used to the set the number of clock cycles used for the timeout periods. These registers are described in section [77.9.](#779-axi-support) 

The reset value for the timer registers can be set in two different ways selected by the usegen VHDL generic. If usegen is set to 1, the sysfreq VHDL generic is used to generate reset values for the discon- nect, 6.4 us and 12.8 us timers. Otherwise, the input signals dcrstval and timerrstval will be used as reset values. If the system clock frequency is 10 MHz or above the disconnect time will be within the limits specified in the SpaceWire standard. 

**77.8.4** **Synchronization**

The VHDL generic nsync selects how many synchronization registers are used between clock domains. The default is one and should be used when maximum performance is needed. It allows the transmitter to be clocked 4 times faster than the system clock and the receiver 2 times faster. These are theoretical values without consideration for clock skew and jitter. Note also that the receiver clocks data at both negative and positive edges. Thus, the bitrate is twice as high as the clock-rate. The synchronization limits the Tx and Rx clocks to be at most 4 and 2 times faster than the system clock. But it might not be possible to achieve such high clock rates for the Tx and Rx clocks for all technologies.

The asynchronous reset to the receiver clock domain has to have a maximum delay of one receiver clock cycle to ensure correct operation. This is needed because the receiver uses has a completely

GRIP Frontgrade Gaisler AB

asynchronous reset. To make sure that nothing bad happens the is a synchronous reset guard which prevents any signals from being assigned before all registers have their reset signals released.

**77.8.5** **Fault-tolerance**

The core can optionally be implemented with fault-tolerance against SEU errors in the FIFO memo- ries. The fault-tolerance is enabled through the _ft_ VHDL generic. Possible options are byte parity pro- tection (_ft = 1_) or TMR registers (_ft = 2_). Note: the GPL version of GRLIB does not include fault- tolerance, and the core will not work unless the _ft_ VHDL generic is 0.

**77.8.6** **Synthesis**

Since the receiver and transmitter may run on very high frequency clocks their clock signals have been coupled through a clock buffer with a technology wrapper. This clock buffer will utilize a low skew net available in the selected technology for the clock. 

The clock buffer will also enable most synthesis tools to recognize the clocks and it is thus easier to find them and place constraints on them. The fact there are three clock domains in the GRSPW of which all are possibly high frequency clocks makes it necessary to declare all paths between the clock domains as false paths. 

In Synplify this is most easily done by declaring all the clocks to be in different clockgroups in the sdc file (if Synplify does not automatically put them in different groups). This will disable any timing considerations between the clock domains and these constraints will also propagate to the place and route tool.

The type of clock buffer is selectable with a VHDL generic and the value zero provides a normal feed through which lets the synthesis tool infer the type of net used.

**77.8.7** **Technology mapping**

The core has three generics for technology mapping: _tech_, _techfifo_ and _memtech_. _Tech_ selects the tech- nology used for the clock buffers and also adds reset to some registers for technologies where they would otherwise cause problems with gate-level simulations. _Techfifo_ selects whether _memtech_ should be used to select the technology for the FIFO memories (the RMAP buffer is not affected by the this generic) or if they should be inferred. _Tech_ and _memtech_ can be set to any value from 0 to NTECH as defined in the GRLIB.TECH package.

**77.8.8** **RAM usage**

The core maps all RAM memories on the syncram\_2p component if the _ft_ generic is 0 and to the syn- cram\_2pft component for other values. The syncrams are located in the technology mapping library (TECHMAP). The organization of the different memories are described below. If techfifo and/or memtech is set to 0 the synthesis tool will infer the memories. Either RAM blocks or flip-flops will be used depending on the tool and technology. The number of flip-flops used is _syncram depth x syncram_ _width_ for all the different memories. The receiver AHB FIFO with fifosize 32 will for example use 1024 flips-flops. 

**Receiver ahb FIFO** The receiver AHB fifo consists of one syncram\_2p block with a width of 32-bits. The depth is deter- mined by the configured FIFO depth. Table 1337 shows the syncram organization for the allowed configurations.

GRIP Frontgrade Gaisler AB

_Table 1337._syncram\_2p sizes for GRSPW receiver AHB FIFO.

| **Fifosize** | **Syncram\_2p organization** |
| ---: | --- |
| 4 | 4x32 |
| 8 | 8x32 |
| 16 | 16x32 |
| 32 | 32x32 |

**Transmitter ahb FIFO** The transmitter AHB fifo consists of one syncram\_2p block with a width of 32-bits. The depth is determined by the configured FIFO depth. Table 1338 shows the syncram organization for the allowed configurations.

_Table 1338._syncram\_2p sizes for transmitter AHB FIFO.

| **Fifosize** | **Syncram\_2p organization** |
| ---: | --- |
| 4 | 4x32 |
| 8 | 8x32 |
| 16 | 16x32 |
| 32 | 32x32 |

**Receiver N-Char FIFO** The receiver N-Char fifo consists of one syncram\_2p block with a width of 10-bits. The depth is determined by the configured FIFO depth. Table 1339 shows the syncram organization for the allowed configurations.

_Table 1339._syncram\_2p sizes for the receiver N-Char FIFO.

| **Fifosize** | **Syncram\_2p organization** |
| ---: | --- |
| 16 | 16x10 |
| 32 | 32x10 |
| 64 | 64x10 |

**RMAP buffer** The RMAP buffer consists of one syncram\_2p block with a width of 8-bits. The depth is determined by the number of configured RMAP buffers. Table 1340 shows the syncram organization for the allowed configurations.

_Table 1340._syncram\_2p sizes for RMAP buffer memory.

| **RMAP buffers** | **Syncram\_2p organization** |
| --- | --- |
|  |  |
|  |  |
|  |  |

GRIP Frontgrade Gaisler AB

### 77.9 AXI support

|  | The core is designed  | for  | an  | AMBA  | system  | but  | can  | be  | adapted  | for  | AXI  | using  | the  | AHBM2AXI |
| ---: | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  | adapter. |  |  |  |  |  |  |  |  |  |  |  |  |  |
| **77.10** | **Registers** |  |  |  |  |  |  |  |  |  |  |  |  |  |

The core is programmed through registers mapped into APB address space.

_Table 1341._GRSPW registers

| **APB address offset** | **Register** |
| --- | --- |
| 0x0 | Control |
| 0x4 | Status/Interrupt-source |
| 0x8 | Node address |
| 0xC | Clock divisor |
| 0x10 | Destination key |
| 0x14 | Time |
| 0x18 | Timer and Disconnect |
| 0x20 | DMA channel 1 control/status |
| 0x24 | DMA channel 1 rx maximum length |
| 0x28 | DMA channel 1 transmit descriptor table address. |
| 0x2C | DMA channel 1 receive descriptor table address. |

GRIP Frontgrade Gaisler AB

### 77.10.1 Control Register

_Table 1342._0x00 - CTRL - control register

31 30 29 28 27 26 25 22 21 20 19 18 17 16 15 12 11 10 9 8 7 6 5 4 3 2 1 0

| RA | RX | RC | R | PO | RESERVED | PS | NP | R | RD | RE | RESERVED | TR | TT | LI | TQ | R | RS | PM | TI | IE | AS | LS | LD |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| \* | \* | \* | 0 | \* | 0 | 0 | \* | 0 | 0 | \* | 0 | 0 | 0 | NR | NR | 0 | 0 | 0 | 0 | 0 | \* | 0 | 0 |
| r | r | r | r | r | r | rw\* | rw\* | r | rw\* | rw\* | r | rw | rw | rw | rw | rw | rw\* | rw | rw | rw | rw | rw | rw |

| 31 |  | RMAP available (RA) - Set to one if the RMAP command handler is available. Only readable. |
| ---: | ---: | --- |
| 30 |  | RX unaligned access (RX) - Set to one if unaligned writes are available for the receiver. Only read- |
|  |  | able. |
| 29 |  | RMAP CRC available (RC) - Set to one if RMAP CRC is enabled in the core. Only readable. |
| 28: | 27 | RESERVED |
| 26 |  | Number of ports (PO) - The number of available SpaceWire ports minus one. Only readable. |
| 25: | 22 | RESERVED |
| 21 |  | Port select (PS) - Selects the active port when the no port force bit is zero. ‘0’ selects the port con- |

nected to data and strobe on index 0 while ‘1’ selects index 1. Only available if the ports VHDL generic is set to 2. Reset value: ‘0’.

20 No port force (NP) - Disable port force. When disabled the port select bit cannot be used to select the active port. Instead, it is automatically selected by checking the activity on the respective receive links. Only available if the ports VHDL generic is set to 2. Reset value: ‘0’ if the RMAP command handler is not available. If available the reset value is set to the value of the rmapen input signal.

19: 18 RESERVED 17 RMAP buffer disable (RD) - If set only one RMAP buffer is used. This ensures that all RMAP com-

mands will be executed consecutively. Only available if the rmap VHDL generic is set to 1. Reset value: ‘0’.

| 16 | RMAP Enable (RE) - Enable RMAP command handler. Only available if rmap VHDL generic is set  |
| ---: | --- |
|  | to 1. Reset value: ‘1’.  |
| 15: 12 | RESERVED |
| 11 | Time Rx Enable (TR) - Enable time-code receptions. Reset value: ‘0’. |
| 10 | Time Tx Enable (TT) - Enable time-code transmissions. Reset value: ‘0’. |

9 Link error IRQ (LI) - Generate interrupt when a link error occurs. Not reset. 8 Tick-out IRQ (TQ) - Generate interrupt when a valid time-code is received. Not reset. 7 RESERVED

6 Reset (RS) - Make complete reset of the SpaceWire node. Self clearing. Reset value: ‘0’. 5 Promiscuous Mode (PM) - Enable Promiscuous mode. Reset value: ‘0’.

4 Tick In (TI) - The host can generate a tick by writing a one to this field. This will increment the timer counter and the new value is transmitted after the current character is transferred. A tick can also be generated by asserting the tick\_in signal. Reset value: ‘0’.

3 Interrupt Enable (IE) - If set, an interrupt is generated when one or both of bit 8 to 9 is set and its cor- responding event occurs. Reset value: ‘0’.

2 Autostart (AS) - Automatically start the link when a NULL has been received. Reset value: ‘0’ if the RMAP command handler is not available. If available the reset value is set to the value of the rmapen input signal.

1 Link Start (LS) - Start the link, i.e. allow a transition from ready to started state. Reset value: ‘0’. 0 Link Disable (LD) - Disable the SpaceWire codec. Reset value: ‘0’.

GRIP Frontgrade Gaisler AB

| **77.10.2** | **Status Register** |  |
| ---: | --- | ---: |
| _Table 1343._ | 0x04 - STS - status register |  |
| 31 | 24 | 23 21 20 10 9 8 7 6 5 4 3 2 1 0 |

| RESERVED | LS | RESERVED | AP | EE | IA | WE | R | PE | DE | ER | CE | TO |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| r | r | r | rw\* | wc | wc | wc | r | wc | wc | wc | wc | wc |

| 31: | 24 | RESERVED |
| ---: | ---: | --- |
| 23: | 21 | Link State (LS) - The current state of the start-up sequence. 0 = Error-reset, 1 = Error-wait, 2 =  |
|  |  | Ready, 3 = Started, 4 = Connecting, 5 = Run. Reset value: 0. |
| 20: | 10 | RESERVED |

9 Active port (AP) - Shows the currently active port. ‘0’ = Port 0 and ‘1’ = Port 1 where the port num- bers refer to the index number of the data and strobe signals. Only available if the ports generic is set to 2.

8 Early EOP/EEP (EE) - Set to one when a packet is received with an EOP after the first byte for a non-rmap packet and after the second byte for a RMAP packet. Cleared when written with a one. Reset value: ‘0’.

7 Invalid Address (IA) - Set to one when a packet is received with an invalid destination address field, i.e it does not match the nodeaddr register. Cleared when written with a one. Reset value: ‘0’.

6 Write synchronization Error (WE) - A synchronization problem has occurred when receiving N- Chars. Cleared when written with a one. Reset value: ‘0’.

5 RESERVED 4 Parity Error (PE) - A parity error has occurred. Cleared when written with a one. Reset value: ‘0’. 3 Disconnect Error (DE) - A disconnection error has occurred. Cleared when written with a one. Reset 

value: ‘0’. 2 Escape Error (ER) - An escape error has occurred. Cleared when written with a one. Reset value: ‘0’. 1 Credit Error (CE) - A credit has occurred. Cleared when written with a one. Reset value: ‘0’. 0 Tick Out (TO) - A new time count value was received and is stored in the time counter field. Cleared 

|  | when written with a one. Reset value: ‘0’. |
| ---: | --- |
| **77.10.3** | **Node Address Register** |
| _Table 1344._ | 0x08 - NODEADDR - node address register |
| 31 | 8 7 0 |

| RESERVED | NODEADDR |
| --- | --- |
| 0 | \* |
| r | rw |

31: 8 RESERVED 7: 0 Node address (NODEADDR) - 8-bit node address used for node identification on the SpaceWire 

network. Reset value: 254 (taken from the nodeaddr VHDL generic when /= 255, else from the rmapnodeaddr input signal)

GRIP Frontgrade Gaisler AB

| **77.10.4** | **Clock Divisor Register** |
| ---: | --- |
| _Table 1345._ | 0x0C - CLKDIV - clock divisor register |
| 31 | 16 15 8 7 0 |

| RESERVED | CLKDIVSTART | CLKDIVRUN |
| --- | --- | --- |
| 0 | \* | \* |
| r | rw | rw |

31: 16 RESERVED 15: 8 Clock divisor startup (CLKDIVSTART) - 8-bit Clock divisor value used for the clock-divider 

during startup (link-interface is in other states than run). The actual divisor value is Clock Divi- sor register + 1. Reset value: clkdiv10 input signal.

7: 0 Clock divisor run (CLKDIVRUN) - 8-bit Clock divisor value used for the clock-divider when the link-interface is in the run-state. The actual divisor value is Clock Divisor register + 1. Reset value: clkdiv10 input signal.

| **77.10.5** | **Destination Key** |
| ---: | --- |
| _Table 1346._ | 0x10 - DKEY - destination key |
| 31 | 8 7 0 |

| RESERVED | DESTKEY |
| --- | --- |
| 0 | \* |
| r | rw |

31: 8 RESERVED 7: 0 Destination key (DESTKEY) - RMAP destination key. Only available if the rmap VHDL generic is 

set to 1. Reset value: 0 (taken from the deskey VHDL generic)

### 77.10.6 Time Register

_Table 1347._0x14 - TIME - time register

31 8 7 6 5 0

| RESERVED | TCTRL | TIMECNT |
| --- | --- | --- |
| 0 | 0 | 0 |
| r | rw | rw |

| 31: | 8 | RESERVED |
| --- | --- | --- |
| 7: | 6 | Time control flags (TCTRL) - The current value of the time control flags. Sent with time-code  |
|  |  | resulting from a tick-in. Received control flags are also stored in this register. Reset value: ‘0’. |
| 5: | 0 | Time counter (TIMECNT) - The current value of the system time counter. It is incremented for each  |

tick-in and the incremented value is transmitted. The register can also be written directly but the written value will not be transmitted. Received time-counter values are also stored in this register. Reset value: ‘0’.

GRIP Frontgrade Gaisler AB

| **77.10.7** | **Timer and Disconnect Register** |
| ---: | --- |
| _Table 1348._ | 0x18 - TDR - timer and disconnect register. |
| 31 | 22 21 12 11 0 |

| RESERVED | DISCONNECT | TIMER64 |
| --- | --- | --- |
|  |  |  |
|  |  |  |

31: 22 RESERVED 21: 12 Disconnect (DISCONNECT) - Used to generate the 850 ns disconnect time period. The disconnect 

period is the number is the number of clock cycles in the disconnect register + 3. So to get a 850 ns period, the smallest number of clock cycles that is greater than or equal to 850 ns should be calcu- lated and this values - 3 should be stored in the register. Reset value is set with VHDL generics or with input signals depending on the value of the usegen VHDL generic.

11: 0 6.4 us timer (TIMER64) - Used to generate the 6.4 and 12.8 us time periods. Should be set to the smallest number of clock cycles that is greater than or equal to 6.4 us. Reset value is set with VHDL 

generics or with input signals depending on the value of the usegen VHDL generic.

| **77.10.8** | **DMA Control Register** |
| ---: | --- |
| _Table 1349._ | 0x20 - DMACTRL - dma control register |
| 31 | 17 16 15 13 12 11 10 9 8 7 6 5 4 3 2 1 0 |

| RESERVED | LE | RESERVED | NS | RD | RX | AT | RA | TA | PR | PS | AI | RI | TI | RE | TE |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | NR | NR | NR | 0 | 0 |
| r | rw | r | rw | rw | r | r | wc | wc | wc | wc | rw | rw | rw | rw | rw |

| 31: | 17 | RESERVED |
| ---: | ---: | --- |
| 16 |  | Link error disable (LE) - Disable transmitter when a link error occurs. No more packets will be trans- |
|  |  | mitted until the transmitter is enabled again. Reset value: ‘0’. |
| 15: | 13 | RESERVED |
| 12 |  | No spill (NS) - If cleared, packets will be discarded when a packet is arriving and there are no active  |
|  |  | descriptors. If set, the GRSPW will wait for a descriptor to be activated. |
| 11 |  | Rx descriptors available (RD) - Set to one, to indicate to the GRSPW that there are enabled descrip- |

tors in the descriptor table. Cleared by the GRSPW when it encounters a disabled descriptor: Reset value: ‘0’.

10 RX active (RX) - Is set to ‘1’ if a reception to the DMA channel is currently active otherwise it is ‘0’. Only readable.

9 Abort TX (AT) - Set to one to abort the currently transmitting packet and disable transmissions. If no transmission is active the only effect is to disable transmissions. Self clearing. Reset value: ‘0’.

8 RX AHB error (RA) - An error response was detected on the AHB bus while this receive DMA 

channel was accessing the bus. Cleared when written with a one. Reset value: ‘0’.

7 TX AHB error (TA) - An error response was detected on the AHB bus while this transmit DMA 

channel was accessing the bus. Cleared when written with a one. Reset value: ‘0’.

6 Packet received (PR) - This bit is set each time a packet has been received. never cleared by the SW- node. Cleared when written with a one. Reset value: ‘0’.

5 Packet sent (PS) - This bit is set each time a packet has been sent. Never cleared by the SW-node. Cleared when written with a one. Reset value: ‘0’.

4 AHB error interrupt (AI) - If set, an interrupt will be generated each time an AHB error occurs when this DMA channel is accessing the bus. Not reset.

3 Receive interrupt (RI) - If set, an interrupt will be generated each time a packet has been received. This happens both if the packet is terminated by an EEP or EOP. Not reset.

2 Transmit interrupt (TI) - If set, an interrupt will be generated each time a packet is transmitted. The interrupt is generated regardless of whether the transmission was successful or not. Not reset.

GRIP Frontgrade Gaisler AB

_Table 1349._0x20 - DMACTRL - dma control register

| 1 | Receiver enable (RE) - Set to one when packets are allowed to be received to this channel. Reset  |
| --- | --- |
|  | value: ‘0’. |
| 0 | Transmitter enable (TE) - Write a one to this bit each time new descriptors are activated in the table.  |

Writing a one will cause the SW-node to read a new descriptor and try to transmit the packet it points to. This bit is automatically cleared when the SW-node encounters a descriptor which is disabled. Reset value: ‘0’.

| **77.10.9** | **RX Maximum Length Register** |
| ---: | --- |
| _Table 1350._ | 0x24 - DMAMAXLEN - RX maximum length register. |
| 31 | 25 24 2 1 0 |

| RESERVED | RXMAXLEN | R |
| --- | --- | --- |
| 0 | NR | 0 |
| r | rw | r |

| 31: | 25 | RESERVED |
| ---: | --- | --- |
| 24: | 2 | RX maximum length (RXMAXLEN) - Receiver packet maximum length in bytes. Only bits 24 - 2  |
|  |  | are writable. Bits 1 - 0 are always 0. Not reset. |
| 1: | 0 | RESERVED |

### 77.10.10Transmitter Descriptor Table Address Register

_Table 1351._0x28 - DMATYDESC - transmitter descriptor table address register.

31 10 9 4 3 0

| DESCBASEADDR | DESCSEL | RESERVED |
| --- | --- | --- |
| NR | 0 | 0 |
| rw | rw | r |

| 31: | 10 | Descriptor table base address (DESCBASEADDR) - Sets the base address of the descriptor table.  |
| ---: | ---: | --- |
|  |  | Not reset. |
| 9: | 4 | Descriptor selector (DESCSEL) - Offset into the descriptor table. Shows which descriptor is cur- |

rently used by the GRSPW. For each new descriptor read, the selector will increase with 16 and eventually wrap to zero again. Reset value: 0. 

3: 0 RESERVED

### 77.10.11Receiver Descriptor Table Address Register

_Table 1352._0x2C - DMARXDESC - receiver descriptor table address register.

31 10 9 3 2 0

| DESCBASEADDR | DESCSEL | RESERVED |
| --- | --- | --- |
| NR | 0 | 0 |
| rw | rw | r |

| 31: | 10 | Descriptor table base address (DESCBASEADDR) - Sets the base address of the descriptor table.  |
| ---: | ---: | --- |
|  |  | Not reset. |
| 9: | 3 | Descriptor selector (DESCSEL) - Offset into the descriptor table. Shows which descriptor is cur- |

rently used by the GRSPW. For each new descriptor read, the selector will increase with 8 and even- tually wrap to zero again. Reset value: 0.

2: 0 RESERVED

### 77.11 Vendor and device identifiers

The core has vendor identifier 0x01 (Frontgrade Gaisler) and device identifier 0x1F. For description of vendor and device identifiers see GRLIB IP Library User’s Manual.

GRIP Frontgrade Gaisler AB

### 77.12 Implementation

### 77.12.1 Endianness

The core does not change endianness behaviour depending on the settings in the GRLIB configuration package (see GRLIB User’s Manual). The core is designed for big-endian systems.

### 77.13 Configuration options

Table [1353 ](#7712-implementation)shows the configuration options of the core (VHDL generics).

_Table 1353._Configuration options

| **Generic** | **Function** | **Allowed range** | **Default** |
| --- | --- | --- | ---: |
| tech | Technology for clock buffers | 0 - NTECH | inferred |
| hindex | AHB master index. | 0 - NAHBMST-1 | 0 |
| pindex | APB slave index | 0 - NAPBSLV-1 | 0 |
| paddr | Addr field of the APB bar. | 0 - 16#FFF# | 0 |
| pmask | Mask field of the APB bar. | 0 - 16#FFF# | 16#FFF# |
| pirq | Interrupt line used by GRSPW. | 0 - NAHBIRQ-1 | 0 |
| sysfreq | Frequency of clock input “clk” in kHz. | - | 10000 |
| usegen | Use values calculated from sysfreq generic as reset values for 6.4 us timer and disconnect timer.  | 0 - 1 | 1 |
| nsync | Number of synchronization registers.Warning: Value 2 only to be used when bit rate is equal or less than the system clock frequency. | 1 - 2 | 1 |
| rmap | Include hardware RMAP target. RMAP CRC logic will also be added. If set to 2 the core will only implement the RMAP target, provide a limited APB interface, enable time code recep-tion and its interrupt. | 0 - 2 | 0 |
| rmapcrc | Enable RMAP CRC logic.  | 0 - 1 | 0 |
| fifosize1 | Sets the number of entries in the 32-bit receiver and trans-mitter AHB fifos. | 4 - 32 | 32 |
| fifosize2 | Sets the number of entries in the 9-bit receiver fifo (N-Char fifo). | 16 - 64 | 64 |
| rxclkbuftype | Select clock buffer type for receiver clock. 0 does not select a buffer, instead i connects the input directly to the output (synthesis tools may still infer a buffer). 1 selects hardwired clock while 2 selects routed clock. | 0 - 2 | 0 |
| rxunaligned | Receiver unaligned write support. If set, the receiver can write any number of bytes to any start address without writing any excessive bytes. | 0 - 1 | 0 |
| rmapbufs | Sets the number of buffers to hold RMAP replies. | 2 - 8 | 4 |
| ft | Enable fault-tolerance against SEU errors | 0 - 2 | 0 |
| scantest | Enable support for scan test | 0 - 1 | 0 |
| techfifo | Implement FIFO with RAM cells (1) or flip-flops (0) | 0 - 1 | 1 |
| netlist | Use netlist rather then RTL code | 0 - 1 | 0 |
| ports | Sets the number of ports | 1 - 2 | 1 |
| memtech | Technology for RAM blocks | 0 - NTECH | inferred |
| nodeaddr | Sets the reset value for the core’s node address.Value 255 enables rmapnodeaddr input instead. | 0 - 254255 | 254 |
| destkey | Sets the reset value for the core’s destination key. | 0 - 255 | 0 |

GRIP Frontgrade Gaisler AB

### 77.14 Signal descriptions

Table [1354 ](#7714-signal-descriptions)shows the interface signals of the core (VHDL ports). As indicated in the table the core consists of two different entities, called GRSPW and GRSPW\_PHY. The GRSPW entity is the main part and includes most core’s functionality, while the GRSPW\_PHY only handles the receiver clock generation and the lower parts of the PHY layer. One GRSPW\_PHY entity is used per port. See sec- tion 77.18 for information on how to interface GRSPW with GRSPW\_PHY.

_Table 1354._ Signal descriptions

| **Entity** | **Signal name** | **Field** | **Type** | **Function** | **Active** |
| --- | --- | --- | --- | --- | --- |
| GRSPW | RST | N/A | Input | Reset | Low |
| GRSPW | CLK | N/A | Input | Clock | - |
| GRSPW | RXCLK\[1:0\] | N/A | Input | Receiver clock. One clock per port. | - |
| GRSPW | TXCLK | N/A | Input | Transmitter default run-state clock | - |
| GRSPW | AHBMI | \* | Input | AHB master input signals | - |
| GRSPW | AHBMO | \* | Output | AHB master output signals | - |
| GRSPW | APBI | \* | Input | APB slave input signals | - |
| GRSPW | APBO | \* | Output | APB slave output signals | - |
| GRSPW | SWNI | D\[1:0\] | Input | Data input synchronous to RXCLK (ris-ing edge). One bit per port. | - |
| GRSPW | SWNI | ND\[9:0\] | Input | Data input synchronous to RXCLK (fall-ing edge). Five bits per port. |  |
| GRSPW | SWNI | TICKIN | Input | Time counter tick input | High |
| GRSPW | SWNI | CLKDIV10 | Input | Clock divisor value used during initial-ization and as reset value for the clock divisor register | - |
| GRSPW | SWNI | RMAPEN | Input | Reset value for the rmapen control regis-ter bit | - |
| GRSPW | SWNI | RMAPNODEADDR | Input | Reset value for nodeaddr register bits when nodeaddr VHDL generic /= 255 | - |
| GRSPW | SWNI | DCRSTVAL | Input | Reset value for disconnect timer. Used if usegen VHDL generic is set to 0. | - |
| GRSPW | SWNI | TIMERRSTVAL | Input | Reset value for 6.4 us timer. Used if use-gen VHDL generic is set to 0. | - |
| GRSPW | SWNI | DCONNECT\[3:0\] | Input | Disconnect strobes. Two bits per port. |  |
| GRSPW | SWNO | D\[1:0\] | Output | SpaceWire data output. One bit per port. | - |
| GRSPW | SWNO | S\[1:0\] | Output | SpaceWire strobe output. One bit per port. | - |
| GRSPW | SWNO | TICKOUT | Output | Time counter tick output | High |
| GRSPW | SWNO | LINKDIS | Output | Linkdisabled status | High |
| GRSPW | SWNO | RMAPACT | Output | RMAP command processing active | High |
| GRSPW | SWNO | RXRST | Output | Receiver reset. | Low |

GRIP Frontgrade Gaisler AB

_Table 1354._ Signal descriptions

| **Entity** | **Signal name** | **Field** | **Type** | **Function** | **Active** |
| --- | --- | --- | --- | --- | --- |
| GRSPW\_PHY | RXRST | N/A | Input | Receiver reset. | Low |
| GRSPW\_PHY | DI | N/A | Input | SpaceWire data input. | - |
| GRSPW\_PHY | SI | N/A | Input | SpaceWire strobe input. | - |
| GRSPW\_PHY | RXCLKO | N/A | Output | Receiver clock recovered from data and strobe input. | - |
| GRSPW\_PHY | DO | N/A | Ouput | Recovered data, synchronous to RXCLKO (rising edge). | - |
| GRSPW\_PHY | NDO\[4:0\] | N/A | Ouput | Recovered data, synchronous to RXCLKO (falling edge) | - |
| GRSPW\_PHY | DCON-NECT\[1:0\] | N/A | Ouput | Disconnect strobe signals. | - |
| GRSPW\_PHY | TESTEN | N/A | Input | Scan test enable | High |
| GRSPW\_PHY | TESTCLK | N/A | Input | Scan test clock. Used inside the GRSP-W\_PHY entity instead of recovered RXCLK when TESTEN is active. | - |
|  | \* see GRLIB IP Library User’s Manual | \* see GRLIB IP Library User’s Manual | \* see GRLIB IP Library User’s Manual | \* see GRLIB IP Library User’s Manual | \* see GRLIB IP Library User’s Manual |

### 77.15 Signal definitions and reset values

The signals and their reset values are described in table [1355.](#7715-signal-definitions-and-reset-values) 

_Table 1355._Signal definitions and reset values

| **Signal name** | **Type** | **Function** | **Active** | **Reset value** |
| --- | --- | --- | --- | --- |
| spw\_clk | Input | Transmitter default run-state clock | Rising edge | - |
| spw\_rxd | Input, LVDS | Data input, positive | High | - |
| spw\_rxdn | Input, LVDS | Data input, negative | Low | - |
| spw\_rxs | Input, LVDS | Strobe input, positive | High | - |
| spw\_rxsn | Input, LVDS | Strobe input, negative | Low | - |
| spw\_txd | Output, LVDS | Data output, positive | High | Logical 0 |
| spw\_txdn | Output, LVDS | Data output, negative | Low | Logical 1 |
| spw\_txs | Output, LVDS | Strobe output, positive | High | Logical 0 |
| spw\_txsn | Output, LVDS | Strobe output, negative | Low | Logical 1 |

### 77.16 Timing

The timing waveforms and timing parameters are shown in figure 194 and are defined in table 1356. The SpaceWire jitter and skew timing waveforms and timing parameters are shown in figure 195 and are defined in table 1357.

GRIP Frontgrade Gaisler AB

tSPW0 spw\_clk

|  |  |
| --- | --- |
|  |  |
|  |  |
|  |  |
|  |  |
|  |  |
|  |  |
|  |  |

spw\_txd, spw\_txdn tSPW1 tSPW1 spw\_txs, spw\_txsn

spw\_rxd, spw\_rxdn tSPW2 tSPW3 spw\_rxs, spw\_rxsn

- tSPW4 tSPW4 spw\_txd, spw\_txdn

|  |
| --- |
|  |

tSPW4 spw\_txs, spw\_txsn

- tSPW5 tSPW5 spw\_rxd, spw\_rxdn

|  |  |
| --- | --- |
|  |  |

| tSPW6 |
| --- |
|  |

tSPW5 spw\_rxs, spw\_rxsn

tSPW6 tSPW7

|  |  |  |  |
| --- | --- | --- | --- |
|  |  |  |  |
|  |  |  |  |

|  |  |  |  |
| --- | --- | --- | --- |
|  |  |  |  |
|  |  |  |  |

spw\_txd, spw\_txdn

spw\_txs, spw\_txsn

_Figure 194._ Timing waveforms

_Table 1356._Timing parameters

| **Name** | **Parameter** | **Reference edge** | **Min** | **Max** | **Unit** |
| --- | --- | --- | --- | --- | --- |
| tSPW0 | transmit clock period | - | TBD | - | ns |
| tSPW1 | clock to output delay | rising _spw\_clk_ edge | TBD | TBD | ns |
| tSPW2 | input to clock hold | - | - | - | not applicable |
| tSPW3 | input to clock setup | - | - | - | not applicable |
| tSPW4 | output data bit period | - | - |  | _clk_ periods |
| tSPW4 | output data bit period | - | tSPW0 - TBD | tSPW0 +TBD | ns |
| tSPW5 | input data bit period | - | TBD | - | ns |
| tSPW6 | data & strobe edge separation | - | TBD | - | ns |
| tSPW7 | data & strobe output skew | - | - | TBD | ns |

GRIP Frontgrade Gaisler AB

_d_ _ideal_ _s_ _ideal_

d s

_receive clock_ t_skew_

t_jitter_ t_dclk_ t_hold_ t_jitter_ t_ds_

t_ui_ _Figure 195._ Skew and jitter timing waveforms

_Table 1357._Skew and jitter timing parameters

| **Name** | **Parameter** | **Reference edge** | **Min** | **Max** | **Unit** |
| --- | --- | --- | --- | --- | --- |
| tskew | skew between data and strobe | - | - | TBD | ns |
| tjitter | jitter on data or strobe | - | - | TBD | ns |
| tds | minimum separation between data and strobe edges | - | TBD | - | ns |
| tdclk | delay from edge of data or strobe to the receiver flip-flop | - | - | TBD | ns |
| thold | hold timer on receiver flip-flop | - | TBD | - | ns |
| tui | unit interval (bit period) | - | TBD | - | ns |

**77.17** **Library dependencies** Table 1358 shows libraries used when instantiating the core (VHDL libraries).

_Table 1358._Library dependencies

| **Library** | **Package** | **Imported unit(s)** | **Description** |
| --- | --- | --- | --- |
| GRLIB | AMBA | Signals | AMBA signal definitions |
| GAISLER | SPACEWIRE | Signals, component | Component and record declarations. |

**77.18** **Instantiation** This example shows how the core can be instantiated. 

Normally di, si, do and so should be connected to input and output pads configured with LVDS driv- ers. How this is done is technology dependent.

The GRSPW in the example is a 2-port core configured with non-ft memories of size 4, 64 and 8 entries for AHB FIFOs, N-Char FIFO and RMAP buffers respectively. The system frequency (clk) is 40 MHz and the transmitter frequency (txclk) is 20 MHz. 

The memory technology is inferred which means that the synthesis tool will select the appropriate components. The rx clk buffer uses a hardwired clock.

The hardware RMAP command handler is enabled which also automatically enables rxunaligned and rmapcrc. Finally, the DMA channel interrupt line is 2 and the number of synchronization registers is

GRIP Frontgrade Gaisler AB

library ieee; use ieee.std\_logic\_1164.all;

library grlib; use grlib.amba.all;

use grlib.tech.all; library gaisler;

use gaisler.spacewire.all;

entity spacewire\_ex is port (

 clk : in std\_ulogic; rstn : in std\_ulogic;

 -- spacewire signals

 di : in std\_logic\_vector(1 downto 0); si : in std\_logic\_vector(1 downto 0); do : out std\_logic\_vector(1 downto 0); so : out std\_logic\_vector(1 downto 0) );

end;

architecture rtl of spacewire\_ex is

 -- AMBA signals signal apbi : apb\_slv\_in\_type;

 signal apbo : apb\_slv\_out\_vector := (others =\> apb\_none); signal ahbmi : ahb\_mst\_in\_type;

 signal ahbmo : ahb\_mst\_out\_vector := (others =\> ahbm\_none); 

 -- Spacewire signals signal swni : grspw\_in\_type;

 signal swno : grspw\_out\_type; signal rxclk : std\_logic\_vector(1 downto 0); 

begin 

 -- AMBA Components are instantiated here ...

 -- GRSPW sw0 : grspw

 generic map (tech =\> inferred, hindex =\> 5, pindex =\> 7, paddr =\> 7, nsync =\> 1, rmap =\> 1, rxunaligned =\> 0, rmapcrc =\> 0, rxclkbuftype =\> 0, sysfreq =\> 40000, pirq =\> 2, fifosize1 =\> 4, fifosize2 =\> 64, rmapbufs =\> 8, ft =\> 0, ports =\> 2) port map (rstn, clk, rxclk, apbi, apbo(7), ahbmi, ahbmo(5), swni, swno);

phy0 : grspw\_phy generic map (tech =\> inferred, rxclkbuftype =\> 0, scantest =\> 0) port map (rxrst =\> swno.rxrst, di =\> di(0), si =\> si(0), rxclko =\> rxclk(0), do =\> swni.d(0), ndo =\> swni.nd(4 downto 0), dconnect =\> swni.dconnect(1 downto 0));

phy1 : grspw\_phy generic map (tech =\> inferred, rxclkbuftype =\> 0) port map (rxrst =\> swno.rxrst, di =\> di(1), si =\> si(1), rxclko =\> rxclk(1), do =\> swni.d(1), ndo =\> swni.nd(9 downto 5), dconnect =\> swni.dconnect(3 downto 2));

swni.rmapen \<= ‘1’; swni.clkdiv10 \<= “00000001”;

swni.tickin \<= ‘0’; do(0) \<= swno.d(0);

so(0) \<= swno.s(0); do(1) \<= swno.d(1);

so(1) \<= swno.s(1); end;

GRIP Frontgrade Gaisler AB

### 77.19 API

A simple Application Programming Interface (API) is provided together with the GRSPW. The API is located in $(GRLIB)/software/spw. The files are rmapapi.c, spwapi.c, rmapapi.h, spwapi.h. The spwapi.h file contains the declarations of the functions used for configuring the GRSPW and transfer- ring data. The corresponding definitions are located in spwapi.c. The rmapapi is structured in the same manner and contains a function for building RMAP packets.

These functions could be used as a simple starting point for developing drivers for the GRSPW. The different functions are described in this section.

### 77.19.1 GRSPW Basic API

The basic GRSPW API is based on a struct spwvars which stores all the information for a single GRSPW core. The information includes its address on the AMBA bus as well as SpaceWire parame- ters such as node address and clock divisor. A pointer to this struct is used as a input parameter to all the functions. If several cores are used, a separate struct for each core is created and used when the specific core is accessed.

_Table 1359._The spwvars struct

| **Field** | **Description** | **Allowed range** |
| --- | --- | --- |
| regs | Pointer to the GRSPW | - |
| nospill | The nospill value used for the core. | 0 - 1 |
| rmap | Indicates whether the core is configured with RMAP. Set by spw\_init. | 0 - 1 |
| rxunaligned | Indicates whether the core is configured with rxunaligned support. Set by spw\_init. | 0 - 1 |
| rmapcrc | Indicates whether the core is configured with RMAPCRC support. Set by spw\_init. | 0 - 1 |
| clkdiv | The clock divisor value used for the core. | 0 - 255 |
| nodeaddr | The node address value used for the core. | 0 - 255 |
| destkey | The destination key value used for the core. | 0 - 255 |
| rxmaxlen | The Receiver maximum length value used for the core. | 0 - 33554431 |
| rxpnt | Pointer to the next receiver descriptor. | 0 - 127 |
| rxchkpnt | Pointer to the next receiver descriptor that will be polled. | 0 - 127 |
| txpnt | Pointer to the next transmitter descriptor. | 0 - 63 |
| txchkpnt | Pointer to the next transmitter descriptor that will be polled. | 0 - 63 |
| timetxen | The timetxen value used for this core. | 0 - 1 |
| timerxen | The timerxen value used for this core. | 0 - 1 |
| txd | Pointer to the transmitter descriptor table. | - |
| rxd | Pointer to the receiver descriptor table | - |

### The following functions are available in the basic API:

```
int spw_setparam(int nodeaddr, int clkdiv, int destkey, int nospill, int timetxen, int timerxen, int rxmaxlen, int spwadr, struct spwvars *spw);
```

GRIP Frontgrade Gaisler AB

Used for setting the different parameters in the spwvars struct. Should always be run first after creat- ing a spwvars struct. This function only initializes the struct. Does not write anything to the Space- Wire core.

_Table 1360._Return values for spw\_setparam

| **Value** | **Description** |
| ---: | --- |
| 0 | The function completed successfully |
| 1 | One or more of the parameters had an illegal value |

_Table 1361._Parameters for spw\_setparam

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| nodeaddr | Sets the node address value of the struct spw passed to the function. | 0-255 |
| clkdiv | Sets the clock divisor value of the struct spw passed to the function. | 0-255 |
| destkey | Sets the destination key of the struct spw passed to the function. | 0-255 |
| nospill | Sets the nospill value of the struct spw passed to the function. | 0 - 1 |
| timetxen | Sets the timetxen value of the struct spw passed to the function. | 0 - 1 |
| timerxen | Sets the timerxen value of the struct spw passed to the function. | 0 - 1 |
| rxmaxlen | Sets the receiver maximum length field of the struct spw passed to the function. | 0 - 225-1 |
| spwadr | Sets the address to the GRSPW core which will be associated with the struct passed to the function. | 0 - 232-1 |

```
int spw_init(struct spwvars *spw);
```

Initializes the GRSPW core located at the address set in the struct spw. Sets the following registers: node address, destination key, clock divisor, receiver maximum length, transmitter descriptor table address, receiver descriptor table address, ctrl and dmactrl. All bits are set to the values found in the spwvars struct. If a register bit is not present in the struct it will be set to zero. The descriptor tables are allocated to an aligned area using malloc. The status register is cleared and lastly the link interface is enabled. The run state frequency will be set according to the value in clkdiv.

_Table 1362._Return values for spw\_init

| **Value** | **Description** |
| ---: | --- |
| 0 | The function completed successfully |
| 1 | One or more of the parameters could not be set correctly or the link failed to initialize. |

_Table 1363._Parameters for spw\_init

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | The spwvars struct associated with the GRSPW core that should be initialized. | - |

```
int set_txdesc(int pnt, struct spwvars *spw);
```

GRIP Frontgrade Gaisler AB

Sets a new address to the transmitter descriptor table address register. Should only be used when no transmission is active. Also resets the pointers for spw\_tx and spw\_checktx (Explained in the section for those functions).

_Table 1364._Return values for spw\_txdesc

| **Value** | **Description** |
| ---: | --- |
| 0 | The function completed successfully |
| 1 | The new address could not be written correctly |

_Table 1365._Parameters for spw\_txdesc

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| pnt | The new address to the descriptor table area | 0 - 232-1 |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be configured | - |

```
int set_rxdesc(int pnt, struct spwvars *spw);
```

Sets a new address to the Receiver descriptor table address register. Should only be used when no transmission is active. Also resets the pointers for spw\_rx and spw\_checkrx (Explained in the section for those functions).

_Table 1366._Return values for spw\_rxdesc

| **Value** | **Description** |
| ---: | --- |
| 0 | The function completed successfully |
| 1 | The new address could not be written correctly |

_Table 1367._Parameters for spw\_rxdesc

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| pnt | The new address to the descriptor table area | 0 - 232-1 |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be configured | - |

```
void spw_disable(struct spwvars *spw);
```

Disables the GRSPW core (the link disable bit is set to ‘1’).

GRIP Frontgrade Gaisler AB

_Table 1368._Parameters for spw\_disable

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be configured | - |

```
void spw_enable(struct spwvars *spw);
```

Enables the GRSPW core (the link disable bit is set to ‘0’).

_Table 1369._Parameters for spw\_enable

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be configured | - |

```
void spw_start(struct spwvars *spw);
```

Starts the GRSPW core (the link start bit is set to ‘1’).

_Table 1370._Parameters for spw\_start

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be configured | - |

```
void spw_stop(struct spwvars *spw);
```

Stops the GRSPW core (the link start bit is set to ‘0’).

_Table 1371._Parameters for spw\_start

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be configured | - |

```
int spw_setclockdiv(struct spwvars *spw);
```

Sets the clock divisor register with the clock divisor value stored in the spwvars struct. 

_Table 1372._Return values for spw\_setclockdiv

| **Value** | **Description** |
| ---: | --- |
| 0 | The function completed successfully |
| 1 | The new clock divisor value is illegal. |

GRIP Frontgrade Gaisler AB

_Table 1373._Parameters for spw\_setclockdiv

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be configured | - |

```
int spw_set_nodeadr(struct spwvars *spw);
```

Sets the node address register with the node address value stored in the spwvars struct. 

_Table 1374._Return values for spw\_set\_nodeadr

| **Value** | **Description** |
| ---: | --- |
| 0 | The function completed successfully |
| 1 | The new node address value is illegal. |

_Table 1375._Parameters for spw\_set\_nodeadr

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be configured | - |

```
int spw_set_rxmaxlength(struct spwvars *spw);
```

Sets the Receiver maximum length register with the rxmaxlen value stored in the spwvars struct. 

_Table 1376._Return values for spw\_set\_rxmaxlength

| **Value** | **Description** |
| ---: | --- |
| 0 | The function completed successfully |
| 1 | The new node address value is illegal. |

_Table 1377._Parameters for spw\_set\_rxmaxlength

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be configured | - |

```
int spw_tx(int crc, int skipcrcsize, int hsize, char *hbuf, int dsize, char *dbuf, struct spwvars *spw);
```

Transmits a packet. Separate header and data buffers can be used. If CRC logic is available the GSPW inserts RMAP CRC values after the header and data fields if crc is set to one. This function only sets a descriptor and initiates the transmission. Spw\_checktx must be used to check if the packet has been transmitted. A pointer into the descriptor table is stored in the spwvars struct to keep track of the next location to use. It is incremented each time the function returns 0. 

_Table 1378._Return values for spw\_tx

| **Value** | **Description** |
| ---: | --- |
| 0 | The function completed successfully |
| 1 | There are no free transmit descriptors currently available |
| 2 | There was illegal parameters passed to the function |

GRIP Frontgrade Gaisler AB

_Table 1379._Parameters for spw\_tx

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| crc | Set to one to append RMAP CRC after the header and data fields. Only available if hardware CRC is available in the core. | 0 - 1 |
| skipcrcsize | The number of bytes in the beginning of a packet that should not be included in the CRC calculation | 0 - 15 |
| hsize | The size of the header in bytes | 0 - 255 |
| hbuf | Pointer to the header data | - |
| dsize | The size of the data field in bytes | 0 - 224-1 |
| dbuf | Pointer to the data area. | - |
| spw | Pointer to the spwvars struct associated with GRSPW core that should transmit the packet | - |

```
int spw_rx(char *buf, struct spwvars *spw);
```

Enables a descriptor for reception. The packet will be stored to buf. Spw\_checkrx must be used to check if a packet has been received. A pointer in the spwvars struct is used to keep track of the next location to use in the descriptor table. It is incremented each time the function returns 0. 

_Table 1380._Return values for spw\_rx

| **Value** | **Description** |
| ---: | --- |
| 0 | The function completed successfully |
| 1 | There are no free receive descriptors currently available |

_Table 1381._Parameters for spw\_rx

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| buf | Pointer to the data area. | - |
| spw | Pointer to the spwvars struct associated with GRSPW core that should receive the packet | - |

```
int spw_checkrx(int *size, struct rxstatus *rxs, struct spwvars *spw);
```

Checks if a packet has been received. When a packet has been received the size in bytes will be stored in the size parameter and status is found in the rxs struct. A pointer in the spwvars struct is used to keep track of the location in the descriptor table to poll. It is incremented each time the function returns nonzero. 

_Table 1382._Return values for spw\_checkrx

| **Value** | **Description** |
| ---: | --- |
| 0 | No packet has been received |
| 1 | A packet has been received |

_Table 1383._Parameters for spw\_checkrx

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| size | When the function returns 1 this variable holds the number of bytes received | - |
| rxs | When the function returns 1 this variable holds status information | - |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be polled | - |

GRIP Frontgrade Gaisler AB

_Table 1384._The rxstatus struct

| **Field** | **Description** | **Allowed range** |
| --- | --- | --- |
| truncated | Packet was truncated | 0 - 1 |
| dcrcerr | Data CRC error bit was set. Only indicates an error if the packet received was an RMAP packet. | 0 - 1 |
| hcrcerr | Header CRC error bit was se.t. Only indicates an error if the packet received was an RMAP packet. | 0 - 1 |
| eep | Packet was terminated with EEP | 0 - 1 |

```
int spw_checktx(struct spwvars *spw);
```

Checks if a packet has been transmitted. A pointer is used to keep track of the location in the descrip- tor table to poll. It is incremented each time the function returns nonzero. 

_Table 1385._Return values for spw\_checktx

| **Value** | **Description** |
| ---: | --- |
| 0 | No packet has been transmitted |
| 1 | A packet has been correctly transmitted |
| 2 | A packet has been incorrectly transmitted |

_Table 1386._Parameters for spw\_checktx

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be polled | - |

```
void send_time(struct spwvars *spw);
```

Sends a new time-code. Increments the time-counter in the GRSPW and transmits the value.

_Table 1387._Parameters for send time

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be polled | - |

```
int check_time(struct spwvars *spw);
```

Check if a new time-code has been received. 

_Table 1388._Return values for check\_time

| **Value** | **Description** |
| ---: | --- |
| 0 | No time-code has been received |
| 1 | A new time-code has been received |

_Table 1389._Parameters for check\_time

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be polled | - |

GRIP Frontgrade Gaisler AB

```
int get_time(struct spwvars *spw);
```

### Get the current time counter value. 

_Table 1390._Return values for get\_time

| **Value** | **Description** |
| --- | --- |
| 0 - 63 | Returns the current time counter value |

_Table 1391._Parameters for get\_time

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be polled | - |

```
void spw_reset(struct spwvars *spw);
```

### Resets the GRSPW. 

_Table 1392._Parameters for spw\_reset

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be reset | - |

```
void spw_rmapen(struct spwvars *spw); 
```

Enables hardware RMAP. Has no effect if the RMAP command handler is not available in GRSPW.

_Table 1393._Parameters for spw\_rmapen

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be set | - |

```
void spw_rmapdis(struct spwvars *spw); 
```

### Disables hardware RMAP. Has no effect if the RMAP command handler is not available in GRSPW

_Table 1394._Parameters for spw\_rmapdis

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be set | - |

```
int spw_setdestkey(struct spwvars *spw); 
```

Set the destination key of the GRSPW. Has no effect if the RMAP command handler is not available. The value from the spwvars struct is used.

_Table 1395._Return values for spw\_setdestkey

| **Value** | **Description** |
| ---: | --- |
| 0 | The function completed successfully |
| 1 | The destination key parameter in the spwvars struct contains an illegal value |

GRIP Frontgrade Gaisler AB

_Table 1396._Parameters for spw\_setdestkey

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be set. | - |

### 77.19.2 GRSPW RMAP API

The RMAP API contains only one function which is used for building RMAP headers.

```
int build_rmap_hdr(struct rmap_pkt *pkt, char *hdr, int *size);
```

Builds a RMAP header to the buffer pointed to by hdr. The header data is taken from the rmap\_pkt struct. 

_Table 1397._Return values for build\_rmap\_hdr

| **Value** | **Description** |
| ---: | --- |
| 0 | The function completed successfully |
| 1 | One or more of the parameters contained illegal values |

_Table 1398._Parameters for build\_rmap\_hdr

| **Parameter** | **Description** | **Allowed range** |
| --- | --- | --- |
| pkt | Pointer to a rmap\_pkt struct which contains the data from which the header should be built |  |
| hdr | Pointer to the buffer where the header will be built |  |
| spw | Pointer to the spwvars struct associated with GRSPW core that should be set | - |

GRIP Frontgrade Gaisler AB

_Table 1399._rmap\_pkt struct fields

| **Field** | **Description** | **Allowed Range** |
| --- | --- | --- |
| type | Selects the type of packet to build.  | writecmd, readcmd, rmwcmd, writerep, readrep, rmwrep |
| verify | Selects whether the data should be verified before writing | yes, no |
| ack  | Selects whether an acknowledge should be sent | yes, no |
| incr | Selects whether the address should be incremented or not | yes, no |
| destaddr | Sets the destination address  | 0 - 255 |
| destkey | Sets the destination key  | 0 - 255 |
| srcaddr | Sets the source address  | 0 - 255 |
| tid | Sets the transaction identifier field | 0 - 65535 |
| addr | Sets the address of the operation to be performed. The extended address field is currently always set to 0. | 0 - 232-1 |
| len | The number of bytes to be writte, read or read-modify-written | 0 - 224-1 |
| status | Sets the status field | 0 - 11 |
| dstspalen | Number of source path address bytes to insert before the destination address | 0 - 228 |
| dstspa | Pointer to memory holding the destination path address bytes | - |
| srcspalen | Number of source path address bytes to insert in a command. For a reply these bytes are placed before the return address | 0 - 12 |
| srcspa | Pointer to memory holding the source path address bytes | - |

GRIP Frontgrade Gaisler AB

**77.20** **Appendix A Clarifications of the GRSPW implementation of the standard**

6.3.1 page 9 RMAP draft F "The user application at destination will be informed that there was an error 

 in the data transferred. The source will be informed of the data error if the acknowledge bit in the command has been set."

We view the RMAP command handler as both protocol parser and user application. All commands are parsed in the first stage and various (internal) status bits are set. The next step (which can be viewed as the user application) will make the decision of how to act upon the received command from these bits. Therefore, the various errors that can occur are not externally observable. If an error occurs when the command handler is accessing the AHB bus through the DMA interface errors will be externally observable using the AHB status register in GRLIB. 

6.3.6 page 13 RMAP draft F “The Write Command packet arrives at the destination and its header is found to be in error. This fact is added to the error statistics in the destination node.”

This text does not state how and if these statistics should be observable. At the moment the error han- dling is internal to the RMAP command handler and therefore no statistics are internally observable. A counter for this particular error might be added in the future.

6.3.6 page 15 RMAP draft F "These various errors will be reported to the user application running on the 

destination node (Write Data Error Indication)." Again the RMAP command handler is the user application and all these errors are handled internally.

6.5.6 page 31 RMAP draft F "The source user application, in fact immediately rejects this as an authorisation failure as the command is trying to RMW an area of protected memory."

It should probably be destination user application instead of source. It is unclear what immediately means. Should it be rejected before any accesses are done on the bus and thus requiring the RMAP command handler to include a complete bus decoding. The GRSPW does (probably) not comply to this paragraph at the moment. If a bus error occurs a general error code will be returned.

6.5.6 page 32 "If the header of the RMW reply packet is received intact but the data field is 

corrupted as indicated by an incorrect data field length (too long or too short) or by a CRC error, then an error can be flagged to the application immediately

(RMW Data Failure) without having to wait for an application timeout."

GRIP Frontgrade Gaisler AB

This is not applicable to the GRSPW since it does not handle replies. However this is practically an unnecessary comment since it is not specified in the standard in which manner received replies are indicated to the higher layers.

GRIP Frontgrade Gaisler AB

---

[^310]: HEADERADDRESS

[^264]: x8

[^412]: 8x8

[^825]: 6x8
