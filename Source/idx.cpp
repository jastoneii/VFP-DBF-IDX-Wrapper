//  Idx.cpp
//  Implementing the use of indices .idx
//

#include "stdafx.h"

#define FS_DB
#include "dbhdr.h"

void build_complex_key(char *constructed_key, idxoperations idxop, int fldidxnum, char *newvalue);

//   Local variables and error messages.
char pbuff[500], tkey[500], lastK[500], newKey[500], outKey[500];

// Read a node.
void inline loadNode(struct idx *x, unsigned long posi)
{
        x->fh.xLseek(posi, SEEK_SET);
        x->fh.xRead(&x->node, SZ_NODE);
}

// Write a node. In order to be able to analyze and debug the program,
// write bytes 'EE' in the data space that is not occupied.
// #define WRITEEE
void inline writeNode(struct idx *x, long posi)
{
        #ifdef WRITEEE  // OJO - Only func with the test file
        memset(x->node.data + (x->node.nkey * 10), '\xEE',
                   500 - (x->node.nkey * 10));
        #endif
        x->fh.xLseek(posi, SEEK_SET);
        x->fh.xWrite(&x->node, SZ_NODE);        // OJO - Struct node has more than SZ_NODE
}

//   De acuerdo con las especificaciones, lee de los nodos un puntero
//   'intraindex', en caso que no sea nodo extremo, o el nro de record de
//   la dbf, en caso que sea nodo extremo.
//   According to the specifications, read node pointer
//   'intraindex', in case node is not at the end, or the # of the dbf record if it is the end
unsigned long gethex(char *p)
{
        unsigned long res;
        register int i;

        for(res = 0l, i = 4; i; i--)
                res = (res * 256) + (((unsigned long) *p++) & 0xffl);
        return res;
}

// Same as above, but in reverse (long)--->(hexa)
char *sethex(unsigned long value)
{
        static char vl[5];
        char *p1, *p2;
        int i;

        for(p1 = &vl[3], p2 = (char *) &value, i = 4; i; i--)
                *p1-- = *p2++;
        return vl;
}

// Homologous key. This is passed in ASCII. The type of the same
// type follows the key index. The approved key is placed in the
// space reserved for it in the index structure.
void homologa(char *k, struct idx *id)
{
         char *tb, *p, *p1;
         double nro;
         int i;

         tb = id->tempKey;              // temporary key
         switch(id->keyType)
         {
                  case 'C':                 // Characters are ready
                           for(i = id->hd.LenKey; i; i--)
                                   if(*k)
                                                *tb++ = *k++;
                                         else
                                                *tb++ = ' ';
						   return;
                           break;

                  case 'N':                 // Numero (double float)
                          nro = atof(k);
						  break;
				  case 'B':
					  memmove(&nro, k, 8);
					  break;
				  case 'I':
					  memmove(&i, k, 8);
					  nro = (double) i;
					  break;

         }

		 for (p1 = tb, p = ((char *)&nro) + 7; p >= ((char *)&nro);)
			 *p1++ = *p--;
		 if (nro < 0)
		 {
			 for (p = tb + 7; p >= tb; p--)   // for (p = tb + 7; p >= tb; p++) orig
				 *p = (char)(~*p);
		 }
		 else
		 {
			 tb[0] ^= '\x80';
		 }

         return;
}

//   The following function goes to the TOP / BOTTOM of an index, positioning
//   the recno of the master database.
int dbf::idxGo(int offset, struct idx *x)
{
        int lk;

        loadNode(x, x->hd.RootNode);
        lk = x->hd.LenKey;
        if(offset == DB_GO_TOP)
        {
                while( !(x->node.attrib & ATT_LEAF))
                        loadNode(x, gethex(x->node.data + lk));
                return dbGo(gethex(x->node.data + lk));
        }

        // So it's DB_GO_BOTTOM.
        while( !(x->node.attrib & ATT_LEAF))
                loadNode(x, gethex(x->node.data + (lk+4) * (x->node.nkey-1) + lk));
        return dbGo(gethex(x->node.data + (lk+4) * (x->node.nkey-1) + lk));
}

//   The following function empties the index.
void dbf::idxZap(struct idx *x)
{
        x->node.attrib = ATT_LEAF | ATT_ROOT;
        x->node.nkey = 0;
        x->node.nleft = x->node.nright = -1l;
        memset(x->node.data, 0, 500);

        x->hd.RootNode = 512l;
        x->hd.EofNode = 1024l;
        x->hd.FreeNode = -1l;
        x->hd.signature = 1;

        x->fh.xChsize(1024l);
        x->fh.xLseek(0l, SEEK_SET);
        x->fh.xWrite(&x->hd, SZ_NODE);
        x->fh.xWrite(&x->node, SZ_NODE);

        x->lFile = 1024l;
        x->NodeTreeCount = 0l;
}

