#ifndef LWPH
#define LWPH
#include <sys/types.h>
#include <stdint.h>

#define FILEMASK		    0170000
#define DIRECTORYMASK		040000

typedef struct direct
{
  uint32_t inode;
  char name[60];  
} DIRECT;

typedef struct super_block
{
  uint32_t ninodes;
  uint16_t unused;
  uint16_t i_blocks;
  uint16_t z_blocks;
  uint16_t firstdata;
  uint16_t log_zone_size;
  uint16_t padding1;
  uint32_t max_file;
  uint32_t zones;
  uint16_t magic;
  uint16_t padding2;
  uint16_t blocksize;
  uint16_t subversion;

  /* computed */
  uint16_t firstIblock;
  uint16_t zonesize;
  uint16_t ptrs_per_zone;
  uint16_t zones_per_block;   
  uint16_t ino_per_block;
  uint16_t wrongended;   
} SUPERBLOCK;



typedef struct inode
{
  uint16_t mode;
  uint16_t links;
  uint16_t uid;
  uint16_t gid;
  uint32_t size;
  uint32_t atime;  
  uint32_t mtime;
  uint32_t ctime;

  /* Zones */
  uint32_t zone[7];
  uint32_t zoneindirect;
  uint32_t zonedouble;  
  uint32_t unused;  
  
} INODE;

void getSuperBlock(SUPERBLOCK *disk, FILE *fp);
void printSuperBlock(SUPERBLOCK *disk);

void getNode(INODE *node, FILE *fp, SUPERBLOCK *disk, int number); 
void printNode(INODE *node);

void printMode(uint16_t mode);
void printItem(INODE *node, char *name);
void printUsage();


void print_bits(uint32_t number);
#endif