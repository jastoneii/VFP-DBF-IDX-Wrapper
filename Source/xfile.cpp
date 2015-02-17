//
//   xFile.c
//   Implementación del i/o a archivos.
//
#include "stdafx.h"

#define FS_DB
#include "dbhdr.h"

//Maintain a list of open files, not to 'reopen' the
//themselves and for external routines may need to use CLOSEd
//files (files that are 'RE-OPENd' the next time they are needed).
class xFile *fList = 0;
char  tFileName[304];

/////////////////////////////////////////////////////////////////////////
//Transactional system initialization MUST be done or yes,
//so that they can recover from outages aborted transactions, etc.
//Also, the outputs should be able to be intercepted like crazy system to
//undo transactions that are pending. To accomplish this, I create an object
//whose construction / destruction by the program, when loaded, does all those things.
class TT_txn
{
public:
        TT_txn::TT_txn();                       // initializer.
        TT_txn::~TT_txn();                      // Deconstructor.
};

//   Initialization routine (constructor).
TT_txn::TT_txn()
{
}

// Turn off routine.
TT_txn::~TT_txn()
{
}

//  Start a transaction level.
int xTTStart()
{
        return 1;
}

//  Closes a transaction level.
int xTTCommit()
{
        return 1;
}

//  Undo a transaction level.
int xTTRollback()
{
        return 1;
}

// Function is called while waiting for the transactional system.
void xTTBusy()
{
}

/////////////////////////////////////////////////////////////////////////
//   Class management and cache files. Constructor.
xFile::xFile()
{
        memset(this, 0, sizeof(xFile));
        this->next = fList;
        fList = this;
}

//   File Destructor.
xFile::~xFile()
{
        class xFile *f;

        //   Close the file, if it was open.
        if(flags.opened) xClose();

        //   Remove it from the list of structures created.
        if(fList == this)
                fList = next;
        else
        {
                try
                {
                        for(f = fList; f != 0; f = f->next)
                        {
                                if(f->next == this)
                                {
                                        f->next = next;
                                        break;
                                }
                        }
                }
                catch(System::NullReferenceException ^)
                {
                }
        }
}


//   Returns the file flag either opened / closed.
//   It is not fixed on the flag to 'reopen', because that is for internal handling.
int xFile::xOpened()
{
        if(flags.opened) return 1;
        return 0;
}

//   Activate the cursor flag for a file.
void xFile::xCursor()
{
        flags.iscursor = 1;
        xCache(CACHE_LOAD_FILE);
}

//   Open a file.
int  xFile::xOpen(const char *fn, int flags1, int flags2)
{
        char *tChar;

        if(flags.opened) xClose();
        h = sopen(fn, flags1, flags2);

        flags.loaded = flags.writePend = flags.opened = flags.iscursor = flags.reopen = 0;
        if(h >= 0)
        {
                OpenFlags1 = flags1;
                OpenFlags2 = flags2;
                tChar = _fullpath(tFileName, fn, 300);
                lfn = strdup(tChar? tChar: fn);
                flags.opened = 1;
        }
        return h;
}

//   Reopen a file that was temporarily closed.
void xFile::xReopen()
{
        if(!flags.opened || !flags.reopen) return;
        h = sopen(lfn, OpenFlags1, OpenFlags2);
        //   OOOPS!!!
        if(h < 0) exit(1);
        flags.reopen = flags.loaded = 0;
        lseek(h, chPos, SEEK_SET);
}

//   Temporarily close a file.
void xFile::xTmpClose()
{
        if(!flags.opened || flags.reopen) return;
        if(flags.iscursor && flags.loaded) return;
        if(flags.loaded && flags.writePend)
        {
                lseek(h, 0l, SEEK_SET);
                write(h, chFile, szMem);
                flags.writePend = 0;
        }
        close(h);
        flags.reopen = 1;
}

//   Empty cache buffers.
void xFile::xFlush()
{
        if(!flags.opened) return;
        if(flags.reopen) xReopen();
        if(flags.iscursor && flags.loaded) return;
        if(flags.loaded && flags.writePend)
        {
                lseek(h, 0l, SEEK_SET);
                write(h, chFile, szMem);
                flags.writePend = 0;
        }

        //   There should be something more practical than 'close' and 're-open' the file?
        close(h);
        h = sopen(lfn, OpenFlags1, OpenFlags2);
}

//   Create a new file.
int  xFile::xCreate(const char *fn, int flags1)
{
        char *tChar;

        if(flags.opened) xClose();
        flags.iscursor = flags.opened = flags.loaded = flags.writePend = 0;
        h = creat(fn, flags1);
        if(h >= 0)
        {
                OpenFlags2 = _SH_DENYRW;
                OpenFlags1 = O_RDWR | O_BINARY;
                tChar = _fullpath(tFileName, fn, 300);
                lfn = strdup(tChar? tChar: fn);
                flags.opened = 1;
        }
        return h;
}

