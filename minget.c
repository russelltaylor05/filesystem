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
  char *buffer, *zeroBuffer;
  int size, tempSize, zoneQty, dataZone;
  int partitionOffset = 0;
  int processedZones = 0;
  int indirectCount = 0;  
  int double1 = 0;      // Level 1 of Double Indirect      
  int double2 = 0;      // Level 2 of Double Indirect      
  int holeFlag = 0;     // Flag for file hole

  SUPERBLOCK *diskinfo;
  INODE *node;
  ARGSP *argsp;

  diskinfo = malloc(sizeof(SUPERBLOCK));
  node = malloc(sizeof(INODE));
  argsp = malloc(sizeof(ARGSP));

  /* Grab Args */
  getArgs(argsp, argc, argv);  
  if(argsp->hflag) {
    printUsage();
    exit(1);
  }
  
  /* Open File */
  if ((file = fopen(argsp->image, "r")) == NULL) {
    perror("File Error");
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

  /* Check if regular file */
  if((node->mode & FILEMASK & REGULAR_MASK)) {

    tempSize = node->size;    
    buffer = (char*) malloc (sizeof(char)*(diskinfo->zonesize));
    zeroBuffer = (char*) calloc (sizeof(char)*(diskinfo->zonesize), 1);

    /* Open destination file for writting */
    if((dest = fopen(argsp->dest, "w")) == NULL){
      perror("File Error");
      exit (1);    
    }
  
    /* How many zones need to hold data? */
    if(node->size < diskinfo->zonesize){
      zoneQty = 1;
    } else if(node->size % diskinfo->zonesize == 0) {
      zoneQty = node->size / diskinfo->zonesize;
    } else {
      zoneQty = node->size / diskinfo->zonesize + 1;  
    }     

    while(processedZones < zoneQty)  {

      holeFlag = 0; /* reset hole flag */

      /* Are we looking for a full zones worth of data? */
      if(tempSize >= diskinfo->zonesize) {
        tempSize = tempSize - diskinfo->zonesize;        
        size = diskinfo->zonesize;
      } else {
        size = tempSize;
      }

      /* Main Zones */                        
      if (processedZones < REGULAR_ZONES) { 

        if(!(dataZone = node->zone[processedZones])) {
          holeFlag = 1;
        }

      /* Indirect Zones*/
      } else if (processedZones < diskinfo->ind_per_zone + REGULAR_ZONES){ 
      
        
        dataZone = grabIndirect(file, 
                                diskinfo->zonesize, 
                                partitionOffset, 
                                node->zoneindirect,
                                indirectCount);
        if (!dataZone || !node->zoneindirect) {
          holeFlag = 1;
        }        
        indirectCount++;  

      /* Double Indirect Zones*/  
      } else { 

        if(double2 > diskinfo->ind_per_zone - 1) {
          double1++;
          double2 = 0;          
        }
        
        dataZone = grabIndirect(file, 
                                diskinfo->zonesize, 
                                partitionOffset, 
                                node->zonedouble,
                                double1);        
        if(dataZone) {
          dataZone = grabIndirect(file, 
                                  diskinfo->zonesize, 
                                  partitionOffset, 
                                  dataZone,
                                  double2);
        }

        if (!dataZone || !node->zonedouble) {
          holeFlag = 1;
        }                
        double2++;  
      
      }
      
      /* Write data */
      fseek(file, partitionOffset +  
                  diskinfo->zonesize * dataZone
                  , SEEK_SET);
      if(holeFlag) {
        fwrite(zeroBuffer, 1, size, dest);
      } else {
        fread (buffer, 1, size, file);
        fwrite(buffer, 1, size, dest);
      }

      processedZones++;   
    }
 
    fclose(dest);
  
  } else {
    fputs("Not a regular file\n",stderr);
    exit(1);
  }
  

  freeArgs(argsp);
  free(buffer);
  free(zeroBuffer);  
  free(argsp);
  free(node);
  free(diskinfo);
  fclose(file);
    
  exit(0);
}
