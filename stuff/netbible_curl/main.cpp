/*
Copyright 2019 Scott Smith

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <curl/curl.h>
#include <bits/stdc++.h>

using namespace std;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main(void)
{
    CURL *curl;
    CURLcode res;
    ofstream myfile;
    stringstream ss;
    bool next_book;
    bool next_chap;
    int16_t book;
    int16_t chapter;
    int16_t verse;
    string filename;
    string bibleurl;
    string readBuffer;
    string verse_oneone;
    string verse_chapone;

    vector<string> books = {
        "Gen",      "Exo",      "Lev",      "Num",      "Deut",     "Josh",     "Judg",
        "Ruth",     "1Sa",      "2Sa",      "1Ki",      "2Ki",      "1Ch",      "2Ch",
        "Ezra",     "Neh",      "Esth",     "Job",      "Psa",      "Prov",     "Eccl",
        "Song",     "Isa",      "Jer",      "Lam",      "Ezek",     "Dan",      "Hos",
        "Joel",     "Amos",     "Oba",      "Jonah",    "Mic",      "Nah",      "Hab",
        "Zeph",     "Hag",      "Zech",     "Mal",
        "Matt",     "Mark",     "Luke",     "John",     "Acts",     "Rom",      "1Co",
        "2Co",      "Gal",      "Eph",      "Php",      "Col",      "1Th",      "2Th",
        "1Tim",     "2Tim",     "Tit",      "Phm",      "Heb",      "Jam",      "1Pe",
        "2Pe",      "1Jn",      "2Jn",      "3Jn",      "Jude",     "Rev"
    };

    //vector<string> books = { "3Jn" };

    if (mkdir("books", 0777) == -1)
        cout << "Error creating dir: " << strerror(errno) << endl;
    else
        cout << "Directory created" << endl;

    curl = curl_easy_init();
    if (curl)
    {
        for( book=0; book<books.size(); book++)
        {
            ss.str("");
            ss << "books/" << books.at(book) << ".txt";
            filename = ss.str();
            cout << filename << endl;

            myfile.open(filename.c_str(),ios::out);

            next_book = false;
            chapter = 0;

            if (myfile.is_open())
            {

                do {
                    next_chap = false;
                    chapter++;

                    verse = 0;

                    do {
                        verse++;

                        ss.str("");
                        ss << "http://labs.bible.org/api/?passage=" << books.at(book)
                           << "+" << chapter << ":" << verse << "&formatting=plain";
                        bibleurl = ss.str();

                        // cout << bibleurl << endl;

                        readBuffer.clear();
                        curl_easy_setopt(curl, CURLOPT_URL, bibleurl.c_str());
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
                        res = curl_easy_perform(curl);

                        // cout << readBuffer << endl;

                        if ((chapter == 1) && (verse == 1))
                        {
                            verse_oneone = readBuffer;
                            verse_chapone = readBuffer;

                            next_book = false;
                            next_chap = false;
                        }
                        else if ((chapter > 1) && (verse == 1))
                        {
                            verse_chapone = readBuffer;

                            next_book = false;
                            next_chap = false;
                        }

                        if ((chapter > 1) && (verse_oneone.compare(readBuffer) == 0))
                        {
                            next_book = true;
                            next_chap = true;
                        }
                        else if ((verse > 1) && (verse_chapone.compare(readBuffer) == 0))
                        {
                            next_book = false;
                            next_chap = true;
                        }

                        if (next_chap == false)
                        {
                            myfile << books.at(book) << " " << readBuffer << endl;
                        }

                    } while ((next_chap == false) && (next_book == false));

                } while (next_book == false);

                myfile.close();
            }
            else
            {
                cout << "Error could not open file: " << filename;
            }
        }

        curl_easy_cleanup(curl);
    }

    return 0;
}
