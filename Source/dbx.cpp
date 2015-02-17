//  Dbx.cpp
//  Implementation of access to databases.
//


#include "stdafx.h"
#include <fstream>

#define FS_DB
#include "dbhdr.h"

using namespace System::Collections::Generic;
using namespace std;

//   List of error messages, system database.
char *dberrMSG[] =
{
        "",                     "Database Not Found",
        "Not Foxpro file type", "Empty Field List",
        "Not enough memory",    "No open file",
        "Invalid Registration", "Record locked by another user",
        "Field not found",      "You can not replace record not locked",
        "File in use exclusive","End of file found",
        "Home of file found",   "Key Index not allowed",
        "Denied access to an index",  "Index not found"
};

bool strtok_called, NULL_not_returned;

//   Functions of class 'dbf' implementation
dbf::dbf()
{
        memset(this, 0, sizeof(class dbf));
        //cliext::fill_n(this, sizeof(class dbf), 0.0);
}

dbf::~dbf()
{
        Close();
}

//  Closes a database file, if it's open.
void dbf::Close()
{
        if(!flags.in_use) return;
        ClearRelation(0);

        //   Empty data file, unlocks and closes.
        flushWrite();
        Lock(DB_UNLOCK);
        closeIndex();
        fh.xClose();

        //   Free the buffers.
        free(dbName);
        free(uRec);
        free(Flds);

        //   Mark the area as inactive.
        flags.in_use = 0;
}

//   Download all buffers and pending writes to the physical disk file
void dbf::Flush()
{
        if(!flags.in_use) return;
        flushWrite();                   // Download pending writes.
        fh.xFlush();                    // Download disk caches.
}

#define NONE_CHAR 0
#define NONE_DBL  1
#define NONE_INT  2
#define SHORTER   3
#define SHORTER_NUMBER  4
#define SKIP_CONVERSION 5
#define I_to_B  6
#define D_to_B  7
#define C_to_B  8
#define N_to_B  9
#define B_to_I 10
#define D_to_I 11
#define C_to_I 12
#define N_to_I 13
#define I_to_N 14
#define D_to_N 15
#define C_to_N 16
#define B_to_N 17
#define I_to_C 18
#define D_to_C 19
#define N_to_C 20
#define B_to_C 21
#define L_to_C 22
#define I_to_D 23
#define C_to_D 24
#define N_to_D 25
#define B_to_D 26
#define I_to_L 27
#define N_to_L 28
#define B_to_L 29
#define C_to_L 30

int copy_field(dbf *otherDb, struct dbField *f_in, dbf *myDb, struct dbField *f_out, int convert_code, char *tBuff)
{
	static long intval;
	static int  i, len;
 static double dblval;
 static char tBuff2[260];
 stringstream ss;
 string s;

 switch (convert_code)
 {
  case NONE_CHAR:   //for all matching fld types other than int and dbl
  case L_to_C:
         otherDb->Get(f_in->name, tBuff);
         myDb->Replace(f_out->name, tBuff);
         return 1;
       break;
  case NONE_DBL:
  case N_to_B:
         otherDb->GetD(f_in->name, &dblval);
         myDb->ReplaceD(f_out->name, dblval);
         return 1;
       break;
  case NONE_INT:
         otherDb->GetI(f_in->name, &intval);
         myDb->ReplaceI(f_out->name, intval);
         return 1;
       break;
  case SHORTER_NUMBER:
         //new number field has less characters than source, so drop decimals if necessary and possible
         otherDb->GetD(f_in->name, &dblval);
         i = 1 + log10(fabs(dblval)) + (dblval < 0) ? 1 : 0;
         if (i <= f_out->FieldLen - (f_out->FieldDec > 0 ? f_out->FieldDec + 1 : 0))
         {
           ss << fixed << setprecision(min((int) f_in->FieldDec, max(0, i - f_out->FieldLen))) << dblval;
           myDb->ReplaceD(f_out->name, dblval); //number will fit although there could be some decimal trunction
           return 1;
         }
         else
         {
           myDb->ReplaceD(f_out->name, 0.0); //number will not fit so just set with 0
           return 0;
         }
       break;
  case SHORTER:
       otherDb->Get(f_in->name, tBuff);
       myDb->Replace(f_out->name, tBuff);
       rtrim_to(tBuff, tBuff2);
       if (strlen(tBuff2) > f_out->FieldLen)
        return 0;  //non-space portion of field was truncated
       else
        return 1;
       break;
  case I_to_B:
         otherDb->GetI(f_in->name, &intval);
         myDb->ReplaceD(f_out->name, (double) intval);
         return 1;
       break;
  case D_to_B:
       otherDb->Get(f_in->name, tBuff);
       sscanf(tBuff, "%lf",&dblval);
       myDb->ReplaceD(f_out->name, dblval);
       return 1;
       break;
  case C_to_B:
       otherDb->Get(f_in->name, tBuff);
       dblval = atof(tBuff);  //if not string, value will be defaulted to 0
       myDb->ReplaceD(f_out->name, dblval);
       return 1;
       break;
  case B_to_I:
  case N_to_I:
       otherDb->GetD(f_in->name, &dblval);
       if (dblval > 2147483647.5 || dblval < -2147483647.5)
       {
         myDb->ReplaceI(f_out->name, 0);
         return 0;
       }
       else
       {
         myDb->ReplaceI(f_out->name, (int) round(dblval));
         return 1;
       }
       break;
  case C_to_I:
       otherDb->Get(f_in->name, tBuff);
       dblval = atof(tBuff);  //if not string, value will be defaulted to 0
       if (dblval > 2147483647.5 || dblval < -2147483647.5)
       {
         myDb->ReplaceI(f_out->name, 0);
         return 0;
       }
       else
       {
         myDb->ReplaceI(f_out->name, (int) round(dblval));
         return 1;
       }
       break;
  case D_to_I:
       otherDb->Get(f_in->name, tBuff);
       sscanf(tBuff, "%ld",&intval);
       myDb->ReplaceI(f_out->name, intval);
       return 1;
       break;
  case I_to_N:
         otherDb->GetI(f_in->name, &intval);
         i = 1 + (int) log10(abs(intval)) + (intval < 0) ? 1 : 0;
         if (i <= f_out->FieldLen - (f_out->FieldDec > 0 ? f_out->FieldDec + 1 : 0))
         {
           myDb->ReplaceD(f_out->name, (double) intval);
           return 1;
         }
         else
         {
           myDb->ReplaceD(f_out->name, 0.0); //number will not fit so just set with 0
           return 0;
         }
       break;
  case D_to_N:
       if (f_out->FieldLen - f_out->FieldDec == 8)
       {
        otherDb->Get(f_in->name, tBuff);
        sscanf(tBuff, "%ld",&intval);
        myDb->ReplaceI(f_out->name, intval);
        return 1;
       }
       else
         {
           myDb->ReplaceD(f_out->name, 0.0); //number will not fit so just set with 0
           return 0;
         }
       break;
  case C_to_N:
       otherDb->Get(f_in->name, tBuff);
       dblval = atof(tBuff);  //if not string, value will be defaulted to 0
       if (pow(10, f_out->FieldLen - (f_out->FieldDec > 0 ? f_out->FieldDec + 1 : 0)) < dblval)
       {
         myDb->ReplaceD(f_out->name, 0.0);
         return 0;
       }
       else
       {
         myDb->ReplaceD(f_out->name, dblval);
         return 1;
       }
       break;
  case B_to_N:
         otherDb->GetD(f_in->name, &dblval);
         i = 1 + (int) log10(fabs(dblval)) + (dblval < 0) ? 1 : 0;
         if (i <= f_out->FieldLen - (f_out->FieldDec > 0 ? f_out->FieldDec + 1 : 0))
         {
           myDb->ReplaceD(f_out->name, dblval);   //possible truncation
           return 1;
         }
         else
         {
           myDb->ReplaceD(f_out->name, 0.0); //number will not fit so just set with 0
           return 0;
         }
       break;
  case I_to_C:
         otherDb->GetI(f_in->name, &intval);
         i = 1 + log10(abs(intval)) + (intval < 0) ? 1 : 0;
         if (i <= f_out->FieldLen)
         {
           ss << fixed << setprecision(0) << intval;
           ss >> s;
           i = f_out->FieldLen - s.length();
           if(i) memset(tBuff, ' ', i);
           strncpy(tBuff + i, s.c_str(), s.length());

           myDb->Replace(f_out->name, tBuff);
           return 1;
         }
         else
         {
           myDb->Replace(f_out->name, " "); //number will not fit so just set with blank
           return 0;
         }
       break;
  case D_to_C:
       otherDb->Get(f_in->name, tBuff);
       if (f_out->FieldLen >= 10)
       {
         strncpy(tBuff+20, tBuff+4, 2);
         tBuff[22] = '/';
         strncpy(tBuff+23, tBuff+6, 2);
         tBuff[25] = '/';
         strncpy(tBuff+26, tBuff, 4);
         tBuff[30] = '\0';
         myDb->Replace(f_out->name, tBuff+20);
         return 1;
       }
       if (f_out->FieldLen >= 8)
       {
         myDb->Replace(f_out->name, tBuff);
         return 1;
       }
       myDb->Replace(f_out->name, " ");
       return 0;
       break;
  case N_to_C:
  case B_to_C:
         otherDb->GetD(f_in->name, &dblval);
         i = 1 + log10(fabs(dblval)) + (dblval < 0) ? 1 : 0;
         if (i <= f_out->FieldLen)
         {
           ss << fixed << setprecision(min((int) f_in->FieldDec, max(0, i - (f_out->FieldDec > 0 ? f_out->FieldDec + 1 : 0)))) << dblval;
           ss >> s;
           i = f_out->FieldLen - s.length();
           if(i) memset(tBuff, ' ', i);
           strncpy(tBuff + i, s.c_str(), s.length());

           myDb->Replace(f_out->name, tBuff);
           return 1;
         }
         else
         {
           myDb->ReplaceD(f_out->name, 0.0); //number will not fit so just set with 0
           return 0;
         }
       break;
  case I_to_D:
       otherDb->GetI(f_in->name, &intval);
       if (intval > 99999999 || intval <= 9999999)
       {
         myDb->Replace(f_out->name, " ");
         return 0;
       }
       sprintf(tBuff, "%10ld", intval);
       myDb->Replace(f_out->name, tBuff);
       return 1;
       break;
  case C_to_D:
       otherDb->Get(f_in->name, tBuff);
       if (f_in->FieldLen >= 10)
       {
         strncpy(tBuff+20, tBuff+6, 4);
         strncpy(tBuff+24, tBuff, 2);
         strncpy(tBuff+26, tBuff+3, 2);
         tBuff[28] = '\0';
         myDb->Replace(f_out->name, tBuff+20);
         return 1;
       }
       if (f_out->FieldLen >= 8)
       {
         myDb->Replace(f_out->name, tBuff);
         return 1;
       }
       myDb->Replace(f_out->name, " ");
       return 0;
       break;
  case N_to_D:
  case B_to_D:
       otherDb->GetD(f_in->name, &dblval);
       if (dblval > 99999999.99 || dblval <= 9999999.99)
       {
         myDb->Replace(f_out->name, " ");
         return 0;
       }
       sprintf(tBuff, "%10.0lf", dblval);
       myDb->Replace(f_out->name, tBuff);
       return 1;
       break;
  case I_to_L:
       otherDb->GetI(f_in->name, &intval);
       if (intval == 0)
       {
         myDb->Replace(f_out->name, "N");
         return 1;
       }
       else
       {
         myDb->Replace(f_out->name, "Y");
         return 1;
       }
       break;
  case N_to_L:
  case B_to_L:
       otherDb->GetD(f_in->name, &dblval);
       if (dblval == 0.0)
       {
         myDb->Replace(f_out->name, "N");
         return 1;
       }
       else
       {
         myDb->Replace(f_out->name, "Y");
         return 1;
       }
       break;
  case C_to_L:
       otherDb->Get(f_in->name, tBuff);
       if (tBuff[0] == 'Y' || tBuff[0] == 'y' || tBuff[0] == 'T' || tBuff[0] == 't')
         myDb->Replace(f_out->name, "Y");
       else
         myDb->Replace(f_out->name, "N");
       return 1;
       break;
 }
}


