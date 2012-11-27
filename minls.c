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
#include "min.h"
#include "argp.h"


#define MAXSNAKES  100
#define INITIALSTACK 2048

int main(int argc, char *argv[])
{

  FILE * file;
  //size_t result;
  uint32_t *ibitmap;
  uint32_t *zbitmap;
  int i, j, partFound, indirectCount, indirectNode;
  int zoneCount = 0;
  //int argError = 0;


  SUPERBLOCK *diskinfo;
  INODE *node, *tempNode;
  DIRECT *direct;
  ARGSP *argsp;
  
  diskinfo = malloc(sizeof(SUPERBLOCK));
  node = malloc(sizeof(INODE));
  tempNode = malloc(sizeof(INODE));
  direct = malloc(sizeof(DIRECT));
  argsp = malloc(sizeof(ARGSP));

  ibitmap = malloc(diskinfo->blocksize);
  zbitmap = malloc(diskinfo->blocksize);

  getArgs(argsp, argc, argv);

  if(argsp->hflag) {
    printUsage();
    return OK;
  }
   
  file = fopen(argsp->image, "r");
  if (file==NULL) {fputs ("File error",stderr); exit (1);}

  /* Super Block */
  getSuperBlock(diskinfo, file);
  
  /* grab root/first node */
  getNode(node, file, diskinfo, 1);

  
  /* Traverse pathParts */  
  for(i = 0; i < argsp->partsCount; i++) {
    
    partFound = 0;
    indirectCount = 0;
    /* set back to zone[0] for new node */
    zoneCount = 0;
    fseek(file, diskinfo->blocksize * node->zone[zoneCount], SEEK_SET);
    
    /* Here we look through each directory entry */
    for(j= 0; j < node->size / sizeof(DIRECT); j++) {
      fread(direct, sizeof(DIRECT), 1, file); 
  
      if(!strcmp(argsp->pathParts[i], direct->name) && direct->inode != 0) {        
        getNode(node, file, diskinfo, direct->inode);
        partFound = 1;
      }

      /* check to see if we are at the end of a block and need to move to next zone*/
      if(((j+1) * sizeof(DIRECT) % diskinfo->blocksize) == 0) {
        if (zoneCount < 6) {
          zoneCount++;
          fseek(file, diskinfo->blocksize * node->zone[zoneCount], SEEK_SET);
        } else {
          /* jump to indirect block plus offset */
          fseek(file, diskinfo->blocksize * node->zoneindirect + (sizeof(uint32_t) * indirectCount), SEEK_SET);
          indirectCount++;
          
          /* grab new zone value */
          fread(&indirectNode, sizeof(uint32_t), 1, file);           
          
          /* set filepointer to new indirect zone */
          fseek(file, diskinfo->blocksize * indirectNode, SEEK_SET);
        }
      }
    }

    /* if any part of path not found throw error */
    if(!partFound) {
      printf("%s: File not found.\n", argsp->path);
      exit(0);
    } 
  }
  
  /*
  fseek(file, diskinfo->blocksize * 32, SEEK_SET);
  for(i = 0; i < 10; i++) {
    fread(direct, sizeof(DIRECT), 1, file);
    printf("node: %d \t name: %s\n", direct->inode, direct->name);
  }

  fseek(file, diskinfo->blocksize * 28, SEEK_SET);
  for(i = 0; i < 10; i++) {
    fread(&nodes, sizeof(uint32_t), 1, file);
    printf("node: %d \n", nodes);
  } 
  */ 
  
  
  /* Output */  
  if(argsp->vflag) {  
    printSuperBlock(diskinfo);
    printf("\n");
    printNode(node);
    printf("\n");
  }  
  
  zoneCount = 0;
  indirectCount = 0;  
  
  /* current node will be the matching diretory or file */
  if((node->mode & FILEMASK & DIRECTORYMASK)) {

    printf("%s:\n", argsp->path);
    fseek(file, diskinfo->blocksize * node->zone[zoneCount], SEEK_SET);

    for(i = 0; i < node->size / sizeof(DIRECT); i++) {
      fread(direct, sizeof(DIRECT), 1, file);

      if(direct->inode) {
        getNode(tempNode, file, diskinfo, direct->inode);
        printItem(tempNode, direct->name);
      }

      /* check to see if we are at the end of a block and need to move to next zone*/
      if(((i+1) * sizeof(DIRECT) % diskinfo->blocksize) == 0) {
        if (zoneCount < 6) {
          zoneCount++;
          fseek(file, diskinfo->blocksize * node->zone[zoneCount], SEEK_SET);
        } else {

          /* jump to indirect block plus offset */
          fseek(file, diskinfo->blocksize * node->zoneindirect + (sizeof(uint32_t) * indirectCount), SEEK_SET);
          indirectCount++;

          /* grab new zone id */
          fread(&indirectNode, sizeof(uint32_t), 1, file);

          /* set filepointer to new indirect zone/block */
          fseek(file, diskinfo->blocksize * indirectNode, SEEK_SET);
        }
      }
      
    }
  } else {
    printItem(node, argsp->path);
  }
    
  
  
  free(argsp);
  free(ibitmap);
  free(direct);
  free(node);
  free(diskinfo);
  fclose(file);
  
  return 0;
}