// The following function adds a blank key in the indexes for
// each 'AppendBlank' that takes place at the base.
// Complex keys may require 0 for numeric fields
void dbf::InsertBlankKey()
{
        struct idx *x;
        char urecC[6], uKey[3], keystr[50];

        memmove(urecC, sethex(recno), 4);
        for(x = idxlist; x; x = x->next)
        {
          if (x->complex_idx)
          {
            build_complex_key(keystr, &x->idx_ops, -1, NULL);
           idxSearch(x, keystr);
           InsertKey(urecC, x->tempKey, x);
          }
          else
          {
             strcpy(uKey, "");
             idxSearch(x, uKey);
             InsertKey(urecC, x->tempKey, x);
          }
        }
}

//   The following function is called when a dbf field is being "replaced".
//   Check if the change must update an index and, if applicable,
//   make the modification.
void dbf::CheckIndexUpdate(const char *field, char *previo, char *nuevo, int len)
{
        struct idx *x;
        char urecC[6], nFld[100], keystr[50];
        int i;

        if(!len || !memcmp(previo, nuevo, len)) return;  //if field is unchanged or has 0 length return as index unaffected

        memmove(urecC, sethex(recno), 4);  //What exactly is sethex(recno) doing?????jas

        for(x = idxlist; x; x = x->next)
        {
          //do we need to construct a complex key
          if (x->complex_idx)
          {
         //  nFld = current key   -- how do we do this???

           for (i = 0; i < 4; i++)
           {
             if (strcmp(field, &x->idx_ops.fldname[i*11]) == 0)
               break;
           }
           if (i == 4) return;


           build_complex_key(keystr, &x->idx_ops, i, nuevo);
           idxReseek(x);                   // Reclose the registry.
           DeleteKey(recno, x);            // Delete the old key.
           idxSearch(x, keystr);            // Locate the new key.
           InsertKey(urecC, keystr, x);     // inserts in the index.
          }
          else
          {
               //  Some process on the key. It will change with the evaluator.
                if(!memcmp(x->hd.key, "UPPER(", 6))
                {
                        strcpy(nFld, x->hd.key + 6);
                        nFld[strlen(nFld) - 1] = 0;
                }
                else
                        strcpy(nFld, x->hd.key);

                //   Search index which is updatable.
                if(!strcmp(field, nFld))
                {
                        idxReseek(x);                   // Reclose the registry.
                        DeleteKey(recno, x);            // Delete the old key.
                        idxSearch(x, nuevo);            // Locate the new key.
						homologa(nuevo, x);             //added by jas
                        InsertKey(urecC, x->tempKey, x);// inserts in the index. --previously nuevo instead of tempKey
                }
          }
        }
}

//   The following function 'reseeks' the key corresponding to the current
//   record. It is used by the update function indexes, skip,
//   etc.
int dbf::idxReseek(struct idx *x)
{
        //static long xrec, anode;
        //static int cnt, lk, st, uUpper;
		long xrec, anode;
		int cnt, lk, st, uUpper, cnt2;
		double viewnum;

		char *n, *n2;
        static char uKey[30];

        //put some data into local variables for performance.
        lk = x->hd.LenKey;
        uUpper = 0;

        if (x->complex_idx)
        {
         build_complex_key(x->reKey, &x->idx_ops, -1, NULL);

        }
        else
        {
         // First, 'reseeks' index to point to the current record.
         if(!memcmp(x->hd.key, "UPPER(", 6))
         {
                 strcpy(uKey, x->hd.key + 6);
                 uKey[strlen(uKey) - 1] = 0;
                 uUpper = 1;
         }
         else
                 strcpy(uKey, x->hd.key);

         Get(uKey, x->reKey);
         if(uUpper) strupr(x->reKey);
        }

        xrec = recno;
        if(!idxSearch(x, x->reKey))
        {
                x->KeyFound = x->node.data;
                return 0;
        }

        // The key pointed to by the result of 'idxsearch' is not
        // necessarily the one we seek, if the index has duplicate keys.
        // Remember that, in this case, it returns 'IdxSearch' the FIRST found.
        // The key that concerns us even can be in any node on the right.
		if (x->keyType == 'C')
          memmove(x->reKey + lk, sethex(xrec), 4); //if not char index lk won't contain correct value

        // Now lift the node where the key was seeked.
		//but first check type in case key needs to be to numeric value for non-char indices
		if (x->keyType == 'C')
		{
			anode = x->NodeTree[x->NodeTreeCount - 1];
			do
			{
				loadNode(x, anode);
				anode = x->node.nright;
				for (cnt = x->node.nkey, n = x->node.data;
					cnt > 0 && ((st = memcmp(x->reKey, n, lk + 4)) > 0);
					n += 4 + lk, cnt--)
					;
			} while (st && (anode != -1l));
		}
		else
		{
			homologa(x->reKey, x);
			memcpy(x->tempKey + lk, sethex(xrec), 4);
			anode = x->NodeTree[x->NodeTreeCount - 1];
			do
			{
				loadNode(x, anode);
				anode = x->node.nright;
				for (cnt = x->node.nkey, n = x->node.data;
					cnt > 0 && ((st = memcmp(x->tempKey, n, lk + 4)) > 0);
					n += 4 + lk, cnt--)
					;
			} while (st && (anode != -1l));

			////temporary viewing code - remove after testing
			//memcpy(&viewnum, x->tempKey, lk);
			//for (cnt2 = x->node.nkey, n2 = x->node.data;
			//	cnt2 > 0 && ((st = memcmp(x->tempKey, n, lk + 4)) != 0);
			//	n2 += 4 + lk, cnt2--)
			//	memcpy(&viewnum, n2, lk);

		}
        x->KeyFound = n;
        return cnt;
}

