//
//  db_wrapper.cpp
//  Contains classes 'wrapper' so you can use the xbase from C # or VB system.
//

#include "stdafx.h"
#include "db_wrapper.h"

//  We need the managed classes 'String', 'Marshal', etc.
using namespace System;
using namespace System::Runtime::InteropServices;

//  View the 'set' of the environment.
view_ view;

namespace db
{
////////////////////////////////////////////////////////////////////////////////////////////////
//   Implementation of the wrapper class 'dbf'.

        //   Build the underlying class.
        xdbf::xdbf()
        {
                mydb = new dbf;
                ms = gcnew kernel::MarshalString;
        }

        //   Deletes the underlying class and temporary strings used.
        xdbf::~xdbf()
        {
                delete mydb;
        }

        //   Returns a pointer to the internal file manager. (Property ReadOnly).
 //       dbf *xdbf::get_Dbf()
 //       {
 //               return mydb;
 //       }

        //       Opens a database.
        bool xdbf::Use(String ^fn)
        {
                //   Performs the operation.
                bool res = (mydb->Use(ms->GetString(fn)) > 0)? true: false;
                ms->FreeTemp();

                //   Dispara al evento y retorna.
                //  Shoot the event and returns.
                DBUsedEvent(fn, res);
                return res;
        }

        //   Close the file.
        void xdbf::Close(void)
        {
                mydb->Close();
                DBClosedEvent();
        }

        //   Position to a certain record.
        bool xdbf::Go(unsigned long rec)
        {
                bool res = (mydb->Go(rec) > 0)? true : false;
                DBGoedEvent(rec, res);
                return res;
        }

        //   Lock / unlock the current record.
        bool xdbf::Lock(bool makeLock)
        {
                bool res = (mydb->Lock(makeLock? 1: 0) > 0)? true : false;
                DBLockedEvent(makeLock, res);
                return res;
        }

        // Extract data from a particular field.
        // If there is a problem with the extraction, return "<Error in Fld>".        String *xdbf::Get(String *fld)
        String ^xdbf::Get(String ^fld)
        {
                char *p_fname = ms->GetString(fld);
                char *buff = new char[mydb->FieldLen(p_fname) + 10];
                if(mydb->Get(p_fname, buff) > 0)
                {
                        String ^res = ms->SetString(buff);
                        ms->FreeTemp();
                        DBGettedFieldValueEvent(fld, res, true);
                        return res;
                }
                ms->FreeTemp();
                DBGettedFieldValueEvent(fld, "", false);
                return "<Error in Fld>";
        }

		//bool xdbf::GetFld(String ^fld, String ^strout)
		//{
		//	char *p_fname = ms->GetString(fld);
		//	char *buff = new char[mydb->FieldLen(p_fname) + 10];
		//	if (mydb->Get(p_fname, buff) > 0)
		//	{
		//		strout = ms->SetString(buff);
		//		ms->FreeTemp();
		//		DBGettedFieldValueEvent(fld, strout, true);
		//		return true;
		//	}
		//	ms->FreeTemp();
		//	DBGettedFieldValueEvent(fld, "", false);
		//	return "false";
		//}

        // Extract data from a double or numeric field
        double xdbf::GetD(String ^fld)
        {
                char *p_fname = ms->GetString(fld);
                double dblvar;
                if(mydb->GetD(p_fname, &dblvar) > 0)
                {
                    ms->FreeTemp();
                    DBGettedDFieldValueEvent(fld, dblvar, true);
                    return dblvar;
                }
                ms->FreeTemp();
                DBGettedDFieldValueEvent(fld, 0.0, false);
                return 0.0;
        }

        // Extract data from an int field
        long xdbf::GetI(String ^fld)
        {
                char *p_fname = ms->GetString(fld);
                long intvar;
                if(mydb->GetI(p_fname, &intvar) > 0)
                {
                        ms->FreeTemp();
                        DBGettedIFieldValueEvent(fld, intvar, true);
                        return intvar;
                }
                ms->FreeTemp();
                DBGettedDFieldValueEvent(fld, 0, false);
                return (long) 0;
        }

        //   Returns the size of a field.
        int xdbf::FieldLen(String ^fld)
        {
                int res = mydb->FieldLen(ms->GetString(fld));
                ms->FreeTemp();
                return res;
        }

