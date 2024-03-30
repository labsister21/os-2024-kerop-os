#include "header/cpu/idt.h"
#include "header/cpu/gdt.h"

void initialize_idt(void)
{

    /*
     * TODO:
     * Iterate all isr_stub_table,


     * Set all IDT entry with set_interrupt_gate()
     * with following values:
     * Vector: i
     * Handler Address: isr_stub_table[i]
     * Segment: GDT_KERNEL_CODE_SEGMENT_SELECTOR
     * Privilege: 0
     */
    for (int i = 0; i < ISR_STUB_TABLE_LIMIT; i++)
    {
        /* code */
        set_interrupt_gate(i, isr_stub_table[i], GDT_KERNEL_CODE_SEGMENT_SELECTOR, 0);
    }

    __asm__ volatile("lidt %0" : : "m"(_idt_idtr));
    __asm__ volatile("sti");
}

void set_interrupt_gate(
    uint8_t int_vector,
    void *handler_address,
    uint16_t gdt_seg_selector,
    uint8_t privilege)
{
    struct IDTGate *idt_int_gate = &interrupt_descriptor_table.table[int_vector];
    // TODO : Set handler offset, privilege & segment
    uint32_t offset = (uint32_t)handler_address;
    uint16_t offset_low = offset & 0xFFFF;
    // Use &-bitmask, bitshift, and casting for offset

    // Target system 32-bit and flag this as valid interrupt gate
    idt_int_gate->offset_low = offset_low;
    idt_int_gate->_reserved = 0;
    idt_int_gate->segment = (gdt_seg_selector << 3) | (privilege & 0b11);
    idt_int_gate->_r_bit_1 = INTERRUPT_GATE_R_BIT_1;
    idt_int_gate->_r_bit_2 = INTERRUPT_GATE_R_BIT_2;
    idt_int_gate->_r_bit_3 = INTERRUPT_GATE_R_BIT_3;
    idt_int_gate->gate_32 = 1;
    idt_int_gate->valid_bit = 1;
}
