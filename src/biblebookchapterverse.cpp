/*
 ** Copyright (©) 2018- Matt Postiff.
 **  
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 3 of the License, or
 ** (at your option) any later version.
 **  
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **  
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **  
 */

#include "biblebookchapterverse.h"

book::book(bible *_bbl, const ustring &_bookname, unsigned int _booknum) : chapters(1)
{
    // Note: chapters[0] is unused; simply here to avoid the "off by one" indexing error
	bbl = _bbl;
	bookname = _bookname;
    booknum = _booknum;
}

book_byz::book_byz(bible *_bbl, const ustring &_bookname, unsigned int _booknum): book(bbl,  _bookname,  _booknum)
{
  // nothing special needs done       
}

book_sblgnt::book_sblgnt(bible *_bbl, const ustring &_bookname, unsigned int _booknum): book(bbl,  _bookname,  _booknum)
{
  // nothing special needs done       
}

book_sblgntapp::book_sblgntapp(bible *_bbl, const ustring &_bookname, unsigned int _booknum): book(bbl,  _bookname,  _booknum)
{
  // nothing special needs done
}

book_engmtv::book_engmtv(bible *_bbl, const ustring &_bookname, unsigned int _booknum): book(bbl,  _bookname,  _booknum)
{
  // nothing special needs done       
}

book_leb::book_leb(bible *_bbl, const ustring &_bookname, unsigned int _booknum): book(bbl,  _bookname,  _booknum)
{
  // nothing special needs done       
}

book_bixref::book_bixref(bible *_bbl, const ustring &_bookname, unsigned int _booknum) : book(_bbl, _bookname, _booknum)
{
   // nothing special needs done   
}

book::book() : chapters(1)
{
  // Note: chapters[0] is unused; simply here to avoid the "off by one" indexing error
   bbl = NULL;
   bookname = "Unset book name";
   booknum = 0;
}

chapter::chapter(book *_bk, int _num) : verses(1)
{
    // Note: verses[0] is unused; simply here to avoid the "off by one" indexing error
	bk = _bk;
	chapnum = _num;
}

chapter::~chapter()
{
    for (auto it: verses) {
        delete it;
    }
    verses.clear();
}

book::~book()
{
    bbl = NULL;
    bookname = "";
    booknum = 0;
    for (auto it: chapters) {
       delete it;   
    }
}

verse::verse(chapter *_ch, int _vs, ustring _txt)
{
	ch = _ch;
	vsnum = _vs;
	text = _txt;
}

verse::~verse()
{
  // nothing to do here. just defining for "virtualness"   
}

verse_xref::verse_xref(chapter *_ch, int _vsnum, ustring _txt) : verse(_ch, _vsnum, _txt)
{
   // nothing else needed for now   
}

verse_xref::~verse_xref()
{
   // nothing to do here. vector destroys itself
}

void verse::print(void)
{
  cerr << ch->bk->bookname << " " << ch->chapnum << ":" << vsnum << " " << text << endl;
  return;
}

#include "stylesheetutils.h"
#include "utilities.h"

