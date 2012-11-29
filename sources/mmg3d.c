/** mmg3d: 3d mesh adaptation
 *
 * Written by Cecile Dobrzynski (IMB), Charles Dapogny and Pascal Frey (LJLL)
 * Copyright (c) 2004- IMB/LJLL.
 * All rights reserved.
 */
#include "compil.date"
#include "mmg3d.h"

/* global */
Info   info;
unsigned char inxt2[3] = {1,2,0};
unsigned char iprv2[3] = {2,0,1};
unsigned char idir[4][3] = { {1,2,3}, {0,3,2}, {0,1,3}, {0,2,1} };
char idirinv[4][4] = {{-1,0,1,2},{0,-1,2,1},{0,1,-1,2},{0,2,1,-1}};
unsigned char iarf[4][3] = { {5,4,3}, {5,1,2}, {4,2,0}, {3,0,1} };
unsigned char iarfinv[4][6] = { {-1,-1,-1,2,1,0}, {-1,1,2,-1,-1,0},{2,-1,1,-1,0,-1},{1,2,-1,0,-1,-1}};
unsigned char inxt3[7] = { 1,2,3,0,1,2,3 };
unsigned char iprv3[7] = { 3,0,1,2,3,0,1 };
unsigned char iare[6][2] = { {0,1}, {0,2}, {0,3}, {1,2}, {1,3}, {2,3} };
unsigned char ifar[6][2] = { {2,3}, {1,3}, {1,2}, {0,3}, {0,2}, {0,1} };
unsigned char isar[6][2] = { {2,3}, {3,1}, {1,2}, {0,3}, {2,0}, {0,1} };
unsigned char arpt[4][3] = { {0,1,2}, {0,4,3}, {1,3,5}, {2,5,4} };

static void excfun(int sigid) {
  fprintf(stdout,"\n Unexpected error:");  fflush(stdout);
  switch(sigid) {
  case SIGABRT:
    fprintf(stdout,"  *** potential lack of memory.\n");  break;
  case SIGFPE:
    fprintf(stdout,"  Floating-point exception\n"); break;
  case SIGILL:
    fprintf(stdout,"  Illegal instruction\n"); break;
  case SIGSEGV:
    fprintf(stdout,"  Segmentation fault\n");  break;
  case SIGTERM:
  case SIGINT:
    fprintf(stdout,"  Program killed\n");  break;
  }
  exit(1);
}


static void usage(char *prog) {
  fprintf(stdout,"\nUsage: %s [-v [n]] [opts..] filein [fileout]\n",prog);

  fprintf(stdout,"\n** Generic options :\n");
  fprintf(stdout,"-h      Print this message\n");
  fprintf(stdout,"-v [n]  Tune level of verbosity, [-10..10]\n");
  fprintf(stdout,"-m [n]  Set memory size to n Mbytes\n");
  fprintf(stdout,"-d      Turn on debug mode\n");

  fprintf(stdout,"\n**  File specifications\n");
  fprintf(stdout,"-in  file  input triangulation\n");
  fprintf(stdout,"-out file  output triangulation\n");
  fprintf(stdout,"-sol file  load solution file\n");

  fprintf(stdout,"\n**  Parameters\n");
  fprintf(stdout,"-ar val    angle detection\n");
  fprintf(stdout,"-nr        no angle detection\n");
  fprintf(stdout,"-hmin val  minimal mesh size\n");
  fprintf(stdout,"-hmax val  maximal mesh size\n");
  fprintf(stdout,"-hausd val control Hausdorff distance\n");
  fprintf(stdout,"-hgrad val control gradation\n");
  fprintf(stdout,"-ls        levelset meshing \n");

  exit(1);
}