//  The next function, from the current position of the index
//  seeks the next / previous key immediately.
void dbf::idxSkip(int offset, struct idx *x)
{
        int lk, cnt;
        char *n;

        if(offset == 0) return;

        // Initializes some quick control variables
        lk = x->hd.LenKey;

        // Aim for the current record.
        cnt = idxReseek(x);

        //  Advance a record.
        n = x->KeyFound;
        if(offset > 0)
        {
                if(cnt > 1)                     // Are keys in the node !!!
                        n += (lk + 4);
                else
                        if(x->node.nright != -1l)
                        {
                                loadNode(x, x->node.nright);
                                n = x->node.data;
                        }
                        else
                        {
                                dbGo(DB_GO_BOTTOM);
                                flags.eof = 1;
                                return;
                        }

                // 'n' points to the next key to be used.
                dbGo(gethex(n + lk));
                return;
        }

        // Back one place.
        if(n > x->node.data)    // Are keys in the node !!!
                n -= (lk + 4);
        else
                if(x->node.nleft != -1l)
                {
                        loadNode(x, x->node.nleft);
                        n = x->node.data + (x->node.nkey - 1) * (lk + 4);
                }
                else
                {
                        dbGo(DB_GO_TOP);
                        flags.bof = 1;
                        return;
                }

        // 'n' points to the next key to be used.
        dbGo(gethex(n + lk));
        return;
}

//   The following routine BROWSEs through the tree of the index for the key
//   that is passed as a parameter.  TYPE of key, arising from the evaluation
//   of the key expression.  The key that is passed to the search, is placed
//   in the format STANDARD storage key index.  By necessity the functions
//   that add and delete keys on a index, as the search is carried out are
//   going keeping track of respondent nodes.  For this, each index structure
//   has reserved places for up to 100 nodes.  If this fails, it gives
//   mistake because it most likely is a total destruction of the index.
//
//   Returns:   0 - If the search was unsuccessful.
//          recno - If the search was successful.
//
unsigned long dbf::idxSearch(struct idx *id, char *skey)
{
     char *n, *u;
         //static int lk, st, cnt;
         //static unsigned long nxt;
		  int lk, st, cnt;
		  unsigned long nxt;

         homologa((char *) skey, id);
         u = id->tempKey;
         id->NodeTree[0] = id->hd.RootNode;
         id->NodeTreeCount = 1;
         loadNode(id, id->hd.RootNode);
         lk = id->hd.LenKey;
         for( ;; )
         {
                if(id->node.attrib > 3)         // Corrupcion Interna.
                        return 0l;
                for(cnt = id->node.nkey, n = id->node.data;
                        cnt>0 && ((st = memcmp(u, n, lk)) > 0);
            n += 4 + lk, cnt--)
            ;           // NADA!!

                // cnt == 0 indicates that the key being sought is larger
                // than any key in the index. Also, it should be determined
                // detect this situation only in the root node. Since
                // results of 'seek' are then used to insert
                // and delete keys, then, in this case, walk to the node to
                // extreme right.
                if(!cnt &&  (id->node.attrib & ATT_ROOT))
                {
                        while( !(id->node.attrib & ATT_LEAF))
                        {
                            n = id->node.data + (id->node.nkey - 1) * (lk + 4);
                                nxt = id->NodeTree[id->NodeTreeCount++] = gethex(n + lk);
                                loadNode(id, nxt);
                        }
                        n = id->node.data + (id->node.nkey - 1) * (lk + 4);
                        view.recno_0 = gethex(n + lk);
                        return 0l;
                }

                //  Si llegó a un nodo extremo, toma el recno_0 y retorna.
                //If it came to an end node, and return recno_0 .
                if(id->node.attrib & ATT_LEAF)
                {
                        view.recno_0 = gethex(n + lk);
                        return !st? view.recno_0 : 0l;
                }
                nxt = id->NodeTree[id->NodeTreeCount++] = gethex(n + lk);
                if(nxt + 512l > id->lFile)         // Corrupcion detectada
                {
                        id->NodeTreeCount--;
                        return 0;
                }
                loadNode(id, nxt);
         }
         return 0l;
}

