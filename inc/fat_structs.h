/* FatLib Library
 * Copyright (C) 2013 by William Greiman
 *
 * This file is part of the FatLib Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the FatLib Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef FS_READERS_FAT_STRUCTS_H
#define FS_READERS_FAT_STRUCTS_H

#include <cinttypes>

/**
 * \file
 * \brief FAT file structures
 */
/*
 * mostly from Microsoft document fatgen103.doc
 * http://www.microsoft.com/whdc/system/platform/firmware/fatgen.mspx
 */
//------------------------------------------------------------------------------
/**
 * \struct fat_boot
 *
 * \brief Boot sector for a FAT12/FAT16 volume.
 *
 */
struct fat_boot {
    /**
     * The first three bytes of the boot sector must be valid,
     * executable x 86-based CPU instructions. This includes a
     * jump instruction that skips the next non-executable bytes.
     */
    uint8_t jump[3];
    /**
     * This is typically a string of characters that identifies
     * the operating system that formatted the volume.
     */
    char oemId[8];
    /**
     * The size of a hardware sector. Valid decimal values for this
     * field are 512, 1024, 2048, and 4096. For most disks used in
     * the United States, the value of this field is 512.
     */
    uint16_t bytesPerSector;
    /**
     * Number of sectors per allocation unit. This value must be a
     * power of 2 that is greater than 0. The legal values are
     * 1, 2, 4, 8, 16, 32, 64, and 128.  128 should be avoided.
     */
    uint8_t sectorsPerCluster;
    /**
     * The number of sectors preceding the start of the first FAT,
     * including the boot sector. The value of this field is always 1.
     */
    uint16_t reservedSectorCount;
    /**
     * The number of copies of the FAT on the volume.
     * The value of this field is always 2.
     */
    uint8_t fatCount;
    /**
     * For FAT12 and FAT16 volumes, this field contains the count of
     * 32-byte directory entries in the root directory. For FAT32 volumes,
     * this field must be set to 0. For FAT12 and FAT16 volumes, this
     * value should always specify a count that when multiplied by 32
     * results in a multiple of bytesPerSector.  FAT16 volumes should
     * use the value 512.
     */
    uint16_t rootDirEntryCount;
    /**
     * This field is the old 16-bit total count of sectors on the volume.
     * This count includes the count of all sectors in all four regions
     * of the volume. This field can be 0; if it is 0, then totalSectors32
     * must be non-zero.  For FAT32 volumes, this field must be 0. For
     * FAT12 and FAT16 volumes, this field contains the sector count, and
     * totalSectors32 is 0 if the total sector count fits
     * (is less than 0x10000).
     */
    uint16_t totalSectors16;
    /**
     * This dates back to the old MS-DOS 1.x media determination and is
     * no longer usually used for anything.  0xF8 is the standard value
     * for fixed (nonremovable) media. For removable media, 0xF0 is
     * frequently used. Legal values are 0xF0 or 0xF8-0xFF.
     */
    uint8_t mediaType;
    /**
     * Count of sectors occupied by one FAT on FAT12/FAT16 volumes.
     * On FAT32 volumes this field must be 0, and sectorsPerFat32
     * contains the FAT size count.
     */
    uint16_t sectorsPerFat16;
    /** Sectors per track for interrupt 0x13. Not used otherwise. */
    uint16_t sectorsPerTrack;
    /** Number of heads for interrupt 0x13.  Not used otherwise. */
    uint16_t headCount;
    /**
     * Count of hidden sectors preceding the partition that contains this
     * FAT volume. This field is generally only relevant for media
     * visible on interrupt 0x13.
     */
    uint32_t hidddenSectors;
    /**
     * This field is the new 32-bit total count of sectors on the volume.
     * This count includes the count of all sectors in all four regions
     * of the volume.  This field can be 0; if it is 0, then
     * totalSectors16 must be non-zero.
     */
    uint32_t totalSectors32;
    /**
     * Related to the BIOS physical drive number. Floppy drives are
     * identified as 0x00 and physical hard disks are identified as
     * 0x80, regardless of the number of physical disk drives.
     * Typically, this value is set prior to issuing an INT 13h BIOS
     * call to specify the device to access. The value is only
     * relevant if the device is a boot device.
     */
    uint8_t driveNumber;
    /** used by Windows NT - should be zero for FAT */
    uint8_t reserved1;
    /** 0X29 if next three fields are valid */
    uint8_t bootSignature;
    /**
     * A random serial number created when formatting a disk,
     * which helps to distinguish between disks.
     * Usually generated by combining date and time.
     */
    uint32_t volumeSerialNumber;
    /**
     * A field once used to store the volume label. The volume label
     * is now stored as a special file in the root directory.
     */
    char volumeLabel[11];
    /**
     * A field with a value of either FAT, FAT12 or FAT16,
     * depending on the disk format.
     */
    char fileSystemType[8];
    /** X86 boot code */
    uint8_t bootCode[448];
    /** must be 0X55 */
    uint8_t bootSectorSig0;
    /** must be 0XAA */
    uint8_t bootSectorSig1;
}__attribute__((packed));
/** Type name for FAT Boot Sector */
typedef struct fat_boot fat_boot_t;
//------------------------------------------------------------------------------
/**
 * \struct directoryEntry
 * \brief FAT short directory entry
 *
 * Short means short 8.3 name, not the entry size.
 *
 * Date Format. A FAT directory entry date stamp is a 16-bit field that is
 * basically a date relative to the MS-DOS epoch of 01/01/1980. Here is the
 * format (bit 0 is the LSB of the 16-bit word, bit 15 is the MSB of the
 * 16-bit word):
 *
 * Bits 9-15: Count of years from 1980, valid value range 0-127
 * inclusive (1980-2107).
 *
 * Bits 5-8: Month of year, 1 = January, valid value range 1-12 inclusive.
 *
 * Bits 0-4: Day of month, valid value range 1-31 inclusive.
 *
 * Time Format. A FAT directory entry time stamp is a 16-bit field that has
 * a granularity of 2 seconds. Here is the format (bit 0 is the LSB of the
 * 16-bit word, bit 15 is the MSB of the 16-bit word).
 *
 * Bits 11-15: Hours, valid value range 0-23 inclusive.
 *
 * Bits 5-10: Minutes, valid value range 0-59 inclusive.
 *
 * Bits 0-4: 2-second count, valid value range 0-29 inclusive (0 - 58 seconds).
 *
 * The valid time range is from Midnight 00:00:00 to 23:59:58.
 */
