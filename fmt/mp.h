6350 // See MultiProcessor Specification Version 1.[14]
6351 
6352 struct mp {             // floating pointer
6353   uchar signature[4];           // "_MP_"
6354   void *physaddr;               // phys addr of MP config table
6355   uchar length;                 // 1
6356   uchar specrev;                // [14]
6357   uchar checksum;               // all bytes must add up to 0
6358   uchar type;                   // MP system config type
6359   uchar imcrp;
6360   uchar reserved[3];
6361 };
6362 
6363 struct mpconf {         // configuration table header
6364   uchar signature[4];           // "PCMP"
6365   ushort length;                // total table length
6366   uchar version;                // [14]
6367   uchar checksum;               // all bytes must add up to 0
6368   uchar product[20];            // product id
6369   uint *oemtable;               // OEM table pointer
6370   ushort oemlength;             // OEM table length
6371   ushort entry;                 // entry count
6372   uint *lapicaddr;              // address of local APIC
6373   ushort xlength;               // extended table length
6374   uchar xchecksum;              // extended table checksum
6375   uchar reserved;
6376 };
6377 
6378 struct mpproc {         // processor table entry
6379   uchar type;                   // entry type (0)
6380   uchar apicid;                 // local APIC id
6381   uchar version;                // local APIC verison
6382   uchar flags;                  // CPU flags
6383     #define MPBOOT 0x02           // This proc is the bootstrap processor.
6384   uchar signature[4];           // CPU signature
6385   uint feature;                 // feature flags from CPUID instruction
6386   uchar reserved[8];
6387 };
6388 
6389 struct mpioapic {       // I/O APIC table entry
6390   uchar type;                   // entry type (2)
6391   uchar apicno;                 // I/O APIC id
6392   uchar version;                // I/O APIC version
6393   uchar flags;                  // I/O APIC flags
6394   uint *addr;                  // I/O APIC address
6395 };
6396 
6397 
6398 
6399 
6400 // Table entry types
6401 #define MPPROC    0x00  // One per processor
6402 #define MPBUS     0x01  // One per bus
6403 #define MPIOAPIC  0x02  // One per I/O APIC
6404 #define MPIOINTR  0x03  // One per bus interrupt source
6405 #define MPLINTR   0x04  // One per system interrupt source
6406 
6407 
6408 
6409 
6410 
6411 
6412 
6413 
6414 
6415 
6416 
6417 
6418 
6419 
6420 
6421 
6422 
6423 
6424 
6425 
6426 
6427 
6428 
6429 
6430 
6431 
6432 
6433 
6434 
6435 
6436 
6437 
6438 
6439 
6440 
6441 
6442 
6443 
6444 
6445 
6446 
6447 
6448 
6449 