//  Close all related index list. ONLY may be used internally.
void dbf::closeIndex()
{
        struct idx *i, *nxt;

        if(!flags.in_use) return;
        for(i = idxlist; i; i = nxt)
        {
                i->fh.xClose();
                nxt = i->next;
                free(i->tempKey);
                free(i->reKey);
                free(i->idName);
                free(i);
                i = 0;
        }
        idxlist = idxmaster = 0;
}

//  Open an index and associate it to the db in use.
int dbf::SetIndex(char *fn)
{
        unsigned idNameLen;
        int addExt;
        struct idx *id, *x;

        if(!flags.in_use)
    {
        view.dberr = DB_NO_DBOPEN;
        return !view.dberr;
    }

        //  The question is, in the malloc below, is XFILE Built?
        id = new(idx);
    if(!id)
    {
        view.dberr = DB_NO_MEMORY;
        return !view.dberr;
    }
        memset(id, 0, sizeof(idx));

        idNameLen = strlen(fn);
    addExt = !strchr(fn, '.');
    if(addExt)
                idNameLen += 4;
        id->idName = (char *) malloc(idNameLen + 1);
        strcpy(id->idName, fn);
        if(addExt) strcat(id->idName, ".idx");
        if(id->fh.xOpen(id->idName, O_BINARY | O_RDWR,
                                 view._exclusive? _SH_DENYRW: _SH_DENYNO) <0)
        {
                free(id->idName);
                free(id);
                if(errno == EACCES)
        {
            view.dberr = DB_IDX_EXCLU;
            return !view.dberr;
        }
        else
        {
            view.dberr = DB_IDX_NOTFOUND;
            return !view.dberr;
        }
        }

        //   Read the description of the index.
        id->fh.xRead(&id->hd, sizeof(struct idxHeader));

        //   Evalúa la clave para obtener su tipo.
        //   Evaluates the key to its kind.
        if(!memcmp(id->hd.key, "UPPER(", 6))
                id->keyType = 'C';
        else
        {
                id->keyType = FieldType(id->hd.key);
                if(id->keyType == 'U')
                {
                        id->fh.xClose();
                        free(id->idName);
                        free(id);
                        return DB_BAD_IDXKEY;
                }
        }

        //  Sets the temporary key.
        id->tempKey = (char *) malloc(id->hd.LenKey + 6);
        id->reKey = (char *) malloc(id->hd.LenKey + 6);
        if(!id->tempKey || !id->reKey)
        {
                if(id->tempKey) free(id->tempKey);
                if(id->reKey) free(id->reKey);
                id->fh.xClose();
                free(id->idName);
                free(id);
                return DB_NO_MEMORY;
        }
        id->lFile = id->fh.xFilelen();

        //   Hook the new index in to the list of active indices.
        id->next = NULL;
        if(!idxlist)
                idxlist = id;
        else
        {
                for(x = idxlist; x && x->next; x = x->next)
                        ;               // Nada!!
                x->next = id;
        }
        idxmaster = id;
        activeKey = id->hd.key;
        if(flags.iscursor) id->fh.xCursor();
        return 1;
}

//   Enables an index as a new driver.
void dbf::Setorder(char *nwTag_)
{
        idx *x;
        static char nwTag[20];

        strupr(strcpy(nwTag, nwTag_));
        if(activeKey && !strcmp(activeKey, nwTag)) return;
        for(x = idxlist; x; x = x->next)
                if(!strcmp(strupr(x->hd.key), nwTag))
                {
                        idxmaster = x;
                        activeKey = strupr(x->hd.key);
                        return;
                }
        idxmaster = 0;
        activeKey = 0;
}