        //   Returns the length in bytes of the decimal part of a field.
        int xdbf::FieldDec(String ^fld)
        {
                int res = mydb->FieldDec(ms->GetString(fld));
                ms->FreeTemp();
                return res;
        }

        //   Returns the type of a field.
        char xdbf::FieldType(String ^fld)
        {
                char res = mydb->FieldType(ms->GetString(fld));
                ms->FreeTemp();
                return res;
        }

        //   Replaces the current contents of a character, date, or logical field.
        bool xdbf::Replace(String ^fld, String ^exp)
        {
                bool res = (mydb->Replace(ms->GetString(fld), ms->GetString(exp)) > 0)? true : false;
                ms->FreeTemp();
                DBReplacedEvent(fld, exp, res);
                return res;
        }

        //   Replaces the current contents of a VFP double field.
        bool xdbf::ReplaceD(String ^fld, double dblvar)
        {
                bool res = (mydb->ReplaceD(ms->GetString(fld), dblvar) > 0)? true : false;
                ms->FreeTemp();
                DBReplacedDEvent(fld, dblvar, res);
                return res;
        }

        //   Replaces the current contents of a VFP integer field which is really a double
        bool xdbf::ReplaceI(String ^fld, long intvar)
        {
                bool res = (mydb->ReplaceI(ms->GetString(fld), intvar) > 0)? true : false;
                ms->FreeTemp();
                DBReplacedIEvent(fld, intvar, res);
                return res;
        }

        //   Add a blank record at the end of the table.
        bool xdbf::AppendBlank(void)
        {
                bool res = (mydb->AppendBlank() > 0)? true : false;
                DBAppenedBlankEvent(res);
                return res;
        }

        //   Copy the table's structure to a new table.
        void xdbf::CopyStruct(char *newfile)
        {
                mydb->CopyStruct(newfile);
                DBCopiedStructEvent();
        }

        //   Delete ALL records in the table.
        void xdbf::Zap(void)
        {
                mydb->Zap();
                DBZappedEvent();
        }

        //   Establish a relationship with another table
        void xdbf::SetRelation(xdbf ^target, String ^field)
        {
                mydb->SetRelation(target->Dbf, ms->GetString(field));
                ms->FreeTemp();
        }

        //   Removes a previously established relationship.
        void xdbf::ClearRelation(xdbf ^target)
        {
                mydb->ClearRelation(target->Dbf);
        }

        //   Returns indicating if a table is positioned at the beginning of the file.
        //bool xdbf::get_Bof(void)
        //{
        //        if(mydb->Bof() > 0) return true;
        //        return false;
        //}

        //  Returns indicating if a table is positioned at the end of the file.
        //   Returns the number of records contained in the table.
        //bool xdbf::get_Eof(void)
        //{
        //        if(mydb->Eof() > 0) return true;
        //        return false;
        //}

        //   Returns the number of records contained in the table.
        /*unsigned long xdbf::get_RecCount(void)
        {
                return mydb->RecCount();
        }*/

        //   Returns the record number of the currently selected record.
        /*unsigned long xdbf::get_Recno(void)
        {
                return mydb->Recno();
        }*/

        //   Returns TRUE if the currently selected record is DELETED.
        //bool xdbf::get_Deleted(void)
        //{
        //        if(mydb->Deleted() > 0) return true;
        //        return false;
        //}

        //   Skip given number of records.
        bool xdbf::Skip(long offset)
        {
                bool res = (mydb->Skip(offset) > 0)? true : false;
                DBRecordSkippedEvent(offset, res);
                return res;
        }

        //   Position a table, given a key index.
        bool xdbf::Seek(String ^key)
        {
                bool res = (mydb->Seek(ms->GetString(key)) > 0)? true : false;
                ms->FreeTemp();
                DBRecordSeekedEvent(key, res);
                return res;
        }

        bool xdbf::SeekD(double key)
        {
                bool res = (mydb->SeekD(key) > 0)? true : false;
                ms->FreeTemp();
                DBRecordSeekedDEvent(key, res);
                return res;
        }