void verse::addToWordCount(std::unordered_map<std::string, int, std::hash<std::string>> &wordCounts,
                           std::unordered_map<std::string, std::vector<int>, std::hash<std::string>> &wordLocations)
{
	vector<ustring> words;
    ustring tmp = usfm_get_verse_text_only (text); // liable to be slow, but it will give me just what I want
    //cout << ch->chapnum << ":" << vsnum << " " << tmp << endl;
//     Usfm usfm(stylesheet_get_actual ());
//     UsfmInlineMarkers usfm_inline_markers(usfm);
//     usfm_remove_inline_text_markers(tmp, &usfm_inline_markers);
    
	string::size_type s = 0, e = 0; // start and end markers
    ustring delims(" \\,'`:;!?.\u0022()[]¶\t«»\u201c\u201d\u2018\u2019\u2014"); //  \u0022 is quotation mark; «» are Alt-0171 and 0187; \u201c\u201d are smart double quotes
	ustring nonUSFMdelims(" ,'`:;!?.\u0022()[]¶\t«»\u201c\u201d\u2018\u2019\u2014");        // same list as above except without '\'
	// u0022 is unicode double-quote mark
	// 0xe2 80 98 (U+2018) and e2 80 99 (U+2019) are smart single quotes. These are in UTF-8 encoding I think...found in EMTV
	// 0xe2 80 94 (U+2014) is a long dash, looks like em-dash
	// TO DO : configuration file that has delimiters in it, and "’s" kinds of things to strip out
	// for the target language.
	
#define SKIP_DELIMS(idx) while(nonUSFMdelims.find_first_of(tmp[idx]) != ustring::npos) { idx++; /*cout << "Skipping " << idx-1 << " " << tmp[idx] << endl;*/ }
#define SKIP_NONDELIMS while(nonUSFMdelims.find_first_of(tmp[i]) == ustring::npos) { i++; }

    SKIP_DELIMS(s)

	for (string::size_type i = s; i < tmp.size(); i++) {
        // Walk forward until we run into a character that splits words
		e = i; // move e along to track the iterator i
		bool foundDelim = (delims.find_first_of(tmp[i]) != ustring::npos);
		// If the last character is not already a delimiter, then we have to treat the next "null" as a delimiter
		if (!foundDelim && (i == tmp.size()-1)) { foundDelim = true; e = i + 1; } // point e to one past end of string (at null terminator)
		if (foundDelim) { // We found a delimiter
			// For a zero-length word, we bail. e=i can fall behind s if multiple delims in a row
			if (s >= e) { continue; }
			ustring newWord = tmp.substr(s, e-s); // start pos, length
			//cout << "Found new word at s=" << s << " e=" << e << ":" << newWord << ":" << endl;
			
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
				//cout << " replacing with " << newWord << endl;
			}
			pos = newWord.find("…");
			bool found_ellipsis = (pos != ustring::npos);
			if (found_ellipsis) {
				//cout << "Found new word with …=" << newWord;
				newWord = newWord.substr(0, pos);
				//cout << " replacing with " << newWord << endl;
			}
			
			// Handle paragraph marker
			pos = newWord.find("¶");
			bool found_paragraph_marker = (pos != ustring::npos);
			if (found_paragraph_marker) {
				//cout << "Found paragraph marker" << endl;
				newWord = newWord.substr(0, pos);
			}
			
			/* --------------------------------------------------------------------------
			 * End of KJV-specific modifications
			   --------------------------------------------------------------------------*/
			
			if ((newWord[0] != '\\') && (newWord != "+")) { // + is footnote caller
                words.push_back(newWord); // only add non-usfm words
            }
			
			// Advance past the splitting character unless it is the usfm delimiter '\'
			if (tmp[e] != '\\') { e++; }
			// Now if s has landed on a non-USFM delimiter, move fwd one
			SKIP_DELIMS(e)
			s=e; // ensure starting point is at last ending point
		}
	}
	
	// Print each word
	for (auto &word : words) {
		//cout << "Word: " << word << ":" << endl;
		// TODO: The problem with lowercase is words like Lord, proper names, etc. need to remain uppercase, sometimes.
        ustring canonicalWord = word.lowercase();
		wordCounts[canonicalWord]++;
       
        // add word location to concordance
        // The idea is to construct an int with three 8-bit fields, like this:
        // |--------|   bk   |   ch   |   vs   |
        int ref = ch->bk->booknum << 16 | ch->chapnum << 8 | vsnum;
        wordLocations[canonicalWord].push_back(ref);
	}
}

bible::bible(const ustring &_proj, const ustring &_font) : books(1)
{
    // Note: books[0] is unused; simply here to avoid the "off by one" indexing error
    projname = _proj;
    font = _font;
}

void bible::font_set(const ustring &_font)
{
  font = font;   
}

ustring bible::font_get(void)
{
  return font;   
}  

bible_byz::bible_byz(const ustring &_proj, const ustring &_font) : bible (_proj, _font)
{
  // Nothing special to do here   
}

bible_sblgnt::bible_sblgnt(const ustring &_proj, const ustring &_font) : bible (_proj, _font)
{
  // Nothing special to do here   
}

bible_sblgntapp::bible_sblgntapp(const ustring &_proj, const ustring &_font) : bible (_proj, _font)
{
  // Nothing special to do here
}

bible_engmtv::bible_engmtv(const ustring &_proj, const ustring &_font) : bible (_proj, _font)
{
  // Nothing special to do here   
}

bible_leb::bible_leb(const ustring &_proj, const ustring &_font) : bible (_proj, _font)
{
  // Nothing special to do here   
}

void bible::clear(void)
{
  projname = "";
  for (auto it: books) {
     delete it;
     it = NULL;
  }
  books.clear();
}

bible::~bible()
{
    clear();
}