//   Make an empty index node.
unsigned long getFreeNode(struct idx *id)
{
        unsigned long npos;

        npos = id->hd.EofNode;
        id->hd.EofNode += SZ_NODE;
        id->lFile += SZ_NODE;
        id->fh.xLseek(0l, SEEK_SET);
        id->fh.xWrite(&id->hd, sizeof(struct idxHeader));
        return npos;
}

//   The following function INSERTS a key into the index indicated ..
//   OJO - The sequence of nodes where the key is inserted, is ALREADY IN
//   THE ARRAY NodeTree. Parameter 'rec' contains the value to             a
//   add the key. 'key' contains the key.

void dbf::InsertKey(void *rec, char *key, struct idx *id)
{
        short maxKeyInNode;
        int lk, cnt, st, expTree, emptyNode;
        char *n, tRec[6];
        long npos, pRight, anode;
        bool matching;

        if(!id->NodeTreeCount) return;
        lk = id->hd.LenKey;
        maxKeyInNode = 500 / (lk + 4);
        anode = id->NodeTree[id->NodeTreeCount - 1];
        loadNode(id, anode);

        //jas -- first do quick check to make sure that if new key equals last key in current node but record number is higher
        //       then we need to check the next node to the right to see if there are matching keys there with lower recno's
        matching = true;
        while (matching)
        {
         if ((st = memcmp(key, id->node.data + (lk + 4) * (id->node.nkey-1), lk)) == 0 && id->node.nright > 0)
         {
          npos = id->node.nright;
          loadNode(id, npos);
          if ((st = memcmp(key, id->node.data, lk + 4)) < 0)  //compare key and recno
          {
           loadNode(id, anode); //return to previous node as no records in right node have same key and lower recno
           matching = false;
          }
		  else
		  {
			  anode = npos;
			  id->NodeTree[id->NodeTreeCount - 1] = npos;
		  }
         }
         else
          matching = false;
        }
        //end jas additions

        //   Preserves the last key, because if you change, you must alter
        //   the structure of the tree.
        if(id->node.nkey)
        {
                if(id->node.nkey > maxKeyInNode)    //perhaps this condition is not possible here??
                        emptyNode = 1;
                //else  //should there be an else here?? not in orig code jas
                emptyNode = 0;
                memmove(lastK, id->node.data + ((lk + 4) * (id->node.nkey-1)),
                            lk + 4);
        }
        else
                emptyNode = 1;

        //  Prepara la clave a usar. (La clave se compone de la clave en si
        //  misma y del recno del registro.
        //  El siguiente 'if' es porque InsertKey se llama recursivamente
        //  y en ese caso, el parámetro key es tkey, por lo que evita un
        //  movimiento innecesario.
        //  Prepare to use the key. (The key consists of the key itself and the
        //  recno of the file. The following 'if' is because InsertKey is called
        //  recursively and in that case, the key parameter is tkey, thereby
        //  preventing unnecessary movement.
        if(tkey != key)
                memmove(tkey, key, lk);
        memmove(tkey + lk, (char *) rec, 4);
        for(cnt = id->node.nkey, n = id->node.data;
                cnt>0 && ((st = memcmp(tkey, n, lk + 4)) > 0);
                n += 4 + lk, cnt--)
                ;                               // Nada

        //   Move the necessary keys, insert the new one.
        //   OJO - The inserted key may be equal to the last
        //   node key. Should be handled ok...
		if (cnt)
		{
			if (cnt == maxKeyInNode)
				cnt = cnt;  //for testing jas
			memmove(n + lk + 4, n, (lk + 4) * cnt);
		}

        memmove(n, tkey, lk + 4);               // Insert the key
        expTree = 0;
        if(id->node.nkey + 1 > maxKeyInNode)
        {
                expTree++;
                memmove(outKey, id->node.data + (lk + 4) * id->node.nkey,
                        lk + 4);
        }
        else
                id->node.nkey++;
    writeNode(id, anode);

        //  If you changed the value of the last key of the node, then
        //  'walking' up the indices updates nodes.
        //  This way processing in the previous block, id-> node.nkey
        //  ALWAYS points to the last available key in the current node.

        if(!emptyNode && memcmp(lastK, id->node.data + ((lk+4)*(id->node.nkey-1)), lk))
        {
                int treeSP;

                memmove(newKey, id->node.data+((lk + 4) * (id->node.nkey - 1)),lk);
                for(treeSP = id->NodeTreeCount-2; treeSP >= 0; treeSP--)
                {
                        memmove(lastK + lk, sethex(id->NodeTree[treeSP + 1]), 4);
                        loadNode(id, id->NodeTree[treeSP]);
                        for(cnt = id->node.nkey, n = id->node.data;
                                cnt > 0 && ((st = memcmp(lastK, n, lk + 4)) > 0);
                                n += 4 + lk, cnt--)
                                ;                               // Nada
                        if(!st)
                        {
                                memmove(n, newKey, lk);
                    writeNode(id, id->NodeTree[treeSP]);
                                if(cnt > 1) break;
                        }
                        else
                                break;  // found none. Error?
                }
        }

        // Try to insert the key to be 'dropped' as first key node that is
        // immediately to the right, if there is any.  If successful, expTree off
        // so you do not create a new node.  This maintains balanced nodes as much
        // as possible.
        if(expTree)
        {
                loadNode(id, anode);
                npos = id->node.nright;
                if(npos > 0)
                {
                        loadNode(id, npos);
                        if(id->node.nkey < maxKeyInNode)
                        {
                                if(id->node.nkey)
                                        memmove(id->node.data + lk + 4, id->node.data,
                                                        id->node.nkey * (lk + 4));
                                memmove(id->node.data, outKey, lk + 4);
                                id->node.nkey++;
                                writeNode(id, npos);
                                expTree = 0;
                        }
                }
        }

        // Checks if the node that 'dropped' a key, was the
        // root node. If so, this is special treatment
        // as the old root node (but the key is 'dropped') is
        // in two, these two nodes becomes nodes 'intraindex' or
        // 'leaf', as appropriate, and a new node is created that will
        // be the new root.
        if(expTree)
        {
                loadNode(id, anode);
                if(id->node.attrib & 1)
                {
                        int k1, k2;

                        npos = getFreeNode(id);
                        memmove(id->node.data + ((lk + 4) * id->node.nkey),
                                    outKey, lk + 4);
                        k1 = id->node.nkey / 2;
                        k2 = id->node.nkey - k1 + 1;

                        // Create the new left node.
                        memmove(pbuff, id->node.data + ((lk + 4) * k1),
                                    (lk + 4) * k2);
                        memmove(newKey, id->node.data + ((lk + 4) * (k1 - 1)), lk);
                        id->node.nkey = k1;
                        id->node.attrib &= ~ATT_ROOT;
                        id->node.nleft = -1l;
                        id->node.nright = npos;
                        writeNode(id, anode);

                        // Create the new right node.
                        memmove(id->node.data, pbuff, ((lk + 4) * k2));
                        memmove(outKey, id->node.data + ((lk + 4) * (k2 - 1)), lk);
                        id->node.nkey = k2;
                        id->node.nleft = anode;
                        id->node.nright = -1l;
                        writeNode(id, npos);

                        // Create be the new root node.
                        memmove(id->node.data, newKey, lk);     // Ptro nodo izquierdo =  Another left node?
                        memmove(id->node.data + lk, sethex(anode), 4);
                        memmove(id->node.data + lk + 4, outKey, lk);    // right
                        memmove(id->node.data + lk + lk + 4, sethex(npos), 4);
                        id->node.nkey = 2;
                        id->node.nleft = id->node.nright = -1l;
                        id->node.attrib = ATT_ROOT;
                        npos = getFreeNode(id);
                        writeNode(id, npos);

                        //indicates the new root node in the header
                        id->hd.RootNode = npos;
                        id->fh.xLseek(0l, SEEK_SET);
                        id->fh.xWrite(&id->hd, sizeof(struct idxHeader));

                        //  Indicates that the operation is finished.
                        expTree = 0;
                }
        }

        // Gets the position of the new node, removing it from the list
        // of nodes released or creating a new one if applicable.
        if(expTree)
        {
                loadNode(id, anode);
                npos = getFreeNode(id);
                pRight = id->node.nright;
                id->node.nright = npos;
                writeNode(id, anode);

                //  Build the new node. (The attribute is inherited).
                id->node.nkey = 1;
                id->node.nright = pRight;
                id->node.nleft = anode;
                memmove(id->node.data, outKey, lk + 4);
                writeNode(id, npos);

				//Update the original right node, if one existed
				if (pRight > 0)
				{
					loadNode(id, pRight);
					id->node.nleft = npos;
					writeNode(id, pRight);
				}

                // Enter the reference in the top node.
                id->NodeTreeCount--;
                memmove(tkey, outKey, lk);
                memmove(tRec, sethex(npos), 4);
                InsertKey(tRec, tkey, id);
        }
}

