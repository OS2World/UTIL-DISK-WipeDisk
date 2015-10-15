/*----------------------------------------------------------------------------
 -----  (C) Copyright    Baxter Diagnostics 1991                        ------
 -----------------------------------------------------------------------------
 -----  Project       : Status IIntellect (tm)
 -----
 -----  Function      : Query Directory file size
 -----
 -----  AUTHOR        : Nick Bethman - Wed  02-26-1992
 -----
 ---------------------------------------------------------------------------*/
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_DOSDEVICES
#include <os2.h>
#ifndef BSEDEV_INCLUDED
#include <bsedev.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXDEPTH 15
#define NOTREMOVE 0xfffe
#define FATTRFIND (FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY)
#define FATTRRESET (FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM)

     /* this will work with 2.0 or 1.2+ */
#ifdef INCL_32
  #define DosDevIO( hDev, cat, fun, pParm, cbParmSize, pcbParmLen, \
                    pData, cbDataSize, pcvDataLen ) \
     DosDevIOCtl( hDev, cat, fun, pParm, cbParmSize, pcbParmLen, \
                  pData, cbDataSize, pcvDataLen )
#else
  #define DosDevIO( hDev, cat, fun, pParm, cbParmSize, pcbParmLen, \
                    pData, cbDataSize, pcvDataLen ) \
     DosDevIOCtl2( pData, cbDataSize, pParm, cbParmSize, fun, cat, hDev )
#endif


/*----------------------------------------------------------------------------
* Is a drive removeable.
*---------------------------------------------------------------------------*/
unsigned NEAR isRemoveableDrive( char drive,
                                 BOOL *isRemoveable )
{
char     dname[3];
HFILE    handle;
unsigned result;
int      data = 0;
int      parms = 0;
unsigned dataIO = sizeof(data);
unsigned parmIO = sizeof(parms);
unsigned action;

  dname[0] = drive;
  dname[1] = ':';
  dname[2] = 0;


  result = DosOpen( dname, &handle, &action, 0,
                    FILE_NORMAL, FILE_OPEN,
                    OPEN_ACCESS_READWRITE | OPEN_FLAGS_DASD | OPEN_SHARE_DENYWRITE, 0 );
  if ( result == 0 ) {
    result = DosDevIO( handle, IOCTL_DISK, DSK_BLOCKREMOVABLE, 
                       &parms, sizeof(parms), &parmIO,
                       &data, sizeof(data), &dataIO );
    *isRemoveable = (data == 0);
    DosClose( handle );
  }

  return( result );

} /* isRemoveableDrive */


unsigned NEAR wipeDir( char    *path,
                       unsigned depth )
{
HDIR         hdir = HDIR_CREATE;
FILEFINDBUF  buffer;
unsigned     count = 1;
unsigned     err;
unsigned     pathLen;
char         fullName[CCHMAXPATH];
static char  EADATA[] = "EA DATA. SF";

  if ( depth == MAXDEPTH )
    return( 1 );

  puts( path );

  pathLen = strlen( path ) - 1;  /* ingnore the '*' */
  strcpy( fullName, path );
  err = DosFindFirst( path, &hdir, FATTRFIND,
                      &buffer, sizeof(buffer), &count, 0L );
  if ( err != 0 )
    printf( "file err %u\n", err );
  else {
    do {
      strcpy( &fullName[pathLen], buffer.achName );
      if ( (buffer.attrFile & FATTRRESET) &&
           strcmp( buffer.achName, EADATA ) != 0 )
        DosSetFileMode( fullName, FILE_NORMAL, 0 );
      if ( buffer.attrFile & FILE_DIRECTORY ) {
        if ( strcmp( buffer.achName, "." ) != 0 &&
             strcmp( buffer.achName, ".." ) != 0 ) {
          strcat( fullName, "\\*" );
          wipeDir( fullName, depth+1 );
             /* rip \* off and remove it */
          strcpy( &fullName[pathLen], buffer.achName );
          err = DosRmDir( fullName, 0 );
          if ( err != 0 )
            printf( "%s remove err %u\n", path, err );
        }
      }else if ( strcmp( buffer.achName, EADATA ) != 0 ) {
        printf( fullName );
        err = DosDelete( fullName, 0 );
        if ( err != 0 )
          printf( "err %u", err );
        printf( "\n" );
      }
    }while ( DosFindNext( hdir, &buffer, sizeof(buffer), &count ) == 0 );
    DosFindClose( hdir );
  }

  return( err );

}  /* wipeDir */


unsigned NEAR WipeDisk( unsigned drive )
{
static char  path[] = "x:\\*";
unsigned err;
BOOL     removeable;

     /* don't let them wap any hard drives */
  err = isRemoveableDrive( (char)drive, &removeable );
  if ( err != 0 )
    return( err );
  else if ( !removeable )
    return( NOTREMOVE );

  path[0] = (char)drive;
  wipeDir( path, 1 );
  return( 0 );

}  /* WipeDisk */


unsigned _cdecl main( int   argc,
                      char *argv[] )
{
unsigned err;

  DosError( FERR_DISABLEHARDERR );

  puts( "Wipe Disk v1.0, Copyright Ngb Technologies, 1992, All rights reserved\n" );
  if ( argc >= 2 ) {
    err = WipeDisk( (char)toupper( argv[1][0] ) );
    if ( err == NOTREMOVE )
      printf( "Drive %c is not removeable, will not wipe!\n", argv[1][0] );
    else if ( err != 0 )
      printf( "Drive %c is not accessable, err %u, will not wipe!\n",
               argv[1][0], err );
  }else {
    err = 0;
    puts( "No drive specified. try: wipedisk a" );
  }

  return( err );

}  /* main */