//  This is for the concordance to load from an existing project
void chapter::load(int book, int chapter, 
                   std::unordered_map<std::string, int, std::hash<std::string>> &wordCounts,
                   std::unordered_map<std::string, std::vector<int>, std::hash<std::string>> &wordLocations)
{
	int vscnt = 0;
	// With these, it will be interesting to leave them at first; then to optimize
	// so that I can see how much faster it will go.
	// TODO. This copies a huge vector. Not efficient. Need to pass references.
	vector<ustring> data = project_retrieve_chapter (bk->bbl->projname, book, chapter);
	//cout << "Processing " << bk->bookname << " " << chapnum << endl;
	for (auto it = data.begin(); it != data.end(); ++it) {
		// TODO: probably more inefficiencies here
		ustring marker = usfm_extract_marker(*it);
		if (usfm_is_verse(marker)) {
			// We found a verse, not just a line of usfm codes like \p or \s or whatever
			vscnt++;
			// Remove verse number from usfm text
			ustring &s = *it;
			size_t endposition = s.find_first_of(" ", 1);
			if (endposition != string::npos) {
				ustring versenum = s.substr(0, endposition);
				s.erase(0, endposition+1); // +1 grabs the space after the verse number
			}
			verse *newVerse = new verse(this, vscnt, s);
			//newVerse.print();
			verses.push_back(newVerse);
			newVerse->addToWordCount(wordCounts, wordLocations);
		}
	}
}

ustring verse::retrieve_verse(const Reference &ref) 
{
    //  This is a single verse,  so ref.verse_get() should equal vsnum
    //  Otherwise,  I don't use the ref parameter.
    return text;
}

ustring chapter::retrieve_verse(const Reference &ref) 
{
    // I assume that the verse is not of the form 2:x-y,  but just 2:x
    // 1. Check if the verse is in range
    unsigned int vs = ref.verse_get_single();
    if ((vs == 0) ||  (vs >= verses.size())) { return "Verse out of range"; }
    return verses[vs]->retrieve_verse(ref);
}

void verse::append(const ustring &addlText)
{
    text = text + addlText;
    //cout << "Appended [[" << addlText << "]] to " "verse " << vsnum << endl;
}

void verse::prepend(const ustring &addlText)
{
    text = addlText + text;
    //cout << "Prepended [[" << addlText << "]] to " "verse " << vsnum << endl;
}

void chapter::appendToLastVerse(const ustring &addlText)
{
    verse *lastVerse = verses[verses.size()-1];
    lastVerse->append(addlText);
}

void chapter::prependToLastVerse(const ustring &addlText)
{
    // This was "invented" to handle adding the title of a Psalm to the beginning of verse 1 of the Psalm, like in LEB
    verse *lastVerse = verses[verses.size()-1];
    lastVerse->prepend(addlText);
}

ustring book::retrieve_verse(const Reference &ref)
{
    // Assumes it is already loaded
    // 1. Check if chapter is in range
    unsigned int ch = ref.chapter_get();
    if ((ch ==  0) ||  (ch >= chapters.size())) { return "Chapter out of range"; }
    return chapters[ch]->retrieve_verse(ref);
}

void book_byz::byzasciiConvert(ustring &vs)
{
  // From the README:
  // The Greek is keyed to the Online Bible format, which differs from
  // nearly all Greek fonts in the following respects: Theta = y; Psi = Q,
  // final Sigma = v.
  // I happen to know that these BYZ strings, though stored as ustring for convenience
  // everywhere in Bibledit, are just ascii strings. So I will convert to c_str and then
  // walk the entire string a single time, to make this operation faster.
  char * c = new char[vs.length()+1];
  char *cptr = c;
  strcpy(c, vs.c_str()); // I have to copy this out of the string before I clear the ustring...since c_str
  // gives us an internal pointer.
  vs.clear();
  //ustring newc;
  //printf("Char ptr=%08lX, *ptr=%c\n", (unsigned long)c, *c);
  while (*cptr != 0) {
     gunichar newc;
     //  Convert to unicode font. This is easy because the BYZ is unaccented and in lowercase.
      switch (*cptr) {
          case 'a' : newc = 945; break; 
          case 'b' : newc = 946; break; 
          case 'g' : newc = 947; break; 
          case 'd' : newc = 948; break; 
          case 'e' : newc = 949; break; 
          case 'z' : newc = 950; break; 
          case 'h' : newc = 951; break; 
          case 'y' : newc = 952; break;                     // theta transform 'q' in ASCII
          case 'i' : newc = 953; break; 
          case 'k' : newc = 954; break; 
          case 'l' : newc = 955; break; 
          case 'm' : newc = 956; break; 
          case 'n' : newc = 957; break; 
          case 'x' : newc = 958; break; 
          case 'o' : newc = 959; break; 
          case 'p' : newc = 960; break; 
          case 'r' : newc = 961; break; 
          case 'v' : newc = 962; break;
          case 's' : newc = 963; break; 
          case 't' : newc = 964; break; 
          case 'u' : newc = 965; break; 
          case 'f' : newc = 966; break; 
          case 'c' : newc = 967; break; 
          case 'q' : newc = 968; break;                 // psi transform, 'y' in ASCII
          case 'w' : newc = 969; break; 
          case ' ' : newc = 32; break;                      //  space
          default  : newc = 8855;                           //&otimes;,  just to mark as not done
        }
     vs.push_back(newc);
     cptr++;
  }
  delete[] c; // make sure to delete the original pointer...not the "moving" target
}