        //   Changes the active index.
        void xdbf::Setorder(String ^key)
        {
                mydb->Setorder(ms->GetString(key));
                ms->FreeTemp();
                DBOrderSettedEvent(key, true);
        }

        //  Opens and associates an index.
        bool xdbf::SetIndex(String ^fn)
        {
                bool res = (mydb->SetIndex(ms->GetString(fn)) > 0)? true : false;
                ms->FreeTemp();
                DBIndexOpenedEvent(fn, res);
                return res;
        }

        //   Searches for a record in sequence order.
        bool xdbf::Locate(String ^fld, String ^exo)
        {
                int res = mydb->Locate(ms->GetString(fld), ms->GetString(exo));
                ms->FreeTemp();
                return (res > 0)? true: false;
        }

        //   Create an index and make active.
        bool xdbf::IndexOn(String ^key, String ^fn)
        {
                int res = mydb->IndexOn(ms->GetString(key), ms->GetString(fn));
                ms->FreeTemp();
                return (res > 0)? true: false;
        }

        //   Rebuild all indexes that are open.
        void xdbf::Reindex(void)
        {
                mydb->Reindex();
        }

        //   Create a new table according to the structure shown, and make active.
        bool xdbf::createTable(String ^fn, String ^fld)
        {
                bool res = (mydb->createTable(ms->GetString(fn), ms->GetString(fld)) > 0)? true : false;
                ms->FreeTemp();
                DBTableCreatedEvent(fn, fld, res);
                return res;
        }

        //   Create a new table type CURSOR and make active.
        bool xdbf::creaCursor(String ^fn, String ^fld)
        {
                bool res = (mydb->creaCursor(ms->GetString(fn), ms->GetString(fld)) > 0)? true : false;
                ms->FreeTemp();
                DBCursorCreatedEvent(fn, fld, res);
                return res;
        }

        //   Append all records from one table to another.
        void xdbf::AppendFrom(char *filename)
        {
                mydb->AppendFrom(filename);
                DBTableAppendedEvent(filename);
        }

        //  Append from CSV
        int xdbf::AppendFromCSV(char *filename, bool headerrow, bool commas_in_text)
        {
                int res = mydb->AppendFromCSV(filename, headerrow, commas_in_text);
                DBTableAppendedCSVEvent(filename);
        return res;
        }

        //   Copy 1 record to another table -- allows VFP COPY TO equivalent
        //   when combined with own "do while"/"for" loop
        void xdbf::Copy1To(char *filename)
        {
                mydb->Copy1To(filename);
                DBTableCopied1ToEvent(filename);
        }

        //   Pack the current file
        void xdbf::Pack(void)
        {
                mydb->Pack();
                DBPackedEvent();
        }

        //   Download all table data to disk
        void xdbf::Flush(void)
        {
                mydb->Flush();
        }

        //   Deletes the currently selected record.
        void xdbf::Delete(void)
        {
                mydb->Delete();
                DBRecordDeletedEvent();
        }

        //   Truncates the database from the record (+1) indicated.
        void xdbf::Trunc(unsigned long rec)
        {
                mydb->Trunc(rec);
        }

        //   Returns a flag indicating whether the database is open or not.
        //bool xdbf::get_Opened(void)
        //{
        //        if(mydb->Opened() > 0) return true;
        //        return false;
        //}

        int xdbf::CheckIdx(const char *field)
        {
      return mydb->CheckIdx(field);
        }

//////////////////////////////////////////////////////////////////////////////////////////
//   Implementation of the wrapper class 'xfile'.

        //   Constructor
        xfile::xfile()
        {
                myfile = new xFile;
                ms = gcnew kernel::MarshalString;
                m_cache = 0;
                m_cursor = false;
        }

        //   Destructer.
        xfile::~xfile()
        {
                delete myfile;
        }

        //   Equivalent to 'sopen (...)'
        bool xfile::Open(String ^fn, bool openAsText, bool openExclusive)
        {
                int fgs1, fgs2;

                //   Build flags to use.
                fgs1 = O_RDWR | ((openAsText)? (int) O_TEXT : (int) O_BINARY);
                fgs2 = (openExclusive)? _SH_DENYNO : _SH_DENYRW;

                //   Perform the opening.
                int res = myfile->xOpen(ms->GetString(fn), fgs1, fgs2);
                ms->FreeTemp();

                //   Fire the event and return.
                bool rValue = (res > 0)? true: false;
                OpenedEvent(fn, rValue);
                return rValue;
        }

