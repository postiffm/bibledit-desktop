/*
 ** Copyright (©) 2018-2024 Matt Postiff.
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
#include <locale>
#include "gtkwrappers.h"

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

Concordance::Concordance(const ustring &_projname) : bbl(_projname, "DummyFont")
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

//  Given a list of encoded verses (each one a bitmap,  basically), write out html for the verses,  
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

// Find all occurrences of boldstr in src, and insert <b>...</b> around them
void Concordance::boldface(ustring src, const ustring &boldstr, HtmlWriter2 &htmlwriter)
{
    // Make sure you find different capitalizations; and interpret <b> properly.
    // Walk the string and print out its parts, inserting <b> and </b> as needed.
    ustring::size_type idx1 = 0, idx2 = 0;
    ustring srcLower = src.lowercase();
    while (idx1 != ustring::npos) {
      // Idea of this code is to walk srcLower to match the lower-case strings
      // stored in the concordance, and then extract the same-indexed portions
      // from the original string so the original capitalization remains.
      idx2 = srcLower.find(boldstr, /*len*/idx1);
      ustring part;
      if (idx2 != ustring::npos) {
          part = src.substr(idx1, idx2-idx1);
          htmlwriter.text_add(part);
          htmlwriter.bold_open();
          idx1 = idx2;
          idx2 = idx2 + boldstr.size();
          part = src.substr(idx1, /*len*/idx2-idx1);
          htmlwriter.text_add(part);
          htmlwriter.bold_close();
          idx1 = idx2;
      }
      else { /* idx2 is at end of string */
        part = src.substr(idx1/*, len=npos*/);
        htmlwriter.text_add(part);
        idx1 = idx2;
      }
    }
    // There is one known problem with this, and that is that if I have, say "said to my LORD,..."
    // then when the htmlwriter inserts the bold code, I get "said to my LORD<space>,..."
    // and I don't know why that space is there. Perhaps something with webkit?
}

void Concordance::writeVerses(vector<int> &locations, const ustring &word, HtmlWriter2 &htmlwriter)
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
          boldface(verse, word, htmlwriter); // put in bold every time the word occurs
          //htmlwriter.text_add(" " + verse);
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
    writeVerses(locations, word, htmlwriter);
}