// Load Byzantine Text from shared resource directory
void book_byz::load(void)
{
  ustring filenames[27] = { "MT_BYZ.txt",  "MR_BYZ.txt",  "LU_BYZ.txt", "JOH_BYZ.txt", "AC_BYZ.txt",
            "RO_BYZ.txt",  "1CO_BYZ.txt", "2CO_BYZ.txt", "GA_BYZ.txt",  "EPH_BYZ.txt", "PHP_BYZ.txt", "COL_BYZ.txt",
            "1TH_BYZ.txt", "2TH_BYZ.txt", "1TI_BYZ.txt", "2TI_BYZ.txt", "TIT_BYZ.txt", "PHM_BYZ.txt", 
            "HEB_BYZ.txt", "JAS_BYZ.txt", "1PE_BYZ.txt", "2PE_BYZ.txt", "1JO_BYZ.txt", "2JO_BYZ.txt", "3JO_BYZ.txt", 
            "JUDE_BYZ.txt", "RE_BYZ.txt" };
    
    // We know our book number and localized name already
    unsigned int bookidx = booknum - 40;
    if ((bookidx < 0) ||  (bookidx > 26)) {
      cerr <<  "ERROR: booknumber out of range: " <<  booknum <<  endl;
    }
   
    //  From utilities.cpp
    ReadText rt(Directories->get_package_data() + "/bibles/byzascii/" + filenames[bookidx], /*silent*/false, /*trimming*/true);
    unsigned int currchapnum = 0;
    chapter *currchap = NULL;
    
    //  builds the chapters verse by verse
    for (auto &it: rt.lines) {
         //  Extract chapter and verse,  and leading space. The lines are always
         //  well formed: 1:5<space>Verse text.
         size_t colonposition = it.find_first_of(":");
	 // substr takes start position and length
         ustring chapstring = it.substr(0,  colonposition);
         unsigned int chapnum = convert_to_int(chapstring);
         size_t spaceposition = it.find_first_of(" ");
         ustring versestring = it.substr(colonposition+1, spaceposition-colonposition-1);
         unsigned int versenum = convert_to_int(versestring);
         it.erase(0,  spaceposition+1);
         if (chapnum != currchapnum) {
             chapter *newchap = new chapter(this,  chapnum);
             chapters.push_back(newchap);
             currchap = newchap;
             currchapnum = chapnum;
             //cerr << "Created new chapter " << chapnum << " from " << filenames[bookidx] << endl;
         }
         byzasciiConvert(it);
         verse *newverse = new verse(currchap, versenum, it); //  takes a copy of the ustring text (it)
         currchap->verses.push_back(newverse); //  append verse to current chapter
         //newverse->print(); // debug
    }
}

