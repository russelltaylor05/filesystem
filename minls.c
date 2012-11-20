#define _GNU_SOURCE
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "min.h"

#define MAXSNAKES  100
#define INITIALSTACK 2048

int main(int argc, char *argv[])
{

  FILE * file;
  size_t result;
  uint32_t *data;
  uint32_t *ibitmap;
  uint32_t *zbitmap;
  int i, zone;
  //int argError = 0;
  int verboseFlag = 0;
  char size[20];

  char *imagePath, *startLoc;

  SUPERBLOCK *diskinfo;
  INODE *node, *tempNode;
  DIRECT *direct;
  
  diskinfo = malloc(sizeof(SUPERBLOCK));
  node = malloc(sizeof(INODE));
  tempNode = malloc(sizeof(INODE));
  direct = malloc(sizeof(DIRECT));

  for(i = 1; i < argc; i++) {
    if(strcmp(*(argv+i), "-v") == 0) {
      verboseFlag = 1;    
    } else if (*(argv+i)[0] != '-') {
      imagePath = *(argv+i);
      if(i + 1 != argc) {
        startLoc = *(argv+i+1);
        i++;
      } else {
        asprintf(&startLoc, "/");
      }
    }
  }  
  //printf("iamge: %s\n", imagePath);
  printf("startLoc: %s\n\n", startLoc);
 
  file = fopen(imagePath, "r");
  if (file==NULL) {fputs ("File error",stderr); exit (1);}

  /* Super Block */
  fseek(file, 1024, SEEK_SET);
  getSuperBlock(diskinfo, file);
  
  if(verboseFlag) {
    printSuperBlock(diskinfo);
    printf("\n");
  }

  /* iNode */
  getNode(node, file, diskinfo, 1);
  if(verboseFlag) {  
    printNode(node);
    printf("\n");
  }  

  /* Inode bitmap */
  ibitmap = malloc(diskinfo->blocksize);
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
  zbitmap = malloc(diskinfo->blocksize);
  /*
  printf("Zone bitmap\n");  
  fseek(file, diskinfo->blocksize * 3, SEEK_SET);
  fread(zbitmap, diskinfo->blocksize, 1, file);
  for(i=0; i < 10; i++) {
    print_bits(zbitmap[i]);
  }  
  printf("\n");
  */
    

  
  if((node->mode & FILEMASK & DIRECTORYMASK)) {

    zone = node->zone0;
    fseek(file, diskinfo->blocksize * zone, SEEK_SET);
    result = fread(direct, sizeof(DIRECT), 1, file);

    while (*(direct->name)) {
      if(direct->inode > 0) {
        getNode(tempNode, file, diskinfo, direct->inode);
        printMode(tempNode->mode);
        putchar(' ');
        sprintf(size, "%d ", tempNode->size);
        printf("%10s", size);
        printf("%s \n", direct->name);         
      }
      fread(direct, sizeof(DIRECT), 1, file);      
    }
  }
    
  

  /* get directory */  
  /*
  printf("size(INDOE): %d\n",sizeof(INODE));
  printf("size(int): %d\n",sizeof(int));
  printf("size(uint32_t): %d\n",sizeof(uint32_t));
  */  
  data = malloc(sizeof(int32_t));
  //printf("pos: %lu   ", ftell(file));
  //result = fread(data, 1, 4, file);
  //printf("res: %d   data: %d\n", result, *data);  
  for(i=0; i < 10; i++) {
    *data = 0;
    //printf("pos: %lu   ", ftell(file));
    result = fread(data, sizeof(int32_t), 1, file);
    //printf("res: %d   data: %s\n", result, data);  
    //printf("res: %d   data: 0x%x\n", result, *data);  
    //print_bits(*data);
    break;
  }
  
  
  fclose(file);

  free(ibitmap);
  free(direct);
  free(node);
  free(diskinfo);
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
  printf("\tzone[0]:     %u\n", node->zone0);  
  printf("\tzone[1]:     %u\n", node->zone1);
  printf("\tzone[2]:     %u\n", node->zone2);  
  printf("\tzone[3]:     %u\n", node->zone3);  
  printf("\tzone[4]:     %u\n", node->zone4);  
  printf("\tzone[5]:     %u\n", node->zone5);  
  printf("\tzone[6]:     %u\n", node->zone6);      
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
