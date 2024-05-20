#ifndef _PAGING_H
#define _PAGING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Note: MB often referring to MiB in context of memory management
#define SYSTEM_MEMORY_MB     128

#define PAGE_ENTRY_COUNT     1024
// Page Frame (PF) Size: (1 << 22) B = 4*1024*1024 B = 4 MiB
#define PAGE_FRAME_SIZE      (1 << (2 + 10 + 10))
// Maximum usable page frame. Default count: 128 / 4 = 32 page frame
#define PAGE_FRAME_MAX_COUNT ((SYSTEM_MEMORY_MB << 20) / PAGE_FRAME_SIZE)

#define KERNEL_VIRTUAL_ADDRESS_BASE      0xC0000000

// Operating system page directory, using page size PAGE_FRAME_SIZE (4 MiB)
extern struct PageDirectory _paging_kernel_page_directory;




/**
 * Page Directory Entry Flag, only first 8 bit
 * 
 * @param present_bit       Indicate whether this entry is exist or not
 * ...
 */
struct PageDirectoryEntryFlag {
    uint8_t present_bit        : 1;
    uint8_t read_write         : 1;
    uint8_t user_supervisor    : 1;
    uint8_t pwt                : 1;
    uint8_t pcd                : 1;
    uint8_t access             : 1;
    uint8_t dirty              : 1;
    uint8_t page_size          : 1;
} __attribute__((packed));

/**
 * Page Directory Entry, for page size 4 MB.
 * Check Intel Manual 3a - Ch 4 Paging - Figure 4-4 PDE: 4MB page
 *
 * @param flag            Contain 8-bit page directory entry flag
 * @param global_page     Is this page translation global & cannot be flushed?
 * ...
 * @param reserved_2      Reserved bit (1-bit)
 * @param lower_address   10-bit page frame lower address, note directly correspond with 4 MiB memory (= 0x40 0000 = 1
 * Note:
 * - "Bits 39:32 of address" (higher_address) is 8-bit
 * - "Bits 31:22 of address" is called lower_address in kit
 */
struct PageDirectoryEntry {
    struct PageDirectoryEntryFlag flag;
    uint16_t global_page    : 1;
    uint16_t ignored        : 3;
    uint16_t pat            : 1;
    uint16_t higher_address : 8;
    uint16_t reserved       : 1;
    uint16_t lower_address  : 10;
} __attribute__((packed));

/**
 * Page Directory, contain array of PageDirectoryEntry.
 * Note: This data structure is volatile (can be modified from outside this code, check "C volatile keyword"). 
 * MMU operation, TLB hit & miss also affecting this data structure (dirty, accessed bit, etc).
 * 
 * Warning: Address must be aligned in 4 KB (listed on Intel Manual), use __attribute__((aligned(0x1000))), 
 *   unaligned definition of PageDirectory will cause triple fault
 * 
 * @param table Fixed-width array of PageDirectoryEntry with size PAGE_ENTRY_COUNT
 */
struct PageDirectory {
    struct PageDirectoryEntry table[PAGE_ENTRY_COUNT];
} __attribute__((packed));

/**
 * Containing page manager states.
 * 
 * @param page_frame_map Keeping track empty space. True when the page frame is currently used
 * ...
 */
struct PageManagerState {
    bool     page_frame_map[PAGE_FRAME_MAX_COUNT];
    uint32_t free_page_frame_count;
    // TODO: Add if needed ...
} __attribute__((packed));





/**
 * Edit page directory with respective parameter
 * 
 * @param page_dir      Page directory to update
 * @param physical_addr Physical address to map
 * @param virtual_addr  Virtual address to map
 * @param flag          Page entry flags
 */
void update_page_directory_entry(
    struct PageDirectory *page_dir,
    void *physical_addr, 
    void *virtual_addr, 
    struct PageDirectoryEntryFlag flag
);

/**
 * Invalidate page that contain virtual address in parameter
 * 
 * @param virtual_addr Virtual address to flush
 */
void flush_single_tlb(void *virtual_addr);





/* --- Memory Management --- */
/**
 * Check whether a certain amount of physical memory is available
 * 
 * @param amount Requested amount of physical memory in bytes
 * @return       Return true when there's enough free memory available
 */
bool paging_allocate_check(uint32_t amount);

/**
 * Allocate single user page frame in page directory
 * 
 * @param page_dir     Page directory to update
 * @param virtual_addr Virtual address to be allocated
 * @return             Physical address of allocated frame
 */
bool paging_allocate_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr);

/**
 * Deallocate single user page frame in page directory
 * 
 * @param page_dir      Page directory to update
 * @param virtual_addr  Virtual address to be allocated
 * @return              Will return true if success, false otherwise
 */
bool paging_free_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr);

/* --- Process-related Memory Management --- */
#define PAGING_DIRECTORY_TABLE_MAX_COUNT 32

/**
 * Create new page directory prefilled with 1 page directory entry for kernel higher half mapping
 * 
 * @return Pointer to page directory virtual address. Return NULL if allocation failed
 */
struct PageDirectory* paging_create_new_page_directory(void);

/**
 * Free page directory and delete all page directory entry
 * 
 * @param page_dir Pointer to page directory virtual address
 * @return         True if free operation success 
 */
bool paging_free_page_directory(struct PageDirectory *page_dir);

/**
 * Get currently active page directory virtual address from CR3 register
 * 
 * @note   Assuming page directories lives in kernel memory
 * @return Page directory virtual address currently active (CR3)
 */
struct PageDirectory* paging_get_current_page_directory_addr(void);

/**
 * Change active page directory (indirectly trigger TLB flush for all non-global entry)
 * 
 * @note                        Assuming page directories lives in kernel memory
 * @param page_dir_virtual_addr Page directory virtual address to switch into
 */
void paging_use_page_directory(struct PageDirectory *page_dir_virtual_addr);

#endif