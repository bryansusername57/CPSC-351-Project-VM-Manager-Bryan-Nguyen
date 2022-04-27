//  mem_mgr.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <cassert>

#pragma warning(disable : 4996)

#define ARGC_ERROR 1
#define FILE_ERROR 2

#define FRAME_SIZE  256
#define FIFO 0
#define LRU 1
#define REPLACE_POLICY FIFO

// SET TO 128 to use replacement policy: FIFO or LRU,
#define NFRAMES 256
#define PTABLE_SIZE 256
#define TLB_SIZE 16

struct page_node {    
    size_t npage;
    size_t frame_num;
    bool is_present;
    bool is_used;
};

char* ram = (char*)malloc(NFRAMES * FRAME_SIZE);
page_node pg_table[PTABLE_SIZE];  // page table and (single) TLB
page_node tlb[TLB_SIZE];

const char* passed_or_failed(bool condition) { return condition ? " + " : "fail"; }
size_t failed_asserts = 0;

size_t get_page(size_t x)   { return 0xff & (x >> 8); }
size_t get_offset(size_t x) { return 0xff & x; }

void get_page_offset(size_t x, size_t& page, size_t& offset) {
    page = get_page(x);
    offset = get_offset(x);
    // printf("x is: %zu, page: %zu, offset: %zu, address: %zu, paddress: %zu\n", 
    //        x, page, offset, (page << 8) | get_offset(x), page * 256 + offset);
}

void update_frame_ptable(size_t npage, size_t frame_num) {
    pg_table[npage].frame_num = frame_num;
    pg_table[npage].is_present = true;
    pg_table[npage].is_used = true;
}

int find_frame_ptable(size_t frame) {  // FIFO
    for (int i = 0; i < PTABLE_SIZE; i++) {
        if (pg_table[i].frame_num == frame && 
            pg_table[i].is_present == true) { return i; }
    }
    return -1;
}

size_t get_used_ptable() {  // LRU
    size_t unused = -1;
    for (size_t i = 0; i < PTABLE_SIZE; i++) {
        if (pg_table[i].is_used == false && 
            pg_table[i].is_present == true) { return (size_t)i; }
    }
    // All present pages have been used recently, set all page entry used flags to false
    for (size_t i = 0; i < PTABLE_SIZE; i++) { pg_table[i].is_used = false; }
    for (size_t i = 0; i < PTABLE_SIZE; i++) {
        page_node& r = pg_table[i];
        if (!r.is_used && r.is_present) { return i; }
    }
    return (size_t)-1;
}

int check_tlb(size_t page) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].npage == page) { return i; }
    }
    return -1;
}

void open_files(FILE*& fadd, FILE*& fcorr, FILE*& fback) { 
    fadd = fopen("addresses.txt", "r");
    if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR); }

    fcorr = fopen("correct.txt", "r");
    if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR); }

    fback = fopen("BACKING_STORE.bin", "rb");
    if (fback == NULL) { fprintf(stderr, "Could not open file: 'BACKING_STORE.bin'\n");  exit(FILE_ERROR); }
}
void close_files(FILE* fadd, FILE* fcorr, FILE* fback) { 
    fclose(fadd);
    fclose(fcorr);
    fclose(fback);
}

void initialize_pg_table_tlb() { 
    for (int i = 0; i < PTABLE_SIZE; ++i) {
        pg_table[i].npage = (size_t)i;
        pg_table[i].is_present = false;
        pg_table[i].is_used = false;
    }
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].npage = (size_t)-1;
        tlb[i].is_present = false;
        pg_table[i].is_used = false;
    }
}

void summarize(size_t pg_faults, size_t tlb_hits) { 
    printf("\nPage Fault Percentage: %1.3f%%", (double)pg_faults / 1000);
    printf("\nTLB Hit Percentage: %1.3f%%\n\n", (double)tlb_hits / 1000);
    printf("ALL logical ---> physical assertions PASSED!\n");
    printf("\n\t\t...done.\n");
}