//  The following function removes a key (identified by db-> recno)
//  from the Index indicated.
//   OJO - The sequence of nodes where the key is IS ALREADY IN THE ARRAY NodeTree.
//         The parameter 'rec' only serves to identify key INSIDE the node currently
//         pointed.
void dbf::DeleteKey(unsigned long rec, struct idx *id)
{
         unsigned long nleft, nright;
         int cnt, lk;
		 char *n, last_key[100]; // last_key2[100];
		 bool last_key_changed;

         // If you are in the root node - exit.
         if(!id->NodeTreeCount) return;

         // Seeking the key whose NODE is the PTR to be deleted.
         loadNode(id, id->NodeTree[id->NodeTreeCount - 1]);
         lk = id->hd.LenKey;
         for(cnt = id->node.nkey, n = id->node.data; cnt;
             n += 4 + lk, cnt--)
                 if(rec == gethex(n + lk))
                        break;
         
		 if(!cnt) return;       //  Not found?????
		 last_key_changed = false;
		 //if we're deleting the last key in the node we need to update parent node pointer new last key
		 //assuming there is another key in the node.
		 //if (id->node.nkey == (int)((n - id->node.data) / (lk + 4)))
		 if (cnt == 1 && id->NodeTreeCount > 1)
		 {
			 last_key_changed = true;
			 if (id->node.nkey > 1)
			 {
				 memmove(last_key, n - lk - 4, lk + 4);
				 rec = id->NodeTree[id->NodeTreeCount - 1];
			 }
		 }
         id->node.nkey--;       //  One key less
         if(--cnt)              //  Move back the rest one place. 
                memmove(n, n + lk + 4, cnt * (lk + 4));

         //   If we leave at least one key in the node, we rewrite the new node,
         //   but starting the steps of deletion of empty nodes.
         //         If the node being deleted does NOT have a left or right node, then you are
         //         trying to delete a level.  In this case, the node is empty because no
         //         code exists to delete levels.  This shouldn't happen as we're not going to 
		 //         employ filters
         nleft = id->node.nleft;
         nright = id->node.nright;
		 int ntc;
		 ntc = id->NodeTreeCount;
		 if (id->node.nkey || ((nleft == -1l) && (nright == -1l)))
		 {
			 writeNode(id, id->NodeTree[id->NodeTreeCount - 1]);
			 //if (last_key_changed) //since we just deleted the last key of a non-root node
			 //{                                   //we have to update the parent's reference to this node 
			 // loadNode(id, id->NodeTree[ntc-2]);
			 // for (cnt = id->node.nkey, n = id->node.data; cnt;
			 //	 n += 4 + lk, cnt--)
			 //	 if (rec == gethex(n + lk))
			 //		 break;

			 // if (!cnt) return;   //error finding original key in parent node
			 // memmove(n, last_key, lk);
			 // writeNode(id, id->NodeTree[ntc - 2]);
			 //}

			 ntc = id->NodeTreeCount;
			 while (last_key_changed && ntc > 1) //since we just deleted the last key of a non-root node
			 {                        //we have to update the parent's reference to this node 
				 rec = id->NodeTree[ntc-1];      //assuming a parent node exists
				 loadNode(id, id->NodeTree[ntc-2]);
				 for (cnt = id->node.nkey, n = id->node.data; cnt;
					 n += 4 + lk, cnt--)
					 if (rec == gethex(n + lk))
						 break;

				 if (!cnt) return;   //error finding original key in parent node
				
				 last_key_changed = false;
				 if (cnt == 1 && ntc > 1) //are we are last key of non-root node?
				 {                        //then we're gonna have to update another level up
					 last_key_changed = true;
					 //if (id->node.nkey > 1)
					 //	 memmove(last_key2, n - lk - 4, lk + 4);
				 }
				 
				 memmove(n, last_key, lk);
				 //memmove(last_key, last_key2, 100);
				 writeNode(id, id->NodeTree[ntc - 2]);
				 ntc--;
			 }
		 }
          else
          {
                //  The following steps remove the node, interfacing between
                //  if the nodes before it had this as their left and right node.
                if(nleft != -1l)
                {
                         loadNode(id, nleft);
                         id->node.nright = nright;
                         writeNode(id, nleft);
                }
                if(nright != -1l)
                {
                         loadNode(id, nright);
                         id->node.nleft = nleft;
                         writeNode(id, nright);
                }
                rec = id->NodeTree[--id->NodeTreeCount];
                DeleteKey(rec, id);
          }
}

