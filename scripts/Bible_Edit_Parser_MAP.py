# This seems to work fine on MacOS Windows 10, but not Linux.
# In Windows 10, the issue was the Hebrew characters.
# The error msg was "Unicode decode error...codec cannot decode
# bytes 0x9d...position 72"
# Basically the default encoding was not handling hebrew characters. If
# you force it to read and write in utf-8 and use the surrogate escape
# error handler, then it works fine without corrupting the Hebrew.

import os

filename = "data" #this will eventually need to be whatever the user is inputing from Bible edit
datafile = open(filename,'r', encoding="utf-8", errors="surrogateescape") 

#reading the datafile and parsing out relevant text
for line in datafile:
    line = line.lstrip().rstrip() #removes extraneous white space
    
    if line[0:2]=='\c':      #pulls chapter number
        chapter = line[3:6]
        
    if line[1:2]=='v':       #Note in python '\' is an escape character, so '\v' can't be used, I have used 'v', but you might be able to use '\\v' instead
        verse_number = line[3] #grabs first digit of verse number
        verse_start = 5        #verse starting point assuming 1 digit verse number
        verse_length = len(line)
        if line[4]!=' ':       #values in case verse number is 2 digits long
            verse_number = line[3:5]
            verse_start = 6
            if line[5]!=' ':   #values in case verse number is 3 digits long
                verse_number = line[3:6]
                verse_start = 7
        verse = line[verse_start:verse_length+1] #grabs verse without header info
        verse_filename = str(chapter)+'_'+str(verse_number)+'.bww'
        filepath = "Genesis" #where file is saved to (temp version until I figure this out), r is supposed to make it a raw string because '/' is an escape character
        if not os.path.exists(filepath): #checks if directory exists
            os.makedirs(filepath) #if directory does not exist, creates directory
        fully_specified_path = os.path.join(filepath,verse_filename) # Joins path components and returns string; NOT QUITE: writes verse file into specified directory
        verse_file = open(fully_specified_path, 'w', encoding="utf-8", errors="surrogateescape") #names and opens file to write verse to based on chapter and verse number
        verse_file.write(verse) #writes verse to file
        verse_file.close() #closes file
print ("Code finished") #so I know the code ran without error, will delete from file when project is finished
