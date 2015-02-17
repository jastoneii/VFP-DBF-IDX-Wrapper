//
//   Db.h
//   Prototipos del motor de acceso a bases de Foxpro.
//

#pragma once

//   Define 'decl', which is used to export / import the symbols.
#ifdef FS_DB
#define decl __declspec(dllexport)
#else
#define decl __declspec(dllimport)
#endif

//   Define the class that handles the i / o files.
class decl xFile
{
public:
        char *lfn;                                                      // Copy of the file name used.
        xFile *next;                                                    // Linked list of open files.

private:
        int h;
        char *chFile;                                                   // Buffer for file caching.
        long chPos;                                                     // Position currently seeked.
        long szMem;                                                     // Length of file.
        int OpenFlags1, OpenFlags2;                                     // To redial the sopen(...)
        struct flags_ {
                int opened:1;                                           // opened File.
                int loaded:1;                                           // If entire file is cached
                int writePend: 1;                                       // If there are pending writes.
                int iscursor: 1;                                        // Clear to close  Borrar al cerrar.
                int reopen:1;                                           // The file must be reopened.
        } flags;

public:
        xFile();                                                        //  Constructor and Destructor.
        ~xFile();

        int  xOpen(const char *, int, int);                     // sopen(...)
        int  xCreate(const char *, int);                        // creat(...)
        int  xClose(void);                                      // close(...)
        int  xRead(void *, int);                                // read(...)
        int  xWrite(void *, int);                               // write(...)
        long xLseek(long, int);                                 // lseek(...)
        int  xLocking(int, long);                               // locking(...)
        long xTell(void);                                       // tell(...)
        int  xChsize(long);                                     // _chsize(...)
        long xFilelen(void);                                    // filelength(...)
        int  xEof(void);                                        // eof(...)
        int  xCache(int);                                       // Scheme to use.
        void xCursor(void);                                     // Flag as cursor.
        void xFlush(void);                                      // Empty buffers.
        int  xOpened(void);                                     // Indicates whether file is open.
        void xReopen(void);                                     // Reopen a file.
        void xTmpClose(void);                                   // Temporarily close a file.
};

// Different types of caches to activate. (If none stated,
// orders are passed directly to the low-level functions).
#define CACHE_LOAD_FILE 1

//  Transaction functions are separate, as applied to all
//  files that are open at the time.
int  xTTStart(void);                                                    // Begin a transaction.
int  xTTCommit(void);                                                   // Confirms TT and downloads to disk.
int  xTTRollback(void);                                                 // Rolls back a transaction.
void xTTBusy(void);                                                     // Busy... waiting

// DBF Headers, as found in the file.
#pragma pack(1)
struct dbHeader {
        char dbID;                                                      // File type identifier
        char LastUpdate[3];                                             // Date of last modification
        unsigned long RecCount;                                         // Number of records in the file
        unsigned short FirstDataByte;                                   // Position of the first byte of first record of data.
        unsigned short LenRecord;                                       // Number of bytes occupied by a record
        char PADD[20];                                                  // Fill out the 32 bytes.
};

//  Structure of a field, as it is in the file.
struct dbField {
        char name[11];                                                          // Field name (max. 10 characters)
        char DataType;                                                          // Definition of the data type.
        long OffsetInRec;                                                       // Position of the field in a record
        char FieldLen;                                                          // Length in bytes of the field
        char FieldDec;                                                          // If applicable, number of decimal places.
        char PADD[32-18];                                                       // Fill out the 32 bytes it occupies in the file.
};

//   Possible parameters for RLock ()
#define         DB_UNLOCK       0
#define         DB_LOCK         1

//   Possible parameters for DBGO ()
#define         DB_GO_TOP           0
#define         DB_GO_BOTTOM  -1l

//   Definitions that make up the field 'dbID' in the 'dbHeader'
#define     NOMEMO      '\x03'                                  // FoxBASE+, dBASE III Plus, Foxpro without MEMO
#define     VFP_NOMEM   '\x30'                                  // VFP without MEMO -- jas
#define     MEM_DB3     '\x83'                                  // FoxBASE+, dBASE III Plus with MEMO
#define     MEM_FXP     '\xf5'                                  // FoxPRO with MEMO
#define     MEM_DB4     '\x8b'                                  // dBASE IV with MEMO