//   Specifies the schema cache to use.
int  xFile::xCache(int cMode)
{
        switch(cMode)
        {
                case CACHE_LOAD_FILE:
                        if(!flags.loaded)
                        {
                                if(flags.reopen) xReopen();
                                szMem = xFilelen();
                chFile = (char *) malloc(szMem + 10);
                                if(!chFile) return 0;
                                chPos = tell(h);
                                if(szMem)
                                {
                                        lseek(h, 0l, SEEK_SET);
                                        read(h, chFile, szMem);
                                }
                                flags.loaded = 1;
                                flags.writePend = 0;
                                return 1;
                        }
                        break;
        }
        return 0;
}

//   Closes a file.
int  xFile::xClose()
{
        int x;

        // Si estaba cacheado y hay escrituras pendientes, las graba.
        // If it was cached and there are pending writes
        if(flags.loaded && flags.writePend)
        {
                if(!flags.iscursor)
                {
                        if(flags.reopen) xReopen();
                        lseek(h, 0l, SEEK_SET);
                        write(h, chFile, szMem);
                }
                flags.writePend = 0;
        }

        //Closes the handler.
        x = 0;
        if(flags.opened)
        {
                flags.opened = 0;
                if(!flags.reopen) x = close(h);
                if(flags.iscursor) remove(lfn);
                free(lfn);
				lfn = 0; //jas
                flags.reopen = 0;
        }

        //   Frees the memory cache, if applicable.
    if(flags.loaded) free(chFile);
        flags.loaded = 0;
        return x;
}

//   Read from a file.
int  xFile::xRead(void *buff, int cant)
{
        int cRead;

        if(!flags.opened) return 0;
        if(flags.loaded)
        {
                cRead = szMem - chPos;
                if(cRead >= cant) cRead = cant; else cant = cRead;
                if(cRead <= 0) return 0;
                memmove(buff, chFile + chPos, cRead);
                chPos += cRead;
                return cRead;
        }

        if(flags.reopen) xReopen();
        cRead = read(h, buff, cant);
        chPos = tell(h);
        return cRead;
}

//  Write to a file.
int  xFile::xWrite(void *buff, int cant)
{
        int xRet;

        if(!flags.opened) return 0;
        if(flags.loaded)
        {
                if(chPos + cant > szMem)        //  Change the memory size.
                        xChsize(chPos + cant);
                if(flags.loaded)                        //  If continuous memory.
                {
                        memmove(chFile + chPos, buff, cant);
                        flags.writePend = 1;
                        chPos += cant;
                        return cant;
                }
        }

        if(flags.reopen) xReopen();
        xRet = write(h, buff, cant);
        chPos = tell(h);
        return xRet;
}

//   Locate the file pointer.
long xFile::xLseek(long pos, int flag)
{
        if(!flags.opened) return 0l;
        if(flags.loaded)
        {
                switch(flag)
                {
                        case SEEK_CUR:
                                chPos += pos;
                                break;
                        case SEEK_END:
                                chPos = szMem + pos;
                                break;
                        default:
                                chPos = pos;
                                break;
                }
                if(chPos < 0l) chPos = 0l;
                return chPos;
        }

        if(flags.reopen) xReopen();
        return (chPos = lseek(h, pos, flag));
}

//   Lock / Unlocks a sequence of bytes in the file.
int  xFile::xLocking(int lockflg, long cBytes)
{
        if(!flags.opened) return 0;
        if(flags.reopen) xReopen();
        if(flags.loaded)
                lseek(h, chPos, SEEK_SET);
    return locking(h, (lockflg? LK_NBLCK: LK_UNLCK), cBytes);
}

//   Returns the current position of a file pointer.
long xFile::xTell()
{
        if(!flags.opened) return 0l;
        return chPos;
}

//   Changes the size of a file.
int xFile::xChsize(long nSize)
{
        char *nPos;

        //  If it is a regular file.
        if(flags.reopen) xReopen();
        if(!flags.loaded)
        {
                chsize(h, nSize);
                return nSize;
        }

        //  Write the file modifications from memory.
        if(flags.writePend)
        {
                if(!flags.iscursor)
                {
                        lseek(h, 0l, SEEK_SET);
                        write(h, chFile, szMem);
                }
                flags.writePend = 0;
        }
        if(nSize != szMem)
        {
        nPos = (char *) realloc(chFile, nSize);
                if(!nPos)
                {
                        //   There is no more memory, so convert the file into a regular one.
                        if(flags.iscursor)
                        {
                                lseek(h, 0l, SEEK_SET);
                                write(h, chFile, szMem);
                        }
            free(chFile);
                        flags.loaded = 0;
                        return nSize;
                }
                chFile = nPos;
                szMem = nSize;
        }
        return nSize;
}

//   Returns the length (number of bytes) of a file.
long xFile::xFilelen()
{
        if(flags.loaded) return szMem;
        if(flags.reopen) xReopen();
        return filelength(h);
}

//   Returns TRUE if at the end of file.
int xFile::xEof()
{
        if(flags.loaded) return (szMem > chPos)? 0: 1;
    if(xFilelen() < xTell()) return 1;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////   GLOBAL ROUTINE /////////////////////////////////
void TemporaryClose(char *fn)
{
        class xFile *f;
        char *tChar;

        tChar = _fullpath(tFileName, fn, 300);
        if(!tChar) tChar = fn;
        for(f = fList; f; f = f->next)
        {
                if(!stricmp(f->lfn, tChar))
                {
                        f->xTmpClose();
                        break;
                }
        }
}