static int parsar(int argc,char *argv[],pMesh mesh,pSol met) {
  int     i;
  char   *ptr;

  i = 1;
  while ( i < argc ) {
    if ( *argv[i] == '-' ) {
      switch(argv[i][1]) {
      case '?':
        usage(argv[0]);
        break;

      case 'a':
        if ( !strcmp(argv[i],"-ar") && ++i < argc ) {
          info.dhd = atof(argv[i]);
          info.dhd = MG_MAX(0.0, MG_MIN(180.0,info.dhd));
          info.dhd = cos(info.dhd*M_PI/180.0);
        }
        break;
      case 'd':  /* debug */
        info.ddebug = 1;
        break;
      case 'h':
        if ( !strcmp(argv[i],"-hmin") && ++i < argc )
          info.hmin = atof(argv[i]);
        else if ( !strcmp(argv[i],"-hmax") && ++i < argc )
          info.hmax = atof(argv[i]);
        else if ( !strcmp(argv[i],"-hausd") && ++i <= argc ) {
          info.hausd = atof(argv[i]);
        }
        else if ( !strcmp(argv[i],"-hgrad") && ++i <= argc ) {
          info.hgrad = atof(argv[i]);
          if ( info.hgrad < 0.0 )
            info.hgrad = -1.0;
          else
            info.hgrad = log(info.hgrad);
        }
        else
          usage(argv[0]);
        break;
      case 'i':
        if ( !strcmp(argv[i],"-in") ) {
          if ( ++i < argc && isascii(argv[i][0]) && argv[i][0]!='-') {
            mesh->namein = (char*) calloc(strlen(argv[i])+1,sizeof(char));
            strcpy(mesh->namein,argv[i]);
            info.imprim = 5;
          }else{
            fprintf(stderr,"Missing filname for %c%c\n",argv[i-1][1],argv[i-1][2]);
            usage(argv[0]);
          }
        }
        break;
      case 'l':
        if ( !strcmp(argv[i],"-ls") ) {
          info.iso = 1;
          if ( ++i < argc && isdigit(argv[i][0]) ) {
            info.ls = atof(argv[i]);
          }else{
            fprintf(stderr,"Missing argument option %c%c\n",argv[i-1][1],argv[i-1][2]);
            usage(argv[0]);
          }
        }
        break;
      case 'm':  /* memory */
        if ( ++i < argc && isdigit(argv[i][0]) )
          info.mem = atoi(argv[i]);
        else {
          fprintf(stderr,"Missing argument option %c\n",argv[i-1][1]);
          usage(argv[0]);
        }
        break;
      case 'n':
        if ( !strcmp(argv[i],"-nr") )
          info.dhd = -1.0;
        break;
      case 'o':
        if ( !strcmp(argv[i],"-out") ) {
          if ( ++i < argc && isascii(argv[i][0])  && argv[i][0]!='-') {
            mesh->nameout = (char*) calloc(strlen(argv[i])+1,sizeof(char));
            strcpy(mesh->nameout,argv[i]);
          }else{
            fprintf(stderr,"Missing filname for %c%c%c\n",
                    argv[i-1][1],argv[i-1][2],argv[i-1][3]);
            usage(argv[0]);
          }
        }
        break;
      case 's':
        if ( !strcmp(argv[i],"-sol") ) {
          if ( ++i < argc && isascii(argv[i][0]) && argv[i][0]!='-' ) {
            met->namein = (char*) calloc(strlen(argv[i])+1,sizeof(char));
            strcpy(met->namein,argv[i]);
          }else{
            fprintf(stderr,"Missing filname for %c%c%c\n",
                    argv[i-1][1],argv[i-1][2],argv[i-1][3]);
            usage(argv[0]);
          }
        }
        break;
      case 'v':
        if ( ++i < argc ) {
          if ( argv[i][0] == '-' || isdigit(argv[i][0]) )
            info.imprim = atoi(argv[i]);
          else
            i--;
        }
        else {
          fprintf(stderr,"Missing argument option %c\n",argv[i-1][1]);
          usage(argv[0]);
        }
        break;
      default:
        fprintf(stderr,"Unrecognized option %s\n",argv[i]);
        usage(argv[0]);
      }
    }
    else {
      if ( mesh->namein == NULL ) {
        mesh->namein = (char*) calloc(strlen(argv[i])+1,sizeof(char));
        strcpy(mesh->namein,argv[i]);
        if ( info.imprim == -99 )  info.imprim = 5;
      }
      else if ( mesh->nameout == NULL ){
        mesh->nameout = (char*) calloc(strlen(argv[i])+1,sizeof(char));
        strcpy(mesh->nameout,argv[i]);
      }
      else {
        fprintf(stdout,"Argument %s ignored\n",argv[i]);
        usage(argv[0]);
      }
    }
    i++;
  }

  /* check file names */
  if ( info.imprim == -99 ) {
    fprintf(stdout,"\n  -- PRINT (0 10(advised) -10) ?\n");
    fflush(stdin);
    fscanf(stdin,"%d",&i);
    info.imprim = i;
  }

  if ( mesh->namein == NULL ) {
    mesh->namein = (char *)calloc(128,sizeof(char));
    assert(mesh->namein);
    fprintf(stdout,"  -- INPUT MESH NAME ?\n");
    fflush(stdin);
    fscanf(stdin,"%s",mesh->namein);
  }
  if ( mesh->nameout == NULL ) {
    mesh->nameout = (char *)calloc(128,sizeof(char));
    assert(mesh->nameout);
    strcpy(mesh->nameout,mesh->namein);
    ptr = strstr(mesh->nameout,".mesh");
    if ( ptr ) *ptr = '\0';
    strcat(mesh->nameout,".o.mesh");
    ptr = strstr(mesh->nameout,".meshb");
    if ( ptr )  strcat(mesh->nameout,"b");
  }

  if ( met->namein == NULL ) {
    met->namein = (char *)calloc(128,sizeof(char));
    assert(met->namein);
    strcpy(met->namein,mesh->namein);
    ptr = strstr(met->namein,".mesh");
    if ( ptr ) *ptr = '\0';
    strcat(met->namein,".sol");
  }

  if ( met->nameout == NULL ) {
    met->nameout = (char *)calloc(128,sizeof(char));
    assert(met->nameout);
    strcpy(met->nameout,mesh->nameout);
    ptr = strstr(met->nameout,".mesh");
    if ( ptr ) *ptr = '\0';
    strcat(met->nameout,".sol");
  }
  return(1);
}

