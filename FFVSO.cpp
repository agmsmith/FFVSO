/******************************************************************************
 * $Header: /CommonBe/agmsmith/Programming/AGMSBayesianSpam/Server/RCS/AGMSBayesianSpamServer.cpp,v 1.74 2002/12/14 02:43:57 agmsmith Exp $
 *
 * This is a web server CGI program for selecting events (shows) at the Ottawa
 * Fringe Theatre Festival to make up an individual's custom list.  Choices are
 * made on a web page and the results saved as a big blob of text in a text box
 * on the same web page.  Statistics showing conflicts in time, missing
 * favourite shows and other such info guide the user in selecting shows.
 *
 * Note that this uses the AGMS coding style, not the OpenTracker one.  That
 * means no tabs, indents are two spaces, m_ is the prefix for member
 * variables, g_ is the prefix for global names, C style comments, constants
 * are in all capital letters and most other things are mixed case, it's word
 * wrapped to fit in 79 characters per line to make proofreading on paper
 * easier, and functions are listed in reverse dependency order so that forward
 * declarations (function prototypes with no code) aren't needed.
 *
 * $Log: AGMSBayesianSpamServer.cpp,v $
 */

/* Standard C Library. */

#include <stdio.h>
#include <errno.h>

/* Standard C++ library. */

/* STL (Standard Template Library) headers. */

#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>


/******************************************************************************
 * Global variables, and not-so-variable things too.  Grouped by functionality.
 */

static const char *g_FFVSOAppSignature =
  "application/x-vnd.agmsmith.FringeFestivalVisitorScheduleOptimiser";



/******************************************************************************
 * Finally, the main program which drives it all.
 */

int main (int argc, char**)
{
  return 0;
}