void tlb_add(int index, page_node entry) { 
    tlb[index] = entry;
}  // TODO

void tlb_remove(int index) { 

}  // TODO

void tlb_hit(size_t& frame, size_t& page, size_t& tlb_hits, int result) { 
    for (int i = 0; i < TLB_SIZE; ++i) {
        if (tlb[i].npage == page){
            frame = tlb[i].frame_num;
            ++tlb_hits;
            result = -1;
        }
    }
 }  // TODO

void tlb_miss(size_t& frame, size_t& page, size_t& tlb_track) { } // TODO

void fifo_replace_page(size_t& frame ) { }   // TODO

void lru_replace_page(size_t& frame) { } // TODO

void page_fault(size_t& frame, size_t& page, size_t& frames_used, size_t& pg_faults, 
              size_t& tlb_track, FILE* fbacking) {  
    unsigned char buf[BUFSIZ];
    memset(buf, 0, sizeof(buf));
    bool is_memfull = false;

    ++pg_faults;
    if (frames_used >= NFRAMES) { is_memfull = true; }
    frame = frames_used % NFRAMES;    // FIFO only

    if (is_memfull) { 
        // if (REPLACE_POLICY == FIFO) {  // TODO
        // } else { 
        //     // TODO
        // }
    }
         // load page into RAM, update pg_table, TLB
    fseek(fbacking, page * FRAME_SIZE, SEEK_SET);
    fread(buf, FRAME_SIZE, 1, fbacking);

    for (int i = 0; i < FRAME_SIZE; i++) {
        *(ram + (frame * FRAME_SIZE) + i) = buf[i];
    }
    update_frame_ptable(page, frame);
    tlb_add(tlb_track++, pg_table[page]);
    if (tlb_track > 15) { tlb_track = 0; }
    
    ++frames_used;
} 

void check_address_value(size_t logic_add, size_t page, size_t offset, size_t physical_add,
                         size_t& prev_frame, size_t frame, int val, int value, size_t o) { 
    printf("log: %5lu 0x%04lx (pg:%3lu, off:%3lu)-->phy: %5lu (frm: %3lu) (prv: %3lu)--> val: %4d == value: %4d -- %s", 
          logic_add, logic_add, page, offset, physical_add, frame, prev_frame, 
          val, value, passed_or_failed(val == value));

    if (frame < prev_frame) {  printf("   HIT!\n");
    } else {
        prev_frame = frame;
        printf("----> pg_fault\n");
    }
    if (o % 5 == 4) { printf("\n"); }
// if (o > 20) { exit(-1); }             // to check out first 20 elements

    if (val != value) { ++failed_asserts; }
    if (failed_asserts > 5) { exit(-1); }
//     assert(val == value);
}

void run_simulation() { 
        // addresses, pages, frames, values, hits and faults
    size_t logic_add, virt_add, phys_add, physical_add;
    size_t page, frame, offset, value, prev_frame = 0, tlb_track = 0;
    size_t frames_used = 0, pg_faults = 0, tlb_hits = 0;
    int val = 0;
    char buf[BUFSIZ];

    bool is_memfull = false;     // physical memory to store the frames

    initialize_pg_table_tlb();

        // addresses to test, correct values, and pages to load
    FILE *faddress, *fcorrect, *fbacking;
    open_files(faddress, fcorrect, fbacking);

    for (int o = 0; o < 1000; o++) {     // read from file correct.txt
        fscanf(fcorrect, "%s %s %lu %s %s %lu %s %ld", buf, buf, &virt_add, buf, buf, &phys_add, buf, &value);  

        fscanf(faddress, "%ld", &logic_add);  
        get_page_offset(logic_add, page, offset);

        int result = check_tlb(page);
        if (result >= 0) {  
            tlb_hit(frame, page, tlb_hits, result); 
        } else if (pg_table[page].is_present) {
            tlb_miss(frame, page, tlb_track);
        } else {         // page fault
            page_fault(frame, page, frames_used, pg_faults, tlb_track, fbacking);
        }

        physical_add = (frame * FRAME_SIZE) + offset;
        val = (int)*(ram + physical_add);

        check_address_value(logic_add, page, offset, physical_add, prev_frame, frame, val, value, o);
    }
    close_files(faddress, fcorrect, fbacking);  // and time to wrap things up
    free(ram);
    summarize(pg_faults, tlb_hits);
}


