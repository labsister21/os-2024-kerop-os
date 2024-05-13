#include <stdint.h>

struct Context {
    // TODO: Add important field here
    // CPU register state
    struct CPURegister {
        struct {
            uint32_t edi;   //0
            uint32_t esi;   //4
        };
        struct {
            uint32_t ebp;   //8
            uint32_t esp;   //12
        };
        struct {
            uint32_t ebx;   //16
            uint32_t edx;   //20
            uint32_t ecx;   //24
            uint32_t eax;   //28
        };
        struct {
            uint32_t gs;    //32
            uint32_t fs;    //36
            uint32_t es;    //40
            uint32_t ds;    //44
        };
    };

    // CPU instruction counter to resume execution
    uint32_t eip;           //48

    // Flag register to load before resuming the execution
    uint32_t eflags;        //52

    // CPU register CR3, containing pointer to active page directory
    struct PageDirectory *page_directory_virtual_addr;
};