//   Copy ALL the contents of a database to another, keeping
//   a relationship for field.  -- NOT SURE THIS WILL WORK  jas
void dbf::AppendFrom(char *filename)
{
   dbf otherDb;
   struct dbField *f, *f2, *otherflds[200];
   int i, i2, otherint, copyok;
   double otherdbl;
   unsigned long numrecs, curr_rec, r, r2;
   char *tBuff, tmpprice[12];
   int otherflds_conversion[200];

    if (!file_exists(filename))
    {
     //zzz add code to post error somewhere
     return;
    }
    otherDb.Use(filename);
    numrecs = otherDb.dbhd.RecCount;
    tBuff = (char *) malloc(1000);

  for (i = cFlds, f = Flds; i; i--, f++)
  {
    for (i2 = otherDb.cFlds, f2 = otherDb.Flds; i2; i2--, f2++)
    {
      if (!strncmp(f->name, f2->name, 10))
        break;
    }

    if (i2)
    {
      otherflds[i] = f2;
      //do we have to worry about data type conversion or truncating?
      if (f->FieldLen >= f2->FieldLen && f->DataType == f2->DataType)
      {//No conversion required
        if (f->DataType == 'B')
          otherflds_conversion[i] = NONE_DBL;
        else
        {
          if (f->DataType == 'I')
            otherflds_conversion[i] = NONE_INT;
          else
            otherflds_conversion[i] = NONE_CHAR;
        }
      }
      else
      {
        if (f->DataType == 'B')
        {
          switch (f2->DataType)
          {
          case 'I':
            otherflds_conversion[i] = I_to_B;
            break;
          case 'D':
            otherflds_conversion[i] = D_to_B;
            break;
          case 'N':
            otherflds_conversion[i] = N_to_B;
            break;
          case 'C':
            otherflds_conversion[i] = C_to_B;
            break;
          default:
            otherflds_conversion[i] = SKIP_CONVERSION;
            break;
          }
        }
        else
        {
          if (f->DataType == 'I')
          {
            switch (f2->DataType)
            {
            case 'B':
              otherflds_conversion[i] = B_to_I;
              break;
            case 'N':
              otherflds_conversion[i] = N_to_I;
              break;
            case 'C':
              otherflds_conversion[i] = C_to_I;
              break;
            default:
              otherflds_conversion[i] = SKIP_CONVERSION;
              break;
            }
          }
          else
          {
            if (f->DataType == 'N')
            {
              switch (f2->DataType)
              {
              case 'I':
                otherflds_conversion[i] = I_to_N;
                break;
              case 'D':
                otherflds_conversion[i] = D_to_N;
                break;
              case 'B':
                otherflds_conversion[i] = B_to_N;
                break;
              case 'C':
                otherflds_conversion[i] = C_to_N;
                break;
              case 'N':
                otherflds_conversion[i] = SHORTER_NUMBER;
                break;
              default:
                otherflds_conversion[i] = SKIP_CONVERSION;
                break;
              }
            }
            else
            {
              if (f->DataType == 'C')
              {
                switch (f2->DataType)
                {
                case 'I':
                  otherflds_conversion[i] = I_to_C;
                  break;
                case 'D':
                  otherflds_conversion[i] = D_to_C;
                  break;
                case 'N':
                  otherflds_conversion[i] = N_to_C;
                  break;
                case 'B':
                  otherflds_conversion[i] = B_to_C;
                  break;
                case 'L':
                  otherflds_conversion[i] = L_to_C;
                  break;
                case 'C':
                  otherflds_conversion[i] = SHORTER;
                  break;

                default:
                  otherflds_conversion[i] = SKIP_CONVERSION;
                  break;
                }
              }
              else
              {
                if (f->DataType == 'D')
                {
                  switch (f2->DataType)
                  {
                  case 'I':
                    otherflds_conversion[i] = I_to_D;
                    break;
                  case 'C':
                    otherflds_conversion[i] = C_to_D;
                    break;
                  case 'N':
                    otherflds_conversion[i] = N_to_D;
                    break;
                  case 'B':
                    otherflds_conversion[i] = B_to_D;
                    break;
                  default:
                    otherflds_conversion[i] = SKIP_CONVERSION;
                    break;
                  }
                }
                else
                {
                  if (f->DataType == 'L')
                  {
                    switch (f2->DataType)
                    {
                    case 'I':
                      otherflds_conversion[i] = I_to_L;
                      break;
                    case 'N':
                      otherflds_conversion[i] = N_to_L;
                      break;
                    case 'B':
                      otherflds_conversion[i] = B_to_L;
                      break;
                    case 'C':
                      otherflds_conversion[i] = C_to_L;
                      break;
                    default:
                      otherflds_conversion[i] = SKIP_CONVERSION;
                      break;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    else
      otherflds[i] = NULL;


  }

    for (curr_rec = 1; curr_rec <= numrecs; curr_rec++)
    {
    //if (curr_rec == 4700l)
     // r = Recno();

        AppendBlank();
        otherDb.Go(curr_rec);
    for(i = cFlds, f = Flds; i; i--, f++)
    {
      if (otherflds[i] != NULL)
      {
		  if (otherflds_conversion[i] != SKIP_CONVERSION)
			  copyok = copy_field(&otherDb, otherflds[i], this, f, otherflds_conversion[i], tBuff);
      }
    }


        //for(i = otherDb.cFlds, f = Flds; i; i--, f++)
        //{
        //        otherDb.Get(f->name, tBuff);
         //     Replace(f->name, tBuff);
        //}
    //flushWrite();
    //temp lines to correct insert issue for index
//    CheckIdx("UPPER(DES)+UPPER(COD)+STR(DBLTST,9,2)");
    //CheckIdx("PRICE");
    //Go(0);
    //r = Recno();
    //Get("price", tBuff);
    //Skip(1);
    //for (i = 2; i <= dbhd.RecCount; i++)
    //{
    //  r2 = Recno();
    //  Get("price", tmpprice);
    //  if (memcmp(tmpprice, tBuff, 8) == 0)
    //  {
    //    if (r >= Recno())
    //    {
    //      r = r;
    //    }
    //  }
    //  else
    //    memmove(tBuff, tmpprice, 8);

    //  r = Recno();
    //  Skip(1);
    //}
    }
    otherDb.Close();
    free(tBuff);
}

//Too hard to create true Copy To command/function as it would require parsing
//Instead, use Copy1To command with own "do while"/"for" loops to
//achieve same objective
//This routine can be made faster by copying the entire data record rather than field by field
void dbf::Copy1To(char *filename)
{
     dbf otherDb;
     struct dbField *f;
     int i;
     unsigned long numrecs, curr_rec;
     char *tBuff;

    otherDb.Use(filename);
    tBuff = (char *) malloc(1000);
    otherDb.AppendBlank();
    for(i = otherDb.cFlds, f = Flds; i; i--, f++)
    {
     Get(f->name, tBuff);
     otherDb.Replace(f->name, tBuff);
    }
    otherDb.Close();
    free(tBuff);
}

//routine to pack a dbf
void dbf::Pack()
{
     CopyStruct("__temp.dbf");
     Go(DB_GO_TOP);
     while (!Eof())
     {
      if (!Deleted())
        Copy1To("__temp.dbf");
      Skip(1);
     }
     Zap();
     AppendFrom("__temp.dbf");
     remove("__temp.dbf");
}

// Opens a database. Receives as a parameter, the file
// name. If the name  received has no extension, then
// adds the extension 'dbf'.
int dbf::Use(const char *fn)
{
    unsigned dbNameLen, i;
    unsigned long LastOffset;
    int addExt;
    struct dbField *f;

    Close();
    view.dberr = DB_NO_ERROR;

    // Sets the file name and opens it.
    dbNameLen = strlen(fn);
    addExt = !strchr(fn, '.');
    if(addExt) dbNameLen += 4;
        dbName = (char *) malloc(dbNameLen + 1);
        strcpy(dbName, fn);
        if(addExt) strcat(dbName, ".dbf");
        if(fh.xOpen(dbName, O_BINARY | O_RDWR,
                        view._exclusive? _SH_DENYRW: _SH_DENYNO) <0)
        {
                free(dbName);
                if(errno == EACCES)
        {
            view.dberr = DB_FILE_EXCLU;
            return !view.dberr;
        }
        else
        {
            view.dberr = DB_FILE_NOT_FOUND;
            return !view.dberr;
        }
        }

        //  Reads the header of the base and checks the type of the same.
        fh.xRead(&dbhd, sizeof(struct dbHeader));
        switch(dbhd.dbID)
        {
                case NOMEMO:           //   FoxBase w/o memo.
                case VFP_NOMEM:        //   VFP w/o memo.
                        break;

                default:
                        free(dbName);
            view.dberr = DB_NOT_DB;
            return !view.dberr;
        }

        //   Make final checks.
        if (dbhd.dbID == VFP_NOMEM)
         cFlds = (dbhd.FirstDataByte - 296) / 32;
        else
         cFlds = (dbhd.FirstDataByte / 32) - 1;

        if(!cFlds)
        {
                free(dbName);
        view.dberr = DB_FILE_EMPTY;
        return !view.dberr;
        }

        //   Locate memory to hold the list of definition of fields.
        Flds = (struct dbField *)  malloc(sizeof(struct dbField) * cFlds);

        //   Locate memory to hold data for the current record.
        uRec = (unsigned char *) malloc(dbhd.LenRecord);
        if(!Flds || !uRec)
        {
                free(dbName);
                if(uRec) free(uRec);
                if(Flds) free(Flds);
        view.dberr = DB_NO_MEMORY;
        return !view.dberr;
        }

        //   Load the list of fields.
        LastOffset = 1;
        for(f = Flds, i = 0; i < cFlds; i++)
        {
                fh.xLseek((i + 1) * 32, SEEK_SET);
                fh.xRead(f, sizeof(struct dbField));
                f->OffsetInRec = LastOffset;
                LastOffset += f->FieldLen;
                f++;
        }

        //   Initializes some other control variables.
        rel = 0;
        idxlist = idxmaster = 0;
        activeKey = 0;

        //   Set final flags.
        flags.eof = flags.bof = flags.wri = flags.lock = flags.rdy = 0;
        flags.excl = view._exclusive? 1 : 0;
        flags.in_use = 1;
        recno = 2; dbGo(DB_GO_TOP);
        return 1;
}

//  Establish a relationship with another database.
void dbf::SetRelation(dbf *target_, char *field_)
{
        dbRelation *p;

        if(!flags.in_use) return;
        for(p = rel; p; p = p->next)
        if((dbf *) p->target == target_)
                {
                        free(p->field);
                        break;
                }
        if(!p)
        {
                p = (dbRelation *) malloc(sizeof(dbRelation));
                p->next = rel;
                rel = p;
        }
        #ifdef __SC__
    p->target = (dbRelation::dbf *) target_;
        #else
    p->target = target_;
        #endif
        p->field = (char *) malloc(strlen(field_) + 1);
        strcpy(p->field, field_);
        GoRelation();
}

//  Removes relationships.
void dbf::ClearRelation(dbf *target_)
{
        dbRelation *p, *p2;

        if(!target_)
        {
                for(p = rel; p; p = p2)
                {
                        p2 = p->next;
                        free(p->field);
                        free(p);
                }
                rel = 0;
        }
        else
        {
                for(p = rel; p; p = p->next)
                {
                        #ifdef __SC__
            if(p->target == (dbRelation::dbf *) target_) break;
                        #else
            if(p->target == target_) break;
                        #endif
                }
                if(p)
                {
                        if(p == rel)
                                rel = p->next;
                        else
                        {
                                for(p2 = rel; p2->next != p; p2 = p2->next)
                                        ;
                                p2->next = p->next;
                        }
                        free(p->field);
                        free(p);
                }
        }
}

//  Searching sequentially in a file.
int dbf::Locate(char *kfld, char *value)
{
        int uLen;
        char *ubuff;

        uLen = FieldLen(kfld);
        if(uLen == 0) return 0;
    ubuff = (char *) malloc(uLen + 1);
        if((int) strlen(value) < uLen) uLen = (int) strlen(value);
        Go(DB_GO_TOP);
        while(!Eof())
        {
                Get(kfld, ubuff);
                if(!memcmp(ubuff, value, uLen))
                {
            free(ubuff);
                        return 1;
                }
                Skip(1);
        }
    free(ubuff);
        return 0;
}

//  Empty the Writes/Updates that may be pending.
void dbf::flushWrite()
{
        unsigned long pos;

        //  Validates the state to avoid doing unnecessary operations.
        if(!flags.in_use || !flags.wri) return;

        //  Determines the initial position of the record to write to the file.
        pos = ((recno - 1) * dbhd.LenRecord) + dbhd.FirstDataByte;

        //  Blocks the physical record (Logical already locked).
        if(!flags.excl)
        {
                fh.xLseek(pos, SEEK_SET);
                fh.xLocking(LK_LOCK, dbhd.LenRecord);
        }

        //  Actually write the record
        fh.xLseek(pos, SEEK_SET);
        fh.xWrite(uRec, dbhd.LenRecord);

        //  Unlock the physical record (The logic remains locked).
        if(!flags.excl)
        {
                fh.xLseek(pos, SEEK_SET);
                fh.xLocking(LK_UNLCK, dbhd.LenRecord);
        }

        //   Update the flags.
        flags.wri = 0;
}

//   rLock: Place a record level locks, in multiuser environments
//
//   Parametro: DB_UNLOCK to unlock, to lock DB_LOCK.
//
//   Attention:
//        When the action is to lock, and the record was NOT previously locked,
//        but IF I read, this function forces a rereading of the data by changing
//        the flag 'rdy' to zero. This is so because it is the only way to ensure
//        that lockedd data are the most current.
//
//        The results are unpredictable if you call this function AFTER having
//        positioned a record and read or written data, as these cannot be present,
//        or can destroy true integrity of the writes
int dbf::Lock(int fg)
{
        unsigned long pos;

        if(!flags.in_use)
    {
        view.dberr = DB_NO_DBOPEN;
        return !view.dberr;
    }

        //   Some validations, to avoid unnecessary processing.
        if(flags.excl) return 1;                // If it is open in exclusive mode.
        if(fg && flags.lock) return 1;          // If it is already locked and is asking to be locked
        if(!fg && !flags.lock) return 1;        // If it is not locked and is asking to be unlock.

        if(!fg) flushWrite();                           // If the argument is unlock, force pending Writes.

        //   Esta función solo opera sobre registros LOGICOS, a fin de mantener destrabados
        //   y abiertos a los registros fisicos correspondientes.
        //   This feature only works on LOGICal records, in order to keep records unlocked
        //   and open to the corresponding physical registers.
        if(!flags.wCdx)
                pos = 0x40000000 + (((recno - 1) * dbhd.LenRecord) + dbhd.FirstDataByte);
        else
                pos = 0x7ffffffe - recno;
        fh.xLseek(pos, SEEK_SET);
        if(!fg)
        {
                fh.xLocking(LK_UNLCK, 1);
                flags.lock = 0;                                 // Indicates record unlocked
        }
        else
        {
                if(!fh.xLocking(LK_LOCK, 1))
                {
                        flags.lock = 1;                         // Indicates record locked
                        flags.rdy = 0;                          // Forces rereading of record
                }
                else
        {
            view.dberr = DB_REC_LOCKED; // Returns error of record locked by another.
            return !view.dberr;
        }
        }
        return 1;
}

//   Routine internal use, used by those functions that change
//   the value of recno () to make changes in databases automatically with the related dbf :: SetRelation
void dbf::GoRelation()
{
        dbRelation *p;
    dbf *bp;
        char *tBuff;
        int LastLen, uLen;

        tBuff = 0;
        for(p = rel; p; p = p->next)
        {
                //  ´tBuff´ is used to read data from the field
                //  by which the relation is made
                if(!tBuff)
                {
                        LastLen = 100;
            tBuff = (char *) malloc(LastLen);
                }

                uLen = FieldLen(p->field);              // Validates the relationship field.
                if(uLen)
                {
                        //  If the length of the buffer is not enough, it resizes.
                        if(LastLen <= uLen)
                        {
                                LastLen = uLen + 100;
                tBuff = (char *) realloc(tBuff, LastLen);
                        }

                        //   Take the field data and engages the remote database.
                        Get(p->field, tBuff);
            bp = (dbf *) p->target;
            if(!bp->Seek(tBuff))
                        {
                bp->Go(DB_GO_BOTTOM);    // If not found, it is at EOF
                if(!bp->Eof())
                    bp->Skip(1);
                        }
                }
        }

        //  Releases the buffer used.
    if(tBuff) free(tBuff);
}

//   Va a determinado registro. Si 'nrec' es DB_GO_TOP ó DB_GO_BOTTOM
//   y hay índice abierto, lo utiliza para 'seekear' el primer/último
//   registro. Sino, va a dbf::dbGo directamente.
//   Given a record number. If 'nrec' is DB_GO_TOP DB_GO_BOTTOM and there
//   are index files open, use 'seekear' to go to the first / last record.
//   Otherwise, it will dbf :: DBGO directly.
int dbf::Go(unsigned long nrec)
{
        int retValue;

        if(idxmaster && (nrec == DB_GO_BOTTOM || nrec == DB_GO_TOP))
        {
                if(nrec == DB_GO_BOTTOM)
                {
                        retValue = idxGo(DB_GO_BOTTOM, idxmaster);
                        return retValue;
                }
                else
                {
                        retValue = idxGo(DB_GO_TOP, idxmaster);
                        return retValue;
                }
        }
        retValue = dbGo(nrec);
        return retValue;
}

//   Given record indicated by no. USE INTERNAL ROUTINE. The implementation uses dbf :: Go ...
int dbf::dbGo(unsigned long nrec)
{
        int retValue, recno_0;

        //   Validate the use area.
        if(!flags.in_use)
    {
        view.dberr = DB_NO_DBOPEN;
        return !view.dberr;
    }

        flags.eof = flags.bof = flags.rdy = 0;  // Force some states.
        recno_0 = view.recno_0;
        if(nrec == recno) return 1;             // Avoid unnecessary processes.
        flushWrite();                           // Force pending Writes
        Lock(DB_UNLOCK);                        // Unlock locked records.

        //   If the argument is GO TOP
        if(nrec == DB_GO_TOP)
        {
                if(!Go(1l))
                {
                        flags.bof = 1;
                        recno = 1;
                        GoRelation();
            view.dberr = DB_BAD_RECNUM;
                        view.recno_0 = recno_0;
            return !view.dberr;
                }
                GoRelation();
                view.recno_0 = recno_0;
                return 1;
        }

        //  If the argument is GO BOTTOM.
        if(nrec == DB_GO_BOTTOM)
        {
                retValue = Go(dbhd.RecCount);
                GoRelation();
                view.recno_0 = recno_0;
                return retValue;
        }

        //   If the argument is GO <record>, validates the range of the requested record.
        if(nrec > dbhd.RecCount)
        {
                recno = dbhd.RecCount + 1;
                flags.eof = 1;
        view.dberr = DB_BAD_RECNUM;
                view.recno_0 = recno_0;
        return !view.dberr;
        }

        //   Effect the GO
        if(nrec < 1)
                recno = 1;
        else
                recno = nrec;           // Es un record cualquiera
        GoRelation();
        view.recno_0 = recno_0;
        return 1;
}

//   Find a field, put the field contents in the buffer and return.
int dbf::Get(const char *nm, char *buff)
{
        struct dbField *f;
        int i;
        static char uName[32];

        //   Valida el area.
        if(!flags.in_use)
    {
        view.dberr = DB_NO_DBOPEN;
        return !view.dberr;
    }

        _strupr(strcpy(uName, nm));
        for(i = cFlds, f = Flds; i; i--, f++)
        {
           if(!_stricmp(f->name, uName))
           {
                   //   If the pointer is at EOF always returns empty.
                   if(flags.eof)
                   {
                           memset(buff, ' ', f->FieldLen);
                           buff[f->FieldLen] = '\0';
                   }
                   else
                   {
                          //   Force record read, if not yet read.
                          getActualRecord();

                          //   Extracts data from the record buffer.
                          memmove(buff, uRec + f->OffsetInRec, f->FieldLen);
                          buff[ f->FieldLen ] = '\0';
                   }
                   return 1;
           }
        }
        buff[0] = '\0';                 // Field not found.
    view.dberr = DB_FLD_NOTFOUND;
    return !view.dberr;
}

//   Find a field, put the field contents in the buffer and return.
int dbf::GetD(const char *nm, double *dblvar)
{
        struct dbField *f;
        int i;
        char uName[32], buff[20];

        //   Valida el area.
        if(!flags.in_use)
    {
        view.dberr = DB_NO_DBOPEN;
        return !view.dberr;
    }

        _strupr(strcpy(uName, nm));
        for(i = cFlds, f = Flds; i; i--, f++)
        {
           if(!_stricmp(f->name, uName))
           {
                   //   If the pointer is at EOF always returns empty.
                   if(flags.eof)
                   {
                           *dblvar = 0.0;
                   }
                   else
                   {
                          //   Force record read, if not yet read.
                          getActualRecord();
              if (f->DataType == 'N')
              {
                memmove(buff, uRec + f->OffsetInRec, f->FieldLen);
                buff[f->FieldLen] = '\0';
                sscanf(buff, "%lf", dblvar);
              }
              else
                              memmove(dblvar, uRec + f->OffsetInRec, f->FieldLen);
                   }
                   return 1;
           }
        }
    *dblvar = 0.0;    // Field not found.
    view.dberr = DB_FLD_NOTFOUND;
    return !view.dberr;
}

//   Find a field, put the field contents in the buffer and return.
int dbf::GetI(const char *nm, long *intvar)
{
        struct dbField *f;
        int i;
        static char uName[32];

        //   Valida el area.
        if(!flags.in_use)
    {
        view.dberr = DB_NO_DBOPEN;
        return !view.dberr;
    }

        _strupr(strcpy(uName, nm));
        for(i = cFlds, f = Flds; i; i--, f++)
        {
           if(!_stricmp(f->name, uName))
           {
                   //   If the pointer is at EOF always returns empty.
                   if(flags.eof)
                   {
                           *intvar = 0;
                   }
                   else
                   {
                          //   Force record read, if not yet read.
                          getActualRecord();

                          //   Extracts data from the record buffer.
                          memmove(intvar, uRec + f->OffsetInRec, f->FieldLen);
                   }
                   return 1;
           }
        }
    *intvar = 0;    // Field not found.
    view.dberr = DB_FLD_NOTFOUND;
    return !view.dberr;
}

//  The following function returns 1 if the current record is deleted, else returns 0.
int dbf::Deleted()
{
        unsigned long pos;

        //   performs some validations.
        if(!flags.in_use || flags.eof) return 0;

        //   If record has not yet been read it, read it.
        if(!flags.rdy)
        {
                pos = ((recno - 1) * dbhd.LenRecord) + dbhd.FirstDataByte;
                fh.xLseek(pos, SEEK_SET);
                fh.xRead(uRec, dbhd.LenRecord);
                flags.rdy = 1;
        }
        return (uRec[0] == '*')? 1: 0;
}

// DELETEs (erases) the current record.
void dbf::Delete()
{
        //   Performs some validations.
        if(!flags.in_use)
    {
        view.dberr = DB_NO_DBOPEN;
        return;
    }

        //   Force the record to be deleted to be locked .
        Lock(DB_LOCK);
        if(!flags.excl && !flags.lock)
    {
        view.dberr = DB_REC_NO_LOCKED;
        return;
    }

        //  Read the record, if not read
        getActualRecord();

        //   Performs a delete operation.
        if(!flags.eof)
        {
                uRec[0] = '*';          // Mark the record.
                flags.wri = 1;
        }
        return;
}

//   Force the reading of the record actually selected, from disk to buffer reserved for that purpose.
void dbf::getActualRecord()
{
        unsigned long pos;

        if(!flags.rdy)
        {
                pos = ((recno - 1) * dbhd.LenRecord) + dbhd.FirstDataByte;
                fh.xLseek(pos, SEEK_SET);
                fh.xRead(uRec, dbhd.LenRecord);
                flags.rdy = 1;
        }
}

// Write the data passed in the field indicated.
int dbf::Replace(const char *nm, char *buff)
{
        struct dbField *f;
        int i;
        static char uName[32];

        //   Performs some validations.
        if(!flags.in_use)
    {
        view.dberr = DB_NO_DBOPEN;
        return !view.dberr;
    }

        //   Look for the field to be replaced.
        _strupr(strcpy(uName, nm));
        for(i = cFlds, f = Flds; i; i--, f++)
        {
           if(!strcmp(f->name, uName))
           {
                  //   Force the record to be locked.
                  Lock(DB_LOCK);
                  if(!flags.excl && !flags.lock)
                  {
                   view.dberr = DB_REC_NO_LOCKED;
                   return !view.dberr;
                  }

                  //   Force reading record data.
                  getActualRecord();

                  //   Performs replacement, if we are not at the EOF.
                  if(!flags.eof)
                  {
                         int c;
                         char *InDataPtr;

                         //  En el código siguiente, 'c' queda con la cantidad de caracteres
                         //  que se deben poner al comienzo del registro, para completar un
                         //  campo. También, si los datos vienen mas extensos que el ancho del
                         //  campo, trunca los datos.
                         //  In the following code, 'c' is the number of characters
                         //  to be put to the beginning of the record, to complete a
                         //  field. Also, if the data is wider than the width of
                         //  field truncates the data.
                         c = f->FieldLen - strlen(buff);
                         if(c < 0)
                         {
                                c = 0;
                                buff[f->FieldLen] = '\0';
                         }

                         //   Arm 'pbuff' with new content field.
                         if(f->DataType == 'C')
                         {
                                 // Characters, left-justified in the field.
                                 strcpy(pbuff, buff);
                                 for(InDataPtr = pbuff + strlen(pbuff); c; c--)
                                         *InDataPtr++ = ' ';
                                 *InDataPtr = 0;
                         }
                         else
                         {
                                 // The rest, right aligned.
                                 if(c) memset(pbuff, ' ', c);
                                 memmove(pbuff + c, buff, strlen(buff));
                         }

                         //   Chequea si este campo afecta a un indice actualmente abierto y,
                         //   de ser así, lo actualiza.
                         //   Check if this field affects an index currently open and, if so, update it.
                         InDataPtr = (char *)(uRec + f->OffsetInRec);
                         CheckIndexUpdate(uName, InDataPtr, pbuff, f->FieldLen);

                         //   Finalmente, fectúa los cambios en el contenido del registro.
                         //   Finally, make changes to the contents of the record.
                         memmove(InDataPtr, pbuff , f->FieldLen);
                         flags.wri = 1;
                  }
                  return 1;
           }
        }
    view.dberr = DB_FLD_NOTFOUND;
    return !view.dberr;
}

// Write a double   WILL THIS OVERLOAD WORK??  If yes, do same for INT.  What about indices??
int dbf::ReplaceD(const char *nm, double dblvar)
{
  struct dbField *f;
  int i, c;
  static char uName[32];
  char tmpname[32];

    //   Performs some validations.
    if(!flags.in_use)
    {
      view.dberr = DB_NO_DBOPEN;
      return !view.dberr;
    }

    //   Look for the field to be replaced.
    _strupr(strcpy(uName, nm));
  strcpy(tmpname, uName);
    for(i = cFlds, f = Flds; i; i--, f++)
    {
       if(!strcmp(f->name, uName))
       {
              //   Force the record to be locked.
        Lock(DB_LOCK);
        if(!flags.excl && !flags.lock)
        {
         view.dberr = DB_REC_NO_LOCKED;
         return !view.dberr;
        }

        //   Force reading record data.
        getActualRecord();

        //   Performs replacement, if we are not at the EOF.
        if(!flags.eof)
        {
          char *InDataPtr;

          if(f->DataType == 'B')
            memmove(pbuff, &dblvar, 8);

          if(f->DataType == 'N')
          {
           stringstream ss;
           string s;
           ss << fixed << setprecision(f->FieldDec) << dblvar;
           ss >> s;
           c = f->FieldLen - s.length();
           if(c) memset(pbuff, ' ', c);
           strncpy(pbuff + c, s.c_str(), s.length());
          }

           //   Check if this field affects an index currently open and, if so, update it.
              InDataPtr = (char *)(uRec + f->OffsetInRec);
              CheckIndexUpdate(uName, InDataPtr, pbuff, f->FieldLen);

                  //   Finally, make changes to the contents of the record.
                  memmove(InDataPtr, pbuff , f->FieldLen);
                  flags.wri = 1;
           }
           return 1;
       }
    }
    view.dberr = DB_FLD_NOTFOUND;
    return !view.dberr;
}

//Replace the value of an integer dbf field
int dbf::ReplaceI(const char *nm, long intvar)
{
 struct dbField *f;
 int i;
 static char uName[32];

 //   Performs some validations.
 if (!flags.in_use)
 {
         view.dberr = DB_NO_DBOPEN;
         return !view.dberr;
 }

 //   Look for the field to be replaced.
 _strupr(strcpy(uName, nm));
 for (i = cFlds, f = Flds; i; i--, f++)
 {
  if (!strcmp(f->name, uName))
  {
   //   Force the record to be locked.
   Lock(DB_LOCK);
   if (!flags.excl && !flags.lock)
   {
           view.dberr = DB_REC_NO_LOCKED;
           return !view.dberr;
   }

   //   Force reading record data.
   getActualRecord();

   //   Performs replacement, if we are not at the EOF.
   if (!flags.eof)
   {
    char *InDataPtr;

    if (f->DataType == 'I')
    {
            memmove(pbuff, &intvar, 4);
    }

    //INDEXING PROBABLY CAN'T DEAL with a DOUBLE!!!!
    //            //   Check if this field affects an index currently open and, if so, update it.
            InDataPtr = (char *)(uRec + f->OffsetInRec);
            CheckIndexUpdate(uName, InDataPtr, pbuff, f->FieldLen);

    //   Finally, make changes to the contents of the record.
    memmove(InDataPtr, pbuff, f->FieldLen);
    flags.wri = 1;
   }
   return 1;
  }
 }
 view.dberr = DB_FLD_NOTFOUND;
 return !view.dberr;
}

//    Add a blank record at the end of the database.
int dbf::AppendBlank()
{
        unsigned short sz;
        unsigned long pos, svRecno;
        unsigned char eofmarker[1];
        eofmarker[0] = (char)26;
        struct dbField *f;
        int i;

        //  Performs some validations.
        if(!flags.in_use)
    {
        view.dberr = DB_NO_DBOPEN;
        return !view.dberr;
    }

        //Empty pending writes and unlocks the record.
        Lock(DB_UNLOCK);
        flushWrite();

        //   Locks the logical record.
        svRecno = recno; recno = 0l;
        while(!Lock(DB_LOCK))
                ;
        recno = svRecno;

        //   Reread the header and lockup the file.
        sz = sizeof(struct dbHeader);
        if(!flags.excl)
           do
                   fh.xLseek(0l, SEEK_SET);
           while(fh.xLocking(LK_LOCK, sz));

        fh.xLseek(0l, SEEK_SET);
        fh.xRead(&dbhd, sz);

        //   Add the new record.
        dbhd.RecCount++;

        //   Write a new blank record.
        pos = ((dbhd.RecCount - 1) * dbhd.LenRecord) + dbhd.FirstDataByte;
        fh.xLseek(pos, SEEK_SET);
        memset(uRec, ' ', dbhd.LenRecord);

        //check for non-character fields as they need to be stuffed with 0 instead of blank
        for(i = cFlds, f = Flds; i; i--, f++)
        {
          if (f->DataType == 'B' || f->DataType == 'I')
           memset(uRec + f->OffsetInRec, '\0', f->FieldLen);
        }

        fh.xWrite(uRec, dbhd.LenRecord);
        fh.xWrite(eofmarker, 1);  //add eof marker jas

        //   Rewrite the header and unlocks.
        fh.xLseek(0l, SEEK_SET);
        fh.xWrite(&dbhd, sz);
        if(!flags.excl)
        {
                fh.xLseek(0l, SEEK_SET);
                fh.xLocking(LK_UNLCK, sz);
        }

        //   Unlock the logical record.
        recno = 0l;
        Lock(DB_UNLOCK);

        //   Update some variables and flags.
        recno = dbhd.RecCount;
        flags.eof = flags.bof = flags.wri = 0;
        flags.rdy = 1;

        //   Insert blank keys in the indexes if needed.
        InsertBlankKey();
        return 0;
}

//   Searches a file for a string, using an index.
int dbf::Seek(char *k)
{
        unsigned long n, recno_0;;

        //   Efectua algunas validaciones.
        if(!flags.in_use || !idxmaster)
    {
        view.dberr = DB_NO_DBOPEN;
        return !view.dberr;
    }

        //   Performs search index (idxSearch returns the recno).
        n = idxSearch(idxmaster, k);
        if(n)
        return dbGo(n);

        //   did not find it, position at the EOF.
        dbGo(DB_GO_BOTTOM);
        recno_0 = view.recno_0;
        if(!flags.eof) Skip(1);
        view.recno_0 = recno_0;
        return 0;
}
//   Searches a file for a string, using an index.
int dbf::SeekD(double dbl)
{
  unsigned long n, recno_0;
  char k[9];

  memcpy(k, &dbl, 8);
  k[8] = '\0';

  //   Efectua algunas validaciones.
  if (!flags.in_use || !idxmaster)
  {
    view.dberr = DB_NO_DBOPEN;
    return !view.dberr;
  }

  //   Performs search index (idxSearch returns the recno).
  n = idxSearch(idxmaster, k);
  if (n)
    return dbGo(n);

  //   did not find it, position at the EOF.
  dbGo(DB_GO_BOTTOM);
  recno_0 = view.recno_0;
  if (!flags.eof) Skip(1);
  view.recno_0 = recno_0;
  return 0;
}

//   Skip records, forward or backwards.
int dbf::Skip(long cant)
{
        //  Performs some validations.
        if(!flags.in_use)
    {
        view.dberr = DB_NO_DBOPEN;
        return !view.dberr;
    }

        //   The processes are different, if the skip is forward or backward.
        if(cant > 0)
        {
                while(cant)
                {
                        //   The process is different depending on whether or not there is an active index.
                        if(!idxmaster)
                                Go(recno + 1);
                        else
                                idxSkip(1, idxmaster);

                        //   If the file is finished, complete.
                        if(flags.eof)
            {
                view.dberr = DB_EOF_ENCOUNT;
                return !view.dberr;
            }

                        //   Ignore deleted records.
                        if(!view._deleted || !Deleted())
                                cant--;
                }
        }
        else
        {
                while(cant && (recno > 1))
                {
                        //   Performs the physical movement of the records.
                        if(!idxmaster)
                                Go(recno - 1);
                        else
                                idxSkip(-1, idxmaster);

                        //   If you arrived at the beginning of the file.
                        if(flags.bof)
            {
                view.dberr = DB_BOF_ENCOUNT;
                return !view.dberr;
            }

                        //   Leave deleted records
                        if(!view._deleted || !Deleted())
                                cant++;
                }
        }

        //   Leave deleted records if the current is deleted
        while(view._deleted && Deleted() && !Eof())
                if(!idxmaster)
                        Go(recno + 1);
                else
                        idxSkip(1, idxmaster);
        return 1;
}

////////////////// Returning control information ////////////////////
//  Returns the length of a field, in bytes, given its name
int dbf::FieldLen(const char *nm)
{
        int i;
        struct dbField *f;
        static char uName[32];

        //   Performs some validations.
        if(!flags.in_use) return 0;
        _strupr(strcpy(uName, nm));

        //   Look for the field that you want to know the length of.
        for(i = cFlds, f=Flds; i; i--, f++)
           if(!strcmp(f->name, uName))
                        return f->FieldLen;
        return 0;
}

//   Returns the decimal digits of a field, in bytes, given its name.
int dbf::FieldDec(const char *nm)
{
        int i;
        struct dbField *f;
        static char uName[32];

        //   Performs some validations.
        if(!flags.in_use) return 0;
        _strupr(strcpy(uName, nm));

        //   Look for the field that you want to know the decimal length.
        for(i = cFlds, f=Flds; i; i--, f++)
           if(!strcmp(f->name, uName))
                        return f->FieldDec;
        return 0;
}

//  Returns a character as the type of a field given its name.
char dbf::FieldType(const char *nm)
{
        int i;
        struct dbField *f;
        static char uName[32];

        //   Performs some validations.
        if(!flags.in_use) return 'U';
        _strupr(strcpy(uName, nm));

        //   Look for the field for which you want to know the type.
        for(i = cFlds, f=Flds; i; i--, f++)
           if(!strcmp(f->name, uName))
                        return f->DataType;
        return 'U';
}

//  Returns the value of the indicator end of file (EOF).
int dbf::Eof()
{
        //   Performs some validations.
        if(flags.in_use && flags.eof) return 1;

        //   If the database has no records, then it is EOF.
        if(dbhd.RecCount == 0) return 1;

        //   If the current record is beyond the reccount, is EOF.
        if(recno > dbhd.RecCount) return 1;

        //   Not at EOF.
        return 0;
}

//   Returns the value of the indicator of the beginning of file (BOF).
int dbf::Bof()
{
        //   Performs some validations.
        if(flags.in_use && flags.bof) return 1;

        //   If the database has no records, then it is BOF.
        if(dbhd.RecCount == 0) return 1;

        //   If recno is less than one, is BOF.
        if(recno <= 0) return 1;

        //   Not at BOF.
        return 0;
}

//  Returns the current number of records in the table.
unsigned long dbf::RecCount()
{
        if(!flags.in_use) return 0l;
        return dbhd.RecCount;
}

//  Returns the record number of the current record in the table.
unsigned long dbf::Recno()
{
        //   Performs some validations.
        if(!flags.in_use) return 0l;

        //   If you are at the end of file, returns the reccount.
        if(flags.eof) return dbhd.RecCount;

        //   Returns the current record number.
        return recno;
}

//   Truncates a database -- poor attempt at pack ?
void dbf::Trunc(unsigned long nTrunc)
{
        unsigned sz = 32;

        //   Validates some data.
        if(!flags.in_use)
        {
                view.dberr = DB_NO_DBOPEN;
                return;
        }

        //   If it is truncated to 0 records in fact it is zapping.
        if(nTrunc == 0)
        {
                Zap();
                return;
        }

        //  Record the changes that are pending.
        flushWrite();
        Lock(DB_UNLOCK);

        //   Blocks header on network.
        if(!flags.excl)
        {
           do
                   fh.xLseek(0l, SEEK_SET);
           while(fh.xLocking(LK_LOCK, sz));
        }

        //   Read the header.
        fh.xLseek(0l, SEEK_SET);
        fh.xRead(&dbhd, sz);

        //   Performs the update of the number of records in the header.
        dbhd.RecCount = nTrunc;

        //   Changes the physical size.
        if(recno > dbhd.RecCount) recno = dbhd.RecCount;
        fh.xChsize(dbhd.FirstDataByte + (dbhd.LenRecord * dbhd.RecCount));

        //   Record the new header and unlock.
        fh.xLseek(0l, SEEK_SET);
        fh.xWrite(&dbhd, sz);
        if(!flags.excl)
        {
                fh.xLseek(0l, SEEK_SET);
                fh.xLocking(LK_UNLCK, sz);
        }

        //   If you have related indices, rebuild them.
        if(idxlist) Reindex();
}

//   Returns the flag of area USED() or not.
int dbf::Opened()
{
        if(flags.in_use || fh.xOpened()) return 1;
        return 0;
}

// Copy header of open file to new file -- equivalent of "Copy Structure to..."
void dbf::CopyStruct(char *newfile)
{
        struct dbField *f;
        unsigned sz = 32;  //both the base header and field records in the header are 32 bytes
        FILE *emptydbf;
        char hdrchars[32];
        int  i, i2;

        //   Validates some flags.
        if(!flags.in_use)
        {
                view.dberr = DB_NO_DBOPEN;
                return;
        }

        //  First, empty the dbf buffer.
        flushWrite();                           // No need, but accommodates flags.
        Lock(DB_UNLOCK);

        //  Blocks header.
        if(!flags.excl)
        {
           do
                   fh.xLseek(0l, SEEK_SET);
           while(fh.xLocking(LK_LOCK, sz));
        }

        //   Read the header.
        fh.xLseek(0l, SEEK_SET);
        fh.xRead(&dbhd, sz);

        emptydbf=fopen(newfile,"w");

        if(emptydbf==NULL)
        {
         view.dberr = DB_COPY_ERROR;
         return;
        }

        //copy base part of header first after resetting first data byte
        memmove(hdrchars, &dbhd.dbID, sz);
        hdrchars[4] = '\0';  //set number of records to 0
        hdrchars[5] = '\0';
        hdrchars[6] = '\0';
        hdrchars[7] = '\0';
        for (i2 = 0; i2 < sz; i2++)
          putc(hdrchars[i2],emptydbf);

        //now copy the field info
        for(i = cFlds, f = Flds; i; i--, f++)
        {
         memmove(hdrchars, f, sz);
         for (i2 = 0; i2 < sz; i2++)
           putc(hdrchars[i2],emptydbf);
        }
          putc('\r' , emptydbf);        //terminator
          for (i = 0; i < 263; i++)   //jas write backlink area
            putc('\0' , emptydbf);

        fclose(emptydbf);

    //  Unlock the header.
        if(!flags.excl)
        {
         fh.xLseek(0l, SEEK_SET);
         fh.xLocking(LK_UNLCK, sz);
        }

        //  Now, let the variables and flags for programs be accessed.
        recno = 1l;
        flags.bof = flags.eof = 1;
        flags.rdy = flags.wri = 0;
        GoRelation();
}

//   Empty file of ALL records.
void dbf::Zap()
{
        struct idx *x;
        unsigned sz = 32;

        //   Validates some flags.
        if(!flags.in_use)
        {
                view.dberr = DB_NO_DBOPEN;
                return;
        }

        //  Primero, vacía la dbf.
        //  First, empty the dbf buffer.
        flushWrite();                           // No need, but accommodates flags.
        Lock(DB_UNLOCK);

        //  Blocks header.
        if(!flags.excl)
        {
           do
                   fh.xLseek(0l, SEEK_SET);
           while(fh.xLocking(LK_LOCK, sz));
        }

        //   Read the header.
        fh.xLseek(0l, SEEK_SET);
        fh.xRead(&dbhd, sz);

        //   Updates the header data and changes the physical size.
        dbhd.RecCount = 0l;
        fh.xChsize(dbhd.FirstDataByte);
        fh.xLseek(0l, SEEK_SET);
        fh.xWrite(&dbhd, sz);
      //for (f = 0; f < 263; f++)   jas write backlink area   actually not needed here as FirstDataByte position stays the same
      //  fh.xWrite("\0", sizeof(char));
        fh.xLseek(dbhd.FirstDataByte, SEEK_SET);
        fh.xWrite("\0", 1);

    //  Unlock the header.
        if(!flags.excl)
        {
                fh.xLseek(0l, SEEK_SET);
                fh.xLocking(LK_UNLCK, sz);
        }

        //  Now, empty the indexes.
        for(x = idxlist; x; x = x->next)
                idxZap(x);

        //  Now, let the variables and flags for programs be accessed.
        recno = 1l;
        flags.bof = flags.eof = 1;
        flags.rdy = flags.wri = 0;
        GoRelation();
}

// Append from a CSV file
int dbf::AppendFromCSV(char *filename, bool headerrow, bool commas_in_text = false)
{
 char tmp[2048], *logical_save;
 struct dbField *f;
 int i;

//   Performs some validations.
    if(!flags.in_use)
    {
      view.dberr = DB_NO_DBOPEN;
      return !view.dberr;
    }

    if (!file_exists(filename))
    {
      view.dberr = CSV_NONEXISTANT;
      return !view.dberr;
    }

    std::string sline1, sline2;
    std::ifstream fs(filename);

     //skip first line as it's a header
    if (headerrow)
      getline(fs, sline1);  //skip first line as it's a header

    while (getline(fs, sline1))
    {
      //set flags for parsing status
      strtok_called = false;
      NULL_not_returned = true;

      //cleanup the line that was read in
      //it's possible that only text fields are delimited with '"'
      //so let's convert all quotes and comma quote combinations to '|'
      rtrim_to(sline1, sline2);
      replaceAll(sline2, "\",\"", "|");
      replaceAll(sline2, "\",", "|");
      replaceAll(sline2, ",\"", "|");
      if (sline2[sline2.size()] == '\"')
          sline2[sline2.size()] = '|';
      if (sline2[0] == '\"')
              sline2 = sline2.substr(1,sline2.size()-1);

      std::strcpy(tmp, sline2.c_str());

      AppendBlank();
      for(i = cFlds, f = Flds; i; i--, f++)
      {
    //  CheckIdx("UPPER(DES)+UPPER(COD)+STR(DBLTST,9,2)");
        switch (f->DataType)
        {
         case 'D':              // character field that's a date field
         case 'C':              // character field
              if (commas_in_text)
          Replace(f->name, getfieldTxt(tmp));
        else
          Replace(f->name, getfieldTxt2(tmp));
              break;
         case 'L':              // character field that's a logical field
              logical_save = getfieldTxt(tmp);
              if (logical_save[0] == '.')
              {
               if (logical_save[1] == 'Y' || logical_save[1] == 'T' ||
                   logical_save[1] == 'y' || logical_save[1] == 't')
                  Replace(f->name, "Y");
               else
                  Replace(f->name, "N");
              }
              else
              {
               if (logical_save[0] == 'Y' || logical_save[0] == 'T' ||
                   logical_save[0] == 'y' || logical_save[0] == 't')
                  Replace(f->name, "Y");
               else
                  Replace(f->name, "N");
              }
              break;
         case 'B':              // Double field
         case 'N':              // Numeric field
              ReplaceD(f->name, getfieldDbl(tmp));
              break;
         case 'I':              // Integer field
              ReplaceI(f->name, getfieldInt(tmp));
              break;

        }
      }
    }

    fs.close();
  return 1;
}


//function to check for presence of file
bool file_exists(const char *fileName)
{
 std::ifstream infile(fileName);
 return infile.good();
}

///////////////////////////////////////////////////////////////////////
//   Check the result of an operation.
//   If it detects an error, it prints an error and stops the program.
void chkE(int eFlag)
{
        if(eFlag) return;
        printf("\n\ndbERROR - '%s'.\n", dberrMSG[view.dberr]);
        exit(1);
}

