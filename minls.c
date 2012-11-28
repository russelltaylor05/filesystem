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

  /* Grab Args */
  getArgs(argsp, argc, argv);  
  if(argsp->hflag) {
    printUsage();
    exit(1);
  }
  
  /* Open File */
  file = fopen(argsp->image, "r");
  if (file == NULL) {
    perror("Some Error:");
    exit (1);
  }

  /* Grab Partition offset */
  if((partitionOffset = getPartition(file, argsp)) < 0) {
    fputs("Bad partition\n",stderr);
    exit(1);
  }  

  /* Grab node for file */
  if(!(node = minInitialize(file, diskinfo, argsp, partitionOffset))) {
    fputs("File not found\n",stderr);
    exit(1);
  }
  
    
  /* current node should be the matching diretory or file */
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

        if (zoneCount < REGULAR_ZONES-1) {
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
      
  freeArgs(argsp);
  free(argsp);
  free(direct);
  free(tempNode);
  free(node);
  free(diskinfo);
  fclose(file);
  
  exit(0);
}

