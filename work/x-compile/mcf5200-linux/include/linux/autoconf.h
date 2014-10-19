/*
 * Automatically generated C config: don't edit
 */
#define AUTOCONF_INCLUDED
#define CONFIG_UCLINUX 1

/*
 * Code maturity level options
 */
#define CONFIG_EXPERIMENTAL 1

/*
 * Loadable module support
 */
#define CONFIG_MODULES 1
#undef  CONFIG_MODVERSIONS
#undef  CONFIG_KERNELD

/*
 * Platform dependant setup
 */
#undef  CONFIG_M68000
#undef  CONFIG_M68328
#undef  CONFIG_M68EZ328
#undef  CONFIG_M68332
#undef  CONFIG_M68360
#undef  CONFIG_M5206
#undef  CONFIG_M5206e
#define CONFIG_M5307 1
#undef  CONFIG_M5204

/*
 * Platform
 */
#define CONFIG_COLDFIRE 1
#undef  CONFIG_ARNEWSH
#define CONFIG_NETtel 1
#undef  CONFIG_eLIA
#undef  CONFIG_CADRE3
#define CONFIG_RAMKERNEL 1
#undef  CONFIG_ROMKERNEL

/*
 * General setup
 */
#undef  CONFIG_PCI
#define CONFIG_NET 1
#undef  CONFIG_SYSVIPC
#define CONFIG_REDUCED_MEMORY 1
#define CONFIG_BINFMT_FLAT 1
#define CONFIG_KERNEL_ELF 1
#undef  CONFIG_CONSOLE

/*
 * Floppy, IDE, and other block devices
 */
#define CONFIG_BLK_DEV_BLKMEM 1
#define CONFIG_BLK_VFDISK 1
#undef  CONFIG_BLK_DEV_IDE

/*
 * Additional Block Devices
 */
#undef  CONFIG_BLK_DEV_LOOP
#undef  CONFIG_BLK_DEV_MD
#define CONFIG_BLK_DEV_RAM 1
#undef  CONFIG_RD_RELEASE_BLOCKS

/*
 * Networking options
 */
#undef  CONFIG_FIREWALL
#undef  CONFIG_NET_ALIAS
#define CONFIG_INET 1
#undef  CONFIG_IP_FORWARD
#undef  CONFIG_IP_MULTICAST
#undef  CONFIG_SYN_COOKIES
#undef  CONFIG_IP_ACCT
#undef  CONFIG_IP_ROUTER
#undef  CONFIG_NET_IPIP

/*
 * (it is safe to leave these untouched)
 */
#undef  CONFIG_INET_PCTCP
#undef  CONFIG_INET_RARP
#undef  CONFIG_NO_PATH_MTU_DISCOVERY
#undef  CONFIG_IP_NOSR
#undef  CONFIG_SKB_LARGE

/*
 *  
 */
#undef  CONFIG_IPX
#undef  CONFIG_ATALK
#undef  CONFIG_AX25
#undef  CONFIG_BRIDGE
#undef  CONFIG_NETLINK

/*
 * Network device support
 */
#define CONFIG_NETDEVICES 1
#undef  CONFIG_DUMMY
#undef  CONFIG_SLIP
#undef  CONFIG_PPP
#undef  CONFIG_EQUALIZER
#undef  CONFIG_UCCS8900
#undef  CONFIG_SMC9194
#undef  CONFIG_NE2000
#define CONFIG_MX98726 1

/*
 * Filesystems
 */
#undef  CONFIG_QUOTA
#undef  CONFIG_MINIX_FS
#undef  CONFIG_EXT_FS
#define CONFIG_EXT2_FS 1
#undef  CONFIG_XIA_FS
#undef  CONFIG_NLS
#define CONFIG_PROC_FS 1
#define CONFIG_NFS_FS 1
#undef  CONFIG_ROOT_NFS
#define CONFIG_SMB_FS 1
#define CONFIG_SMB_WIN95 1
#undef  CONFIG_HPFS_FS
#undef  CONFIG_SYSV_FS
#undef  CONFIG_AUTOFS_FS
#undef  CONFIG_AFFS_FS
#define CONFIG_ROMFS_FS 1
#undef  CONFIG_UFS_FS

/*
 * Character devices
 */
#define CONFIG_COLDFIRE_SERIAL 1
#undef  CONFIG_MCF_MBUS
#undef  CONFIG_LCDDMA
#undef  CONFIG_DAC0800
#undef  CONFIG_DACI2S
#undef  CONFIG_T6963
#define CONFIG_WATCHDOG 1

/*
 * Kernel hacking
 */
#undef  CONFIG_PROFILE
#undef  CONFIG_MAGIC_SYSRQ
#undef  CONFIG_DUMPTOFLASH
