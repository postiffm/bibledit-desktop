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

#include "concordance.h"
#include <glib/gi18n.h>
#include "progresswindow.h"
#include "directories.h"

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

book_leb::book_leb(bible *_bbl, const ustring &_bookname, unsigned int _booknum): book(bbl,  _bookname,  _booknum)
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
    ustring delims(" \\,:;!?.\u0022()[]¶\t«»\u201c\u201d"); //  \u0022 is quotation mark; «» are Alt-0171 and 0187; \u201c\u201d are smart double quotes
	ustring nonUSFMdelims(" ,:;!?.\u0022()[]¶\t«»\u201c\u201d");        // same list as above except without '\'
	// u0022 is unicode double-quote mark
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

bible::bible(const ustring &_proj) : books(1)
{
    // Note: books[0] is unused; simply here to avoid the "off by one" indexing error
    projname = _proj;
}

bible_byz::bible_byz(const ustring &_proj) : bible (_proj)
{
  // Nothing special to do here   
}

bible_sblgnt::bible_sblgnt(const ustring &_proj) : bible (_proj)
{
  // Nothing special to do here   
}

bible_leb::bible_leb(const ustring &_proj) : bible (_proj)
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

void Concordance::readExcludedWords(const ustring &filename)
{
	// The file is expected to have unicode-encoded strings, one per line
	ifstream myfile;
	string line;
	myfile.open("strings.txt");
	// eventually change to Glib::IOChannel::read_line for unicode
	while (getline(myfile, line)) {
		//cout << "Found word " << line << endl;
		excludedWords.insert(line);
	}
	myfile.close();
	// Iterate over the set and print each one out, to double-check iter_swap
	cout << "Excluded words are:" << endl;
	for (auto &word : excludedWords) {
		cout << "\t" << word << endl;
	}
}

// I should have a better way of accessing this
extern book_record books_table[];

Concordance::Concordance(const ustring &_projname) : bbl(_projname)
{
    projname = _projname;
	readExcludedWords("strings.txt");
	
	// The kinds of things we should be able to do include
	// 1. Build a sorted list of words with frequency counts (DONE 5/27/2016)
	// 2. Attach each verse ref to the words for quickly navigating to where they occur (DONE 2/15/2018)
	// 3. Use exclude list to exclude common words in building an actual concordance (sort of done 5/27/2016)
	// 4. Use important verse list to only include those verses that are deemed important
	// 5. Build actual concordance with verse portions
}

Concordance::~Concordance()
{
  clear();   
}

void Concordance::clear(void)
{
  projname = "";
  bbl.clear();
  wordCounts.clear();
  wordLocations.clear();
  sortedWordCounts.clear();
}

//  Given a list of encoded verses (a bitmap,  basically), write out html for the verses,  
//  like this: " Genesis 1:6 Genesis 1:29... Only write the first num verses,  because
//  there could be thousands.
void Concordance::writeVerseLinks(unsigned int num, vector<int> &locations, HtmlWriter2 &htmlwriter)
{
    unsigned int i = 0;
    for (const auto &ref : locations) {
        int bk = ref >> 16;
        int ch = (ref >> 8) & 0xff;
        int vs = ref & 0xff;
        ustring address = books_id_to_english(bk) + " " + std::to_string(ch) + ":" + std::to_string(vs);
        htmlwriter.text_add (" ");
        htmlwriter.hyperlink_add ("goto " + address, address);
        // I could also std::to_string(ref) instead of address,  above,  but then I'd have to decode it in other places
        //  that aren't aware of this compressed bitmap datatype.
        i++;
        if (i >= num) { break; }                            // just print the first two refs on this summary screen
    }
    if (locations.size() > num) { htmlwriter.text_add("..."); }
}