//#ifdef TESTIDX
char xKey[10];
int CheckNode(struct idx *x, long n)
{
        int cnt, lk, maxKeyInNode, i;
        char *p, pKey[80];
        long nxtNode;


        // The length of the key can only be six (test file).
        lk = x->hd.LenKey;
        //if(lk != 6)
        /*if(lk != 8)
        {
                printf("Error - Length of key != 6 (%d).\n", lk);
                return 0;
        }*/

        // Load the node into memory.
        loadNode(x, n);

        // Since we are only inserting, there can be no empty nodes.
        if(x->node.nkey == 0)
        {
                printf("Node: %ld with 0 keys.\n", n);
                return 0;
        }

        // Verifica la identidad del nodo.

        maxKeyInNode = (500 / (lk + 4));
        if(x->node.nkey > maxKeyInNode)
        {
                printf("This node %lX has invalid entries no. (%d - Max:%d).\n",
                            n, x->node.nkey, maxKeyInNode);
                return 0;
        }

        // The attribute can go 0-3 only.
        if(x->node.attrib < 0 || x->node.attrib > 3)
        {
                printf("Attribute %d is invalid, node %lX\n", x->node.attrib, n);
                return 0;
        }

        // Parameter to check correlative.
		for (i = 0; i < lk; i++) pKey[i] = ' ';
		pKey[lk] = '\0';

        //strcpy(pKey, "        \0\0\0\0");
        for(cnt = x->node.nkey, p = x->node.data; cnt; cnt--,p += lk+4)
        {
                // Detects the error of uncorrelated keys.
			if (x->node.attrib != ATT_ROOT)
			{
				if (memcmp(pKey, p, lk + 4) > 0)
				{
					printf("Nonconsecutive Keys. Node:%lX, nkey:%d.\n",
						n, x->node.nkey);
					return 0;
				}
				memmove(pKey, p, lk + 4);
			}
			else
			{
				if (memcmp(pKey, p, lk) > 0)
				{
					printf("Nonconsecutive Keys. Node:%lX, nkey:%d.\n",
						n, x->node.nkey);
					return 0;
				}
				memmove(pKey, p, lk);
			}
			
			//  Now check the corresponding sub-tree.
                if( !(x->node.attrib & ATT_LEAF))
                {
                        nxtNode = gethex(p + lk);
            if(!CheckNode(x, nxtNode))
                        {
                                printf("CheckNode(x, %lX) gave error.\n", nxtNode);
                                printf("It was called from %lX, key '%s'.\n",
                                            n, pKey);
                                return 0;
                        }

                        //  Check the reference between tree branches.
                        if(memcmp(pKey, xKey, lk))
                        {
                                pKey[lk] = xKey[lk] = 0;
                                printf("The last key of %lX (%s)\n", nxtNode, pKey);
                                printf("Is not equal to the marker of %lX (%s)\n", n, xKey);
                                return 0;
                        }

                        //  Reload the node being checked.
                        loadNode(x, n);
                }
        }
        memmove(xKey, x->node.data + ((lk+4) * (x->node.nkey-1)), lk);
        return 1;
}

