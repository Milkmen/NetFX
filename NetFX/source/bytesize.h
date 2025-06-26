#ifndef BYTESIZE_H
#define BYTESIZE_H

// Binary (IEC) units
#define KiB (1024ULL)
#define Kibibyte KiB
#define MiB (KiB * 1024ULL)
#define Mebibyte MiB
#define GiB (MiB * 1024ULL)
#define Gibibyte GiB
#define TiB (GiB * 1024ULL)
#define Tebibyte TiB
#define PiB (TiB * 1024ULL)
#define Pebibyte PiB
#define EiB (PiB * 1024ULL)
#define Exbibyte EiB

// Decimal (SI) units
#define KB (1000ULL)
#define Kilobyte KB
#define MB (KB * 1000ULL)
#define Megabyte MB
#define GB (MB * 1000ULL)
#define Gigabyte GB
#define TB (GB * 1000ULL)
#define Terabyte TB
#define PB (TB * 1000ULL)
#define Petabyte PB
#define EB (PB * 1000ULL)
#define Exabyte EB

#endif // !BYTESIZE_H