void Concordance::writeAlphabeticSortedHtml(HtmlWriter2 &htmlwriter)
{
    // Now do the work of loading the chapters and verses, splitting into words, 
    // counting in our mapping structure, etc.
//  TO DO: FACTOR OUT THE WORK AND JUST KEEP THE WRITING IN THIS METHOD
    htmlwriter.heading_open (1);
    htmlwriter.text_add (projname + " " + _("Concordance Sorted by Words"));
    htmlwriter.heading_close();

    ProgressWindow progresswindow(_("Building Concordance Data"), false);
    progresswindow.set_iterate(0, 1, 66);
    
    for (int b = 1; b <= 66; b++) {
		ustring bookname = books_table[b].name;
		book *newbk = new book(&bbl, bookname, b);
		bbl.books.push_back(newbk);
		vector <unsigned int> chapters = versification_get_chapters("English", b);
		for (auto c : chapters) {
			chapter *newchap = new chapter(newbk, c);
			newbk->chapters.push_back(newchap);
			newchap->load(b, c, wordCounts, wordLocations);
		}
		progresswindow.iterate();
	}

	unsigned int totalWords = 0;
	unsigned int uniqueWords = 0;

	for (const auto &pair : wordCounts) {
		uniqueWords++;
		totalWords+=pair.second;
		// Add the word/count to a map so I can print it sorted. The reason I 
		// did not use a regular map to begin with is that it has O(n) access
		// time, and I wanted O(1) for speed in the gathering stage of the 
		// algorithm.
		sortedWordCounts[pair.first] = pair.second;
	}
	
	htmlwriter.paragraph_open ();
	htmlwriter.text_add(_("Unique words = ") + std::to_string(uniqueWords));
    htmlwriter.paragraph_close ();
    htmlwriter.paragraph_open ();
    htmlwriter.text_add(_("Total words = ") + std::to_string(totalWords));
    htmlwriter.paragraph_close ();

    for (const auto &pair : sortedWordCounts) {
        std::vector<int> &locations = wordLocations[pair.first];
        htmlwriter.paragraph_open();
        if (locations.size() > 2) {                         //  COMMON CODE BELOW....COULD COMBINE
            //  Write the word as a hyperlink so that the user can click it
            //  to look at the entire list of uses in a new tab.
            htmlwriter.hyperlink_add ("concordance " + pair.first,  pair.first);
            htmlwriter.text_add(" " + std::to_string(pair.second));
          }
        else { 
            //  if the word only has one or two uses,  then it is unecessary
            //  to provide a link,  since the uses will be printed right next to it.
            htmlwriter.text_add(pair.first + " " + std::to_string(pair.second));
          }
        writeVerseLinks(2, locations, htmlwriter);
        htmlwriter.paragraph_close();
      }

    // Done generating word list with handful of verses for each
    return;
}

void Concordance::writeFrequencySortedHtml(HtmlWriter2 &htmlwriter)
{
  // Now I want to sort by count so that 1's appear at the top, etc.

  htmlwriter.heading_open (1);
  htmlwriter.text_add (projname + " " + _("Concordance Sorted by Frequency"));
  htmlwriter.heading_close();
  
  // Got this idea from https://stackoverflow.com/questions/2699060/how-can-i-sort-an-stl-map-by-value
  multimap<int, string> mm;
  for (auto const &kv : sortedWordCounts) {
    mm.insert(make_pair(kv.second, kv.first)); // flip the pairs so we key off of counts
  }
  
  // Done reversing key<->count and sorting by word count
  
  for (auto const &kv : mm) {
      std::vector<int> &locations = wordLocations[kv.second];
      htmlwriter.paragraph_open();
      // print them in "normal" order
      if (locations.size() > 2) {
        //  Write the word as a hyperlink so that the user can click it
        //  to look at the entire list of uses in a new tab.
        htmlwriter.hyperlink_add ("concordance " + kv.second,  kv.second);
        htmlwriter.text_add(" " + std::to_string(kv.first));
      }
      else { 
          //  if the word only has one or two uses,  then it is unecessary
          //  to provide a link,  since the uses will be printed right next to it.
          htmlwriter.text_add(kv.second + " " + std::to_string(kv.first));
      }
      writeVerseLinks(2, locations, htmlwriter);
      htmlwriter.paragraph_close();
  }
  return;
}

void Concordance::writeVerses(vector<int> &locations, HtmlWriter2 &htmlwriter)
{
  // Get data about the project.
  extern Settings *settings;
  ustring project = settings->genconfig.project_get();
  
    for (const auto &ref : locations) {
        unsigned int bk = ref >> 16;
        unsigned int ch = (ref >> 8) & 0xff;
        unsigned int vs = ref & 0xff;
        ustring address = books_id_to_english(bk) + " " + std::to_string(ch) + ":" + std::to_string(vs);
        cout << "Writing verse >> " <<  address <<  endl;
        htmlwriter.paragraph_open();
        htmlwriter.hyperlink_add ("goto " + address, address);
        
        //  Now,  the text of the verse...
        Reference bibleref(bk,  ch,  vs);
        ustring verse = project_retrieve_verse(project, bibleref);
        if (verse.empty()) {
          verse.append(_("<empty>"));
        } else {
          replace_text(verse, "\n", " ");
          CategorizeLine cl(verse);
          cl.remove_verse_number(bibleref.verse_get());
          verse = cl.verse;
          htmlwriter.text_add(" " + verse);
        }
        htmlwriter.paragraph_close();
    }
}

//  This is writes the word list out, with verse text.
void Concordance::writeSingleWordListHtml(const ustring &word,  HtmlWriter2 &htmlwriter)
{
    htmlwriter.heading_open (1);
    htmlwriter.text_add (projname + " " + _("Concordance Entry for ") + word);
    htmlwriter.heading_close();

    std::vector<int> &locations = wordLocations[word];
    writeVerses(locations, htmlwriter);
}

