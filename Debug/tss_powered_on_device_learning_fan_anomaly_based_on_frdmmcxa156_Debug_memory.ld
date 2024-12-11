/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright 2008-2013 Code Red Technologies Ltd,
 * Copyright 2013-2024 NXP
 * Generated linker script file for MCXA156
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.30
 * MCUXpresso IDE v11.10.0 [Build 3148] [2024-07-03] on Dec 11, 2024, 4:13:25 PM
 */

MEMORY
{
  /* Define each memory region */
  PROGRAM_FLASH (rx) : ORIGIN = 0x0, LENGTH = 0xfc000 /* 1008K bytes (alias Flash) */  
  m_model (rx) : ORIGIN = 0xfc000, LENGTH = 0x4000 /* 16K bytes (alias Flash2) */  
  SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 0x1e000 /* 120K bytes (alias RAM) */  
  SRAMX (rwx) : ORIGIN = 0x4000000, LENGTH = 0x3000 /* 12K bytes (alias RAM2) */  
}

  /* Define a symbol for the top of each memory region */
  __base_PROGRAM_FLASH = 0x0  ; /* PROGRAM_FLASH */  
  __base_Flash = 0x0 ; /* Flash */  
  __top_PROGRAM_FLASH = 0x0 + 0xfc000 ; /* 1008K bytes */  
  __top_Flash = 0x0 + 0xfc000 ; /* 1008K bytes */  
  __base_m_model = 0xfc000  ; /* m_model */  
  __base_Flash2 = 0xfc000 ; /* Flash2 */  
  __top_m_model = 0xfc000 + 0x4000 ; /* 16K bytes */  
  __top_Flash2 = 0xfc000 + 0x4000 ; /* 16K bytes */  
  __base_SRAM = 0x20000000  ; /* SRAM */  
  __base_RAM = 0x20000000 ; /* RAM */  
  __top_SRAM = 0x20000000 + 0x1e000 ; /* 120K bytes */  
  __top_RAM = 0x20000000 + 0x1e000 ; /* 120K bytes */  
  __base_SRAMX = 0x4000000  ; /* SRAMX */  
  __base_RAM2 = 0x4000000 ; /* RAM2 */  
  __top_SRAMX = 0x4000000 + 0x3000 ; /* 12K bytes */  
  __top_RAM2 = 0x4000000 + 0x3000 ; /* 12K bytes */  