int dbf::CheckIdx(const char * field)
{
        struct idx *x;
        long actNode, prvNode;
        char pKey[80];
		char nFld[100];
    int lk, i;

	for (x = idxlist; x; x = x->next)
	{

		strcpy(nFld, x->hd.key);

		//   Search index which is updatable.
		if (!strcmp(field, nFld))
		{

			//  Initializes some variables.
			//x = idxmaster;
			lk = x->hd.LenKey;

			//  First, check that the contents of the nodes is not crap.
			if (!CheckNode(x, x->hd.RootNode)) return 0;

			//  Now, we will verify that levels are consistent.
			actNode = x->hd.RootNode;
			loadNode(x, actNode);
			for (;;)
			{
				// Walk to the left end.
				while (x->node.nleft != -1l)
				{
					actNode = x->node.nleft;
					loadNode(x, actNode);
				}

				// Compare the string reference to the first node.
				for (i = 0; i < lk; i++) pKey[i] = ' ';
				pKey[lk] = '\0';
				//strcpy(pKey, "      ");
				prvNode = -1l;
				for (;;)
				{
					if (memcmp(pKey, x->node.data, lk) > 0)
					{
						printf("Intra-Nodes out of sequence: %lX<->%lX\n",
							actNode, prvNode);
						return 0;
					}
					memmove(pKey, x->node.data + (lk + 4) * (x->node.nkey - 1), lk);
					prvNode = actNode;
					actNode = x->node.nright;
					if (actNode == -1l)
						break;
					loadNode(x, actNode);
				}
				if (x->node.attrib & ATT_LEAF)
					break;
				actNode = gethex(x->node.data + lk);
				loadNode(x, actNode);
			}
			return 1;
		}
	}
}
//#endif