/** Deallocations before return */
static void freeAll(pMesh mesh,pSol met){
  free(mesh->point);
  mesh->point=NULL;
  free(mesh->tetra);
  mesh->tetra=NULL;
  free(mesh->adja);
  mesh->adja=NULL;
  free(mesh->nameout);
  mesh->nameout=NULL;
  free(mesh->namein);
  mesh->namein=NULL;
  if(mesh->xpoint){
    free(mesh->xpoint);
    mesh->xpoint=NULL;
  }
  if(mesh->htab.geom){
    free(mesh->htab.geom);
    mesh->htab.geom=NULL;
  }
  if(mesh->tria){
    free(mesh->tria);
    mesh->tria=NULL;
  }
  if(met ->namein){
    free(met ->namein);
    met->namein=NULL;
  }
  if(met ->nameout){
    free(met ->nameout);
    met->nameout=NULL;
  }
  if(met ->m){
    free(met ->m);
    met->m=NULL;
  }
  if(mesh->xtetra){
    free(mesh->xtetra);
    mesh->xtetra=NULL;
  }
}

static void endcod() {
  char    stim[32];

  chrono(OFF,&info.ctim[0]);
  printim(info.ctim[0].gdif,stim);
  fprintf(stdout,"\n   ELAPSED TIME  %s\n",stim);
}


/** set function pointers */
static void setfunc(pMesh mesh,pSol met) {
  if ( met->size < 6 ) {
    caltet = caltet_iso;
    lenedg = lenedg_iso;
    defsiz = defsiz_iso;
    gradsiz = gradsiz_iso;
  }
  else {
    caltet = caltet_ani;
    lenedg = lenedg_ani;
    /*defsiz = defsiz_ani;
      gradsiz = gradsiz_ani;*/
  }
}

