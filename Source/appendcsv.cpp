#include "stdafx.h"
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

//#include "dbhdr.h"

extern bool NULL_not_returned, strtok_called;

//replace all function
void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
        if (from.empty())
                return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos)
        {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
}

//parse line to get next field which is a text field
char* getfieldTxt(char* line)
{
        char* tok;
        if (strtok_called)
                tok = strtok(NULL, "|\n");
        else
        {
                tok = strtok(line, "|\n");
                strtok_called = true;
        }
        return tok;
}
//parse line to get next field which is a text field
//this version recognizes commas as a possible delimiter as well for the text field
char* getfieldTxt2(char* line)
{
        char* tok;
        if (strtok_called)
                tok = strtok(NULL, "|,\n");
        else
        {
                tok = strtok(line, "|,\n");
                strtok_called = true;
        }
        return tok;
}


//parse line to get next field which is a numeric field in text form
char* getfieldNum(char* line)
{
        char* tok;
        if (NULL_not_returned)
        {
                if (strtok_called)
                        tok = strtok(NULL, "|,\n");
                else
                {
                        tok = strtok(line, "|,\n");
                        strtok_called = true;
                }
                if (tok == NULL)
                        NULL_not_returned = false;
                return tok;
        }
        return NULL;
}

//parse line to get next field which is a double field
double getfieldDbl(char* line)
{
        const char* tok;
        if (NULL_not_returned)
        {
                if (strtok_called)
                        tok = strtok(NULL, "|,\n");
                else
                {
                        tok = strtok(line, "|,\n");
                        strtok_called = true;
                }
                if (tok == NULL)
                {
                        NULL_not_returned = false;
                        return 0.0;
                }
                return atof(tok);
        }
        return 0.0;
}

//parse line to get next field which is an VFP integer field
long getfieldInt(char* line)
{
        const char* tok;
        if (NULL_not_returned)
        {
                if (strtok_called)
                        tok = strtok(NULL, "|,\n");
                else
                {
                        tok = strtok(line, "|,\n");
                        strtok_called = true;
                }
                if (tok == NULL)
                {
                        NULL_not_returned = false;
                        return 0.0;
                }
                return (long) atoi(tok);
        }
        return (long) 0;
}

	void ltrim_to(char* str_in, char* str_out)
		{
			char* buffer = str_in;
			while (*str_in && *str_in++ == ' ');
			strcpy(str_out, str_in);
		}

void rtrim_to(char* str_in, char* str_out)
{
	int start = 0; // number of leading spaces
	strcpy(str_out, str_in + start);
	char *end = str_out + strlen(str_out) - 1;
	while (isspace(*end))
		*end-- = 0;
}

//trim trailing spaces
void rtrim_to(std::string& in, std::string& out)
{
	std::string::const_iterator b = in.begin(), e = in.end();

	if (b != e)
	{
		// skipping trailing spaces
		while (isspace(*(e - 1))){
			--e;
		}
	}

	out.assign(b, e);
}

//// trim from start
//static inline std::string &ltrim(std::string &s) {
//	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
//	return s;
//}
//
//// trim from end
//static inline std::string &rtrim(std::string &s) {
//	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
//	return s;
//}
//
//// trim from both ends
//static inline std::string &trim(std::string &s) {
//	return ltrim(rtrim(s));