// Load SBL Greek NT Text from shared resource directory
void book_sblgnt::load(void)
{
    ustring filenames[27] = { "61-Mt.txt", "62-Mk.txt", "63-Lk.txt", "64-Jn.txt", "65-Ac.txt", 
                              "66-Ro.txt", "67-1Co.txt", "68-2Co.txt", "69-Ga.txt", "70-Eph.txt", "71-Php.txt", 
                              "72-Col.txt", "73-1Th.txt", "74-2Th.txt", "75-1Ti.txt", "76-2Ti.txt", "77-Tit.txt", 
                              "78-Phm.txt", "79-Heb.txt", "80-Jas.txt", "81-1Pe.txt", "82-2Pe.txt", "83-1Jn.txt", 
                              "84-2Jn.txt", "85-3Jn.txt", "86-Jud.txt", "87-Re.txt" };
    
    // We know our book number and localized name already
    unsigned int bookidx = booknum - 40;
    if ((bookidx < 0) ||  (bookidx > 26)) {
      cerr <<  "ERROR: booknumber out of range: " <<  booknum <<  endl;
    }
   
    //  From utilities.cpp
    ReadText rt(Directories->get_package_data() + "/bibles/sblgnt/" + filenames[bookidx], /*silent*/false, /*trimming*/true);
    unsigned int currchapnum = 0;
    chapter *currchap = NULL;
    int linecnt = 0;
    
    //  builds the chapters verse by verse
    for (auto &it: rt.lines) {
         // First line is title of book, we don't need them
         if (linecnt == 0) { linecnt++; continue; }
         //  Extract chapter and verse,  and leading space. The lines are always
         //  well formed: Book<space>1:5<tab>Verse text.
         //  Remove book name from the line
         size_t spaceposition = it.find_first_of(" ");
         it.erase(0,  spaceposition+1);
         size_t colonposition = it.find_first_of(":");
         ustring chapstring = it.substr(0,  colonposition);
         unsigned int chapnum = convert_to_int(chapstring);
         size_t tabposition = it.find_first_of("\t");
         ustring versestring = it.substr(colonposition+1, tabposition-colonposition-1);
         unsigned int versenum = convert_to_int(versestring);
         it.erase(0, tabposition+1);
         if (chapnum != currchapnum) {
             chapter *newchap = new chapter(this,  chapnum);
             chapters.push_back(newchap);
             currchap = newchap;
             currchapnum = chapnum;
             //cerr << "Created new chapter " << chapnum << " from " << filenames[bookidx] << endl;
         }
         verse *newverse = new verse(currchap, versenum, it); //  takes a copy of the ustring text (it)
         currchap->verses.push_back(newverse); //  append verse to current chapter
         //newverse->print(); // debug
    }
}

// Load SBL Greek NT Text from shared resource directory
void book_sblgntapp::load(void)
{
    ustring filenames[27] = { "61-Mt-APP.txt", "62-Mk-APP.txt", "63-Lk-APP.txt", "64-Jn-APP.txt",
                              "65-Ac-APP.txt", "66-Ro-APP.txt", "67-1Co-APP.txt", "68-2Co-APP.txt",
                              "69-Ga-APP.txt", "70-Eph-APP.txt", "71-Php-APP.txt", "72-Col-APP.txt",
                              "73-1Th-APP.txt", "74-2Th-APP.txt", "75-1Ti-APP.txt", "76-2Ti-APP.txt",
                              "77-Tit-APP.txt", "78-Phm-APP.txt", "79-Heb-APP.txt", "80-Jas-APP.txt",
                              "81-1Pe-APP.txt", "82-2Pe-APP.txt", "83-1Jn-APP.txt", "84-2Jn-APP.txt",
                              "85-3Jn-APP.txt", "86-Jud-APP.txt", "87-Re-APP.txt" };

    // We know our book number and localized name already
    unsigned int i = 0;
    unsigned int bookidx = booknum - 40;
    if ((bookidx < 0) ||  (bookidx > 26)) {
      cerr <<  "ERROR: booknumber out of range: " <<  booknum <<  endl;
    }

    //  From utilities.cpp
    ReadText rt(Directories->get_package_data() + "/bibles/sblgnt/" + filenames[bookidx], /*silent*/false, /*trimming*/true);
    unsigned int currchapnum = 0;
    unsigned int currversenum = 0;
    chapter *currchap = NULL;
    int linecnt = 0;

    //  builds the chapters verse by verse
    for (auto &it: rt.lines) {
         // First line is title of book, we don't need them
         if (linecnt == 0) { linecnt++; continue; }
         //  Extract chapter and verse,  and leading space. The lines are always
         //  well formed: Book<space>1:5<tab>Verse text.
         //  Remove book name from the line
         size_t spaceposition = it.find_first_of(" ");
         it.erase(0,  spaceposition+1);
         size_t colonposition = it.find_first_of(":");
         ustring chapstring = it.substr(0,  colonposition);
         unsigned int chapnum = convert_to_int(chapstring);
         size_t tabposition = it.find_first_of("\t");
         ustring versestring = it.substr(colonposition+1, tabposition-colonposition-1);
         unsigned int versenum = convert_to_int(versestring);
         it.erase(0, tabposition+1);
         if ((chapnum != currchapnum) && (currchapnum-chapnum > 0)) {
           for (i=currchapnum+1; i<=chapnum; i++) {
             chapter *newchap = new chapter(this, i);
             chapters.push_back(newchap);
             currchap = newchap;
           }
           currchapnum = chapnum;

           cerr << "Created new chapter " << chapnum << " from " << filenames[bookidx] << endl;
         }

         if ((versenum != currversenum) && (versenum-currversenum>0)) {
           if (versenum-currversenum>1) {
           for (i=currversenum+1; i<versenum; i++) {
             verse *newverse = new verse(currchap, i, ustring("No apparatus notes for this verse")); //  takes a copy of the ustring text (it)
             currchap->verses.push_back(newverse); //  append verse to current chapter
             newverse->print(); // debug
           }
           }
           verse *newverse = new verse(currchap, versenum, it); //  takes a copy of the ustring text (it)
           currchap->verses.push_back(newverse); //  append verse to current chapter
           currversenum = versenum;

           cerr << "Created new verse " << versenum << " from " << filenames[bookidx] << endl;
         }
    }
}