int main(int argc, const char * argv[]) {
    run_simulation();
// printf("\nFailed asserts: %lu\n\n", failed_asserts);   // allows asserts to fail silently and be counted
    return 0;
}

/*

OUTPUT:
log: 16916 0x4214 (pg: 66, off: 20)-->phy:    20 (frm:   0) (prv:   0)--> val:    0 == value:    0 --  + ----> pg_fault
log: 62493 0xf41d (pg:244, off: 29)-->phy:   285 (frm:   1) (prv:   0)--> val:    0 == value:    0 --  + ----> pg_fault
log: 30198 0x75f6 (pg:117, off:246)-->phy:   758 (frm:   2) (prv:   1)--> val:   29 == value:   29 --  + ----> pg_fault
log: 53683 0xd1b3 (pg:209, off:179)-->phy:   947 (frm:   3) (prv:   2)--> val:  108 == value:  108 --  + ----> pg_fault
log: 40185 0x9cf9 (pg:156, off:249)-->phy:  1273 (frm:   4) (prv:   3)--> val:    0 == value:    0 --  + ----> pg_fault

log: 28781 0x706d (pg:112, off:109)-->phy:  1389 (frm:   5) (prv:   4)--> val:    0 == value:    0 --  + ----> pg_fault
log: 24462 0x5f8e (pg: 95, off:142)-->phy:  1678 (frm:   6) (prv:   5)--> val:   23 == value:   23 --  + ----> pg_fault
log: 48399 0xbd0f (pg:189, off: 15)-->phy:  1807 (frm:   7) (prv:   6)--> val:   67 == value:   67 --  + ----> pg_fault
log: 64815 0xfd2f (pg:253, off: 47)-->phy:  2095 (frm:   8) (prv:   7)--> val:   75 == value:   75 --  + ----> pg_fault
log: 18295 0x4777 (pg: 71, off:119)-->phy:  2423 (frm:   9) (prv:   8)--> val:  -35 == value:  -35 --  + ----> pg_fault

log: 12218 0x2fba (pg: 47, off:186)-->phy:  2746 (frm:  10) (prv:   9)--> val:   11 == value:   11 --  + ----> pg_fault
log: 22760 0x58e8 (pg: 88, off:232)-->phy:  3048 (frm:  11) (prv:  10)--> val:    0 == value:    0 --  + ----> pg_fault
log: 57982 0xe27e (pg:226, off:126)-->phy:  3198 (frm:  12) (prv:  11)--> val:   56 == value:   56 --  + ----> pg_fault
log: 27966 0x6d3e (pg:109, off: 62)-->phy:  3390 (frm:  13) (prv:  12)--> val:   27 == value:   27 --  + ----> pg_fault
log: 54894 0xd66e (pg:214, off:110)-->phy:  3694 (frm:  14) (prv:  13)--> val:   53 == value:   53 --  + ----> pg_fault

log: 38929 0x9811 (pg:152, off: 17)-->phy:  3857 (frm:  15) (prv:  14)--> val:    0 == value:    0 --  + ----> pg_fault
log: 32865 0x8061 (pg:128, off: 97)-->phy:  4193 (frm:  16) (prv:  15)--> val:    0 == value:    0 --  + ----> pg_fault
log: 64243 0xfaf3 (pg:250, off:243)-->phy:  4595 (frm:  17) (prv:  16)--> val:  -68 == value:  -68 --  + ----> pg_fault
log:  2315 0x090b (pg:  9, off: 11)-->phy:  4619 (frm:  18) (prv:  17)--> val:   66 == value:   66 --  + ----> pg_fault
log: 64454 0xfbc6 (pg:251, off:198)-->phy:  5062 (frm:  19) (prv:  18)--> val:   62 == value:   62 --  + ----> pg_fault

log: 55041 0xd701 (pg:215, off:  1)-->phy:  5121 (frm:  20) (prv:  19)--> val:    0 == value:    0 --  + ----> pg_fault
log: 18633 0x48c9 (pg: 72, off:201)-->phy:  5577 (frm:  21) (prv:  20)--> val:    0 == value:    0 --  + ----> pg_fault
log: 14557 0x38dd (pg: 56, off:221)-->phy:  5853 (frm:  22) (prv:  21)--> val:    0 == value:    0 --  + ----> pg_fault
log: 61006 0xee4e (pg:238, off: 78)-->phy:  5966 (frm:  23) (prv:  22)--> val:   59 == value:   59 --  + ----> pg_fault
log: 62615 0xf497 (pg:244, off:151)-->phy:  6039 (frm:  23) (prv:  23)--> val:  -91 == value:   37 -- fail----> pg_fault

log:  7591 0x1da7 (pg: 29, off:167)-->phy:  6311 (frm:  24) (prv:  23)--> val:  105 == value:  105 --  + ----> pg_fault
log: 64747 0xfceb (pg:252, off:235)-->phy:  6635 (frm:  25) (prv:  24)--> val:   58 == value:   58 --  + ----> pg_fault
log:  6727 0x1a47 (pg: 26, off: 71)-->phy:  6727 (frm:  26) (prv:  25)--> val: -111 == value: -111 --  + ----> pg_fault
log: 32315 0x7e3b (pg:126, off: 59)-->phy:  6971 (frm:  27) (prv:  26)--> val: -114 == value: -114 --  + ----> pg_fault
log: 60645 0xece5 (pg:236, off:229)-->phy:  7397 (frm:  28) (prv:  27)--> val:    0 == value:    0 --  + ----> pg_fault

log:  6308 0x18a4 (pg: 24, off:164)-->phy:  7588 (frm:  29) (prv:  28)--> val:    0 == value:    0 --  + ----> pg_fault
log: 45688 0xb278 (pg:178, off:120)-->phy:  7800 (frm:  30) (prv:  29)--> val:    0 == value:    0 --  + ----> pg_fault
log:   969 0x03c9 (pg:  3, off:201)-->phy:  8137 (frm:  31) (prv:  30)--> val:    0 == value:    0 --  + ----> pg_fault
log: 40891 0x9fbb (pg:159, off:187)-->phy:  8379 (frm:  32) (prv:  31)--> val:  -18 == value:  -18 --  + ----> pg_fault
log: 49294 0xc08e (pg:192, off:142)-->phy:  8590 (frm:  33) (prv:  32)--> val:   48 == value:   48 --  + ----> pg_fault

log: 41118 0xa09e (pg:160, off:158)-->phy:  8862 (frm:  34) (prv:  33)--> val:   40 == value:   40 --  + ----> pg_fault
log: 21395 0x5393 (pg: 83, off:147)-->phy:  9107 (frm:  35) (prv:  34)--> val:  -28 == value:  -28 --  + ----> pg_fault
log:  6091 0x17cb (pg: 23, off:203)-->phy:  9419 (frm:  36) (prv:  35)--> val:  -14 == value:  -14 --  + ----> pg_fault
log: 32541 0x7f1d (pg:127, off: 29)-->phy:  9501 (frm:  37) (prv:  36)--> val:    0 == value:    0 --  + ----> pg_fault
log: 17665 0x4501 (pg: 69, off:  1)-->phy:  9729 (frm:  38) (prv:  37)--> val:    0 == value:    0 --  + ----> pg_fault

log:  3784 0x0ec8 (pg: 14, off:200)-->phy: 10184 (frm:  39) (prv:  38)--> val:    0 == value:    0 --  + ----> pg_fault
log: 28718 0x702e (pg:112, off: 46)-->phy: 10030 (frm:  39) (prv:  39)--> val:    3 == value:   28 -- fail----> pg_fault
log: 59240 0xe768 (pg:231, off:104)-->phy: 10344 (frm:  40) (prv:  39)--> val:    0 == value:    0 --  + ----> pg_fault
log: 40178 0x9cf2 (pg:156, off:242)-->phy: 10482 (frm:  40) (prv:  40)--> val:   57 == value:   39 -- fail----> pg_fault
log: 60086 0xeab6 (pg:234, off:182)-->phy: 10678 (frm:  41) (prv:  40)--> val:   58 == value:   58 --  + ----> pg_fault

log: 42252 0xa50c (pg:165, off: 12)-->phy: 10764 (frm:  42) (prv:  41)--> val:    0 == value:    0 --  + ----> pg_fault
log: 44770 0xaee2 (pg:174, off:226)-->phy: 11234 (frm:  43) (prv:  42)--> val:   43 == value:   43 --  + ----> pg_fault
log: 22514 0x57f2 (pg: 87, off:242)-->phy: 11506 (frm:  44) (prv:  43)--> val:   21 == value:   21 --  + ----> pg_fault
log:  3067 0x0bfb (pg: 11, off:251)-->phy: 11771 (frm:  45) (prv:  44)--> val:   -2 == value:   -2 --  + ----> pg_fault
log: 15757 0x3d8d (pg: 61, off:141)-->phy: 11917 (frm:  46) (prv:  45)--> val:    0 == value:    0 --  + ----> pg_fault

log: 31649 0x7ba1 (pg:123, off:161)-->phy: 12193 (frm:  47) (prv:  46)--> val:    0 == value:    0 --  + ----> pg_fault
log: 10842 0x2a5a (pg: 42, off: 90)-->phy: 12378 (frm:  48) (prv:  47)--> val:   10 == value:   10 --  + ----> pg_fault
log: 43765 0xaaf5 (pg:170, off:245)-->phy: 12789 (frm:  49) (prv:  48)--> val:    0 == value:    0 --  + ----> pg_fault
log: 33405 0x827d (pg:130, off:125)-->phy: 12925 (frm:  50) (prv:  49)--> val:    0 == value:    0 --  + ----> pg_fault
log: 44954 0xaf9a (pg:175, off:154)-->phy: 13210 (frm:  51) (prv:  50)--> val:   43 == value:   43 --  + ----> pg_fault

log: 56657 0xdd51 (pg:221, off: 81)-->phy: 13393 (frm:  52) (prv:  51)--> val:    0 == value:    0 --  + ----> pg_fault
log:  5003 0x138b (pg: 19, off:139)-->phy: 13707 (frm:  53) (prv:  52)--> val:  -30 == value:  -30 --  + ----> pg_fault
log: 50227 0xc433 (pg:196, off: 51)-->phy: 13875 (frm:  54) (prv:  53)--> val:   12 == value:   12 --  + ----> pg_fault
log: 19358 0x4b9e (pg: 75, off:158)-->phy: 14238 (frm:  55) (prv:  54)--> val:   18 == value:   18 --  + ----> pg_fault
log: 36529 0x8eb1 (pg:142, off:177)-->phy: 14513 (frm:  56) (prv:  55)--> val:    0 == value:    0 --  + ----> pg_fault

log: 10392 0x2898 (pg: 40, off:152)-->phy: 14744 (frm:  57) (prv:  56)--> val:    0 == value:    0 --  + ----> pg_fault
log: 58882 0xe602 (pg:230, off:  2)-->phy: 14850 (frm:  58) (prv:  57)--> val:   57 == value:   57 --  + ----> pg_fault
log:  5129 0x1409 (pg: 20, off:  9)-->phy: 15113 (frm:  59) (prv:  58)--> val:    0 == value:    0 --  + ----> pg_fault
log: 58554 0xe4ba (pg:228, off:186)-->phy: 15546 (frm:  60) (prv:  59)--> val:   57 == value:   57 --  + ----> pg_fault
log: 58584 0xe4d8 (pg:228, off:216)-->phy: 15576 (frm:  60) (prv:  60)--> val:    0 == value:    0 --  + ----> pg_fault

log: 27444 0x6b34 (pg:107, off: 52)-->phy: 15668 (frm:  61) (prv:  60)--> val:    0 == value:    0 --  + ----> pg_fault
log: 58982 0xe666 (pg:230, off:102)-->phy: 14950 (frm:  58) (prv:  61)--> val:   57 == value:   57 --  +    HIT!
log: 51476 0xc914 (pg:201, off: 20)-->phy: 15892 (frm:  62) (prv:  61)--> val:    0 == value:    0 --  + ----> pg_fault
log:  6796 0x1a8c (pg: 26, off:140)-->phy: 16012 (frm:  62) (prv:  62)--> val:    0 == value:    0 --  + ----> pg_fault
log: 21311 0x533f (pg: 83, off: 63)-->phy: 15935 (frm:  62) (prv:  62)--> val:   79 == value:  -49 -- fail----> pg_fault

log: 30705 0x77f1 (pg:119, off:241)-->phy: 16369 (frm:  63) (prv:  62)--> val:    0 == value:    0 --  + ----> pg_fault
log: 28964 0x7124 (pg:113, off: 36)-->phy: 16420 (frm:  64) (prv:  63)--> val:    0 == value:    0 --  + ----> pg_fault
log: 41003 0xa02b (pg:160, off: 43)-->phy: 16427 (frm:  64) (prv:  64)--> val:   74 == value:   10 -- fail----> pg_fault
log: 20259 0x4f23 (pg: 79, off: 35)-->phy: 16675 (frm:  65) (prv:  64)--> val:  -56 == value:  -56 --  + ----> pg_fault
log: 57857 0xe201 (pg:226, off:  1)-->phy: 16641 (frm:  65) (prv:  65)--> val:    0 == value:    0 --  + ----> pg_fault

log: 63258 0xf71a (pg:247, off: 26)-->phy: 16922 (frm:  66) (prv:  65)--> val:   61 == value:   61 --  + ----> pg_fault
log: 36374 0x8e16 (pg:142, off: 22)-->phy: 14358 (frm:  56) (prv:  66)--> val:   35 == value:   35 --  +    HIT!
log:   692 0x02b4 (pg:  2, off:180)-->phy: 17332 (frm:  67) (prv:  66)--> val:    0 == value:    0 --  + ----> pg_fault
log: 43121 0xa871 (pg:168, off:113)-->phy: 17521 (frm:  68) (prv:  67)--> val:    0 == value:    0 --  + ----> pg_fault
log: 48128 0xbc00 (pg:188, off:  0)-->phy: 17664 (frm:  69) (prv:  68)--> val:    0 == value:    0 --  + ----> pg_fault

log: 34561 0x8701 (pg:135, off:  1)-->phy: 17921 (frm:  70) (prv:  69)--> val:    0 == value:    0 --  + ----> pg_fault
log: 49213 0xc03d (pg:192, off: 61)-->phy: 17981 (frm:  70) (prv:  70)--> val:    0 == value:    0 --  + ----> pg_fault
log: 36922 0x903a (pg:144, off: 58)-->phy: 18234 (frm:  71) (prv:  70)--> val:   36 == value:   36 --  + ----> pg_fault
log: 59162 0xe71a (pg:231, off: 26)-->phy: 18202 (frm:  71) (prv:  71)--> val:   36 == value:   57 -- fail----> pg_fault

*/