//   Definitions that make up the field 'DataType' of a 'dbField'
#define     CHAR        'C'                                             //  Characters in general
#define     NUM         'N'                                             //  Numbers in general (ascii)
#define     FLOAT       'F'                                             //  Numbers in general (double)
#define     LOGIC       'L'                                             //  Logical (.t. / .f.)
#define     MEMO        'M'                                             //  Memos
#define     GENER       'G'                                             //  Windows / OLE-DDE
#define     DATE        'D'                                             //  Date: yyyymmdd
#define     PICTURE     'P'                                             //  Windows / BMP-PIC-...
#define     DOUBLE      'B'                                             //  numbers in double form added by jas
#define     INTEGER     'I'                                             //  numbers in integer form added by jas

//   List of allowed errors.

extern char *dberrMSG[];

#define DB_NO_ERROR         0
#define DB_FILE_NOT_FOUND   1
#define DB_NOT_DB           2
#define DB_FILE_EMPTY       3
#define DB_NO_MEMORY        4
#define DB_NO_DBOPEN        5
#define DB_BAD_RECNUM       6
#define DB_REC_LOCKED       7
#define DB_FLD_NOTFOUND     8
#define DB_REC_NO_LOCKED    9
#define DB_FILE_EXCLU      10
#define DB_EOF_ENCOUNT     11
#define DB_BOF_ENCOUNT     12
#define DB_BAD_IDXKEY      13
#define DB_IDX_EXCLU       14
#define DB_IDX_NOTFOUND    15
#define DB_CREATE_SINTAX   16
#define DB_COPY_ERROR      17
#define CSV_NONEXISTANT    18

//Structure containing all environment variables
//that depends on the current view.
class decl view_
{
public:
        int _exclusive;                   // Flag exclusive access.
        int dberr;                        // Number of last error.
        int _deleted;                     // Deletion -- missed or not    Saltea o no los deleteados.
        unsigned long recno_0;            // Result of last seek.

        //   Constructor. Initializes the control variables.
        view_()
        {
                dberr = 0;
                _deleted = 1;
                _exclusive = 1;
        }
};

//   The form in which structures are stored in the .idx files
struct idxHeader {
           unsigned long  RootNode;                             //  Pointer to the root node
           unsigned long  FreeNode;                             //  Pointer to the list of free nodes
           unsigned long  EofNode;                              //  Pointer to end of file
           unsigned short LenKey;                               //  Key Length  Largo de la clave
           char idxFlag;                                        //  Mode flags -- Banderas de modo
           char signature;                                      //  Identification
           char key[220];                                       //  key expression
           char forc[220];                                      //  expression filter
};

struct idxNodeHd                                                //  The reindex used.
{
           short attrib;                                        //  Attribute node.
           short nkey;                                          //  Number of keys.
           long  nleft;                                         //  Node directly to the left
           long  nright;                                        //  Directly to the right node
};

struct idxNode
{
           short attrib;                                        //  Node attribute
           short nkey;                                          //  Number of keys in the node
           long nleft;                                          //  Node directly to the left
           long nright;                                         //  Node directly to the right
           char data[1000];                                     //  Content of up to 2 nodes
};

#define     IDH_UNIQUE      1                           //  Does not allow repeated keys
#define     IDH_FILTER      8                           //  Use the clause FOR

#define     ATT_ROOT        1                           //  The current node is the root
#define     ATT_LEAF        2                           //  The current node is extreme -- end? //El nodo actual es extremo

struct idxoperations            //to process an index comprised of more than 1 field, etc
{
 int  numflds;
 char fldname[44];       //allow for up to four 10 character field names with a null terminator
 int  fldidxlen[4];
 long  fldoffset[4];
 long  fldkeyoffset[4];
 int  operation1[4];
 int  stringarg1[4];
 int  stringarg2[4];
};

#define OP_PLAIN     0
#define OP_UPPER     1
#define OP_DTOS      2
#define OP_STRING    3