// Load English Majority Text version from shared resource directory
void book_engmtv::load(void)
{
    ustring filenames[27] = { "Mat.txt", "Mar.txt", "Luk.txt", "Joh.txt", "Act.txt", 
                              "Rom.txt", "1Co.txt", "2Co.txt", "Gal.txt", "Eph.txt", "Phi.txt", 
                              "Col.txt", "1Th.txt", "2Th.txt", "1Ti.txt", "2Ti.txt", "Tit.txt", 
                              "Phm.txt", "Heb.txt", "Jam.txt", "1Pe.txt", "2Pe.txt", "1Jn.txt", 
                              "2Jn.txt", "3Jn.txt", "Jud.txt", "Rev.txt" };

    // We know our book number and localized name already
    unsigned int bookidx = booknum - 40;
    if ((bookidx < 0) ||  (bookidx > 26)) {
      cerr <<  "ERROR: booknumber out of range: " <<  booknum <<  endl;
    }
   
    //  From utilities.cpp
    ReadText rt(Directories->get_package_data() + "/bibles/engmtv/" + filenames[bookidx], /*silent*/false, /*trimming*/true);
    unsigned int currchapnum = 0;
    chapter *currchap = NULL;

    //  builds the chapters verse by verse
    for (auto &it: rt.lines) {
         //  Extract chapter and verse,  and leading space. The lines are always
         //  well formed: Book<space>1:5<space>Verse text.
         //  Remove book name from the line
         size_t spaceposition = it.find_first_of(" ");
         it.erase(0,  spaceposition+1);
         size_t colonposition = it.find_first_of(":");
         ustring chapstring = it.substr(0,  colonposition);
         unsigned int chapnum = convert_to_int(chapstring);
         size_t tabposition = it.find_first_of(" ");
         ustring versestring = it.substr(colonposition+1, tabposition-colonposition-1);
         unsigned int versenum = convert_to_int(versestring);
         it.erase(0, tabposition+1);
         if (chapnum != currchapnum) {
             chapter *newchap = new chapter(this,  chapnum);
             chapters.push_back(newchap);
             currchap = newchap;
             currchapnum = chapnum;
             //cerr << "Created new chapter " << chapnum << " from " << filenames[bookidx] << endl;
         }
         verse *newverse = new verse(currchap, versenum, it); //  takes a copy of the ustring text (it)
         currchap->verses.push_back(newverse); //  append verse to current chapter
         //newverse->print(); // debug
    }
}

