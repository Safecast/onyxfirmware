#ifndef MPR121_H
#define MPR121_H
//MPR121.h - MPR121 Captouch sensor register definitions
//April 8, 2010
//by: Jim Lindblom, modifications by Nava Whiteford and others.

// Touch state registers
#define TCH_STATL 0x00
#define TCH_STATH 0x01
#define TCH_OORL  0x02
#define TCH_OORH  0x03

// Filter configuration registers
#define MHD_R     0x2B // Maximum Half Delta Rising
#define NHD_R     0x2C // Noise Half Delta Rising
#define NCL_R     0x2D // Noise Count Limit Rising
#define FDL_R     0x2E // Filter Delay Count Limit Rising
#define MHD_F     0x2F // Maximum Half Delta Falling
#define NHD_F     0x30 // Noise Half Delta Falling
#define NCL_F     0x31 // Noise Count Limit Falling
#define FDL_F     0x32 // Filter Delay Count Limit Falling
#define NHD_T     0x33 // Noise Half Delta Touched
#define NCL_T     0x34 // Noise Count Limit Touched
#define FDL_T     0x35 // Filter Delay Count Limit Touched

#define DBR       0x5B // Debounce Register
#define AFE_CONF  0x5C // Filter/Global CDC Conf Register
#define FIL_CFG   0x5D // Filter/Global CDT Conf Register
#define ELE_CFG   0x5E // Electrode Conf Register

#define ATO_CFG0  0x7B // Auto-configure control 0
#define ATO_CFG1  0x7C // Auto-configure control 1
#define ATO_CFGU  0x7D // Up-side limit register
#define ATO_CFGL  0x7E // Low-side limit register
#define ATO_CFGT  0x7F // Target level reigster

//GPIO Reigsters
#define GPIO_CTRL0 0x73
#define GPIO_CTRL1 0x74
#define GPIO_DATA 0x75
#define GPIO_DIR  0x76
#define GPIO_EN   0x77
#define GPIO_SET  0x78
#define GPIO_CLEAR 0x79
#define GPIO_TOGGLE 0x7A

// Electrode thresholds
#define ELE0_T    0x41
#define ELE0_R    0x42
#define ELE1_T    0x43
#define ELE1_R    0x44
#define ELE2_T    0x45
#define ELE2_R    0x46
#define ELE3_T    0x47
#define ELE3_R    0x48
#define ELE4_T    0x49
#define ELE4_R    0x4A
#define ELE5_T    0x4B
#define ELE5_R    0x4C
#define ELE6_T    0x4D
#define ELE6_R    0x4E
#define ELE7_T    0x4F
#define ELE7_R    0x50
#define ELE8_T    0x51
#define ELE8_R    0x52
#define ELE9_T    0x53
#define ELE9_R    0x54
#define ELE10_T   0x55
#define ELE10_R   0x56
#define ELE11_T   0x57
#define ELE11_R   0x58

// Electrode data
#define ELE0_DATAL 0x04
#define ELE0_DATAH 0x05
#define ELE1_DATAL 0x06
#define ELE1_DATAH 0x07
#define ELE2_DATAL 0x08
#define ELE2_DATAH 0x09
#define ELE3_DATAL 0x0A
#define ELE3_DATAH 0x0B
#define ELE4_DATAL 0x0C
#define ELE4_DATAH 0x0D
#define ELE5_DATAL 0x0E
#define ELE5_DATAH 0x0F
#define ELE6_DATAL 0x10
#define ELE6_DATAH 0x11
#define ELE7_DATAL 0x12
#define ELE7_DATAH 0x13
#define ELE8_DATAL 0x14
#define ELE8_DATAH 0x15
#define ELE9_DATAL 0x16
#define ELE9_DATAH 0x1A
#define ELE10_DATAL 0x1B
#define ELE10_DATAH 0x1C
#define ELE11_DATAL 0x1D
#define ELE11_DATAH 0x1E

#define ELE0_BASE 0x1E
#define ELE1_BASE 0x1F
#define ELE2_BASE 0x20
#define ELE3_BASE 0x21
#define ELE4_BASE 0x22
#define ELE5_BASE 0x23
#define ELE6_BASE 0x24
#define ELE7_BASE 0x25
#define ELE8_BASE 0x26
#define ELE9_BASE 0x27
#define ELE10_BASE 0x28


// Global Constants


#endif
