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

#include "min.h"


/*
 * verfies disk image and find node match
 * RETURNS: pointer to a matched iNode
 * RETURNS: NULL if nothing found
 */
INODE* minInitialize(FILE *file, 
                     SUPERBLOCK *diskinfo, 
                     ARGSP *argsp, 
                     int partitionOffset) 
{
  int i, j, partFound, indirectCount, indirectNode;
  int zoneCount = 0;
  
  INODE *node;
  DIRECT *direct;
  
  node = malloc(sizeof(INODE));  
  direct = malloc(sizeof(DIRECT));

  /* Super Block */
  fseek(file, partitionOffset + SUPER_OFFSET, SEEK_SET);  
  getSuperBlock(diskinfo, file);
  
  /* Check Magic Number */
  if(diskinfo->magic != MAGIC) {
    printf("Bad magic number. (0x%x)\n", diskinfo->magic);
    printf("This doesn't look like a MINIX filesystem.\n");
    exit(1);
  }
  
  /* grab root/first node */
  getNode(node, file, diskinfo, 1, partitionOffset);  

  /* Traverse pathParts */  
  for(i = 0; i < argsp->partsCount; i++) {
    
    partFound = 0;
    indirectCount = 0;    
    zoneCount = 0; /* set back to zone[0] for new node */
    fseek(file, partitionOffset + diskinfo->zonesize 
      * node->zone[zoneCount], SEEK_SET);
    
    /* Here we look through each directory entry */
    for(j= 0; j < node->size / sizeof(DIRECT); j++) {
      fread(direct, sizeof(DIRECT), 1, file); 
  
      if(!strcmp(argsp->pathParts[i], direct->name) 
        && direct->inode != 0) {        
        getNode(node, file, diskinfo, direct->inode, partitionOffset);
        partFound = 1;
      }

      /* check to see if we are at the end of a zone 
         and need to move to next zone*/
      if(((j+1) * sizeof(DIRECT) % diskinfo->zonesize) == 0) {
        if (zoneCount < REGULAR_ZONES-1) {
          zoneCount++;
          fseek(file, partitionOffset + diskinfo->zonesize 
                  * node->zone[zoneCount], SEEK_SET);
        } else {
          /* jump to indirect block plus offset */
          fseek(file, partitionOffset 
                      + diskinfo->zonesize 
                      * node->zoneindirect + (sizeof(uint32_t) 
                      * indirectCount), SEEK_SET);
          indirectCount++;
          
          /* grab new zone value */
          fread(&indirectNode, sizeof(uint32_t), 1, file);           
          
          /* set filepointer to new indirect zone */
          fseek(file, partitionOffset + diskinfo->zonesize 
            * indirectNode, SEEK_SET);
        }
      }
    }

    /* if any part of path not found return NULL */
    if(!partFound) {
      return NULL;
    } 
  }  

  if(argsp->vflag) {  
    printSuperBlock(diskinfo);
    printf("\n");
    printNode(node);
    printf("\n");
  }  

  free(direct);
  return node;
}


/*
 * Grab node based on node number
 * and the partition offset
 */
void getNode(INODE *node, 
             FILE *fp, 
             SUPERBLOCK *disk, 
             int number, 
             int partOffset) 
{
  uint32_t nodeAddress;
  long originalPos = ftell(fp);
  
  /* calculate inode address */
  nodeAddress = disk->blocksize * disk->firstIblock;
  nodeAddress += sizeof(INODE) * (number-1);

  fseek(fp, nodeAddress + partOffset, SEEK_SET);
  fread(node, sizeof(INODE), 1, fp);

  /* set back to original position  */   
  fseek(fp, originalPos, SEEK_SET); 
}


/* 
 * assumes file pointer is in correct position 
 */
void getSuperBlock(SUPERBLOCK *disk, FILE *fp) 
{
  fread(disk, 1, 32 , fp); // sizeof(SUPERBLOCK)   
  disk->zonesize = disk->blocksize << disk->log_zone_size;
  disk->firstIblock = 2 + disk->i_blocks + disk->z_blocks;
  disk->ino_per_block = disk->blocksize / sizeof(INODE);
  disk->zones_per_block = disk->blocksize / disk->zonesize; 
  disk->ind_per_zone = disk->zonesize / sizeof(uint32_t);
}



/* 
 * Returns indirect Zone Id
 */
