// db.h

#include "mshstr.h"
#include "dbhdr.h"

#pragma once
using namespace System;

//struct idxops            //to process an index comprised of more than 1 field, etc
//{
//	char(*fldname[10])[10];       //allow for up to ten 10 character field names
//	int  fldlen[10];
//	int  fldoffset[10];
//	int  operation1[10];
//	int  operation2[10];
//};

//#define OP_UPPER     1
//#define OP_DTOS      2
//#define OP_DTOYYYYMM 3

//  Class 'wrapper' to access dbf's from C# or VB.
namespace db
{
        //   Wrapper for 'dbf' class.
        public ref class xdbf
        {
        private:
                dbf *mydb;                                              // underlying Class.
                kernel::MarshalString ^ms;                              // For type conversion.

        public:
                xdbf(void);                                                     // Constructer.
                ~xdbf(void);                                                    // Destructer

                //Class Properties.
                //property dbf *get_Dbf();                              // Return 'mydbf'
                                property dbf * Dbf
                                {
                                        dbf * get() { return mydb; }
                                }
                //property unsigned long get_RecCount();// Number of records.
                                property unsigned long RecCount
                                {
                                        unsigned long get() { return mydb->RecCount(); }
                                }
                //property unsigned long get_Recno();   //Current positioned record number.
                                property unsigned long Recno
                                {
                                        unsigned long get() { return mydb->Recno(); }
                                }
                //property bool get_Deleted();                  // Returns the 'Deleted' flag for record
                                property bool Deleted
                                {
                                        bool get()
                                        {
                                                if (mydb->Deleted() > 0) return true;
                                                return false;
                                        }
                                }
                //property bool get_Eof();                              // indicate end of file status.
                                property bool Eof
                                {
                                        bool get()
                                        {
                                                if (mydb->Eof() > 0) return true;
                                                return false;
                                        }
                                }
                //property bool get_Bof();                              // Indicate beginning of file status.
                                property bool Bof
                                {
                                        bool get()
                                        {
                                                if (mydb->Bof() > 0) return true;
                                                return false;
                                        }
                                }
                                //property bool get_Opened();                   // Indicate opened status
                                property bool Opened
                                {
                                        bool get()
                                        {
                                                if (mydb->Opened() > 0) return true;
                                                return false;
                                        }
                                }
                //   Delegates to use at events.
                delegate void _dDBUsedEvent(String ^fn, bool success);
                delegate void _dDBClosedEvent(void);
                delegate void _dDBGoedEvent(unsigned long nRec, bool success);
                delegate void _dDBLockedEvent(bool makeLock, bool success);
                delegate void _dDBGettedEvent(String ^fld, String ^value, bool success);
                delegate void _dDBGettedDEvent(String ^fld, double dblvar, bool success);
                delegate void _dDBGettedIEvent(String ^fld, long intvar, bool success);
                delegate void _dDBReplacedEvent(String ^fld, String ^value, bool success);
                delegate void _dDBReplacedDEvent(String ^fld, double dblvar, bool success);
                delegate void _dDBReplacedIEvent(String ^fld, int intvar, bool success);
                delegate void _dDBAppenedBlankEvent(bool success);
                delegate void _dDBCopiedStructEvent(void);
                delegate void _dDBZappedEvent(void);
                delegate void _dDBRecordSeekedEvent(String ^value, bool success);
                delegate void _dDBRecordSeekedDEvent(double value, bool success);
                delegate void _dDBRecordSkippedEvent(long offset, bool success);
                delegate void _dDBOrderSettedEvent(String ^key, bool success);
                delegate void _dDBIndexOpenedEvent(String ^filename, bool success);
                delegate void _dDBRecordLocatedEvent(String ^field, String ^value, bool success);
                delegate void _dDBTableCreatedEvent(String ^filename, String ^fldlist, bool success);
                delegate void _dDBCursorCreatedEvent(String ^filename, String ^fldlist, bool success);
                delegate void _dDBTableAppendedEvent(char *filename);
                delegate void _dDBTableAppendedCSVEvent(char *filename);
                delegate void _dDBTableCopied1ToEvent(char *filename);
                delegate void _dDBPackedEvent(void);
                delegate void _dDBRecordDeletedEvent(void);

                //  Available events.
                event _dDBUsedEvent^ DBUsedEvent;
                event _dDBClosedEvent^ DBClosedEvent;
                event _dDBGoedEvent^ DBGoedEvent;
                event _dDBLockedEvent^ DBLockedEvent;
                event _dDBGettedEvent^ DBGettedFieldValueEvent;
                event _dDBGettedDEvent^ DBGettedDFieldValueEvent;
                event _dDBGettedIEvent^ DBGettedIFieldValueEvent;
                event _dDBReplacedEvent^ DBReplacedEvent;
                event _dDBReplacedDEvent^ DBReplacedDEvent;
                event _dDBReplacedIEvent^ DBReplacedIEvent;
                event _dDBAppenedBlankEvent^ DBAppenedBlankEvent;
                event _dDBCopiedStructEvent^ DBCopiedStructEvent;
                event _dDBZappedEvent^ DBZappedEvent;
                event _dDBRecordSeekedEvent^ DBRecordSeekedEvent;
                event _dDBRecordSeekedDEvent^ DBRecordSeekedDEvent;
                event _dDBRecordSkippedEvent^ DBRecordSkippedEvent;
                event _dDBOrderSettedEvent^ DBOrderSettedEvent;
                event _dDBIndexOpenedEvent^ DBIndexOpenedEvent;
                event _dDBRecordLocatedEvent^ DBRecordLocatedEvent;
                event _dDBTableCreatedEvent^ DBTableCreatedEvent;
                event _dDBCursorCreatedEvent^ DBCursorCreatedEvent;
                event _dDBTableAppendedEvent^ DBTableAppendedEvent;
                event _dDBTableAppendedCSVEvent^ DBTableAppendedCSVEvent;
                event _dDBTableCopied1ToEvent^ DBTableCopied1ToEvent;
                event _dDBPackedEvent^ DBPackedEvent;
                event _dDBRecordDeletedEvent^ DBRecordDeletedEvent;

                //   Methods available.
                bool Use(String ^fn);                           // Open the file.
                void Close(void);                               // Close the file.
                bool Go(unsigned long rec);                     // Go to given record.
                bool Lock(bool makeLock);                       // Unlock / Lock the current record.
                String ^Get(String ^fld);                       // Extracts data from a field.
                double GetD(String ^fld);                       // Extracts data from a double field.
                long GetI(String ^fld);                         // Extracts data from a dbf integer field -- use type long for compiler safety
                int  FieldLen(String ^fld);                     // Returns the length in bytes of 1 fld.
                int  FieldDec(String ^fld);                     // Returns the value of the fractional part.
                char FieldType(String ^fld);                    // Returns the type of a field.
                bool Replace(String ^fld, String ^exp);         // Replaces data in a field.
                bool ReplaceD(String ^fld, double dblvar);      // Replaces data in a field.
                bool ReplaceI(String ^fld, long intvar);        // Replaces data in a field.
                bool AppendBlank(void);                         // Add a blank record.
                void CopyStruct(char *newfile);                 // Copy DBF structure to another file
                void Zap(void);                                 // Delete ALL records.
                void SetRelation(xdbf ^target, String ^exp);    // Establishes a relationship.
                void ClearRelation(xdbf ^target);               // Removes a relationship.

                bool Skip(long offset);                         // Skip records.
                bool Seek(String ^key);                         // Search index for a string
				bool SeekD(Double key);                         // Search index for a double var
                void Setorder(String ^key);                     // Change active index.
                bool SetIndex(String ^fn);                      // Opens and associates an index.
                bool Locate(String ^fld, String ^exo);          // sequential Search

				bool IndexOn(String ^key, String ^fn);          // Create an index and make it active.
                void Reindex(void);                             // Rebuild all indexes.
                bool createTable(String ^fn, String ^fld);        // Create a database.
				//bool createTableFrom(String ^newdb, String ^extfile); // Create dbf from structure extended file
                bool creaCursor(String ^fn, String ^fld);       // Creates a cursor based.
                void AppendFrom(char *filename);                // Append from a file to current file.
                int  AppendFromCSV(char *filename, bool headerrow, bool commas_in_text); // Append from a CSV file to current file.
                void Copy1To(char *filename);                   // Copy 1 record to another table -- allows for VFP COPY TO equivalent
                void Pack();                                    // Pack the dbf
                void Flush(void);                               // Download data to disk.
                void Delete(void);                              // Marks the current record as deleted
                void Trunc(unsigned long rec);                  // Truncates the database from the record specified.
                int  CheckIdx(const char *field);               // Index integrity test function
        };

