#ifndef LWPH
#define LWPH

#define _GNU_SOURCE
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#define FILEMASK		    0170000
#define DIRECTORYMASK		040000
#define REGULAR_MASK		0100000
#define MAGIC           0x4d5a

#define VALID_BYTE      510
#define PART_TABLE_OFF  0x1BE
#define MINIX_PART      0x81
#define SECTOR_SIZE     512


typedef struct ptable
{
  uint8_t bootind;
  uint8_t start_head;
  uint8_t start_sec;
  uint8_t start_cyl;
  uint8_t type;
  uint8_t end_head;
  uint8_t end_sec;
  uint8_t end_cyl;
  uint32_t lfirst;  
  uint32_t size;  
} PTABLES;


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
  uint32_t zonesize;
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


typedef struct argsp
{
  int sflag;
  int vflag;  
  int pflag;
  int hflag;
  int svalue;
  int pvalue;      
  int error;
  int partsCount;
  char *image;
  char *path;
  char *dest;
  char *pathParts[30];
} ARGSP;


void getArgs(ARGSP *argsp, int argc, char *argv[]);

INODE* minInitialize(FILE *file, 
                     SUPERBLOCK *diskinfo, 
                     ARGSP *argsp,
                     int *offset);

void getSuperBlock(SUPERBLOCK *disk, 
                   FILE *fp);
void getNode(INODE *node, 
             FILE *fp, 
             SUPERBLOCK *disk, 
             int number, 
             int partOffset); 

void printSuperBlock(SUPERBLOCK *disk);
void printNode(INODE *node);
void printMode(uint16_t mode);
void printItem(INODE *node, char *name);
void printUsage();

/* Partition Declarations*/
void printPTable(PTABLES *ptable);
int checkPTable(FILE *file, int offset);
int getPartition(FILE *file, ARGSP *argsp);


void print_bits(uint32_t number);
#endif