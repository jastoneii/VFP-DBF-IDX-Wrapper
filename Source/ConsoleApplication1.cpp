// ConsoleApplication1.cpp : main project file.

#include "stdafx.h"
#include "mshstr.h"
#include "db_wrapper.h"
#include "dbhdr.h"

using namespace System;
using namespace db;

void PrintItemData(xdbf% myDbf);

bool createTableFrom(String ^newdb, String ^extfile)
{
	int use_result, fld_len, fld_dec;
	//char buildstr[1000], fld_type, tempstr1[40], tempstr2[40];
	xdbf myDb;
	String ^str_out, ^buildstr, ^fld_type, ^tempstr;

	use_result = myDb.Use(extfile);
	if (use_result != 1)
		return use_result;

	myDb.Go(DB_GO_TOP);
	while (!myDb.Eof)
	{
		if (myDb.Recno == 1)
		{
			str_out = myDb.Get("Field_name");
			 buildstr = str_out->Trim();
		}
		else
		{
			buildstr += ", ";
			buildstr += myDb.Get("field_name")->Trim();
		}

		buildstr += " ";
		fld_type = myDb.Get("field_type")->ToUpper();
		buildstr += fld_type;
		buildstr +=  "(";
		tempstr = myDb.Get("field_len");
		buildstr += tempstr->Trim();
		if (fld_type == "B" || fld_type == "N")
		{
			buildstr +=  ",";
			tempstr = myDb.Get("field_dec");
			buildstr += tempstr->Trim();
		}
		buildstr +=  ")";
		myDb.Skip(1);
	}
	myDb.Close();
	return myDb.createTable(newdb, buildstr);
}