//  A small set of important Bibles is pre-stored within the Bibledit-Desktop package. These 
//  are shown in the Reference Bibles window as an aid to the translator.
//  Goal is to have BYZ (unaccented) first, then maybe NET, SBLGNT, ASV,  ?
//  Second goal is to include another tab for translator notes from the NET or other
//  resources.

ReferenceBibles::ReferenceBibles() : bibles(10)
{
  bibles[0] = new bible_byz("BYZ");                         //  Byzantine Majority Text,  Pierpont/Robinson
  bibles[1] = new bible_sblgnt("SBL");                      //  SBL Greek NT
  bibles[2] = new bible_leb("LEB");                         //  Lexham English Bible
  // I have room for 7 other Bibles at the moment; see above constructor line
}

ReferenceBibles::~ReferenceBibles()
{
    // Not much to do
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
    cout << "Appended [[" << addlText << "]] to " "verse " << vsnum << endl;
}

void verse::prepend(const ustring &addlText)
{
    text = addlText + text;
    cout << "Prepended [[" << addlText << "]] to " "verse " << vsnum << endl;
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
                 cerr << "Created new chapter " << chapnum << " from " << filenames[bookidx] << endl;
             }
             if (versestring == "title") {
                  psalmTitle = it;
             }
             else {
               verse *newverse = new verse(currchap, versenum, it); //  takes a copy of the ustring text (it)
               currchap->verses.push_back(newverse); //  append verse to current chapter
               if (!psalmTitle.empty()) { currchap->prependToLastVerse(psalmTitle+"\n"); psalmTitle.clear(); }
               newverse->print(); // debug
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


ustring bible_byz::retrieve_verse(const Reference &ref)
{
    unsigned int booknum = ref.book_get();
    // 1. Does this Bible support this book? SBLGNT only has books 40-66.
    if ((booknum < 40) || (booknum > 66)) { return "Book doesn't exist"; }
        
    check_book_in_range(booknum);
    
    //  NOTICE SIMILARITY THIS METHOD TO NEXT,  same except new book_byz statement
    
    // 2. Have we already loaded this book? If not, load it and save it for next time around
    if (books[booknum] == NULL) {
        books[booknum] = new book_byz((bible*)this, books_id_to_localname(booknum),  booknum);
        books[booknum]->load();
    }
    return books[booknum]->retrieve_verse(ref);
}

ustring bible_sblgnt::retrieve_verse(const Reference &ref)
{
    unsigned int booknum = ref.book_get();
    // 1. Does this Bible support this book? SBLGNT only has books 40-66.
    if ((booknum < 40) || (booknum > 66)) { return "Book doesn't exist"; }
        
    check_book_in_range(booknum);

    // 2. Have we already loaded this book? If not, load it and save it for next time around
    if (books[booknum] == NULL) {
        books[booknum] = new book_sblgnt(this, books_id_to_localname(booknum),  booknum);
        books[booknum]->load();
    }
    return books[booknum]->retrieve_verse(ref);
}
//  Remember,  books[0] is unused,  so books[1..66] are valid
ustring bible_leb::retrieve_verse(const Reference &ref)
{
    unsigned int booknum = ref.book_get();
    // 1. Does this Bible support this book? LEB has books 1-66.
    if ((booknum < 1) || (booknum > 66)) { return "Book doesn't exist"; }
        
    check_book_in_range(booknum);

    // 2. Have we already loaded this book? If not, load it and save it for next time around
    if (books[booknum] == NULL) {
        books[booknum] = new book_leb(this, books_id_to_localname(booknum),  booknum);
        books[booknum]->load();
    }
    return books[booknum]->retrieve_verse(ref);
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

// This virtual method should probably be abstract,  making the class as a whole abstract...
ustring bible::retrieve_verse(const Reference &ref)
{
   return "Unknown Bible type in retrieve_verse";
}

void ReferenceBibles::write(const Reference &ref,  HtmlWriter2 &htmlwriter)
{
    for (auto it : bibles) {
        bible *b = it;
        if (b == NULL) { break; }
        ustring verse;
        try {
          verse = b->retrieve_verse(ref);
        }
        catch (std::runtime_error &e) {
          htmlwriter.paragraph_open();
          verse.append(e.what());
          htmlwriter.paragraph_close();
        }
        //  I assume the following post-condition to the above statement:
        //  the verse is loaded into the bible,  book,  chapter,  and the
        //  the verse contains only the text of the verse,  no x:y prefix
        if (verse.empty()) {
            htmlwriter.paragraph_open();
            verse.append(_("<empty>"));
            htmlwriter.paragraph_close();
        }
        else {
              replace_text(verse, "\n", " ");
              htmlwriter.paragraph_open();
              htmlwriter.text_add(b->projname + " " + books_id_to_localname(ref.book_get()) + " " + std::to_string(ref.chapter_get()) + ":" + ref.verse_get());
              htmlwriter.font_open("Symbol");
              htmlwriter.text_add(verse);
              htmlwriter.font_close();
            }
        htmlwriter.paragraph_close();
      }
}