int grabIndirect(FILE *file, 
                 int zoneSize,
                 int partitionOffset,
                 int startZone,
                 int indirectCount) 
{
  int indirectZone;

  /* jump to indirect block plus offset */
  fseek(file, partitionOffset + 
              zoneSize * startZone  + 
              (sizeof(uint32_t) * indirectCount)
              , SEEK_SET);
  
  /* grab new zone value */
  fread(&indirectZone, sizeof(uint32_t), 1, file);

  return indirectZone;
}


/**********************/
/**  Print Outs      **/
/**********************/

void printNode(INODE *node)
{
  printf("File inode:\n");
  printf("  mode:     0o%o\n", node->mode);
  printf("  links:    %d\n", node->links);
  printf("  uid:      %d\n", node->uid);
  printf("  gid:      %d\n", node->gid);
  printf("  size:     %u\n", node->size);
  printf("  atime:    %u\n", node->atime);
  printf("  mtime:    %u\n", node->mtime);
  printf("  ctime:    %u\n", node->ctime);  
  printf("\n");
  printf("  Zones\n");
  printf("\tzone[0]:     %u\n", node->zone[0]);  
  printf("\tzone[1]:     %u\n", node->zone[1]);
  printf("\tzone[2]:     %u\n", node->zone[2]);  
  printf("\tzone[3]:     %u\n", node->zone[3]);  
  printf("\tzone[4]:     %u\n", node->zone[4]);  
  printf("\tzone[5]:     %u\n", node->zone[5]);  
  printf("\tzone[6]:     %u\n", node->zone[6]);      
  printf("\tindirect:    %u\n", node->zoneindirect);      
  printf("\tdouble:      %u\n", node->zonedouble);
  
}


void printSuperBlock(SUPERBLOCK *disk) 
{
  printf("Superblock Contents:\n");
  printf("Stored Fields:\n");  
  printf("  ninodes:        %u\n", disk->ninodes);
  printf("  i_blocks:       %d\n", disk->i_blocks);
  printf("  z_blocks:       %d\n", disk->z_blocks);
  printf("  firstdata:      %d\n", disk->firstdata);
  printf("  log_zone_size:  %d\n", disk->log_zone_size);
  printf("  max_file:       %u\n", disk->max_file);
  printf("  magic:          0x%x\n", disk->magic);
  printf("  zones:          %d\n", disk->zones);
  printf("  blocksize:      %d\n", disk->blocksize);
  printf("  subversion:     %d\n", disk->subversion); 

  printf("Computed Fields:\n");
  printf("  firstIblock     %d\n", disk->firstIblock);
  printf("  zonesize        %d\n", disk->zonesize);
  printf("  ind_per_zone    %d\n", disk->ind_per_zone);
  printf("  ino_per_block   %d\n", disk->ino_per_block);
  printf("  wrongended      %d\n", disk->wrongended);
}

void printItem(INODE *node, char *name)
{  
  printMode(node->mode);
  putchar(' ');
  printf("%9d ", node->size);
  //printf("%4d ", direct->inode);
  printf("%s\n", name);
}


void printMode(uint16_t mode) 
{
  if(mode & FILEMASK & DIRECTORYMASK) {putchar('d');} else {putchar('-');}
  if(mode & 0400) {putchar('r');} else {putchar('-');}
  if(mode & 0200) {putchar('w');} else {putchar('-');}
  if(mode & 0100) {putchar('x');} else {putchar('-');}    
  if(mode & 040) {putchar('r');} else {putchar('-');}
  if(mode & 020) {putchar('w');} else {putchar('-');}
  if(mode & 01) {putchar('x');} else {putchar('-');}    
  if(mode & 04) {putchar('r');} else {putchar('-');}
  if(mode & 02) {putchar('w');} else {putchar('-');}
  if(mode & 01) {putchar('x');} else {putchar('-');}    
}

void printUsage()
{
  printf("usage: given/minls ");
  printf("[ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n");
  printf("Options:\n");
  printf(" -p	 part    -- select partition for filesystem (default: none)\n");
  printf(" -s	 sub     -- select partition for filesystem (default: none)\n");
  printf(" -h	 help    -- print usage information and exit\n");    
  printf(" -v	 verbose -- increase verbosity level\n");
}



/**********************/
/**  ARGS            **/
/**********************/