//   Definition of the class that handles the file index.
struct idx
{
           struct   idxHeader hd;                               //  Header
           struct   idxNode   node;                             //  current node
           char     *idName;                                    //  File name.
           xFile    fh;                                         //  Handle to the physical file.
           char     keyType;                                    //  Type of the keyword (Not in idx)
           char     *tempKey;                                   //  Location of the working key    Lugar para la clave de trabajo
           char     *reKey;                                     //  Location of the reseeked key   Lugar para la clave de reseek.
           long     NodeTree[50];                               //  Tree nodes used.
           unsigned NodeTreeCount;                              //  Counter nodes used.
           unsigned long lFile;                                 //  To detect corruptions
           char     *KeyFound;                                  //  Where reseek returns a pointer.
           struct   idx *next;                                  //  Next IDX on the list     Siguiente idx en la lista
           bool     complex_idx;                                //  indicate complex index requiring more than a single field, etc
           struct   idxoperations  idx_ops;
           //void     (*idx_Functn)(char * ETIkey, dbHeader *xdbhd);//  Index key construction routine if needed, else NULL
};

//   Structure to store relations.
struct dbRelation {
        class dbf *target;                                              // Base relation
        char *field;                                                    // Related field.
        dbRelation *next;                                               // Following relationship.
};

//   Defining the class that manages a database file.
class decl dbf
{
private:
        struct dbHeader dbhd;                                           // Header read.
        char *dbName;                                                   // File name.
        xFile fh;                                                       // To handle the physical file.
        struct dbField *Flds;                                           // Field List.
        unsigned char *uRec;                                            // Current Record.
        unsigned short cFlds;                                           // Number of fields.
        unsigned long recno;                                            // Current Record position.
        dbRelation *rel;                                                // Active Relations.

        struct flags_ {                                                 // Flags of the base state.
                int in_use: 1;                                          // The base is in use.
                int eof: 1;                                             // It is positioned beyond the end.
                int wri: 1;                                             // There are pending writes.
                int bof: 1;                                             // Is before the beginning.
                int excl: 1;                                            // It opened exclusively.
                int rdy: 1;                                             // The record read is NOT in buffer.
                int lock: 1;                                            // The current record is locked.
                int wOnce: 1;                                           // Registration is automatic TT.
                int iscursor: 1;                                        // Clear to closing.
                int wCdx: 1;                                            // Have attached structural CDX.
        } flags;

        struct idx *idxlist;                                            // List of open indices.
        struct idx *idxmaster;                                          // Index controller (NULL = none)
        char *activeKey;                                                // Key of the controlling index.

        //  Funciones privadas.
        void flushWrite(void);                                          // Empty pending writes.
        void closeIndex(void);                                          // Closes active indices.
        unsigned long idxSearch(struct idx *, char *);
        void DeleteKey(unsigned long, struct idx *);
        void InsertKey(void *, char *, struct idx *);
        void idxSkip(int, struct idx *);
        int  idxGo(int, struct idx *);
        int  dbGo(unsigned long nrec);
        int  idxReseek(struct idx *);
        void idxZap(struct idx *);
        void getActualRecord(void);
        void CheckIndexUpdate(const char *, char *, char *, int);
        void InsertBlankKey(void);
        void GoRelation(void);
        void DefaultTally(int, const char *);
        void lReindex(struct idx *,                                     // Rebuilds an index.
                  void (*)(unsigned long));
        int parse_key_string(char *idxcmmd, idxoperations * idxop);
        void cnvrt_numfield_to_keystring(long offset, int fldlen, char * keystr, int strlen, int numdecs, char *newvalue);
        void cnvrt_charfield_to_keystring(long offset, int fldlen, char * keystr, char *newvalue);
		void build_complex_key(char *constructed_key, idxoperations * idxop, int fldidxnum, char *newvalue);

public:
        dbf(void);                                              // Constructor.
        ~dbf(void);                                             // Destruye la estructura.

