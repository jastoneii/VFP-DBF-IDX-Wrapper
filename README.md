# VFP-DBF-IDX-Wrapper
C++ Code to Read/Write/Create VFP DBF and IDX Files.  

The attached files were originally posted to C# Corner by Ricardo Federico  on Feb 25, 2002. The purpose of the code was to create a C# or VB.net wrapper that allowed access to DBF and IDX files without a VFP driver.  Given VFP is no longer supported by MS and future versions of Windows may make existing VFP drivers/DLLs obsolete, I decided to update Ricardo's code to work with VS Express and give it more complete functionality. Ricardo's originally code comments were in Spanish.  I have translated them to English or added my own comments. 

As I am relatively new to VS Express C++, I'm sure the code could be cleaned up more to make it more standard; but, it does work.  Index files can be created based on any single field or multiple fields so long as the result is a concatenated string. An index may convert a number to a string, convert a string/character field to upper-case and convert a date field to a string. 

The ConsoleApplication.cpp file shows various examples on how to create .idx files and most of the existing available functions.

"AppendFrom" could/should be faster than it is as it takes noticeably longer to append an existing .DBF file with the C++ code than in VFP itself. I tried to make it so that if you are appending from a file that has corresponding field names but those fields are of different data types, AppendFrom will attempt to do the appropriate conversion when possible.  

Tested datatypes are: char, double, int,  number, date and logical. Memo field capability would be a good addition.
Also, .DBC and .CDX functionality would be good additions as well.

The "Locate" works just fine.  "Continue" functionality would be good.

A partial list of functions/commands are:
  createTable
  createTableFrom
  AppendBlank
  AppendFrom
  AppendFromCSV
  CopyStruct
  Copy1To -- used after CopyStruct, copies 1 record out to existing DBF
  Go
  Skip
  Locate
  Use
  Close
  IndexOn
  Setorder
  Seek
  Get -- for char, date, numeric and logical fields
  GetD -- for double and numeric fields
  GetI -- for integer fields
  Replace -- for char, date, numeric and logical fields
  ReplaceD -- for double and numeric fields
  ReplaceI -- for integer fields
