struct flash_partition_map {
    char *chip_name;
    int flash_size;
    int bootimage_size;
    int bootscr_size;
    int devicetree_size;
    int kernel_size;
    int rootfs_size;
};

#define FLASH_MAP(cn, sz, bis, bss, dts, ks, rs) \
{               \
    .chip_name = cn, \
    .flash_size = sz,   \
    .bootimage_size = bis, \
    .bootscr_size = bss,    \
    .devicetree_size = dts,  \
    .kernel_size = ks,  \
    .rootfs_size = rs,  \
}


/* Last array item should contain default offset map in case of missing chip name */
#ifdef CONFIG_ARCH_ZYNQMP
static struct flash_partition_map flash_maps[] = {
    FLASH_MAP("15eg", 64, 0x01b00000, 0x00080000, 0x00080000, 0x00e00000, 0),
    FLASH_MAP("default", 0, 0x01b00000, 0x00080000, 0x00080000, 0x00e00000, 0),
};
#else
static struct flash_partition_map flash_maps[] = {
    FLASH_MAP("7z010", 16, 0x00600000, 0x00080000, 0x00080000, 0x00500000, 0x00380000),
    FLASH_MAP("7z015", 16, 0x00600000, 0x00080000, 0x00080000, 0x00500000, 0x00380000),
    FLASH_MAP("7z020", 16, 0x00600000, 0x00080000, 0x00080000, 0x00500000, 0x00380000),
    FLASH_MAP("7z030", 16, 0x00700000, 0x00080000, 0x00080000, 0x00500000, 0x03280000),
    FLASH_MAP("7z030", 64, 0x00700000, 0x00080000, 0x00080000, 0x00500000, 0x03280000),
    FLASH_MAP("7z035", 64, 0x00E00000, 0x00080000, 0x00080000, 0x00500000, 0),
    FLASH_MAP("7z045", 64, 0x00E00000, 0x00080000, 0x00080000, 0x00500000, 0),
    FLASH_MAP("default", 0, 0x00E00000, 0x00080000, 0x00080000, 0x00500000, 0),
};
#endif

static inline int setenv_hex_if_empty(const char *varname, ulong value)
{
    if (getenv(varname) == NULL)
        setenv_hex(varname, value);

    return 0;
}

// return best match for chip name and flash size
struct flash_partition_map *match_flash_entry(char *cn, int sz)
{
    int i;
    struct flash_partition_map *fm = NULL;
    for(i=0; i<ARRAY_SIZE(flash_maps); i++){
        if (!strcmp(cn, flash_maps[i].chip_name) && (sz >= flash_maps[i].flash_size)){
            if (!fm || (flash_maps[i].flash_size > fm->flash_size))
                fm = &flash_maps[i];
        }
    }
    /* use default map if match not found */
    if(!fm){
        fm = &flash_maps[ARRAY_SIZE(flash_maps)-1];
    }
    return fm;
}

static inline int calculate_rootfs_size(struct flash_partition_map* fm, int fsz)
{
    int space_left;

    space_left = fsz*1024*1024 - (fm->bootimage_size + fm->bootscr_size +
                                 fm->devicetree_size + fm->kernel_size +
                                 QSPI_BOOTARGS_SIZE);

    if(space_left < 0)
        space_left = 0;

    return space_left;
}

static inline int setup_qspi_args(int flash_sz, char *chip_name)
{
    struct flash_partition_map *fm;
    int boot_off, env_off, kern_off, dtb_off, bscr_off, rfs_off;

    fm = match_flash_entry(chip_name, flash_sz);
    if(fm)
    if(fm){
       boot_off = QSPI_BOOT_OFFSET;
       env_off = boot_off + fm->bootimage_size;
       kern_off = env_off + QSPI_BOOTARGS_SIZE;
       dtb_off = kern_off + fm->kernel_size;
       bscr_off = dtb_off + fm->devicetree_size;
       rfs_off = bscr_off + fm->bootscr_size;
       if(fm->rootfs_size == 0)
            fm->rootfs_size = calculate_rootfs_size(fm, flash_sz);
       setenv_hex_if_empty("ramdisk_size", fm->rootfs_size);
       setenv_hex_if_empty("jffs2_size", fm->rootfs_size);
       setenv_hex_if_empty("ubifs_size", fm->rootfs_size);
       setenv_hex_if_empty("kernel_size", fm->kernel_size);
       setenv_hex_if_empty("devicetree_size", fm->devicetree_size);
       setenv_hex_if_empty("bootscript_size", fm->bootscr_size);
       setenv_hex_if_empty("bootimage_size", fm->bootimage_size);
       setenv_hex_if_empty("fullboot_size", flash_sz*1024*1024);
       setenv_hex_if_empty("qspi_bootimage_offset", boot_off);
       setenv_hex_if_empty("qspi_kernel_offset", kern_off);
       setenv_hex_if_empty("qspi_ramdisk_offset", rfs_off);
       setenv_hex_if_empty("qspi_devicetree_offset", dtb_off);
       setenv_hex_if_empty("qspi_bootscript_offset", bscr_off);
    }

    return 0;
}