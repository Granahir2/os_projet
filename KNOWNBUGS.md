# Known bugs
- FAT32 systems are inoperative (to check, add -F 32; BPB\_BackupBootSector complains and, if bypassed,
long directory names don't work.)
- The physical memory manager doesn't work if compiled with `-O3` or `-O2`.

