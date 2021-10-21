#ifndef __STRUCTS__
#define __STRUCTS__

#define I_MODE_FOLDER 16893 //значение i_mode в inode соответсвующее директории

struct superblock {
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
    char s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    int	s_algo_bitmap;

    uint8_t	s_prealloc_blocks;
    uint8_t	s_prealloc_dir_blocks;
    uint16_t useless_1;

    char s_journal_uuid[16];
    int	s_journal_inum;
    int	s_journal_dev;
    int	s_last_orphan;

    int s_hash_seed[4];
    uint8_t	s_def_hash_version;
    uint16_t useless_2;
    uint8_t useless_3;

    int	s_default_mount_options;
    int	s_first_meta_bg;
};


struct block_group_descriptor_table {
    int   bg_block_bitmap;
    int   bg_inode_bitmap;
    int   bg_inode_table;
    uint16_t   bg_free_blocks_count;
    uint16_t   bg_free_inodes_count;
    uint16_t   bg_used_dirs_count;
    uint16_t   bg_pad;
};




struct inode {
    uint16_t i_mode;
    uint16_t i_uid;
    int i_size;
    int i_atime;
    int i_ctime;
    int i_mtime;
    int i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    int i_blocks;
    int i_flags;
    int i_osd1;
    int i_block[15];
    int i_generation;
    int i_file_acl;
    int i_dir_acl;
    int i_faddr;
    int i_osd2;
};




struct directory_entry {
    int inode_number;
    uint16_t record_length;
    uint8_t name_length;
    uint8_t file_type;
    char name[255];
};

#endif
