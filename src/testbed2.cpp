#include <iostream>
#include <vector>
#include <glibmm/ustring.h>
#include <glibmm/regex.h>
#include <unordered_map>
#include <string>
using namespace Glib;
using namespace std;

class ParseLine
{
public:
  ParseLine (const ustring & text);
  ~ParseLine () {};
  vector <ustring> lines;
private:
};

ustring trimEnd(const ustring & s)
{
  if (s.length() == 0) { return s; }
  // Strip spaces, tabs, new lines, carriage returns from end of string
  size_t end = s.find_last_not_of(" \t\n\r");
  // No non-spaces  
  return ustring(s, 0, end + 1);
}

ParseLine::ParseLine(const ustring & text)
// Parses text into a vector of separate lines, each in a ustring
{
  ustring s = text;
  size_t start = s.find_first_not_of(" \t\n\r");
  size_t newlineposition = s.find("\n");
  // Loop intial condition: start points to first non-whitespace and newlineposition points at
  // following newline, if there is one, or end of string if not
  while (newlineposition != string::npos) {
    cout << "DEBUG: start="<<start<<" newlineposition="<<newlineposition<<endl;
    ustring line = s.substr(start, newlineposition-start);
    lines.push_back(trimEnd(line)); // trim away any spaces or \r or \t that are just before the \n
    start = newlineposition + 1;
    start = s.find_first_not_of(" \t\n\r", start); // fast forward past any initial whitespace
    newlineposition = s.find("\n", start);
  }
  if (start != string::npos) {
    ustring line = s.substr(start, string::npos);
    // very end was already trimmed at beginning of this routine
    lines.push_back(line);
  }
}
#include "usfm-inline-markers.h"

void addToWordCount(void)
{
	vector<ustring> words;
    
    std::unordered_map<std::string, int, std::hash<std::string>> wordCounts;
    //ustring tmp("This ( is a verse) that has \t some 10 delimiters in    it \nd howbeit \nd*.");
    ustring tmp ("¶ And God said, Let there be a firmament in the midst of the waters, and let it divide the waters from the waters.\f + \fr 1.6\n\ft firmament: Heb. expansion\f*");
	// Split string into its components, deleting USFM codes along the way
	string::size_type s = 0, e = 0; // start and end markers
    ustring delims(" \\,:;!?.\u0022()¶\t");
	ustring nonUSFMdelims(" ,:;!?.\u0022()¶\t"); // same list as above except without '\'
	// u0022 is unicode double-quote mark
	// TO DO : configuration file that has delimiters in it, and "’s" kinds of things to strip out
	// for the target language.

#define SKIP_DELIMS(idx) while(nonUSFMdelims.find_first_of(tmp[idx]) != ustring::npos) { idx++; /*cout << "Skipping " << idx-1 << " " << tmp[idx] << endl;*/ }
#define SKIP_NONDELIMS while(nonUSFMdelims.find_first_of(tmp[i]) == ustring::npos) { i++; }
                       //while (nonUSFMdelims.find_first_of(text[e]) != ustring::npos) { e++; }

    SKIP_DELIMS(s)

	// This is a quick hack of a "parser." Needs to be redone.
	for (string::size_type i = s; i < tmp.size(); i++) {
		// Walk forward until we run into a character that splits words
		e = i; // move e along to track the iterator i
		bool foundDelim = (delims.find_first_of( tmp[i]) != ustring::npos);
		// If the last character is not already a delimiter, then we have to treat the next "null" as a delimiter
		if (!foundDelim && (i == tmp.size()-1)) { foundDelim = true; e = i + 1; } // point e to one past end of string (at null terminator)
		if (foundDelim) { // We found a delimiter
			// For a zero-length word, we bail. e=i can fall behind s if multiple delims in a row
			if (s >= e) { continue; }
			ustring newWord = tmp.substr(s, e-s); // start pos, length
			cout << "Found new word at s=" << s << " e=" << e << ":" << newWord << ":" << endl;
			
			/* --------------------------------------------------------------------------
			 * These are KJV-only modifications possible because I understand
			 * English. Other things will have to be done for other languages,
			 * or nothing at all.
			   --------------------------------------------------------------------------*/			
			
			ustring::size_type pos = newWord.find("’s");
			bool found_apostrophe_s = (pos != ustring::npos);
			if (found_apostrophe_s) {
				//cout << "Found new word with apostrophe s=" << newWord;
				newWord = newWord.substr(0, pos);
				cout << " replacing with " << newWord << endl;
			}
			pos = newWord.find("…");
			bool found_ellipsis = (pos != ustring::npos);
			if (found_ellipsis) {
				//cout << "Found new word with …=" << newWord;
				newWord = newWord.substr(0, pos);
				cout << " replacing with " << newWord << endl;
			}
			
			// Handle paragraph marker
			pos = newWord.find("¶");
			bool found_paragraph_marker = (pos != ustring::npos);
			if (found_paragraph_marker) {
				cout << "Found paragraph marker" << endl;
				newWord = newWord.substr(0, pos);
			}
			
			/* --------------------------------------------------------------------------
			 * End of KJV-specific modifications
			   --------------------------------------------------------------------------*/
			
			if (newWord[0] != '\\') { words.push_back(newWord); } // only add non-usfm words
			
			// Advance past the splitting character unless it is the usfm delimiter '\'
			if (tmp[e] != '\\') { e++; }
			// Now if s has landed on a non-USFM delimiter, move fwd one
			//while (nonUSFMdelims.find_first_of(text[e]) != ustring::npos) { e++; }
			SKIP_DELIMS(e)
			s=e; // ensure starting point is at last ending point
		}
	}
	
	// Print each word
	for (auto &word : words) {
		cout << "Word: " << word << ":" << endl;
		// TODO: The problem with lowercase is words like Lord, proper names, etc. need to remain uppercase, sometimes.
		wordCounts[word.lowercase()]++;
	}
}