        //   Wrapper de la clase 'xFile'.
        public ref class xfile
        {
        private:
                xFile *myfile;                                   // XFILE underlying Class
                int m_cache;                                     // Cache type you use.
                bool m_cursor;                                   // Whether or not a cursor.
                kernel::MarshalString ^ms;                       // To convert types.

        public:
                //property long get_Position();                  // Current file position (tell).
                                property long Position
                                {
                                        long get()
                                        {
                                                return myfile->xTell();
                                        }
                                }
                //property long get_Length();                           // Length in bytes of the file.
                                //property void set_Length(long value);
                                property long Length
                                {
                                        long get()
                                        {
                                                return myfile->xFilelen();
                                        }

                                        void set(long newvalue)
                                        {
                                                myfile->xChsize(newvalue);
                                        }
                                }
                //property bool get_Eof();                               // indicate end of file status.
                                property bool Eof
                                {
                                        bool get()
                                        {
                                                if (myfile->xEof() > 0) return true;
                                                return false;
                                        }
                                }
                //property int  get_CacheEsq();                         // Cache scheme used.
                //property void set_CacheEsq(int newesq);
                                property int  CacheEsq
                                {
                                        int get()
                                        {
                                                return m_cache;
                                        }

                                        void set(int newesq)
                                        {
                                                m_cache = myfile->xCache(newesq);
                                        }
                                }
                //property bool get_IsCursor();                         // Flag indicating whether or not the cursor.
                //property void set_IsCursor(bool curs);
                                property bool IsCursor
                                {
                                        bool get() { return m_cursor; }
                                        void set(bool cursflag)
                                        {
                                                myfile->xCursor();
                                                m_cursor = true;
                                        }
                                }
                //property bool get_IsOpened();                         // Is file opened?
                                property bool IsOpened
                                {
                                        bool get()
                                        {
                                                if (myfile->xOpened() > 0) return true;
                                                return false;
                                        }
                                }

                xfile();                                                 //  Constructor y Destructor.
                ~xfile();

                //   Enumerations.
                enum class SeekPos
                {
                        FromStartOfFile,
                        FromEndOfFile,
                        FromCurrentPosition
                };

                //   Delegates to use at events.
                delegate void _dOpenedEvent(String ^fn, bool success);
                delegate void _dCreatedEvent(String ^fn);
                delegate void _dClosedEvent(void);
                delegate void _dReadedEvent(String ^readed, int requested);
                delegate void _dWritedEvent(String ^writed, int requested, int efectiveWrited);
                delegate void _dSeekedEvent(long start, SeekPos flag, long AbsolutePos);
                delegate void _dLockedEvent(int len, bool makeLock, bool Success);
                delegate void _dFlushedEvent(void);
                delegate void _dReopenedEvent(void);
                delegate void _dTemporaryClosedEvent(void);

                //   Events that can be used
                event _dOpenedEvent^ OpenedEvent;
                event _dCreatedEvent^ CreatedEvent;
                event _dClosedEvent^ ClosedEvent;
                event _dReadedEvent^ ReadedEvent;
                event _dWritedEvent^ WritedEvent;
                event _dSeekedEvent^ SeekedEvent;
                event _dLockedEvent^ LockedEvent;
                event _dFlushedEvent^ FlushedEvent;
                event _dReopenedEvent^ ReopenedEvent;
                event _dTemporaryClosedEvent^ TemporaryClosedEvent;

                //   Methods available.
                bool Open(String ^fn, bool openAsText, bool openExclusive);     // sopen(...)
                bool Create(String ^fn);                                        // creat(...)
                bool Close(void);                                               // close(...)
                String ^Read(int nLen);                                         // read(...)
                int  Write(String ^buff, int sz);                               // write(...)
                long Lseek(long pos, SeekPos fg);                               // lseek(...)
                bool Locking(long len, bool makeLock);                          // locking(...)
                void Flush(void);                                               // flush buffers.
                void Reopen(void);                                              // Reopen a file.
                void TemporaryClose(void);                                      // Temporary Close
        };


}

