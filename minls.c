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

int main(int argc, char *argv[])
{
  FILE * file;
  uint32_t *ibitmap;
  uint32_t *zbitmap;
  int i, indirectCount, indirectNode;
  int zoneCount = 0;
  int partitionOffset = 0;

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
    exit(1);
  }
  
  
  //printf("argsp->image: %s\n",argsp->image);   
  file = fopen(argsp->image, "r");
  if (file == NULL) {
    perror("Some Error:");
    exit (1);
  }
  
  node = minInitialize(file, diskinfo, argsp, &partitionOffset);
  
    
  /* current node will be the matching diretory or file */
  if((node->mode & FILEMASK & DIRECTORYMASK)) {

    zoneCount = 0; 
    indirectCount = 0;  

    printf("%s:\n", argsp->path);    
    fseek(file, partitionOffset + diskinfo->zonesize 
      * node->zone[zoneCount], SEEK_SET);

    for(i = 0; i < node->size / sizeof(DIRECT); i++) {
      fread(direct, sizeof(DIRECT), 1, file);

      if(direct->inode) {
        getNode(tempNode, file, diskinfo, direct->inode, partitionOffset);
        printItem(tempNode, direct->name);
      }

      /* check to see if we are at the end of a zone 
         and need to move to next zone*/
      if(((i+1) * sizeof(DIRECT) % diskinfo->zonesize) == 0) {
        if (zoneCount < 6) {
          zoneCount++;
          fseek(file, partitionOffset + diskinfo->zonesize 
            * node->zone[zoneCount], SEEK_SET);
        } else {
        
          /* jump to indirect block plus offset */
          fseek(file, partitionOffset + diskinfo->zonesize 
            * node->zoneindirect 
            + (sizeof(uint32_t) * indirectCount), SEEK_SET);
          indirectCount++;
          
          /* grab new zone value */
          fread(&indirectNode, sizeof(uint32_t), 1, file);           
          
          /* set filepointer to new indirect zone */
          fseek(file, partitionOffset +  diskinfo->zonesize 
            * indirectNode, SEEK_SET);
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
  
  exit(0);
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
  

