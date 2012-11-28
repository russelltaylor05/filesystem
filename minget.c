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

  FILE *file;
  FILE *dest;
  uint32_t *ibitmap;
  uint32_t *zbitmap;
  int partitionOffset = 0;
  char *buffer;
  char *single;
  int i;

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
  
  file = fopen(argsp->image, "r");
  if (file == NULL) {
    perror("Some Error:");
    exit (1);
  }
  
  node = minInitialize(file, diskinfo, argsp, &partitionOffset);

  
  /* Check for regular file */
  if((node->mode & FILEMASK & REGULAR_MASK)) {


    //printNode(node);  
    //printf("Dest: %s\n", argsp->dest);  
    dest = fopen(argsp->dest, "w");
  
    buffer = (char*) malloc (sizeof(char)*(node->size + 3));
    single = (char*) malloc (sizeof(char));    
  
    fseek(file, partitionOffset +  diskinfo->zonesize 
                * node->zone[0], SEEK_SET);
                
    //printf("size: %d\n", node->size);
    
    fread (buffer, 1, node->size, file);
    fwrite(buffer, 1, node->size, dest);


  
  } else {
    fputs("Not a regular file\n",stderr);
    exit(1);
  }



  
  free(buffer);
  free(argsp);
  free(ibitmap);
  free(direct);
  free(node);
  free(diskinfo);
  fclose(file);
    
  exit(0);
}

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