void byzasciiConvert(ustring &vs)
{
  // From the README:
  // The Greek is keyed to the Online Bible format, which differs from
  // nearly all Greek fonts in the following respects: Theta = y; Psi = Q,
  // final Sigma = v.
  // I happen to know that these BYZ strings, though stored as ustring for convenience
  // everywhere in Bibledit, are just ascii strings. So I will convert to c_str and then
  // walk the entire string a single time, to make this operation faster.
  char * c = new char[vs.length()+1];
  strcpy(c, vs.c_str());
  vs.clear();
  char newc;
  //printf("Char ptr=%08lX, *ptr=%c\n", (unsigned long)c, *c);
  while (*c != 0) {
     // cout << "Considering char " << *c << endl;
     if (*c == 'y') { newc = 'Q'; } // theta transform
     else if (*c == 'Q') { newc = 'y'; } // psi transform
     else if (*c == 'v') { newc = 'V'; } // final sigma transform
     else { newc = *c; }
     vs.push_back(newc);
     c++;
  }
  delete[] c;
}

#define TEST4

class ReadText
{
public:
  ReadText (const ustring & file, bool silent = false, bool trimAll = true, bool trimEnding=false);
  ~ReadText ();
  vector < ustring > lines;
private:
};

ReadText::ReadText(const ustring & file, bool silent, bool trimAll, bool trimEnding)
{
  // Reads the text and stores it line by line, trimmed, into "lines".
  // If "silent" is true, then no exception will be thrown in case of an error.
  // The lines will be trimmed if "trimming" is true.
  ifstream in(file.c_str());
  if (!in) {
    if (!silent) {
      cerr << "Error opening file " << file << endl;
      throw std::runtime_error("error opening file");
    }
    return;
  }
  string s;
  while (getline(in, s)) {
    if (trimAll) {
      //s = trim(s);
    }
    else if (trimEnding) {
      //s = trimEnd(s);
    }
    lines.push_back(s);
  }
}


ReadText::~ReadText()
{
}

#include <locale>

// How to compile this file
// g++ testbed2.cpp -o testbed2 `pkg-config --cflags --libs glibmm-2.4`
// If you use g++ `pkg-config --cflags --libs glibmm-2.4` testbed2.cpp -o testbed2
// it will NOT work
// ./testbed2 or ./testbed2.exe if on Windows

int main(void)
{
#ifdef TEST4
   std::locale::global(std::locale(""));
   //setlocale(LC_ALL, "");
   ReadText rt("badHebrew.txt");
   for (auto &it: rt.lines) {
       // The problem comes in printing the line when on Windows. On Linux, the std::locale... command
       // fixes things up nicely, but not on Windows/msys2. Lesson: don't print these strings...
      cout << "Line::" << it << "::" << endl;   
   }

#endif
#ifdef TEST3
    ustring x = "outwv gar hgaphsen o yeov ton kosmon wste ton uion autou ton monogenh edwke n ina pav o pisteuwn eiv auton mh apolhtai all ech zwhn aiwnion";
    cout << "Before=" << x << endl;
    byzasciiConvert(x);
    cout << "After =" << x << endl;
#endif
#ifdef TEST2
    addToWordCount();
#endif
#ifdef TEST1
  ustring test = "First line   \n   Second line\t\r\nThird line\n";
  ustring nums = "01234567890123 456789012345678 9 0 12345678901 23456789\n";
  ParseLine parser(test);
  for (unsigned int i = 0; i < parser.lines.size(); i++) {
    cout << "String " << i << " ***" << parser.lines[i] << "***" << endl;
  }
  
  std::vector<Glib::ustring> vLines = Glib::Regex::split_simple("\n", test);
  std::cout << "size: " << vLines.size() << std::endl;
  for(int i=0; i<vLines.size(); i++) {
      std::cout << vLines.at(i) << std::endl;
  } 
#endif
}

