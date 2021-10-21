#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>



struct Superblock {
    int s_inodes_count;
    int s_blocks_count;
    int s_r_blocks_count;
    int s_free_blocks_count;
    int s_free_inodes_count;
    int s_first_data_block;
    int s_log_block_size;
    int s_log_frag_size;
    int s_blocks_per_group;
    int s_frags_per_group;
    int s_inodes_per_group;
    int s_mtime;
    int s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    int	s_lastcheck;
    int	s_checkinterval;
    int	s_creator_os;
    int	s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;

    int	s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    int	s_feature_compat;
    int	s_feature_incompat;
    int	s_feature_ro_compat;
    char[16] s_uuid;
    char[16] s_volume_name;
    char[64] s_last_mounted;
    int	s_algo_bitmap;

    uint8_t	s_prealloc_blocks;
    uint8_t	s_prealloc_dir_blocks;
    uint16_t useless_1;

    char[16] s_journal_uuid;
    int	s_journal_inum;
    int	s_journal_dev;
    int	s_last_orphan;

    int[4] s_hash_seed;
    uint8_t	s_def_hash_version;
    uint16_t useless_2;
    uint8_t useless_3;

    int	s_default_mount_options;
    int	s_first_meta_bg;
};