// Load Lexham English Bible text from shared resource directory
void book_leb::load(void)
{
    ustring filenames[66] = {
        "Gen.txt",  "Exo.txt",  "Lev.txt",  "Num.txt",   "Deut.txt", "Josh.txt", "Judg.txt",
        "Ruth.txt", "1Sa.txt",  "2Sa.txt",  "1Ki.txt",   "2Ki.txt",  "1Ch.txt",  "2Ch.txt",
        "Ezra.txt", "Neh.txt",  "Esth.txt", "Job.txt",   "Psal.txt", "Prov.txt", "Eccl.txt",
        "Song.txt", "Isa.txt",  "Jer.txt",  "Lam.txt",   "Ezek.txt", "Dan.txt",  "Hos.txt",
        "Joel.txt", "Amos.txt", "Obad.txt", "Jonah.txt", "Mic.txt",  "Nah.txt",  "Hab.txt", 
        "Zeph.txt", "Hag.txt",  "Zech.txt", "Mal.txt",   "Matt.txt", "Mark.txt", "Luke.txt",
        "John.txt", "Acts.txt", "Rom.txt",  "1Co.txt",   "2Co.txt",  "Gal.txt",  "Eph.txt", 
        "Php.txt",  "Col.txt",  "1Th.txt",  "2Th.txt",   "1Tim.txt", "2Tim.txt", "Tit.txt", 
        "Phm.txt",  "Heb.txt",  "Jam.txt",  "1Pe.txt",   "2Pe.txt",  "1Jn.txt",  "2Jn.txt", 
        "3Jn.txt", "Jude.txt", "Rev.txt" };

    // In Isaiah 22:5, LEB has a note that contains some transliterated Hebrew. This causes a Glib::ConvertError exception to 
    // be thrown, but only in Windows. This was a known problem back several years ago. The recommended solution is to include <locale> 
    // and to run the following line of code. Refer to https://mail.gnome.org/archives/gtkmm-list/2012-November/msg00021.html and 
    // https://bugzilla.gnome.org/show_bug.cgi?id=661588
    //std::locale::global(std::locale(""));
    //setlocale(LC_ALL, "");
    // Further notes: testing indicated that the line was read in correctly, but the problem came in printing it to cout/cerr.
    // Therefore I am going to try to avoid the problem by stopping the print of debug info below (newverse->print())
    // as well as in verse::append and verse::prepend. That will avoid exposing the bug.

    //  bookidx points into the filenames[] array. It has to start at zero. booknum is from 1-66 (or more for apocrypha)
    unsigned int bookidx = booknum - 1; 
    // We know our book number and localized name already
    if ((bookidx < 0) ||  (bookidx > 65)) {
      cerr <<  "ERROR: booknumber out of range: " <<  booknum <<  endl;
    }
    
    //  From utilities.cpp
    ReadText rt(Directories->get_package_data() + "/bibles/engleb/" + filenames[bookidx], /*silent*/false, /*trimAll*/false, /*trimEnd*/true);
    unsigned int currchapnum = 0;
    chapter *currchap = NULL;
    int linecnt = 0;
    ustring psalmTitle = "";
    
    //  builds the chapters verse by verse
    for (auto &it: rt.lines) {
         // First 3 lines are title of book with dashed lines above and below, we don't need them
         if (linecnt <= 3) { linecnt++; continue; }
         if (it.size() == 0) { continue; }                 //  ignore blank lines
         // ignore lines that are like CHAPTER 1
         if (it.find("CHAPTER ") !=  ustring::npos) { continue; }
             //  The above I could parameterize somehow and make these routines share code.
         //  Extract chapter and verse,  and leading space. The lines are ALMOST always
         //  well formed: Book<space>1:5<tab>Verse text. But look at John 1:23, for instance:
         // Jn 1:23 He said,
         //  "I am...
         //  just as Isaiah...
         // So we have a quotation. This occurs about 300 times in the entire LEB. these lines
         // always start with a space.
         if ((it[0] == ' ') || (it[0] == '\t')) {
             // This is a special case: attach it to the prior verse.
             currchap->appendToLastVerse(it);
         }
         else {
             //  Remove book name from the line
             size_t spaceposition = it.find_first_of(" ");
             it.erase(0,  spaceposition+1);
             size_t colonposition = it.find_first_of(":");
             ustring chapstring = it.substr(0,  colonposition);
             unsigned int chapnum = convert_to_int(chapstring);
             size_t tabposition = it.find_first_of("\t");
             ustring versestring = it.substr(colonposition+1, tabposition-colonposition-1);
             unsigned int versenum = convert_to_int(versestring);
             it.erase(0, tabposition+1);
             if (chapnum != currchapnum) {
                 chapter *newchap = new chapter(this,  chapnum);
                 chapters.push_back(newchap);
                 currchap = newchap;
                 currchapnum = chapnum;
                 //cerr << "Created new chapter " << chapnum << " from " << filenames[bookidx] << endl;
             }
             if (versestring == "title") {
                  psalmTitle = it;
             }
             else {
               verse *newverse = new verse(currchap, versenum, it); //  takes a copy of the ustring text (it)
               currchap->verses.push_back(newverse); //  append verse to current chapter
               if (!psalmTitle.empty()) { currchap->prependToLastVerse(psalmTitle+"\n"); psalmTitle.clear(); }
               //newverse->print(); // debug
             }
         }
    }
}

void book::load(void)
{
  //  This stub can't do anything because it has to know exactly what kind of book it is loading
  //  This would argue that the book base class be abstract only,  and not be able to have any
  // instantiations of it.
}

bool bible::validateBookNum(const unsigned int booknum)
{ // maybe should make this a do-nothing, or bible as pure virtual
  if ((booknum < 1) || (booknum > 66)) { return false; }
  return true;
}

bool bible_byz::validateBookNum(const unsigned int booknum)
{
  if ((booknum < 40) || (booknum > 66)) { return false; }
  return true;
}

