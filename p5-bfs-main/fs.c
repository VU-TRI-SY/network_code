// ============================================================================
// fs.c - user FileSytem API
// ============================================================================

#include "bfs.h"
#include "fs.h"

// ============================================================================
// Close the file currently open on file descriptor 'fd'.
// ============================================================================
i32 fsClose(i32 fd) { 
  i32 inum = bfsFdToInum(fd);
  bfsDerefOFT(inum);
  return 0; 
}



// ============================================================================
// Create the file called 'fname'.  Overwrite, if it already exsists.
// On success, return its file descriptor.  On failure, EFNF
// ============================================================================
i32 fsCreate(str fname) {
  i32 inum = bfsCreateFile(fname);
  if (inum == EFNF) return EFNF;
  return bfsInumToFd(inum);
}



// ============================================================================
// Format the BFS disk by initializing the SuperBlock, Inodes, Directory and 
// Freelist.  On succes, return 0.  On failure, abort
// ============================================================================
i32 fsFormat() {
  FILE* fp = fopen(BFSDISK, "w+b");
  if (fp == NULL) FATAL(EDISKCREATE);

  i32 ret = bfsInitSuper(fp);               // initialize Super block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitInodes(fp);                  // initialize Inodes block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitDir(fp);                     // initialize Dir block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitFreeList();                  // initialize Freelist
  if (ret != 0) { fclose(fp); FATAL(ret); }

  fclose(fp);
  return 0;
}


// ============================================================================
// Mount the BFS disk.  It must already exist
// ============================================================================
i32 fsMount() {
  FILE* fp = fopen(BFSDISK, "rb");
  if (fp == NULL) FATAL(ENODISK);           // BFSDISK not found
  fclose(fp);
  return 0;
}



// ============================================================================
// Open the existing file called 'fname'.  On success, return its file 
// descriptor.  On failure, return EFNF
// ============================================================================
i32 fsOpen(str fname) {
  i32 inum = bfsLookupFile(fname);        // lookup 'fname' in Directory
  if (inum == EFNF) return EFNF;
  return bfsInumToFd(inum);
}



