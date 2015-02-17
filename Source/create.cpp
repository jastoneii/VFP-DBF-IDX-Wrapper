//
//  Create.cpp
//  Code to Create Table...
//

#include <stdafx.h>

#define FS_DB
#include "dbhdr.h"

//  Code that creates a base cursor. (Clear to close it).
int dbf::creaCursor(char *fn, char *fld)
{
        if(!createTable(fn, fld)) return 0;
        fh.xCursor();
        flags.iscursor = 1;
        return 1;
}

//   Code to create a table. Much of the code is similar to the USE code.
int dbf::createTable(char *fn, char *fld)
{
        static unsigned dbNameLen, i;
        static int addExt, lstCount, sigue, tLen, tDec, DataOffset;
        struct dbField *f, *flist;
        char *pn;
        static char tName[11], tField;
		int ii;

        Close();
        view.dberr = DB_NO_ERROR;

        //   Compile the list of fields.
    flist = (dbField *) malloc(sizeof(dbField) * (MAXFIELDS + 2));
        dbhd.LenRecord = cFlds = lstCount = 0;
        DataOffset = 1;
        for(sigue = 1; sigue; )
        {
                //  Initialize variables.
                tLen = tDec = 0;
                tName[0] = 0;

                //   Set the field name.
                while(isspace(*fld)) fld++;
                pn = fld;
                for(i = 0; *fld && !isspace(*fld); fld++) i++;
                if(i > 10) i = 10;
                memmove(tName, pn, i); 
				for (ii=i; ii <= 10; ii++)
					tName[ii] = 0;
				_strupr(tName);

                //   Set the field type.
                while(isspace(*fld)) fld++;
                switch(*fld)
                {
                        case 'n': case 'N':
                                tField = 'N';
                                break;
                        case 'c': case 'C':
                                tField = 'C';
                                break;
						case 'd': case 'D':
							tField = 'D';
							break;
						case 'i': case 'I':
							tField = 'I';
							break;
						case 'l': case 'L':
                                tField = 'L';
                                break;
                        case 'b': case 'B':
                                tField = 'B';
                                break;
                        default:
                                sigue = 0;
                                continue;
                }

                //  Process the fields inside the parentheses
                fld++; while(isspace(*fld)) fld++;
                if(*fld == '(')
                {
                        //  Set the length of the field.
                        fld++; while(isspace(*fld)) fld++;
                        tLen = atoi(fld);
                        while(isdigit(*fld)) fld++;

                        //  Find the separating comma parameters.
                        while(isspace(*fld)) fld++;
                        if(*fld == ',')
                        {
                                //  set the length of the decimal part.
                                fld++; while(isspace(*fld)) fld++;
                                tDec = atoi(fld);
                                while(isdigit(*fld)) fld++;
                        }

                        //  Get the closing ')'
                        while(isspace(*fld)) fld++;
                        if(*fld != ')') { sigue = 0; continue; }
                        fld++;
                }

                //   Check the generated variables and add the information to the list of created fields.
                // Need to add code to deal with Double field -- jas
                if(tField == 'D') { tLen = 8; tDec = 0; }
				else
				{
					if (tField == 'L') { tLen = 1; tDec = 0; }
					else
						if (tField == 'I') { tLen = 4; tDec = 0; }
				}
				if (tField != 'N' && tField != 'B') { tDec = 0; }
                if(tLen > 255) tLen = 255;
                if(tDec > tLen) tDec = tLen - 1;
                if(tDec < 0) tDec = 0;
                f = (flist + lstCount); lstCount++;
                //strcpy(f->name, tName);
				memmove(f->name, tName, 11);  //needed to clear out name trash for vfp  -- jas
				for (i = 0; i < 14; i++) f->PADD[i] = '\0';
                f->DataType = tField;
                f->FieldLen = (char) tLen;
                f->FieldDec = (char) tDec;
                f->OffsetInRec = DataOffset;
                DataOffset += tLen;
                cFlds++;

                //  See if there are more field definitions.
                while(isspace(*fld)) fld++;
                if(*fld != ',') { sigue = 0; continue; }
                fld++;
        }

        //  Incorporates the field list to the definition of the database.
        if(!cFlds)
        {
        free(flist);
        view.dberr = DB_CREATE_SINTAX;
        return !view.dberr;
        }
    Flds = (dbField *) malloc(sizeof(dbField) * cFlds);
        memmove(Flds, flist, cFlds * sizeof(dbField));
    free(flist);

        //  Sets the file name and creates it.
        dbNameLen = strlen(fn);
    addExt = !strchr(fn, '.');
    if(addExt)
                dbNameLen += 4;
        dbName = (char *) malloc(dbNameLen + 1);
        strcpy(dbName, fn);
        if(addExt) strcat(dbName, ".dbf");
        remove(dbName);
        fh.xCreate(dbName, S_IREAD | S_IWRITE);
        fh.xClose();
        if(fh.xOpen(dbName, O_BINARY | O_RDWR, _SH_DENYRW) < 0)
        {
        free(Flds);
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

        //   Build and write the header.
        //dbhd.dbID = NOMEMO;
        //dbhd.FirstDataByte = (cFlds + 1) * 32;
        dbhd.dbID = VFP_NOMEM;  //jas
        dbhd.FirstDataByte = (cFlds + 1) * 32 + 264;  //jas  263 is VFP backlink area for a .DBC file + 1 for terminator
        dbhd.LenRecord = DataOffset;
        dbhd.RecCount = 0l;
		dbhd.PADD[16] = '\0';  //force to no memo just in case
        fh.xLseek(0l, SEEK_SET);
        fh.xWrite(&dbhd, sizeof(struct dbHeader));
        for(f = Flds, i = 0; i < cFlds; i++)
        {
                fh.xLseek((i + 1) * 32, SEEK_SET);
                fh.xWrite(f++, sizeof(dbField));
        }
		fh.xWrite("\r" , 1);        //terminator
		for (i = 0; i < 263; i++)   //jas write backlink area
          fh.xWrite("\0", 1);

        //   Write the first byte, so that the database is consistent with the header.
        fh.xLseek(dbhd.FirstDataByte, SEEK_SET);
        //fh.xWrite("\r", 1);

        //   Create the area to store copy of the current record.
    uRec = (unsigned char *) malloc(dbhd.LenRecord);
        if(!uRec)
        {
        free(dbName);
        if(uRec) free(uRec);
        if(Flds) free(Flds);
        view.dberr = DB_NO_MEMORY;
        return !view.dberr;
        }

        //   Descarga lo necesario al disco.
        // force to the disk if necessary?
        fh.xFlush();

        //   Initialize some other control variables.
        rel = 0;
        idxlist = idxmaster = 0;
        activeKey = 0;

        //  Finally set flags.
        flags.eof = flags.bof = flags.wri = flags.lock = flags.rdy = 0;
        flags.excl = view._exclusive? 1 : 0;
        flags.wOnce = flags.iscursor = 0;
        flags.in_use = 1;
        recno = 2; dbGo(DB_GO_TOP);
        return 1;
}

