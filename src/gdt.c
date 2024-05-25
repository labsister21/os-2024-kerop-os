#include "header/cpu/gdt.h"
#include "header/cpu/interrupt.h"
/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            // TODO : Implement
            .seg_limit = 0,
            .segment_low = 0,
            .base_high = 0,
            .base_mid = 0,
            .base_low = 0,
            .type_bit = 0,
            .non_system = 0,
            .dpl = 0,
            .present = 0,
            .l = 0,
            .op_size = 0,
            .granularity = 0,

        },
        {
            // TODO : Implement
            .seg_limit = 0xF,
            .segment_low = 0xFFFF,
            .base_high = 0,
            .base_mid = 0,
            .base_low = 0,
            .type_bit = 0xA,
            .non_system = 1,
            .dpl = 0,
            .present = 1,
            .l = 0,
            .op_size = 1,
            .granularity = 1,
        },
        {
            // TODO : Implement
            .seg_limit = 0xF,
            .segment_low = 0xFFFF,
            .base_high = 0,
            .base_mid = 0,
            .base_low = 0,
            .type_bit = 0x2,
            .non_system = 1,
            .dpl = 0,
            .present = 1,
            .l = 0,
            .op_size = 1,
            .granularity = 1,
        },
        {/* TODO: User   Code Descriptor */
            .seg_limit = 0xF,
            .segment_low = 0xFFFF,
            .base_high = 0,
            .base_mid = 0,
            .base_low = 0,
            .type_bit = 0xA,
            .non_system = 1,
            .dpl = 0x3,
            .present = 1,
            .l = 0,
            .op_size = 1,
            .granularity = 1,
        },
        {/* TODO: User   Data Descriptor */
            .seg_limit = 0xF,
            .segment_low = 0xFFFF,
            .base_high = 0,
            .base_mid = 0,
            .base_low = 0,
            .type_bit = 0x2,
            .non_system = 1,
            .dpl = 0x3,
            .present = 1,
            .l = 0,
            .op_size = 1,
            .granularity = 1,
        },
        {
            .seg_limit         = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
            .segment_low       = sizeof(struct TSSEntry),
            .base_high         = 0,
            .base_mid          = 0,
            .base_low          = 0,
            .non_system        = 0,    // S bit
            .type_bit          = 0x9,
            .dpl               = 0,    // DPL
            .present           = 1,    // P bit
            .op_size           = 1,    // D/B bit
            .l                 = 0,    // L bit
            .granularity       = 0,    // G bit
        },
        {0}
    }
};

/**
 * _gdt_gdtr, predefined system GDTR.
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
void gdt_install_tss(void) {
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low  = base & 0xFFFF;
}

struct GDTR _gdt_gdtr = {
    // TODO : Implement, this GDTR will point to global_descriptor_table.
    //        Use sizeof operator
    .size = sizeof(global_descriptor_table) - 1,
    .address = &global_descriptor_table};