// ============================================================================
// Read 'numb' bytes of data from the cursor in the file currently fsOpen'd on
// File Descriptor 'fd' into 'buf'.  On success, return actual number of bytes
// read (may be less than 'numb' if we hit EOF).  On failure, abort
// ============================================================================
i32 fsRead(i32 fd, i32 numb, void* buf) {

  // ++++++++++++++++++++++++
  // Insert your code here
  // ++++++++++++++++++++++++
  
  // convert fd to inum
  i32 inum = bfsFdToInum(fd);
  i32 bytesRead = numb;
  //take current woking position on file 'fd'
  i32 currentCurs = bfsTell(fd);
  //take the stating fbn to read
  i32 fbnStart = currentCurs / BYTESPERBLOCK;
  i32 fbnEnd = (currentCurs + numb) / BYTESPERBLOCK;
  i32 internalOffset = currentCurs % BYTESPERBLOCK;
  //take the file size and compute the max fbn and check the validation of fbnStart and fbnEnd
  i32 fileSize = bfsGetSize(inum); 

  // if(currentCurs >= fileSize) {
  //   FATAL(ENYI);
  //   return 0;
  // }

  i32 numberOfBlocks = (fileSize / BYTESPERBLOCK);
  if(fileSize % BYTESPERBLOCK != 0) numberOfBlocks++;

  // if filesize mode BYTESPERBLOCK = 0 --> return 0, else return 1
  // condition ? value_for_true : value_for_false;
  // compute the number of read blocks
  // fbnEnd = min(fbnEnd, maxFbn);
  if(fbnEnd >= numberOfBlocks){
    fbnEnd = numberOfBlocks-1;
    bytesRead = fileSize - currentCurs;
  }
  
  i32 totalBlocks = fbnEnd - fbnStart + 1;
  //create an array of read bytes
  i8 readBuf[totalBlocks * BYTESPERBLOCK];
  i32 readBuffOffset = 0;
  i8 tempBuf[BYTESPERBLOCK];
  //read from fbnStart to fbnEnd into readBuf
  for(i32 curFbn = fbnStart; curFbn <= fbnEnd; curFbn++){
    bfsRead(inum, curFbn, tempBuf); //read a block with fbn 'curFbn'
    //pour from tempBuff to readBuff starting readBuffOffset
    memcpy(readBuf + readBuffOffset, tempBuf, BYTESPERBLOCK);
    readBuffOffset += BYTESPERBLOCK;
  }

  //copy bytesRead bytes from readBuff to given buf
  memcpy(buf, readBuf + internalOffset, bytesRead);
  fsSeek(fd, bytesRead, SEEK_CUR); //move a distance from current position
  return bytesRead;

  //   i32 inum = bfsFdToInum(fd);             // inum from fd
  // i32 currentCursor = bfsTell(fd);              // current cursor position
	// i32 startFbn = currentCursor/ BYTESPERBLOCK;  // file's starting fbn
	// i32 endFbn = (currentCursor + numb) / BYTESPERBLOCK;   // file's last fbn
  // i32 bytesRead = numb;                   // bytes that should be read
  // i32 fileSize = bfsGetSize(inum);
  // // if 'numb' hits EOF, calculate end position
  // if (currentCursor + numb > fileSize){
	//   bytesRead = fileSize - currentCursor;
	// 	endFbn = (currentCursor + bytesRead) / BYTESPERBLOCK;
	// }

	// // allocate read buffer, totalBlocks * 512 bytes long
	// i8 totalBlocks = endFbn - startFbn + 1;
	// i8 readBuf[(totalBlocks)* BYTESPERBLOCK];
  // i8 tempBuf[BYTESPERBLOCK];
  // i32 offset = currentCursor % BYTESPERBLOCK;
  // i32 bufferOffset = 0;
  // // read the DBN that holds the FBN into readBuf
  // for (i32 i = startFbn; i <= endFbn; i++){
  //   bfsRead(inum, i, tempBuf);
  //   memcpy(readBuf + bufferOffset, tempBuf, BYTESPERBLOCK);
	// 	bufferOffset += BYTESPERBLOCK;
	// }
	// // move to buf and return bytes read
	// memcpy(buf, readBuf + offset, bytesRead);
	// fsSeek(fd, bytesRead, SEEK_CUR);
	// return bytesRead;   
}


// ============================================================================
// Move the cursor for the file currently open on File Descriptor 'fd' to the
// byte-offset 'offset'.  'whence' can be any of:
//
//  SEEK_SET : set cursor to 'offset'
//  SEEK_CUR : add 'offset' to the current cursor
//  SEEK_END : add 'offset' to the size of the file
//
// On success, return 0.  On failure, abort
// ============================================================================
i32 fsSeek(i32 fd, i32 offset, i32 whence) {

  if (offset < 0) FATAL(EBADCURS);
 
  i32 inum = bfsFdToInum(fd);
  i32 ofte = bfsFindOFTE(inum);
  
  switch(whence) {
    case SEEK_SET:
      g_oft[ofte].curs = offset;
      break;
    case SEEK_CUR:
      g_oft[ofte].curs += offset;
      break;
    case SEEK_END: {
        i32 end = fsSize(fd);
        g_oft[ofte].curs = end + offset;
        break;
      }
    default:
        FATAL(EBADWHENCE);
  }
  return 0;
}



// ============================================================================
// Return the cursor position for the file open on File Descriptor 'fd'
// ============================================================================
i32 fsTell(i32 fd) {
  return bfsTell(fd);
}



// ============================================================================
// Retrieve the current file size in bytes.  This depends on the highest offset
// written to the file, or the highest offset set with the fsSeek function.  On
// success, return the file size.  On failure, abort
// ============================================================================
i32 fsSize(i32 fd) {
  i32 inum = bfsFdToInum(fd);
  return bfsGetSize(inum);
}



// ============================================================================
// Write 'numb' bytes of data from 'buf' into the file currently fsOpen'd on
// filedescriptor 'fd'.  The write starts at the current file offset for the
// destination file.  On success, return 0.  On failure, abort
// ============================================================================
i32 fsWrite(i32 fd, i32 numb, void* buf) {

  // ++++++++++++++++++++++++
  // Insert your code here
  // ++++++++++++++++++++++++

  FATAL(ENYI);                                  // Not Yet Implemented!
  return 0;
}
