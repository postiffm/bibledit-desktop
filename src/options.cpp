/*
 ** Copyright (©) 2016 Matt Postiff.
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

#include "options.h"
#include <getopt.h>
#include "gwrappers.h"
#include "debug.h"
#include <stdlib.h>

// To process and store command-line arguments

// See
// https://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/Getopt-Long-Options.html#Getopt-Long-Options and
// https://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/Getopt-Long-Option-Example.html#Getopt-Long-Option-Example
Options::Options(int argc, char **argv)
{
  int c;

  // Set all flags to their default value
  debug = 0;
  
  while (1) {
      static struct option long_options[] =
        {
	  // name,  has_arg,             flag, val
          {"debug", optional_argument,   NULL, 'd'},
          {0, 0, 0, 0}
        };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "d", // we accept -d or --debug[=N]
                       long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1) { break; }

      switch (c) {
      case 'd':
	// for --debug, we set debug to 1, or to the long option value passed
	printf ("Option --%s", long_options[option_index].name);
	if (optarg) {
	  printf (" with arg %s", optarg);
	  debug = atoi(optarg);
	}
	else {
	  // for -d, we just set debug to 1
	  debug = 1;
	}
	printf ("\n");
	break;

      case 0:
      case '?':
      default:
	abort ();
	break;
      }
    }

  /* Print any remaining command line arguments (not options). */
  if (optind < argc) {
    printf ("Non-option ARGV-elements: ");
    while (optind < argc) {
      extraArgs.push_back(argv[optind]);
      printf ("%s ", argv[optind]);
      optind++;
    }
    putchar ('\n');
  }
}

void Options::print(void)
{
  gw_message("Command-line option --debug="+std::to_string(debug));
  for(auto const& value: extraArgs) {
    gw_message("Argument "+ ustring(value));
  }
}