struct directoryEntry {
    /** Short 8.3 name.
     *
     * The first eight bytes contain the file name with blank fill.
     * The last three bytes contain the file extension with blank fill.
     */
    uint8_t name[11];
    /** Entry attributes.
     *
     * The upper two bits of the attribute byte are reserved and should
     * always be set to 0 when a file is created and never modified or
     * looked at after that.  See defines that begin with DIR_ATT_.
     */
    uint8_t attributes;
    /**
     * Reserved for use by Windows NT. Set value to 0 when a file is
     * created and never modify or look at it after that.
     */
    uint8_t reservedNT;
    /**
     * The granularity of the seconds part of creationTime is 2 seconds
     * so this field is a count of tenths of a second and its valid
     * value range is 0-199 inclusive. (WHG note - seems to be hundredths)
     */
    uint8_t creationTimeTenths;
    /** Time file was created. */
    uint16_t creationTime;
    /** Date file was created. */
    uint16_t creationDate;
    /**
     * Last access date. Note that there is no last access time, only
     * a date.  This is the date of last read or write. In the case of
     * a write, this should be set to the same date as lastWriteDate.
     */
    uint16_t lastAccessDate;
    /**
     * High word of this entry's first cluster number (always 0 for a
     * FAT12 or FAT16 volume).
     */
    uint16_t firstClusterHigh;
    /** Time of last write. File creation is considered a write. */
    uint16_t lastWriteTime;
    /** Date of last write. File creation is considered a write. */
    uint16_t lastWriteDate;
    /** Low word of this entry's first cluster number. */
    uint16_t firstClusterLow;
    /** 32-bit unsigned holding this file's size in bytes. */
    uint32_t fileSize;
}__attribute__((packed));
//------------------------------------------------------------------------------
// Definitions for directory entries
//
/** Type name for directoryEntry */
typedef struct directoryEntry dir_t;

#endif //FS_READERS_FAT_STRUCTS_H
