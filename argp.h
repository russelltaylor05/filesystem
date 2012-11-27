#ifndef ARGP
#define ARGP

typedef struct argsp
{
  int sflag = 0;
  int vflag = 0;  
  int pflag = 0;
  int hflag = 0;
  int svalue;
  int pvalue;      
  int error;
  int partsCount;
  char *image;
  char *path;
  char *pathParts[30];
} ARGSP;


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

#endif