/******************************************************************************
 * $Header: /CommonBe/agmsmith/Programming/Fringe\040Festival\040Visitor\040Schedule\040Optimiser/RCS/FFVSO.cpp,v 1.1 2014/06/09 23:27:24 agmsmith Exp agmsmith $
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
 * $Log: FFVSO.cpp,v $
 * Revision 1.1  2014/06/09 23:27:24  agmsmith
 * Initial revision
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
 * Class which contains information about a single show.  Basically just the
 * title and favourite status, since the show can have multiple venues and
 * times.
 */

struct Show
{
  std::string m_ShowName;
  bool m_IsFavourite;
};

typedef struct ShowStruct
{
  bool m_IsFavourite;
    /* TRUE if the show is one of the user's favourite ones.  They get
    highlighted differently and there is a count of favourite shows not yet
    scheduled. */

  int m_ScheduledCount;
    /* Number of times this show has been selected in the schedule.  Normally
    only zero or one.  If more than one then it's been overscheduled, unless
    the user really wants to see it several times. */

} ShowRecord, *ShowPointer;

typedef std::map<std::string, ShowRecord> ShowMap;
  /* A collection of all the shows, each one just listed once. */


/******************************************************************************
 * Global variables, and not-so-variable things too.  Grouped by functionality.
 */

ShowMap g_AllShowsMap;
  /* A list of all the shows. */


/******************************************************************************
 * Finally, the main program which drives it all.
 */

int main (int argc, char**)
{
  printf ("Content-Type: text/html\r\n\r\n");
  printf ("The end.\n");
  return 0;
}
