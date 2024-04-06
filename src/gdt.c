#include "header/cpu/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            0, // segment low
            0, // Base low
            0, // base mid
            0, // type bit
            0, // non system
            0, // descriptor privilege
            0, // segment present
            0, // segment limit
            0, // available
            0, // long code seg
            0, // default operation
            0, // granularity
            0, // base high

        },
        {
            0xFFFF, // segment low
            0,      // Base low
            0,      // base mid
            0xA,    // type bit
            1,      // non system
            0,      // descriptor privilege
            1,      // segment present
            0xF,    // segment limit
            0,      // available
            0,      // long code seg
            1,      // default operation
            1,      // granularity
            0,      // base high
        },
        {
            0xFFFF, // segment low
            0,      // Base low
            0,      // base mid
            0x2,    // type bit
            1,      // non system
            0,      // descriptor privilege
            1,      // segment present
            0xF,    // segment limit
            0,      // available
            0,      // long code seg
            1,      // default operation
            1,      // granularity
            0,      // base high
        }}};

/**
 * _gdt_gdtr, predefined system GDTR.
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    sizeof(global_descriptor_table) - 1, &global_descriptor_table};