void getArgs(ARGSP *argsp, int argc, char *argv[]) 
{
  int c, index;
  char *pch;
  char *tempPath;
  index = 0;
  
  
  while ((c = getopt (argc, argv, "p:s:vh")) != -1) {
    switch (c)
    {
      case 'p':
        argsp->pflag = 1;
        argsp->pvalue = atoi(optarg);
        break;
      case 's':
        argsp->sflag = 1;
        argsp->svalue = atoi(optarg);
        break;
      case 'v':
        argsp->vflag = 1;
        break;
      case 'h':
        argsp->hflag = 1;
        break;
    }
  }
  argsp->image = argv[optind];

  if(optind + 1 < argc) {
    asprintf(&(argsp->path), argv[optind + 1]);
  } else {
    asprintf(&(argsp->path), "/");
  }

  if(optind + 2 < argc) {
    asprintf(&(argsp->dest), argv[optind + 2]);
  } else {
    asprintf(&(argsp->dest), "/dev/stdout");
  }

  
  asprintf(&tempPath, argsp->path);
  pch = strtok(tempPath,"/");
  c = 0;
  while (pch != NULL)
  {
    argsp->partsCount++;
    asprintf(&(argsp->pathParts[c]), pch);
    c++;
    pch = strtok (NULL, "/");
  }
  /*
  printf ("Splitting string \"%s\" into tokens:\n",argsp->path);
  for(c = 0; c < argsp->partsCount; c++) {
    printf("part%d: %s, ", c, argsp->pathParts[c]);
  }
  printf("\n");
  printf("ARGS\n");
  printf("   optind: %d   argc: %d\n", optind, argc);
  printf ("   v = %d, s = %d, p = %d\n", 
    argsp->vflag, argsp->sflag, argsp->pflag);
  printf ("   s = %d, p = %d\n", 
    argsp->svalue, argsp->pvalue);
  printf ("   partCount = %d \n", argsp->partsCount);
  printf ("   image = %s \n", argsp->image);
  printf ("   path = %s\n", argsp->path);
  for (index = optind; index < argc; index++) {
    printf ("   Non-option argument %s\n", argv[index]);  
  }
  printf("\n");
  */
}

void freeArgs(ARGSP *argsp)
{
  int i;
  
  for(i = 0; i < MAX_PATH_PARTS; i++) {
    if(argsp->pathParts[i]) {
      free(argsp->pathParts[i]);
    }
  }
}



/**********************/
/**  Partition Stuff **/
/**********************/

void printPTable(PTABLES *ptable)
{
  printf("  first:    %u\n", ptable->lfirst);
  printf("  size:     %u\n", ptable->size);
  printf("  type:     0x%x\n", ptable->type);
  printf("\n");  
}


int checkPTable(FILE *file, int offset)
{
  uint8_t value1; 
  uint8_t value2; 
  
  fseek(file, offset + VALID_BYTE, SEEK_SET);
  fread(&value1, 1, 1, file); 
  fread(&value2, 1, 1, file);
  
  //printf("1: %x   2: %x\n", value1, value2);
    
  if(value1 == 0x55 && value2 == 0xAA) {
    return 1;
  } else {
    return 0;
  }  
}


/* Returns the offset in bytes to the start
 * of a partition/subpartition if present.
 * Returns 0 if no partition
 * Returns -1 if there is an error
 */
int getPartition(FILE *file, ARGSP *argsp)
{

  PTABLES *ptable, *subtable;
  int offset = 0;

  ptable = malloc(sizeof(PTABLES));
  subtable = malloc(sizeof(PTABLES));
  

  /* Check for Partitions */
  if(argsp->pflag) {

    if(!checkPTable(file, offset)){
      fputs("Invalid Partition Table\n", stderr);
      return -1;
    }
    
    fseek(file, PART_TABLE_OFF + 
                sizeof(PTABLES) * argsp->pvalue 
                , SEEK_SET);    
    fread(ptable, sizeof(PTABLES), 1 , file);

    if(ptable->type != MINIX_PART) {
      fputs("Not a Minix Partition1\n", stderr);
      return -1;
    }    
    offset = ptable->lfirst * SECTOR_SIZE;

    if(argsp->vflag) {
      printf("Partition Table:\n");
      printPTable(ptable);
    }
    
    /* Subpartition */
    if(argsp->sflag) {
    
      if(!checkPTable(file, offset)){
        fputs("Invalid Partition Table\n", stderr);
        return -1;
      }
    
      fseek(file, offset +  PART_TABLE_OFF 
                  + 
                  sizeof(PTABLES) * argsp->svalue , SEEK_SET);
      fread(subtable, sizeof(PTABLES), 1 , file);

      if(subtable->type != MINIX_PART) {
        fputs("Not a Minix Partition2\n", stderr);
        return -1;        
      }        
      offset = subtable->lfirst * SECTOR_SIZE;

      if(argsp->vflag) {
        printf("Sub Partition Table:\n");
        printPTable(subtable);
      }

    }

  }
  
  free(ptable);
  free(subtable);  
  return offset;
}