int main(int argc,char *argv[]) {
  Mesh      mesh;
  Sol       met;
  int       ier;
  char      stim[32];

  fprintf(stdout,"  -- MMG3d, Release %s (%s) \n",MG_VER,MG_REL);
  fprintf(stdout,"     %s\n",MG_CPY);
  fprintf(stdout,"    %s\n",COMPIL);

  signal(SIGABRT,excfun);
  signal(SIGFPE,excfun);
  signal(SIGILL,excfun);
  signal(SIGSEGV,excfun);
  signal(SIGTERM,excfun);
  signal(SIGINT,excfun);
  atexit(endcod);

  tminit(info.ctim,TIMEMAX);
  chrono(ON,&info.ctim[0]);

  /* assign default values */
  memset(&mesh,0,sizeof(Mesh));
  memset(&met,0,sizeof(Sol));
  info.imprim = -99;
  info.ddebug = 0;
  info.mem    = -1;
  info.iso    = 0;
  info.dhd    = ANGEDG;
  info.hmin   = 0.0;
  info.hmax   = FLT_MAX;
  info.hgrad  = 0.1;
  info.hausd  = 0.01;
  info.ls     = 0.0;
  info.fem    = 0;

  /* command line */
  if ( !parsar(argc,argv,&mesh,&met) )  return(1);
  /* load data */
  fprintf(stdout,"\n  -- INPUT DATA\n");
  chrono(ON,&info.ctim[1]);
  /* read mesh file */
  if ( !loadMesh(&mesh) ) RETURN_AND_FREE(&mesh,&met,1);
  met.npmax = mesh.npmax;
  /* read metric if any */
  ier = loadMet(&met);
  if ( !ier )
    RETURN_AND_FREE(&mesh,&met,1);
  else if ( ier > 0 && met.np != mesh.np ) {
    fprintf(stdout,"  ## WARNING: WRONG SOLUTION NUMBER. IGNORED\n");
    free(met.m);
    free(met.namein);
    free(met.nameout);
    met.m=NULL;
    met.namein=NULL;
    met.nameout=NULL;
    memset(&met,0,sizeof(Sol));
    met.np=0;
  }
  chrono(OFF,&info.ctim[1]);
  printim(info.ctim[1].gdif,stim);
  fprintf(stdout,"  -- DATA READING COMPLETED.     %s\n",stim);

  /* analysis */
  chrono(ON,&info.ctim[2]);
  setfunc(&mesh,&met);
  if ( abs(info.imprim) > 0 )  outqua(&mesh,&met);
  fprintf(stdout,"\n  %s\n   MODULE MMG3D: IMB-LJLL : %s (%s)\n  %s\n",MG_STR,MG_VER,MG_REL,MG_STR);
  if ( info.imprim )   fprintf(stdout,"\n  -- PHASE 1 : ANALYSIS\n");
  if ( !scaleMesh(&mesh,&met) ) RETURN_AND_FREE(&mesh,&met,1);
  if ( !met.np ){
    if ( info.iso ){
      fprintf(stdout,"\n  ## ERROR : A VALID SOLUTION FILE IS NEEDED \n");
      RETURN_AND_FREE(&mesh,&met,1);
    }else{
      if ( !DoSol(&mesh,&met,&info) ) RETURN_AND_FREE(&mesh,&met,1);
      setfunc(&mesh,&met);
    }
  }

  if( info.iso && !mmg3d2(&mesh,&met) ) RETURN_AND_FREE(&mesh,&met,1);
  if ( !analys(&mesh) ) {RETURN_AND_FREE(&mesh,&met,1);}

  if ( info.imprim>4 ) {
    prilen(&mesh, &met);
  }

  chrono(OFF,&info.ctim[2]);
  printim(info.ctim[2].gdif,stim);
  if ( info.imprim )
    fprintf(stdout,"  -- PHASE 1 COMPLETED.     %s\n",stim);
  /* solve */
  chrono(ON,&info.ctim[3]);
  if ( info.imprim )
    fprintf(stdout,"\n  -- PHASE 2 : %s MESHING\n",met.size < 6 ? "ISOTROPIC" : "ANISOTROPIC");
  if ( !mmg3d1(&mesh,&met) ) RETURN_AND_FREE(&mesh,&met,1);

  chrono(OFF,&info.ctim[3]);
  printim(info.ctim[3].gdif,stim);
  if ( info.imprim )
    fprintf(stdout,"  -- PHASE 2 COMPLETED.     %s\n",stim);
  fprintf(stdout,"\n  %s\n   END OF MODULE MMG3d: IMB-LJLL \n  %s\n",MG_STR,MG_STR);

  /* save file */
  outqua(&mesh,&met);
  if(info.imprim>4)
    prilen(&mesh,&met);

  chrono(ON,&info.ctim[1]);
  if ( info.imprim )  fprintf(stdout,"\n  -- WRITING DATA FILE %s\n",mesh.nameout);
  if ( !unscaleMesh(&mesh,&met) ) {RETURN_AND_FREE(&mesh,&met,1);}
  if ( !saveMesh(&mesh) ) {RETURN_AND_FREE(&mesh,&met,1);}
  if ( !saveMet(&mesh,&met) ) {RETURN_AND_FREE(&mesh,&met,1);}
  chrono(OFF,&info.ctim[1]);
  if ( info.imprim )  fprintf(stdout,"  -- WRITING COMPLETED\n");

  /* free mem */
  RETURN_AND_FREE(&mesh,&met,0);
}