int main()
{
	int i, heapstatus;
	String ^xstring, ^pricestr;
	double price;
	char  pricechr[13];

        db::xdbf myDb, otherDb;
	//	myDb.Use("d:\\test.dbf");
	//	xstring = myDb.Get("cod");
	//	xstring = myDb.Get("des");
	//	xstring = myDb.Get("price");

	
	//	createTableFrom("d:\\testc.dbf", "d:\\template\\597_flds.dbf");
	//	myDb.Use("d:\\testc");
	//	myDb.AppendFrom("e:\\ey\\pmac\\pmcxxbnk.col");
		myDb.IndexOn("loan_id+dtos(curr_pdate)", "d:\\tst.idx");
	//	myDb.Close();

	//	myDb.creaTable("d:\\test.dbf", "cod c(3), des c(34), price n(9, 2), dbltst b(8, 2)");
		myDb.createTable("d:\\test.dbf", "cod c(3), des c(34), price n(8,2), dbltst b(8, 2), inttst i(4), datetst d(8), booltst l(1)");
		myDb.Use("d:\\test.dbf");
		myDb.IndexOn("cod", "d:\\cod.idx");
		myDb.IndexOn("dbltst", "d:\\testdbl.idx");
		myDb.IndexOn("upper(des)+upper(cod)+str(dbltst,9,2)", "d:\\descoddb.idx");
		//myDb.IndexOn("str(dbltst,9,2)", "d:\\dblstr.idx");
		//myDb.IndexOn("dbltst", "d:\\dbl.idx");


		//  Test Rows inserts.
		//Random RandomPrice = gcnew Random()
		for (i = 1; i <= 5; i++)
		{
			price = i * 1000.233;
			if (i == 3)
				price = -price;

			myDb.AppendBlank();
		//	if (i == 4) break;
			if (i != 2 && i != 4)
				myDb.Replace("cod", i.ToString());
			if (i == 2)
				myDb.Replace("cod", "4");
			if (i == 4)
				myDb.Replace("cod", "2");

			myDb.Replace("des", "(" + (10 - i).ToString() + ") Description for #" + i.ToString());

			//double_to_char(pricechr, price, 8, 2);
			//pricestr = gcnew System::String(pricechr, 0, 8);
			//myDb.Replace("price", pricestr); // price.ToString());
			//myDb.Replace("price", price.ToString());
			myDb.ReplaceD("price", price);
			myDb.ReplaceD("dbltst", price);
			myDb.ReplaceI("inttst", i);
		}


		myDb.Go(3);
		myDb.Replace("cod", "3a");
		Console::WriteLine("\nCurrent Record Number: {0}   # Records {1}", myDb.Recno, myDb.RecCount);

		if (myDb.Locate("cod", "4 "))  //can also just use "4" but result will be an unexact find
			Console::WriteLine("\nFound COD of '4' using LOCATE at Record: {0}  Should be 2", myDb.Recno);
		else
			Console::WriteLine("\nLOCATE Failed");

		//myDb.IndexOn("price", "d:\\testprice.idx");
		//  Test Print Cod Ordered
		Console::WriteLine("\nTest COD Ordered...");
		myDb.Setorder("cod");

		if (myDb.Seek("3a")) //Note: that seek is not exact unless full length is entered!!
			Console::WriteLine("\nSeek COD of '3a' using Seek at Record: {0}  Should be 3", myDb.Recno);
		else
			Console::WriteLine("\nString Seek Failed");

		myDb.Setorder("dbltst");
		if (myDb.SeekD(1000.233)) //Note: double seek is exact
			Console::WriteLine("\nSeek DBLTST of 1000.233 using Seek at Record: {0}  Should be 1", myDb.Recno);
		else
			Console::WriteLine("\nDouble Seek Failed");

		myDb.Go(0);
		while (!myDb.Eof)
		{
			PrintItemData(myDb);
			myDb.Skip(1);
		}


		//  Test Print Des Ordered.
		Console::WriteLine("\nTest Price Order...");
		//myDb.Setorder("upper(des)+upper(cod)+str(dbltst,9,2)");
		//myDb.Setorder("str(dbltst,9,2)");
		myDb.Setorder("price");
		myDb.AppendFromCSV("d:\\tst.csv", true, false);
		//myDb.CheckIdx("UPPER(DES)+UPPER(COD)+STR(DBLTST,9,2)");
		myDb.AppendFrom("d:\\tstc.dbf");
		myDb.AppendFrom("d:\\tstc.dbf");
//		myDb.CheckIdx();
		myDb.AppendFrom("d:\\tstc.dbf");

		myDb.Setorder("upper(des)+upper(cod)+str(dbltst,9,2)");
		myDb.Go(0);
		while (!myDb.Eof)
		{
			PrintItemData(myDb);
			myDb.Skip(1);
			if (myDb.Recno == 1665l)
				i = i;
		}


		//  Test Close and Reopen.
		Console::WriteLine("\nTesting Close and Reopen...");
		myDb.Close();
		myDb.Use("d:\\test.dbf");
		myDb.CopyStruct("d:\\testcopy.dbf");
		//myDb.AppendFrom("d:\\tst2.dbf");
	    
		// Test Copy1To for VFP Copy To equivalence
		myDb.CopyStruct("d:\\tstcopy1to.dbf");
		myDb.Go(DB_GO_TOP);
		while (!myDb.Eof)
		{
			if (myDb.GetD("dbltst") > 2400.0)
			  myDb.Copy1To("d:\\tstcopy1to.dbf");
			myDb.Skip(1);
		}


		//myDb.Go(5);
		//PrintItemData(myDb);
		//myDb.Delete();
		//myDb.Pack();
		//myDb.AppendFromCSV("d:\\tst.csv", true);

		//myDb.SetIndex("d:\\testcod.idx");
		//myDb.SetIndex("d:\\testdes.idx");

		////  Test some random item.
		//Console::WriteLine("\nTesting SEEK using INDEXES...");
		//myDb.Setorder("cod");
		//Console::WriteLine("Seeking 3");  myDb.Seek("3"); //PrintItemData();
		//Console::WriteLine("Seeking 5");  myDb.Seek("5"); //PrintItemData();
		//Console::WriteLine("Seeking 1");  myDb.Seek("1"); //PrintItemData();
		//Console::WriteLine("Seeking 9"); myDb.Seek("9"); //PrintItemData();

		//   Ok. Bye.
		myDb.Close();
		Console::WriteLine("End Tests. Push ENTER to END");
		Console::ReadLine();
		return 0;
}

void PrintItemData(xdbf% myDbf)
{
	if (myDbf.Eof)
		Console::WriteLine("Not Found!!!");
	else
		Console::WriteLine("recno {0} COD:{1}, DES:{2}, PRICE: {3} DBLTST: {4}  INTTST: {5}", myDbf.Recno, myDbf.Get("cod"),	
	//	myDbf.Get("des"), myDbf.Get("price"));
		myDbf.Get("des"), myDbf.GetD("price"), myDbf.GetD("dbltst"), myDbf.GetI("inttst"));
}


    
    