void getNode(INODE *node, FILE *fp, SUPERBLOCK *disk, int number) 
{
  uint32_t nodeAddress;
  long originalPos = ftell(fp);
  
  /* calculate inode address */
  nodeAddress = disk->blocksize * disk->firstIblock;
  nodeAddress += sizeof(INODE) * (number-1);

  fseek(fp, nodeAddress, SEEK_SET);
  fread(node, sizeof(INODE), 1, fp);

  /* set back to original position  */   
  fseek(fp, originalPos, SEEK_SET); 
}

void getSuperBlock(SUPERBLOCK *disk, FILE *fp) 
{
  fseek(fp, 1024, SEEK_SET);  
  fread(disk, 1, 32 , fp); // sizeof(SUPERBLOCK) 
  disk->zonesize = disk->blocksize << disk->log_zone_size;
  disk->firstIblock = 2 + disk->i_blocks + disk->z_blocks;
  disk->ino_per_block = disk->blocksize / sizeof(INODE);
  disk->zones_per_block = disk->blocksize / disk->zonesize; 
}


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
  printf("  ptrs_per_zone   %d\n", disk->ptrs_per_zone);
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
  printf("usage: given/minls  [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n");
  printf("Options:");
  printf("  -p	 part    --- select partition for filesystem (default: none)\n");
  printf("  -s	 sub     --- select partition for filesystem (default: none)");
  printf("  -h	 help    --- print usage information and exit");    
  printf("  -v	 verbose --- increase verbosity level");
}


/**** Debug *****/
void print_bits(uint32_t number){
   unsigned long mask = 1 << 31;
   int cnt = 1;
   printf("H  ");
   while(mask){
      (mask & number) ? printf("X") : printf(".");
      mask = mask >> 1 ;      
      if(!(cnt % 8)){
         putchar('|');
      }
      cnt++;
   }
   printf(" L\n");
}


  /*
  fread(&(disk->ninodes), sizeof(uint32_t), 1, fp);
  fseek(fp, 2, SEEK_CUR); // skip padding
  fread(&(disk->i_blocks), 1, 2, fp);
  fread(&(disk->z_blocks), 1, 2, fp);
  fread(&(disk->firstdata), 1, 2, fp);
  fread(&(disk->log_zone_size), 1, 2, fp);
  fseek(fp, 2, SEEK_CUR); // skip padding  
  fread(&(disk->max_file), sizeof(uint32_t), 1, fp);  
  fread(&(disk->zones), sizeof(uint32_t), 1, fp);  
  fread(&(disk->magic), 1, 2, fp);
  fseek(fp, 2, SEEK_CUR); // skip padding  
  fread(&(disk->blocksize), 1, 2, fp);
  fread(&(disk->subversion), 1, 2, fp);
  */
  
  
    /* Inode bitmap */
  /*
  printf("iNode bitmap\n");
  fseek(file, diskinfo->blocksize * 2, SEEK_SET);
  fread(ibitmap, diskinfo->blocksize, 1, file);
  for(i=0; i < 10; i++) {
    print_bits(ibitmap[i]);
  }  
  printf("\n");
  */


  /* Zone bitmap */
  /*
  printf("Zone bitmap\n");  
  fseek(file, diskinfo->blocksize * 3, SEEK_SET);
  fread(zbitmap, diskinfo->blocksize, 1, file);
  for(i=0; i < 10; i++) {
    print_bits(zbitmap[i]);
  }  
  printf("\n");
  */