        //  Equivalent to 'creat (...)'
        bool xfile::Create(String ^fn)
        {
                //   Create the file.
                int res = myfile->xCreate(ms->GetString(fn), S_IREAD | S_IWRITE);
                ms->FreeTemp();

                //   Fire the event and return.
                CreatedEvent(fn);
                return (res > 0)? true: false;
        }

        //  Equivalent of 'close(...)'
        bool xfile::Close()
        {
                bool rValue = (myfile->xClose() > 0)? true: false;
                ClosedEvent();
                return rValue;
        }

        //  Equivalent of 'read(...)'
        String ^xfile::Read(int nLen)
        {
                //   Do the Read
                char *buff = new char[nLen + 5];
                int rdreal = myfile->xRead(buff, nLen);
                buff[rdreal] = '\0';

                //   Prepares the result, calls the event and returns.
                String ^res = ms->SetString(buff, rdreal);
                ReadedEvent(res, nLen);
                return res;
        }

        //  Equivalent of 'write(...)'
        int xfile::Write(String ^buff, int sz)
        {
                //   Do the Write
                int res = myfile->xWrite(ms->GetString(buff), sz);
                ms->FreeTemp();

                //   Call the event and return.
                WritedEvent(buff, sz, res);
                return res;
        }

        //  Equivalent of 'lseek(...)'
        long xfile::Lseek(long pos, SeekPos flag)
        {
        //   Do the operation.
                int fg;
                if(flag == SeekPos::FromStartOfFile)
                        fg = SEEK_SET;
                else
                        if(flag == SeekPos::FromEndOfFile)
                                fg = SEEK_END;
                        else
                                fg = SEEK_CUR;
                long uPos = myfile->xLseek(pos, fg);

                //   Call the event and return.
                SeekedEvent(pos, flag, uPos);
                return uPos;
        }

        //  Equivalent of 'locking(...)'
        bool xfile::Locking(long len, bool makeLock)
        {
                //   Do the operation.
                bool rValue = (myfile->xLocking((makeLock? 1: 0), len) > 0)? true: false;

                //   Call the event and return.
                LockedEvent(len, makeLock, rValue);
                return rValue;
        }

        //   Returns the current position of the file.
        //long xfile::get_Position()
        //{
        //        return myfile->xTell();
        //}

        //   Returns the length of the file.
        //long xfile::get_Length()
        //{
        //        return myfile->xFilelen();
        //}
        //   Sets the length of the file.
        //void xfile::set_Length(long newvalue)
        //{
        //        myfile->xChsize(newvalue);
        //}

        //  Returns a flag indicating whether we are at the end of the file.
        //bool xfile::get_Eof()
        //{
        //        if(myfile->xEof() > 0) return true;
        //        return false;
        //}

        //   Returns / Sets the type of cache to use.
        //int xfile::get_CacheEsq()
        //{
        //        return m_cache;
        //}
        //void xfile::set_CacheEsq(int newesq)
        //{
        //        m_cache = myfile->xCache(newesq);
        //}

        //  Returns / Sets whether or not the file cursor (deleted when it is closed).
        //bool xfile::get_IsCursor()
        //{
        //        return m_cursor;
        //}
        //void xfile::set_IsCursor(bool cursflag)
        //{
        //        myfile->xCursor();
        //        m_cursor = true;
        //}

        //   Blank pending buffers.
        void xfile::Flush()
        {
                myfile->xFlush();
                FlushedEvent();
        }

        //   Retorna si la clase tiene o no un archivo subyacente abierto.
        //   Returns whether or not the class has an open core file.
        //bool xfile::get_IsOpened()
        //{
        //        if(myfile->xOpened() > 0) return true;
        //        return false;
        //}

        //   Reopens the file, if closed with xTmpClose ().
        void xfile::Reopen()
        {
                myfile->xReopen();
                ReopenedEvent();
        }

        //   Temporarily closes a file that could be opened with 'Reopen'.
        void xfile::TemporaryClose()
        {
                myfile->xTmpClose();
                TemporaryClosedEvent();
        }


}
