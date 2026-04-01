#ifndef _PIC_H_
#define _PIC_H_

#define PIC_EOI 0x20 //End of interrupt code
#define PIC1 0x20 // The command port of the master pic
#define PIC2 0xA0 // The command port of the slave pic
#define PIC1_COMMAND PIC1 
#define PIC2_COMMAND PIC2
#define PIC1_DATA (PIC1 + 1)
#define PIC2_DATA (PIC2 + 1)
#define ICW1_ICW4 0x01 // This bit is saying the ICW4 will be present, wait patietly.
#define ICW1_SINGLE 0x02 //Enable cascade mode
#define ICW1_INIT 0x10 //Init

#define ICW4_8086 0x01 //8086 mode
#define ICW4_AUTO 0x02 //Normal EOI

#define PIC1_OFFSET 0x20
#define PIC2_OFFSET 0xA0

void PIC_remap(void);




#endif
