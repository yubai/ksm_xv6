// KAUST SHARED MEMORY
#define KSM_RDWR           0x00
#define KSM_READ           0x01

// KSM info structure
struct ksminfo_t {
	uint ksmsz;            // The size of the shared memory
	int cpid;              // PID of the creator
	int mpid;              // PID of last modifier
	uint attached_nr;      // Number of attached processes
	uint atime;            // Last attach time
	uint dtime;            // Last detach time
	uint total_shrg_nr;    // Total number of existing shared regions
	uint total_shpg_nr;    // Total number of existing shared pages
};

