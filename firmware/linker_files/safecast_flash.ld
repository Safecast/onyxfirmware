/*
 * Maple (STM32F103RBT6, medium density) linker script for Flash builds.
 */

/*
 * Define memory spaces.
 */
MEMORY
{
  ram (rwx) : ORIGIN = 0x20000C00, LENGTH = 45K
  rom (rx)  : ORIGIN = 0x08000000, LENGTH = 492K
}

/*
 * Use high density device vector table
 */

REGION_ALIAS("REGION_TEXT", rom);
REGION_ALIAS("REGION_DATA", ram);
REGION_ALIAS("REGION_BSS", ram);
REGION_ALIAS("REGION_RODATA", rom);

/*
 * Define the rest of the sections
 */
_FLASH_BUILD = 1;

INCLUDE common.inc