        int  Use(const char *fn);                               // Open the file.
        void Close(void);                                       // Close the file.
        int  Go(unsigned long rec);                             // Go to given record.
        int  Lock(int acc);                                     // Unlock / Lock the current record.
        int  Get(const char *fld, char *buf);                   // Extracts data from a character, date, numeric or logical field.
        int  GetD(const char *fld, double *dblvar);             // Extracts data from a double field.
        int  GetI(const char *fld, long *intvar);               // Extracts data from a dbf Integer field - 4 bytes
        int  xGet(const char *fld);                             // Extract data and place in xstring.
        int  FieldLen(const char *fld);                         // Returns the length in bytes of 1 fld.
        int  FieldDec(const char *fld);                         // Returns the value of the decimal part.
        char FieldType(const char *fld);                        // Returns the type of a field.
        int  Replace(const char *fld, char *exp);               // Replaces data in a character, date, numeric or logical field.
        int  ReplaceD(const char *fld, double dblvar);          // Replaces data in a double field.
        int  ReplaceI(const char *fld, long intvar);            // Replaces data in an integer field -- really a long for safety 4 bytes
        int  AppendBlank(void);                                 // Add a blank record.
        void CopyStruct(char *newfile);                         // Copy Structure to new file
        void Zap(void);                                         // ZAP all records
        void SetRelation(dbf *trgt, char *exp);                 // Establishes a relationship.
        void ClearRelation(dbf *target);                        // Clears a relationship.

        int  Bof(void);                                         // Returns the flag BOF.
        int  Eof(void);                                         // Returns the EOF flag.
        unsigned long RecCount(void);                           // Returns the # of records.
        unsigned long Recno(void);                              // Returns the current record #.
        int  Deleted(void);                                     // TRUE if the record is deleted.
        int  Skip(long offset);                                 // Skip records.
        int  Seek(char *key);                                   // Search index for character string
		int  SeekD(double dbl);                                 // Search index for double
        void Setorder(char *key);                               // Change active index.
        int  SetIndex(char *fn);                                // Opens and associates an index.
        int  Locate(char *fld, char *exo);                      // Sequential Search

        int  IndexOn(char *key, char *fn);                      // Create an index and make active.
        void Reindex(void);                                     // Rebuild all indexes.
        int  createTable(char *fn, char *fld);                  // Create a database.
        int  creaCursor(char *fn, char *fld);                   // Creates a base cursor.
        void AppendFrom(char *filename);                        // Appends from one database to another
        int  AppendFromCSV(char *filename, bool headerrow, bool commas_in_text);     // Appends from a CSV file into the database
        void Copy1To(char *filename);                           // Copies current record to another file
        void dbf::Pack();                                       // Pack the current file to remove records marked as deleted
        void Flush(void);                                       // Force write of data to disk.
        void Delete(void);                                      // Deletes the current record.
        void Trunc(unsigned long rec);                          // Truncates the current file based on remaining records specified by 'rec'
        int  Opened(void);                                      // Indicate if file is open.
		int  CheckIdx(const char * field);
};

extern char pbuff[];                                            // Defined in db.cpp
extern view_ view;                                              // The states db.cpp
void testDb(void);
void chkE(int);                                                 // Check the answer.
#pragma pack()

//   Other global functions.
void Syncronize(int);                                           // Synchronize files.
int  xCopyFile(char *, char *);                                 // Copy a file to another.
void TemporaryClose(char *fn);                                  // Ensures that fn is closed.      o.
void replaceAll(std::string& str, const std::string& from, const std::string& to);
void perform_index_operations_on_field(int operation1, int operation2, char *keystr);
bool file_exists(const char *fileName);
char*  getfieldTxt(char* line);   //treats commas as part of string field
char*  getfieldTxt2(char* line);  //treats commas as field delimiter
char*  getfieldNum(char* line);
double getfieldDbl(char* line);
long   getfieldInt(char* line);
void   rtrim_to(std::string& in, std::string& out);
void   rtrim_to(char *str_in, char *str_out);
//static inline std::string &ltrim(std::string &s);
//static inline std::string &rtrim(std::string &s);
//static inline std::string &trim(std::string &s);
void   double_to_char(char *charstr, double xnum, int len, int decs);
//void   double_to_str(std::string& strx, double xnum, int len, int decs);

//   Constants variables
#define SZ_NODE         512                                     /* Node size in bytes */
#define MAXFIELDS   150                                         /* Max # of database fields*/
