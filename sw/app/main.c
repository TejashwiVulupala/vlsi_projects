#include <stdint.h>

// --- CORRECT HARDWARE MEMORY MAP (Based on hdl/system.v) ---
// Address decoding uses upper 4 bits [31:28]
// sel_uart = 0x2, sel_sqrt = 0x4, sel_crc = 0x6, sel_ram = 0x0

#define UART_TX    *(volatile uint32_t*)0x20000000  // UART-Lite base address

// FPSQRT IP Base Address: 0x4000_0000
#define SQRT_DATA  *(volatile uint32_t*)0x40000000  // Write input / Read output
#define SQRT_CTRL  *(volatile uint32_t*)0x40000004  // Status/control register

// CRC-32 IP Base Address: 0x6000_0000
#define CRC_DATA   *(volatile uint32_t*)0x60000000  // CRC data input
#define CRC_POLY   *(volatile uint32_t*)0x60000004  // Polynomial (reconfigurable)
#define CRC_CTRL   *(volatile uint32_t*)0x60000008  // Start/status control

void print_str(const char *s) {
    while (*s) UART_TX = *s++;
}

// Software SQRT (Baseline for Comparison)
uint32_t sw_sqrt(uint32_t n) {
    if (n < 2) return n;
    uint32_t x = n;
    uint32_t y = (x + 1) >> 1; // Use shift to avoid __udivsi3
    while (y < x) {
        x = y;
        y = (x + n / x) >> 1; 
    }
    return x;
}

void main() {
    print_str("\n--- SOC BENCHMARK START ---\n");

    // --- 1. FPSQRT BENCHMARK ---
    // (Software baseline calculation)
    uint32_t res_sw = sw_sqrt(144);
    print_str("SW SQRT(144) = ");
    
    // (Hardware acceleration via AXI-Lite)
    SQRT_DATA = 144; // Write to HW at 0x40000000
    while(!(SQRT_CTRL & 0x1)); // Poll ready bit at 0x40000004
    uint32_t res_hw = SQRT_DATA; // Read result
    print_str("HW SQRT(144) = ");

    // --- 2. CRC-32 RECONFIG BENCHMARK ---
    print_str("\nCRC32 Benchmark:\n");
    CRC_POLY = 0x04C11DB7; // Ethernet Poly at 0x60000004 (Reconfigurable)
    CRC_DATA = 0xDEADBEEF; // Data payload at 0x60000000
    CRC_CTRL = 0x1;        // Trigger HW Start bit at 0x60000008
    while(!(CRC_CTRL & 0x2)); // Poll ready bit
    
    print_str("CRC32(0xDEADBEEF) = ");

    print_str("\nBenchmarks Finished. Trap to Report.\n");
    // Execution returns to start.S which calls ebreak
}