bool bible_sblgnt::validateBookNum(const unsigned int booknum)
{
  if ((booknum < 40) || (booknum > 66)) { return false; }
  return true;
}

bool bible_sblgntapp::validateBookNum(const unsigned int booknum)
{
  if ((booknum < 40) || (booknum > 66)) { return false; }
  return true;
}

bool bible_engmtv::validateBookNum(const unsigned int booknum)
{
  if ((booknum < 40) || (booknum > 66)) { return false; }
  return true;
}

bool bible_leb::validateBookNum(const unsigned int booknum)
{
  if ((booknum < 1) || (booknum > 66)) { return false; }
  return true;
}

//  There has to be a better name for this method
void bible::check_book_in_range(unsigned int booknum)
{
    // Is this book number in the range that we have already created in our vector? If not, 
    // resize the vector.
    if (booknum >= books.size()) {
        // Suppose book = Genesis, #1, first time around. books.size() == 1 becuase
        // books[0] exists, but is unused. So we resize books to 2 elements.
      books.resize(booknum+1);
    }
}

book *bible::createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum)
{ return NULL; } // do nothing...can't really instantiate a base class bible

book *bible_byz::createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum)
{ return new book_byz(this, _bookname,  _booknum); }

book *bible_sblgnt::createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum)
{ return new book_sblgnt(this, _bookname,  _booknum); }

book *bible_sblgntapp::createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum)
{ return new book_sblgntapp(this, _bookname,  _booknum); }

book *bible_engmtv::createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum)
{ return new book_engmtv(this, _bookname,  _booknum); }

book *bible_leb::createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum)
{ return new book_leb(this, _bookname,  _booknum); }

// This knows how to retrieve a verse from any bible, i.e. those who have inherited from the base bible class
ustring bible::retrieve_verse(const Reference &ref)
{
    unsigned int booknum = ref.book_get();
    // 1. Does this Bible support this book? For instance, SBLGNT only has books 40-66.
    if (!validateBookNum(booknum)) { return "Book doesn't exist"; }
        
    check_book_in_range(booknum);
    
    // 2. Have we already loaded this book? If not, load it and save it for next time around
    if (books[booknum] == NULL) {
        // createNewBook(...) is specialized in each particular bible class, since it knows
        // what kind of book it needs. So whatever type of bible "this" is, it will create
        // the right kind of book for that bible.
        books[booknum] = createNewBook(this, books_id_to_localname(booknum),  booknum);
        books[booknum]->load();
    }
    return books[booknum]->retrieve_verse(ref); 
}

void book::check_chapter_in_range(unsigned int chapnum)
{
    // Is this chapter number in the range that we have already created in our vector? If not, 
    // resize the vector.
    if (chapnum >= chapters.size()) {
        // Suppose book = Genesis, #1, first time around. books.size() == 1 becuase
        // books[0] exists, but is unused. So we resize books to 2 elements.
      chapters.resize(chapnum+1);
    }
}

void chapter::check_verse_in_range(unsigned int vsnum)
{
    // Is this verse number in the range that we have already created in our vector? If not, 
    // resize the vector.
    if (vsnum >= verses.size()) {
        // Suppose book = Genesis, #1, first time around. books.size() == 1 becuase
        // books[0] exists, but is unused. So we resize books to 2 elements.
      verses.resize(vsnum+1);
    }
}

bible_bixref::bible_bixref(const ustring &_proj) : bible(_proj, "NO FONT")
{
  // Nothing else for now.
}

bible_bixref::~bible_bixref()
{
  // nothing to do more than what the base class destructor does
}

vector<unsigned int> *bible_bixref::retrieve_xrefs(const Reference &ref)
{
    // By construction I know that these casts are valid. I also know that
  // every book of the Bible has been created, but not necessarily every
  // chapter or verse has a cross-reference in it.
    book_bixref *bk = static_cast<book_bixref *>(books[ref.book_get()]);
    unsigned int chnum = ref.chapter_get();
    if ((chnum < 1) || (chnum >= bk->chapters.size())) {
	    return NULL;
    }
	
    chapter *ch = bk->chapters[ref.chapter_get()];
    unsigned int vsnum = ref.verse_get_single();
    if ((vsnum < 1) || (vsnum >= ch->verses.size())) {
	    return NULL;
	}
    verse_xref *vs = static_cast<verse_xref *>(ch->verses[vsnum]);
    if (vs == NULL ) { return NULL; }
	return &(vs->xrefs);
}
