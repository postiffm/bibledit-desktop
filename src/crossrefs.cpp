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

#include "crossrefs.h"
#include <glib/gi18n.h>
#include "progresswindow.h"
#include "directories.h"
#include <locale>
#include "gtkwrappers.h"

// I should have a better way of accessing this
extern book_record books_table[];

CrossReferences::CrossReferences()
{
    // Load the whole set of cross references. It is a lot of data--over half a megabyte
    // of memory, but the way it is stored in binary format (see linux/buildcrf.pl and 
    // linux/readcrf.pl) makes it fairly quick.
    crossref_file = Directories->get_package_data() + "/bibles/bi.crf";
    ReadBinary rb(crossref_file);
    uint32_t *dataptr = rb.get_data();
    unsigned int num32BitWords = rb.get_num32BitWords();
    
    if ((dataptr == 0x0) || (num32BitWords == 0)) { 
        // There was a problem opening the file
        bbl = 0x0;
        gtkw_dialog_error(NULL, _("Cannot open file ") + crossref_file);
        return;
    }
    
    bbl = new bible_bixref("BI_XREF");
    
    // Create empty books right now, since we know we are going to need them
    bbl->books.resize(67, 0x0);
    unsigned int b;
    for (b = 1; b <= 66; b++) {
        ustring bookname = books_table[b].name;
        bbl->books[b] = new book_bixref(bbl, bookname, b);
    }
    
    unsigned int i;
    bool startNewList = true;
    book *bk = NULL;
    chapter *ch = NULL;
    verse_xref *vs = NULL;

    ProgressWindow progresswindow (_("Loading cross-reference data"), false);
    progresswindow.set_iterate (0, 1, num32BitWords>>10); // shifting by 10 is dividing by appx 1000 (1024)

    for (i = 0; i < num32BitWords; i++) {
        // Logic is this: whenever last 10 bits of counter i are zero, iterate
        if ((i&0x3FF) == 0) { progresswindow.iterate(); }
        unsigned int encoded = *dataptr;
        dataptr++; // should advance by 4 bytes for next time around
        if (encoded == 0) { startNewList = true; continue; }
        
        if (startNewList) {
            Reference ref(encoded);
            bk = bbl->books[ref.book_get()];
            bk->check_chapter_in_range(ref.chapter_get());
            ch = bk->chapters[ref.chapter_get()];
            if (ch == 0) {
                // First time we are encountering this chapter. We need
                // to create it. 
                ch = new chapter(bk, ref.chapter_get());
                bk->chapters[ref.chapter_get()] = ch;
            }
            // As designed, the "starter" verse only appears
            // one time. Therefore, when we come to it, we know we need to create a
            // new verse to contain the upcoming cross-references.
            ch->check_verse_in_range(ref.verse_get_single());
            vs = new verse_xref(ch, ref.verse_get_single(), "");
            ch->verses[ref.verse_get_single()] = vs;
            startNewList = false;
        }
        else { 
            // Add to an existing list.
            vs->xrefs.push_back(encoded);
        }
    }
    
    // When rb goes out of scope, destructor frees up the buffer that was read
    
}

CrossReferences::~CrossReferences()
{
    if (bbl) { delete bbl; bbl = NULL; }
}

void CrossReferences::write(const Reference &ref, HtmlWriter2 &htmlwriter)
{
    vector <unsigned int> *xrefs; // could transfer over to uint32_t here and elsewhere
    htmlwriter.paragraph_open();
    htmlwriter.text_add(_("Cross references for ") + books_id_to_localname(ref.book_get()) + " " + std::to_string(ref.chapter_get()) + ":" + ref.verse_get());
    htmlwriter.paragraph_close();

    if (bbl == 0x0) {
        htmlwriter.paragraph_open();
        htmlwriter.text_add(_("Could not open cross-reference file ") + crossref_file);
        htmlwriter.paragraph_close();
        return;
    }
    if ((ref.book_get() == 0) || (ref.chapter_get() == 0) || (ref.verse_get_single() == 0)) {
        htmlwriter.paragraph_open();
        htmlwriter.text_add(_("Invalid reference; therefore not looking up cross references"));
        htmlwriter.paragraph_close();
        return;
    }
    
    try {
        xrefs = bbl->retrieve_xrefs(ref); // this copies xrefs from the returned vector<>&
    }
    catch (std::runtime_error &e) {
        htmlwriter.paragraph_open();
        htmlwriter.text_add(e.what());
        htmlwriter.paragraph_close();
    }
    if ((xrefs == NULL) || (xrefs->empty())) {
        htmlwriter.paragraph_open();
        htmlwriter.text_add(_("No cross references"));
        htmlwriter.paragraph_close();
    }
    else {
        extern Settings *settings;
        ustring project = settings->genconfig.project_get();
        Reference firstRef;
        bool finishComplexRange = false;
        for (auto &it : *xrefs) {
            Reference ref(it); // construct it directly from the bit-encoded reference
            
            if (ref.getRefType() == Reference::complexRange) {
              firstRef = ref;
              finishComplexRange = true;
              continue;
              // I am assuming that there is at least one more verse in xrefs...by 
              // construction there has to be.
            }
            
            ustring verse;
            ustring address;
            if (finishComplexRange == true) {
                verse = project_retrieve_verse(project, firstRef);
                address = books_id_to_localname(firstRef.book_get()) + " " + std::to_string(firstRef.chapter_get()) + ":" + firstRef.verse_get();
                // In this iteration of the loop, "ref" is now actually "secondRef" but I don't bother calling it that
            }
            else { // the most common case, a regular-old singleVerse (John 3:16) or multiVerse (John 3:16-17)
                verse = project_retrieve_verse(project, ref);
                address = books_id_to_localname(ref.book_get()) + " " + std::to_string(ref.chapter_get()) + ":" + ref.verse_get();
            }
            // Get data about the project.
            // This is exact same code as Concordance::writeVerses...should re-factor
            
            htmlwriter.paragraph_open();
            htmlwriter.hyperlink_add ("goto " + address, address);
                        
            if (verse.empty()) {
                verse.append(_("<empty>"));
            } else {
                replace_text(verse, "\n", " ");
                CategorizeLine cl(verse);
                cl.remove_verse_number(ref.verse_get());
                verse = cl.verse;
                htmlwriter.text_add(" " + verse);
            }
            
            if (finishComplexRange == true) {
               htmlwriter.text_add(".....");
               address = books_id_to_localname(ref.book_get()) + " " + std::to_string(ref.chapter_get()) + ":" + ref.verse_get();
               htmlwriter.hyperlink_add ("goto " + address, address);
               finishComplexRange = false;
            }
            
            htmlwriter.paragraph_close();
        }
    }
}
