/******************************************************************************
 * $Header: /home/agmsmith/Programming/Fringe\040Festival\040Visitor\040Schedule\040Optimiser/RCS/FFVSO.cpp,v 1.29 2014/08/29 17:10:04 agmsmith Exp agmsmith $
 *
 * This is a web server CGI program for selecting events (shows) at the Ottawa
 * Fringe Theatre Festival to make up an individual's custom list.  Choices are
 * made on a web page and the results saved as a big blob of text in a text box
 * on the same web page.  Statistics showing conflicts in time, missing
 * favourite shows and other such info guide the user in selecting shows.
 *
 * Command line to compile: g++ -Wall -I. -o FFVSO.cgi FFVSO.cpp parsedate.cpp
 *
 * Note that this uses the AGMS vacation coding style.  That means no tabs,
 * indents are two spaces, m_ is the prefix for member variables, g_ is the
 * prefix for global names, C style comments, constants are in all capital
 * letters and most other things are mixed case, it's word wrapped to fit in 79
 * characters per line to make proofreading on paper easier, and functions are
 * listed in reverse dependency order so that forward declarations (function
 * prototypes with no code) aren't needed.
 *
 * $Log: FFVSO.cpp,v $
 * Revision 1.29  2014/08/29 17:10:04  agmsmith
 * Include compile date in the version string global parameter.
 *
 * Revision 1.28  2014/06/26 20:52:20  agmsmith
 * Now displays show and venue URLs as links.
 *
 * Revision 1.27  2014/06/26 15:27:53  agmsmith
 * Reset both time and version strings after reading old form data.
 *
 * Revision 1.26  2014/06/26 15:17:54  agmsmith
 * Include software version in printout.
 *
 * Revision 1.25  2014/06/25 21:37:43  agmsmith
 * Added vertical bar as a field separator, and a setting to control it.
 *
 * Revision 1.24  2014/06/23 16:04:24  agmsmith
 * Wording.
 *
 * Revision 1.23  2014/06/23 03:35:26  agmsmith
 * Wording.
 *
 * Revision 1.22  2014/06/21 15:22:18  agmsmith
 * Add gray highlighting for shows already picked elsewhere, and keep
 * statistics on redundant and missing shows.
 *
 * Revision 1.21  2014/06/21 14:56:52  agmsmith
 * Move global initialisation earlier in the source code.
 *
 * Revision 1.20  2014/06/20 20:47:49  agmsmith
 * Sort the events in the big form listing by time and show name, not by
 * time and venue name.  More convenient for Human readers.
 *
 * Revision 1.19  2014/06/20 20:01:50  agmsmith
 * Wording.
 *
 * Revision 1.18  2014/06/20 19:44:56  agmsmith
 * Now computes and displays conflicts.
 *
 * Revision 1.17  2014/06/20 17:59:03  agmsmith
 * Move selected events and favourite shows to end of raw data listing,
 * so it's easier to cut and paste in new schedule data from the Fringe.
 *
 * Revision 1.16  2014/06/20 17:22:20  agmsmith
 * Printable listing feature added.
 *
 * Revision 1.15  2014/06/19 20:49:22  agmsmith
 * Now reads form data for selecting events and favourite shows.
 * ShowDuration now read and written and displayed.
 *
 * Revision 1.14  2014/06/19 18:35:22  agmsmith
 * Encode the TEXTAREA contents, so tab characters, <, >, & get through
 * on more web browsers.
 *
 * Revision 1.13  2014/06/19 04:25:54  agmsmith
 * Added load/save of the URLs for shows and venues, added the big
 * table listing all events with checkboxes.
 *
 * Revision 1.12  2014/06/18 20:45:16  agmsmith
 * Adding HTML headers.
 *
 * Revision 1.11  2014/06/18 20:22:23  agmsmith
 * Adding settings.
 *
 * Revision 1.10  2014/06/18 19:51:56  agmsmith
 * Now loads in events.
 *
 * Revision 1.9  2014/06/18 03:11:40  agmsmith
 * Now collects the list of shows.
 *
 * Revision 1.8  2014/06/18 01:39:39  agmsmith
 * Mostly reformatting comment text, and a bug fix for multiple fields.
 *
 * Revision 1.7  2014/06/17 23:16:50  agmsmith
 * Starting to process the state info - now parses dates.
 *
 * Revision 1.6  2014/06/17 18:36:09  agmsmith
 * Adding map collections to store shows, venues and events.
 *
 * Revision 1.5  2014/06/16 19:20:40  agmsmith
 * Break the form input into name and value pairs, and apply the decoding
 * to each of the pair elements separately.
 *
 * Revision 1.4  2014/06/11 02:50:05  agmsmith
 * Now receives the text from the form and converts it
 * from Form URL encoded to plain text.  Still have to do
 * the line end conversion from CRLF to LF.
 *
 * Revision 1.3  2014/06/11 01:09:01  agmsmith
 * Read standard input and echo it back.  Seems to be missing
 * data for large text boxes.
 *
 * Revision 1.2  2014/06/10 01:49:50  agmsmith
 * First version which runs, though it doesn't do much.
 *
 * Revision 1.1  2014/06/09 23:27:24  agmsmith
 * Initial revision
 */

/* Standard C Library. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>

/* parsedate library taken from Haiku OS source for Unix, native in BeOS. */

#include <parsedate.h>

/* Standard C++ library. */

/* STL (Standard Template Library) headers. */

#include <algorithm>
#include <map>
#include <string>
#include <vector>


/******************************************************************************
 * Class which contains settings data.  It's just a map of keyword strings and
 * value strings.  Used for storing HTML fragments like the sequence to
 * highlight a favourite show.  Or any other string thing.  The user can edit
 * them too.
 */

typedef std::map<std::string, std::string> SettingMap;
typedef SettingMap::iterator SettingIterator;

SettingMap g_AllSettings;
  /* The collection of all settings.  Initialised to some standard ones, which
  get overwritten by the settings from the user's web page, see keyword
  "Setting" in the SavedState block of text. */


/******************************************************************************
 * Class which contains information about a single show.  Basically just the
 * title (stored as the key) and favourite status, since the show can have
 * multiple venues and times.
 */

typedef struct ShowStruct
{
  int m_EventCount;
    /* Number of performances of this show. */

  bool m_IsFavourite;
    /* TRUE if the show is one of the user's favourite ones.  They get
    highlighted differently and there is a count of favourite shows not yet
    scheduled. */

  int m_ScheduledCount;
    /* Number of times this show has been selected in the schedule.  Normally
    only zero or one.  If more than one then it's been overscheduled, unless
    the user really wants to see it several times. */

  int m_ShowDuration;
    /* How long does this show last?  In seconds.  Default is one hour. */

  std::string m_ShowURL;
    /* A link to a web page about the show. */

  ShowStruct () : m_EventCount(0), m_IsFavourite(false), m_ScheduledCount(0)
  {
    m_ShowDuration = 60 *
      atoi (g_AllSettings["DefaultShowDuration"].c_str ());
  };
} ShowRecord, *ShowPointer;

typedef std::map<std::string, ShowRecord> ShowMap;
typedef ShowMap::iterator ShowIterator;

ShowMap g_AllShows;
  /* A collection of all the uniquely named shows.  Hope the input data uses
  exactly the same name for each show! */


/******************************************************************************
 * Class which contains information about a single venue.  Essentially just the
 * venue name (which is also the key).
 */

typedef struct VenueStruct
{
  int m_EventCount;
    /* Number of performances at this venue. */

  std::string m_VenueURL;
    /* A link to a web page about the venue. */

  VenueStruct () : m_EventCount(0)
  {};

} VenueRecord, *VenuePointer;

typedef std::map<std::string, VenueRecord> VenueMap;
typedef VenueMap::iterator VenueIterator;

VenueMap g_AllVenues;
  /* A collection of all the venues. */


/******************************************************************************
 * Class which contains information about a single event.  An event is uniquely
 * identified by a time and a venue (which will be our key).  The rest of the
 * event data specifies the show and whether the particular event is selected
 * by the user.
 */

typedef struct EventKeyStruct
{
  time_t m_EventTime;
    /* In Unix seconds since the start of time. */

  VenueIterator m_Venue;
    /* Points to the Venue information, we just use the first part of the Venue
    pair (the name) for our composite key. */

  bool operator() ( /* Our less-than comparison function for sorting. */
    const EventKeyStruct ItemA,
    const EventKeyStruct ItemB) const
  {
    double DeltaTime = difftime (ItemA.m_EventTime, ItemB.m_EventTime);
    if (DeltaTime < 0.0)
      return true;

    if (DeltaTime > 0.0)
      return false;

    const char *NameA = ItemA.m_Venue->first.c_str ();
    const char *NameB = ItemB.m_Venue->first.c_str ();
    return (strcmp (NameA, NameB) < 0);
  };

  EventKeyStruct () : m_EventTime(0)
  {};

} EventKeyRecord, *EventKeyPointer;


typedef struct EventStruct
{
  bool m_IsSelectedByUser;
    /* TRUE if the user has selected this event, meaning they want to attend
    the show at this specific place and time. */

  bool m_IsConflicting;
    /* Conflicts with some other show, will be drawn in red or highlighted in
    some way.  All shows involved in the conflict will have this flag set. */

  ShowIterator m_ShowIter;
    /* Identifies the show that is being performed at this place and time. */

  EventStruct () : m_IsSelectedByUser(false), m_IsConflicting(false)
  {};

} EventRecord, *EventPointer;

typedef std::map<EventKeyStruct, EventRecord, EventKeyStruct> EventMap;
typedef EventMap::iterator EventIterator;

EventMap g_AllEvents;
  /* A collection of all the events, indexed by time and venue. */

typedef std::vector<EventIterator> SortedEventVector;
SortedEventVector g_EventsSortedByTimeAndShow;
  /* A list of all the events sorted by time and show.  The natural order of
  time and venue just doesn't read right when you're looking through a list, so
  we'll use this ordering when printing Human readable output. */


/******************************************************************************
 * A collection of global level statistics.
 */

struct StatisticsStruct
{
  int m_TotalNumberOfConflicts;
    /* Number of conflicts present.  Includes counting all events involved in a
    conflict, not just the first one. */

  int m_TotalNumberOfEventsScheduled;
    /* Number of events the user is going to see in their schedule. */

  int m_TotalNumberOfRedundantShows;
    /* Number of shows seen more than once. */

  int m_TotalNumberOfUnseenFavouriteShows;
    /* Number of favourite shows never seen. */

  int m_TotalNumberOfUnseenShows;
    /* Number of shows never seen. */

  int m_TotalSecondsWatched;
    /* How much show watching is the user doing with their schedule? */

  StatisticsStruct () : m_TotalNumberOfConflicts(0),
    m_TotalNumberOfEventsScheduled(0), m_TotalNumberOfRedundantShows(0),
    m_TotalNumberOfUnseenFavouriteShows(0), m_TotalNumberOfUnseenShows(0),
    m_TotalSecondsWatched(0)
  {};

} g_Statistics;


/******************************************************************************
 * Input form data storage.  This is the form data submitted by clicking on the
 * Update Schedule button on the web page.
 */

char *g_InputFormText;
  /* The state of the form on the web page, as received from the web browser
  POST command's encoded data.  Will be overwritten with the equivalent decoded
  text and that's then referenced by the collection of form name and value
  pairs. */

struct CompareCharStruct
{
  bool operator() ( /* Our less-than comparison function for sorting. */
    const char * ItemA,
    const char * ItemB) const
  {
    return (strcmp (ItemA, ItemB) < 0);
  };
};

typedef std::map<char *, const char *, CompareCharStruct> FormNameToValuesMap;

FormNameToValuesMap g_FormNameValuePairs;
  /* The form input data decoded and broken up into name and value pairs.
  Usually one pair for each of the form elements, except checkboxes which are
  simply missing if not selected.  The actual strings are stored in
  g_InputFormText. */


/******************************************************************************
 * Settings which are always updated, overwriting old values from the form.
 * Set the setting for the last update time to the current time and also update
 * the software version string.
 */

void ResetDynamicSettings ()
{
  time_t TimeNow;
  struct tm BrokenUpTime;
  time(&TimeNow);
  localtime_r (&TimeNow, &BrokenUpTime);
  g_AllSettings["LastUpdateTime"].assign (asctime (&BrokenUpTime), 24);

  g_AllSettings["Version"] =
    "$Id: FFVSO.cpp,v 1.29 2014/08/29 17:10:04 agmsmith Exp agmsmith $ "
    "was compiled on " __DATE__ " at " __TIME__ ".";
}


/******************************************************************************
 * Set up the global list of settings, for things like the HTML strings that
 * highlight selected items.  They will be overwritten by user provided
 * settings.
 */

void InitialiseDefaultSettings ()
{
  g_AllSettings["HTMLAlreadyPickedBegin"] = "<FONT COLOR=\"SILVER\">";
  g_AllSettings["HTMLAlreadyPickedEnd"] = "</FONT>";
  g_AllSettings["HTMLConflictBegin"] = "<FONT COLOR=\"RED\">";
  g_AllSettings["HTMLConflictEnd"] = "</FONT>";
  g_AllSettings["HTMLFavouriteBegin"] = "<I>";
  g_AllSettings["HTMLFavouriteEnd"] = "</I>";
  g_AllSettings["HTMLSelectBegin"] = "<B>";
  g_AllSettings["HTMLSelectEnd"] = "</B>";
  g_AllSettings["UseOnlyTabForFieldSeparator"] = "0";
  g_AllSettings["DefaultLineupTime"] = "5";
  g_AllSettings["DefaultShowDuration"] = "60";
  g_AllSettings["DefaultTravelTime"] = "10";
  g_AllSettings["NewDayGapMinutes"] = "360";
  g_AllSettings["TitleEdit"] = "<H1>Title for Edit-Your-Schedule goes here</H1><P>Subtitle for editing the page goes here.  Could be useful for things like the date when the schedule was last updated from the Festival's show times web page, a link to the Festival page, and that sort of thing.";
  g_AllSettings["TitlePrint"] = "<H1>Title for Your-Printable-Listing goes here</H1>";
  ResetDynamicSettings ();
}


/******************************************************************************
 * Convert the given string from Form URL Encoded to plain text.  That means
 * replacing all "%xy" codes (byte hex encoded) with the corresponding hex
 * character, and "+" with a space.  Then run through the text again and
 * replace CRLF or plain CR with just LF, our standard end of line character.
 */

void FormEncodedToPlainText (char *pBuffer)
{
  char *pDest;
  char Letter;
  char *pSource;

  pSource = pDest = pBuffer;
  while ((Letter = *pSource++) != 0)
  {
    if (Letter == '+') // Just a space.
    {
      *pDest++ = ' ';
    }
    else if (Letter == '%') // Hex encoded byte.
    {
      char Hex1 = tolower (pSource[0]);
      int Value1 = -1;
      if (Hex1 >= '0' && Hex1 <= '9')
        Value1 = Hex1 - '0';
      else if (Hex1 >= 'a' && Hex1 <= 'f')
        Value1 = Hex1 - 'a' + 10;

      if (Value1 != -1) // Valid hex digit previously, also not a NUL char.
      {
        char Hex2 = tolower (pSource[1]);
        int Value2 = -1;
        if (Hex2 >= '0' && Hex2 <= '9')
          Value2 = Hex2 - '0';
        else if (Hex2 >= 'a' && Hex2 <= 'f')
          Value2 = Hex2 - 'a' + 10;

        if (Value2 != -1)
          Value1 = 16 * Value1 + Value2;
        else  // Second digit isn't hex, don't have a valid value.
          Value1 = -1;
      }
      if (Value1 == -1) // Not a %xy hex sequence, or too close to end NUL.
        *pDest++ = Letter;
      else // Sucessfully decoded a hex value.
      {
        pSource += 2;
        *pDest++ = Value1;
      }
    }
    else // Some other letter.
    {
      *pDest++ = Letter;
    }
  }
  *pDest = 0;

  // Pass 2 - convert carriage returns and linefeeds to just linefeeds.

  pSource = pDest = pBuffer;
  while ((Letter = *pSource++) != 0)
  {
    if (Letter == '\r')
    {
      if (*pSource == '\n')
        pSource++; // Convert CRLF to just LF.

      *pDest++ = '\n';
    }
    else if (Letter == '\n')
    {
      if (*pSource == '\r')
        pSource++; // Convert the rarer LFCR to just LF.

      *pDest++ = '\n';
    }
    else
      *pDest++ = Letter;
  }
  *pDest = 0;
}


/******************************************************************************
 * Break the form input data into pairs of name and associated value.  Also
 * decodes the text.  Overwrites the global g_InputFormText with the decoded
 * text, also adding NUL bytes after each name and value.  The format is a name
 * followed by an equals sign, followed by the value, followed by an ampersand
 * and then the next name and value etc.  Last one has an end of string NUL at
 * the end instead of an ampersand.
 */

void BuildFormNameAndValuePairsFromFormInput ()
{
  char *pName;
  char *pSource;
  char *pValue;

  pSource = g_InputFormText;
  while (*pSource != 0)
  {
    pName = pSource;
    while (*pSource != '=' && *pSource != 0)
      pSource++;
    if (*pSource == 0)
      break; // Malformed pair, no value portion.
    *pSource++ = 0; // Terminate name portion of the string.

    pValue = pSource;
    while (*pSource != '&' && *pSource != 0)
      pSource++;
    *pSource++ = 0; // Terminate value portion of the string.

    if (*pName != 0) // Ignore empty names.
    {
      FormEncodedToPlainText (pName);
      FormEncodedToPlainText (pValue);
      FormNameToValuesMap::value_type NewPair (pName, pValue);

      g_FormNameValuePairs.insert (NewPair);
    }
  }
}


/******************************************************************************
 * Parse the saved state string, which is stored in a huge TextArea in the web
 * form.  It can be initialised by copying the text from the Fringe's schedule
 * web page (http://ottawafringe.com/schedule/), which has lines listing each
 * time/show/venue separted by days of the week subtitles.  Conveniently it's a
 * table so when you copy the text out, the fields are separated by tab
 * characters.
 *
 * We look for a time or keyword first.  If it's just a date (no more tab
 * separated fields after it) then it becomes the current default date.  If
 * it's a keyword, then the number of fields after it depend on the keyword.
 * If it's a time with three or more fields in total, then it's an event, where
 * the first field is the time (combine with the default date to get an
 * absolute time), the second field is the show name and the third is the venue
 * name, and the fourth is the optional selected flag ("Selected" to pick the
 * event, anything else or missing to not pick it).
 *
 * The keywords are used for storing extra information.  They are:
 *   "Favourite" with the second field being the show name.  That show is then
 *      marked as being one of the higher priority ones for the user to see.
 *   "Setting" followed by the name of the setting and the string value of that
 *     setting.  The global g_AllSettings will be updated with that name/value.
 *   "ShowURL" has a second field that names a show and a third that specifies
 *     a web link to show information about the show.
 *   "ShowDuration" is followed by the show name and the duration (number of
 *     minutes) of that show.  If not specified, a default (usually one hour,
 *     there's a setting for it) is used.
 *   "VenueURL" second field names a venue, third has the URL for information
 *     about it.
 *   "Selected" is followed by a date & time string, and then the name of a
 *     venue.  That specifies an event the user will be attending, and is the
 *     preferred way of marking events rather than the old "Selected" after the
 *     event definition way.
 */

void LoadStateInformation (const char *pBuffer)
{
  // Set the running date to be the current time in the current year, in case
  // they specify events without mentioning the year.  Not set to January 1st,
  // since that may specify a different daylight savings time than the current
  // time (which is more likely to be around when the festival starts), which
  // makes the first time input parsed be off by an hour.

  struct tm BrokenUpDate;
  time_t RunningDate;
  time (&RunningDate);

  // When reading, always consider tabs to be field separators, and optionally
  // have '|' vertical bar as a field separator (some web browsers don't
  // support pasting in text with tabs into a TEXTAREA).  Should be fine so
  // long as the vertical bar isn't used in Show names or other text, and if
  // it is a problem, a setting turns off vertical bars.

  bool bOnlyTab = (0 != atoi (
    g_AllSettings["UseOnlyTabForFieldSeparator"].c_str ()));

  const int MAX_FIELDS = 5;
  std::string aFields[MAX_FIELDS];

  const char *pSource = pBuffer;
  while (*pSource != 0)
  {
    // Starting a new line of text, reset the field markers and start hunting
    // for fields.

    int iField;
    for (iField = 0; iField < MAX_FIELDS; iField++)
      aFields[iField].clear();

    iField = 0;
    char Letter = *pSource;
    while (Letter != 0 && Letter != '\n')
    {
      // Skip leading spaces, and for the first field, also field separators
      // (in case some e-mail software indented the text with tabs).

      while (Letter == ' ' ||
      (iField == 0 && (Letter == '\t' || (!bOnlyTab && Letter == '|'))))
        Letter = *++pSource;
      const char *pFieldStart = pSource;

      while (Letter != '\t' && Letter != '\n' && Letter != 0 &&
      (bOnlyTab || Letter != '|'))
        Letter = *++pSource; // Skip over the contents of the field.

      if (iField < MAX_FIELDS)
      {
        const char *pFieldEnd = pSource - 1;
        while (pFieldEnd >= pFieldStart && *pFieldEnd == ' ')
          pFieldEnd--; // Remove trailing spaces.
        pFieldEnd++;

        int FieldLen = pFieldEnd - pFieldStart;
        aFields[iField].assign (pFieldStart, FieldLen);
      }
      iField++;

      // Leave LF and NUL alone (still in Letter) so outer loop exits.

      if (Letter == '\t' || (!bOnlyTab && Letter == '|'))
      {
        Letter = *++pSource;
        pFieldStart = pSource;
      }
    }
    int nFields = iField; // Keep the count of the number of fields for later.

    // Finished reading a line of input, now process the fields.

    if (nFields > 0 && !aFields[0].empty ())
    {
      time_t NewDate = parsedate (aFields[0].c_str(), RunningDate);
      if (NewDate > 0) // Got a valid date.
      {
        localtime_r (&NewDate, &BrokenUpDate);
#if 0
        printf ("Converted date \"%s\" to %s", aFields[0].c_str(),
          asctime (&BrokenUpDate));
#endif
        // Update the running date, so subsequent times are based off this one.
        // Useful if the date is a subtitle like "Thursday, June 19" and
        // subsequent entries just list the hour and minute, in increasing
        // order.

        RunningDate = NewDate;

        // Load an event, if we have enough fields to define one, otherwise
        // it's just a date update.  Need show and venue after the date field,
        // and optionally "Selected" for backwards compatibility reasons.

        if (nFields >= 3)
        {
          ShowRecord NewShow;
          ShowMap::value_type NewShowPair (aFields[1], NewShow);
          std::pair<ShowIterator, bool> InsertShowResult (
            g_AllShows.insert (NewShowPair));

          VenueRecord NewVenue;
          VenueMap::value_type NewVenuePair (aFields[2], NewVenue);
          std::pair<VenueIterator, bool> InsertVenueResult (
            g_AllVenues.insert (NewVenuePair));

          EventKeyRecord NewEventKey;
          NewEventKey.m_EventTime = NewDate;
          NewEventKey.m_Venue = InsertVenueResult.first;

          EventRecord NewEvent;
          NewEvent.m_ShowIter = InsertShowResult.first;
          if (nFields >= 4 && aFields[3] == "Selected")
            NewEvent.m_IsSelectedByUser = true;

          EventMap::value_type NewEventPair (NewEventKey, NewEvent);
          std::pair<EventIterator, bool> InsertEventResult (
            g_AllEvents.insert (NewEventPair));
          if (!InsertEventResult.second)
          {
            printf ("<P><B>Slight redundancy problem</B>: ignoring redundant "
              "occurance of an identical event (same place and time).  It is "
              "show \"%s\" (prior show is \"%s\"), venue \"%s\", at time %s",
              NewEvent.m_ShowIter->first.c_str(),
              InsertEventResult.first->second.m_ShowIter->first.c_str(),
              NewEventKey.m_Venue->first.c_str(),
              asctime (&BrokenUpDate));
          }
          else // Successfully added a new event.
          {
            InsertShowResult.first->second.m_EventCount++;
            InsertVenueResult.first->second.m_EventCount++;
          }
        }
      }
      else // Don't have a date, look for keywords in the first field.
      if (aFields[0] == "Favourite" && nFields >= 2)
      {
        // Favourite keyword is followed by a field identifying a show.

        ShowIterator iShow = g_AllShows.find (aFields[1]);
        if (iShow != g_AllShows.end ())
          iShow->second.m_IsFavourite = true;
        else
          printf ("<P><B>Unknown show name</B> \"%s\" after Favourite "
            "keyword, ignoring it.\n", aFields[1].c_str ());
      }
      else if (aFields[0] == "Setting" && nFields >= 3)
      {
        // Hopefully the important settings were written near the beginning,
        // like the one which controls parsing of tabs and vertical bars.

        g_AllSettings[aFields[1]] = aFields[2];

        bOnlyTab = (0 != atoi (
          g_AllSettings["UseOnlyTabForFieldSeparator"].c_str ()));
      }
      else if (aFields[0] == "ShowURL" && nFields >= 3)
      {
        ShowIterator iShow = g_AllShows.find (aFields[1]);
        if (iShow != g_AllShows.end ())
          iShow->second.m_ShowURL.assign (aFields[2]);
        else
          printf ("<P><B>Unknown show name</B> \"%s\" after ShowURL "
            "keyword, ignoring it.\n", aFields[1].c_str ());
      }
      else if (aFields[0] == "ShowDuration" && nFields >= 3)
      {
        ShowIterator iShow = g_AllShows.find (aFields[1]);
        if (iShow != g_AllShows.end ())
          iShow->second.m_ShowDuration = 60 * atoi (aFields[2].c_str ());
        else
          printf ("<P><B>Unknown show name</B> \"%s\" after ShowDuration "
            "keyword, ignoring it.\n", aFields[1].c_str ());
      }
      else if (aFields[0] == "VenueURL" && nFields >= 3)
      {
        VenueIterator iVenue = g_AllVenues.find (aFields[1]);
        if (iVenue != g_AllVenues.end ())
          iVenue->second.m_VenueURL.assign (aFields[2]);
        else
          printf ("<P><B>Unknown venue name</B> \"%s\" after VenueURL "
            "keyword, ignoring it.\n", aFields[1].c_str ());
      }
      else if (aFields[0] == "Selected" && nFields >= 3)
      {
        time_t SelectedDate = parsedate (aFields[1].c_str(), RunningDate);
        if (SelectedDate <= 0)
        {
          printf ("<P><B>Bad date</B> \"%s\" after Selected "
            "keyword, ignoring it.\n", aFields[1].c_str ());
        }
        else
        {
          VenueIterator iVenue = g_AllVenues.find (aFields[2]);
          if (iVenue == g_AllVenues.end ())
          {
            printf ("<P><B>Unknown venue name</B> \"%s\" after Selected "
              "keyword, ignoring it.\n", aFields[2].c_str ());
          }
          else
          {
            EventKeyRecord SelectedEventKey;
            SelectedEventKey.m_EventTime = SelectedDate;
            SelectedEventKey.m_Venue = iVenue;
            EventIterator iSelectedEvent = g_AllEvents.find (SelectedEventKey);
            if (iSelectedEvent == g_AllEvents.end ())
            {
              printf ("<P><B>No event exists</B> for date \"%s\" and venue "
                "\"%s\" after Selected keyword, ignoring it.\n",
                aFields[1].c_str (), aFields[2].c_str ());
            }
            else
            {
              iSelectedEvent->second.m_IsSelectedByUser = true;
            }
          }
        }
      }
      else
      {
        printf ("<P><B>Ignoring unparseable line</B> with %d fields: ", nFields);
        for (iField = 0; iField < nFields; iField++)
        {
          if (iField >= MAX_FIELDS)
          {
            printf ("...");
            break;
          }
          printf ("%s%s", aFields[iField].c_str(),
            (iField < nFields - 1) ? ", " : "");
        }
        printf ("\n");
      }
    }

    if (Letter == '\n') // Leave NUL alone so outer loop exits.
      Letter = *++pSource;
  }
}


/******************************************************************************
 * Look at the data from the form controls and add it to the current state.
 */

void ReadFormControls ()
{
  FormNameToValuesMap::iterator iFormPair;

  // Look for the checkboxes for events.  Goofily, if the checkbox is
  // unchecked, there is no data.  So we have to look for each event's checkbox
  // to see if it is on or if it is missing.

  EventIterator iEvent;
  for (iEvent = g_AllEvents.begin(); iEvent != g_AllEvents.end(); ++iEvent)
  {
    time_t EventTime = iEvent->first.m_EventTime;
    char TimeString[32];
    sprintf (TimeString, "Event,%ld,", EventTime);
    std::string CheckboxName (TimeString);
    CheckboxName.append (iEvent->first.m_Venue->first);

    iFormPair = g_FormNameValuePairs.find (
      const_cast<char *>(CheckboxName.c_str ()));
    if (iFormPair != g_FormNameValuePairs.end () &&
    strcmp (iFormPair->second, "On") == 0)
      iEvent->second.m_IsSelectedByUser = true;
    else
      iEvent->second.m_IsSelectedByUser = false;
  }

  // Look for the checkboxes that mark favourite shows.

  ShowIterator iShow;
  for (iShow = g_AllShows.begin(); iShow != g_AllShows.end(); ++iShow)
  {
    std::string CheckboxName ("Show,");
    CheckboxName.append (iShow->first);

    iFormPair = g_FormNameValuePairs.find (
      const_cast<char *>(CheckboxName.c_str ()));
    if (iFormPair != g_FormNameValuePairs.end () &&
    strcmp (iFormPair->second, "On") == 0)
      iShow->second.m_IsFavourite = true;
    else
      iShow->second.m_IsFavourite = false;
  }
}


/******************************************************************************
 * Prints the given string to standard output, converting special characters
 * into their HTML encoded equivalents.  Ampersand becomes "&amp;", tab becomes
 * "&#9;", less than and greater than are also encoded.  This should help with
 * web browsers that can't have tabs in text areas.
 */

void EncodeAndPrintText (const char *pBuffer)
{
  int OutputLength = strlen (pBuffer) * 5 + 1; // &amp; is the worst case one.
  char *pOutputBuffer = new char [OutputLength];

  const char *pSource = pBuffer;
  char *pDest = pOutputBuffer;
  char Letter;

  while ((Letter = *pSource++) != 0)
  {
    if ('\t' == Letter)
    {
      strcpy (pDest, "&#9;");
      pDest += 4;
    }
    else if ('&' == Letter)
    {
      strcpy (pDest, "&amp;");
      pDest += 5;
    }
    else if ('<' == Letter)
    {
      strcpy (pDest, "&lt;");
      pDest += 4;
    }
    else if ('>' == Letter)
    {
      strcpy (pDest, "&gt;");
      pDest += 4;
    }
    else
      *pDest++ = Letter;
  }
  *pDest = 0;

  printf ("%s", pOutputBuffer);
  delete [] pOutputBuffer;
}


/******************************************************************************
 * Write the output web page, which dumps the current state of everything into
 * various form fields so that the user can change it.
 */

void WriteHTMLHeader ()
{
  printf (
"Content-Type: text/html\r\n\r\n" // Magic CGI header.
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
"<HTML>\n"
"<HEAD>\n"
"<TITLE>Fringe Theatre Festival Visitor Schedule Optimiser by AGMS</TITLE>\n"
"<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=utf-8\">\n"
"<META NAME=\"author\" CONTENT=\"Alexander G. M. Smith\">\n"
"<META NAME=\"description\" CONTENT=\"A web app for scheduling attendance at "
"theatre performances so that you don't miss the shows you want, and to pack "
"in as many shows as possible while avoiding duplicates.\">\n"
"<META NAME=\"version\" CONTENT=\"$Id: FFVSO.cpp,v 1.29 2014/08/29 17:10:04 agmsmith Exp agmsmith $\">\n"
"</HEAD>\n"
"<BODY BGCOLOR=\"WHITE\" TEXT=\"BLACK\">\n");
}


void WriteHTMLForm ()
{
  EventIterator iEvent;
  ShowIterator iShow;
  SettingIterator iSetting;
  VenueIterator iVenue;

  struct tm BrokenUpDate;
  char OutputBuffer[4096];
  char TimeString[40];
  int NewDayGapMinutes = atoi (g_AllSettings["NewDayGapMinutes"].c_str ());

  printf ("%s\n", g_AllSettings["TitleEdit"].c_str ());
  printf ("<FORM ACTION=\"http://www.agmsmith.ca/cgi-bin/FFVSO.cgi\" method=\"POST\">\n");
  printf ("<P ALIGN=\"CENTER\">");
  printf ("Jump to <A HREF=\"#Events\">Events</A> <A HREF=\"#Shows\">Shows</A> "
    "<A HREF=\"#Venues\">Venues</A> <A HREF=\"#RawData\">Raw Data</A>\n"
    "<P ALIGN=\"CENTER\">You have %d conflicts and you are seeing %d performances "
    "(total time %d:%02d).\n"
    "<BR>There are %d redundant, %d unseen favourites, and %d unseen shows.\n",
    g_Statistics.m_TotalNumberOfConflicts,
    g_Statistics.m_TotalNumberOfEventsScheduled,
    g_Statistics.m_TotalSecondsWatched / 60 / 60,
    g_Statistics.m_TotalSecondsWatched / 60 % 60,
    g_Statistics.m_TotalNumberOfRedundantShows,
    g_Statistics.m_TotalNumberOfUnseenFavouriteShows,
    g_Statistics.m_TotalNumberOfUnseenShows);
  printf ("<P ALIGN=\"CENTER\">");
  printf ("<INPUT TYPE=\"SUBMIT\" NAME=\"UpdateSchedule\" VALUE=\"Update Schedule with your Changes\">\n");
  printf ("<INPUT TYPE=\"SUBMIT\" NAME=\"PrintSchedule\" VALUE=\"See Printable Schedule\">\n");

  // Write out the event listing with checkboxes beside each event to let the
  // user select it.  Done as a table with five columns: event time, duration,
  // show name, venue name, checkbox.

  printf ("<H2><A NAME=\"Events\"></A>Listing of %ld Events</H2>\n"
    "<P>Use the checkboxes to select the events you want to see, then hit the "
    "Update Schedule button to see if you have conflicts or other problems."
    "&nbsp; Repeat until you're happy with your schedule, then use the "
    "Printable Schedule button to get a clean copy.&nbsp; Use your browser's "
    "Back button to go back to editing after printing.&nbsp; See the Raw Data "
    "box near the bottom of this page for info about saving your schedule.\n"
    "<P><TABLE BORDER=\"1\" CELLPADDING=\"1\">\n", g_AllEvents.size ());

  time_t PreviousTime = 0;
  int iSortedEvent;
  const int iEndSortedEvent = g_EventsSortedByTimeAndShow.size ();
  for (iSortedEvent = 0; iSortedEvent < iEndSortedEvent; ++iSortedEvent)
  {
    iEvent = g_EventsSortedByTimeAndShow[iSortedEvent];

    // If more than 6 hours (or whatever NewDayGapMinutes is set to) since the
    // previous event, print out a new day heading.  The Ottawa Fringe last
    // show is usually at midnight, and the first one after that is at noon.

    time_t EventTime = iEvent->first.m_EventTime;
    localtime_r (&EventTime, &BrokenUpDate);
    double DeltaTime = difftime (EventTime, PreviousTime);
    if (fabs (DeltaTime) > NewDayGapMinutes * 60)
    {
      strftime (TimeString, sizeof (TimeString), "%A, %B %d, %Y",
        &BrokenUpDate);
      printf ("<TR><TH COLSPAN=\"5\">%s</TH></TR>\n", TimeString);
    }
    PreviousTime = EventTime;

    // Print out the event itself.  Just use the hours and minutes for the time
    // to avoid cluttering the display with full date and time values.

    strftime (TimeString, sizeof (TimeString), "%H:%M", &BrokenUpDate);

    // Add highlighting for selected events, conflicts and favourite shows.

    std::string StartHTML;
    std::string EndHTML;

    if (iEvent->second.m_IsSelectedByUser)
    {
      StartHTML.append (g_AllSettings["HTMLSelectBegin"]);
      EndHTML.insert (0, g_AllSettings["HTMLSelectEnd"]);
    }
    else if (iEvent->second.m_ShowIter->second.m_ScheduledCount > 0)
    {
      StartHTML.append (g_AllSettings["HTMLAlreadyPickedBegin"]);
      EndHTML.insert (0, g_AllSettings["HTMLAlreadyPickedEnd"]);
    }

    if (iEvent->second.m_ShowIter->second.m_IsFavourite)
    {
      StartHTML.append (g_AllSettings["HTMLFavouriteBegin"]);
      EndHTML.insert (0, g_AllSettings["HTMLFavouriteEnd"]);
    }

    if (iEvent->second.m_IsConflicting)
    {
      StartHTML.append (g_AllSettings["HTMLConflictBegin"]);
      EndHTML.insert (0, g_AllSettings["HTMLConflictEnd"]);
    }

    // Also include a URL link with the highlighting, for shows and venues.

    std::string StartShowHTML (StartHTML);
    std::string EndShowHTML (EndHTML);
    if (!iEvent->second.m_ShowIter->second.m_ShowURL.empty ())
    {
      StartShowHTML.append ("<A HREF=\"");
      StartShowHTML.append (iEvent->second.m_ShowIter->second.m_ShowURL);
      StartShowHTML.append ("\">");
      EndShowHTML.insert (0, "</A>");
    }

    std::string StartVenueHTML (StartHTML);
    std::string EndVenueHTML (EndHTML);
    if (!iEvent->first.m_Venue->second.m_VenueURL.empty ())
    {
      StartVenueHTML.append ("<A HREF=\"");
      StartVenueHTML.append (iEvent->first.m_Venue->second.m_VenueURL);
      StartVenueHTML.append ("\">");
      EndVenueHTML.insert (0, "</A>");
    }

    // Dump out the event and a checkbox to change it.

    printf ("<TR VALIGN=\"TOP\"><TD>%s%s%s</TD><TD>%s%d%s</TD><TD>%s%s%s</TD>"
      "<TD>%s%s%s</TD><TD><INPUT TYPE=\"CHECKBOX\" NAME=\"Event,%ld,%s\" "
      "VALUE=\"On\"%s></INPUT></TD></TR>\n",
      StartHTML.c_str(), TimeString, EndHTML.c_str(),
      StartHTML.c_str(), iEvent->second.m_ShowIter->second.m_ShowDuration / 60, EndHTML.c_str(),
      StartShowHTML.c_str(), iEvent->second.m_ShowIter->first.c_str(), EndShowHTML.c_str(),
      StartVenueHTML.c_str(), iEvent->first.m_Venue->first.c_str(), EndVenueHTML.c_str(),
      EventTime, iEvent->first.m_Venue->first.c_str(),
      (iEvent->second.m_IsSelectedByUser) ? " CHECKED" : "");
  }

  printf ("</TABLE>\n");

  // Write out a list of the shows.  Includes show name, duration,
  // #performances, #times seen, and a checkbox to select favourite ones.

  printf ("<H2><A NAME=\"Shows\"></A>Listing of %ld Shows</H2><P>"
    "<TABLE BORDER=\"1\" CELLPADDING=\"1\">\n", g_AllShows.size ());
  printf ("<TR><TH>Show Title</TH><TH>Minutes</TH><TH>Perform-<BR>ances</TH>"
    "<TH>Times<BR>Seen</TH><TH>Your<BR>Favourite?</TH></TR>\n");

  for (iShow = g_AllShows.begin(); iShow != g_AllShows.end(); ++iShow)
  {
    // Add highlighting for favourite shows, and a link to the show's page.

    std::string StartHTML;
    std::string EndHTML;

    if (iShow->second.m_IsFavourite)
    {
      StartHTML.append (g_AllSettings["HTMLFavouriteBegin"]);
      EndHTML.insert (0, g_AllSettings["HTMLFavouriteEnd"]);
    }

    std::string StartShowHTML (StartHTML);
    std::string EndShowHTML (EndHTML);
    if (!iShow->second.m_ShowURL.empty ())
    {
      StartShowHTML.append ("<A HREF=\"");
      StartShowHTML.append (iShow->second.m_ShowURL);
      StartShowHTML.append ("\">");
      EndShowHTML.insert (0, "</A>");
    }

    printf ("<TR VALIGN=\"TOP\"><TD>%s%s%s</TD><TD>%s%d%s</TD><TD>%s%d%s</TD>"
      "<TD>%s%d%s</TD><TD><INPUT TYPE=\"CHECKBOX\" NAME=\"Show,%s\" "
      "VALUE=\"On\"%s></INPUT></TD></TR>\n",
      StartShowHTML.c_str(), iShow->first.c_str(), EndShowHTML.c_str(),
      StartHTML.c_str(), iShow->second.m_ShowDuration / 60, EndHTML.c_str(),
      StartHTML.c_str(), iShow->second.m_EventCount, EndHTML.c_str(),
      StartHTML.c_str(), iShow->second.m_ScheduledCount, EndHTML.c_str(),
      iShow->first.c_str(), iShow->second.m_IsFavourite ? " CHECKED" : "");
  }

  printf ("</TABLE>\n");

  // Write out a list of the venues.  Includes venue name, #performances.

  printf ("<H2><A NAME=\"Venues\"></A>Listing of %ld Venues</H2><P>"
    "<TABLE BORDER=\"1\" CELLPADDING=\"1\">\n", g_AllVenues.size ());
  printf ("<TR><TH>Venue Name</TH><TH>Perform-<BR>ances</TH></TR>\n");

  for (iVenue = g_AllVenues.begin(); iVenue != g_AllVenues.end(); ++iVenue)
  {
    std::string StartVenueHTML;
    std::string EndVenueHTML;
    if (!iVenue->second.m_VenueURL.empty ())
    {
      StartVenueHTML.append ("<A HREF=\"");
      StartVenueHTML.append (iVenue->second.m_VenueURL);
      StartVenueHTML.append ("\">");
      EndVenueHTML.insert (0, "</A>");
    }

    printf ("<TR VALIGN=\"TOP\"><TD>%s%s%s</TD><TD>%d</TD></TR>\n",
      StartVenueHTML.c_str (), iVenue->first.c_str(), EndVenueHTML.c_str (),
      iVenue->second.m_EventCount);
  }

  printf ("</TABLE>\n");

  // Write the hidden text box with the last update date, so we can tell if the
  // form controls match the pasted in state data.  If not, the controls will
  // be ignored as things like checkboxes won't match.

  printf ("<INPUT TYPE=\"HIDDEN\" NAME=\"LastUpdateTime\" VALUE=\"%s\">\n",
    g_AllSettings["LastUpdateTime"].c_str ());

  // Write out the SavedState giant text area.  To avoid HTML misinterpretation
  // problems, encode suspect characters for the data inside the textarea.

  char Separator = (0 != atoi (g_AllSettings["UseOnlyTabForFieldSeparator"].
    c_str ())) ? '\t' : '|';

  printf ("<H2><A NAME=\"RawData\"></A>Raw Data</H2>"
    "<P>You can copy this out and save it in a text file to preserve your "
    "selections.  Paste it back in later and hit the update button to "
    "restore your custom schedule.  If things go awry (or updated schedule "
    "times have been posted for your festival), start a fresh session for "
    "your festival and append just the lines near the end that start with "
    "\"Selected\".\n"
    "<P><TEXTAREA NAME=\"SavedState\" cols=80 rows=40>\n");

  // Dump the settings state.  Do it first so default settings get used when
  // reading the events.

  for (iSetting = g_AllSettings.begin(); iSetting != g_AllSettings.end(); ++iSetting)
  {
    snprintf (OutputBuffer, sizeof (OutputBuffer), "Setting%c%s%c%s\n",
      Separator, iSetting->first.c_str(), Separator, iSetting->second.c_str());
    EncodeAndPrintText (OutputBuffer);
  }

  // Dump the event states.

  int PreviousDayOfMonth = 0;
  for (iEvent = g_AllEvents.begin(); iEvent != g_AllEvents.end(); ++iEvent)
  {
    time_t EventTime = iEvent->first.m_EventTime;
    localtime_r (&EventTime, &BrokenUpDate);

    if (PreviousDayOfMonth != BrokenUpDate.tm_mday)
    {
      strftime (TimeString, sizeof (TimeString), "%A, %B %d, %Y",
        &BrokenUpDate);
      printf ("%s\n", TimeString);
      PreviousDayOfMonth = BrokenUpDate.tm_mday;
    }

    strftime (TimeString, sizeof (TimeString), "%H:%M", &BrokenUpDate);
    snprintf (OutputBuffer, sizeof (OutputBuffer), "%s%c%s%c%s\n",
      TimeString, Separator,
      iEvent->second.m_ShowIter->first.c_str(), Separator,
      iEvent->first.m_Venue->first.c_str());
    EncodeAndPrintText (OutputBuffer);
  }

  // Dump the show states, for the duration if not default, and the URL.
  // Favourites done later to make cutting and pasting easier.

  int DefaultShowDuration =
    60 * atoi (g_AllSettings["DefaultShowDuration"].c_str ());
  for (iShow = g_AllShows.begin(); iShow != g_AllShows.end(); ++iShow)
  {
    if (iShow->second.m_ShowDuration != DefaultShowDuration)
    {
      snprintf (OutputBuffer, sizeof (OutputBuffer),
        "ShowDuration%c%s%c%d\n", Separator, iShow->first.c_str (), Separator,
        iShow->second.m_ShowDuration / 60);
      EncodeAndPrintText (OutputBuffer);
    }

    if (!iShow->second.m_ShowURL.empty ())
    {
      snprintf (OutputBuffer, sizeof (OutputBuffer),
        "ShowURL%c%s%c%s\n", Separator, iShow->first.c_str(), Separator,
        iShow->second.m_ShowURL.c_str());
      EncodeAndPrintText (OutputBuffer);
    }
  }

  // Dump the venue states, just the URL.

  for (iVenue = g_AllVenues.begin(); iVenue != g_AllVenues.end(); ++iVenue)
  {
    if (!iVenue->second.m_VenueURL.empty ())
    {
      snprintf (OutputBuffer, sizeof (OutputBuffer), "VenueURL%c%s%c%s\n",
        Separator, iVenue->first.c_str(), Separator,
        iVenue->second.m_VenueURL.c_str());
      EncodeAndPrintText (OutputBuffer);
    }
  }

  // Write out the favourite shows.

  for (iShow = g_AllShows.begin(); iShow != g_AllShows.end(); ++iShow)
  {
    if (iShow->second.m_IsFavourite)
    {
      snprintf (OutputBuffer, sizeof (OutputBuffer),
        "Favourite%c%s\n", Separator, iShow->first.c_str());
      EncodeAndPrintText (OutputBuffer);
    }
  }

  // Write out the selected event flags.  Done after the events, so we can cut
  // and paste in new schedule information without losing our selections.

  for (iEvent = g_AllEvents.begin(); iEvent != g_AllEvents.end(); ++iEvent)
  {
    if (!iEvent->second.m_IsSelectedByUser)
      continue;

    time_t EventTime = iEvent->first.m_EventTime;
    localtime_r (&EventTime, &BrokenUpDate);
    strftime (TimeString, sizeof (TimeString), "%c", &BrokenUpDate);

    snprintf (OutputBuffer, sizeof (OutputBuffer), "Selected%c%s%c%s\n",
      Separator, TimeString, Separator,
      iEvent->first.m_Venue->first.c_str());
    EncodeAndPrintText (OutputBuffer);
  }

  printf ("</TEXTAREA>\n");


  printf ("<H3>Raw Data Keywords and Use</H3>\n");
  printf ("<P>Here's some documentation, in case you're interested in "
    "details.  In general fields are separated by tabs or vertical bar \"|\" "
    "characters.  The first field specifies what to do with the rest of the "
    "line of raw data.  Here's the list of possibilities:\n<UL>\n");
  printf ("<LI>An event is defined by a date and time field followed by a "
    "show name field and then a venue name field.  There's an optional and "
    "obsolete fourth field of \"Selected\" to show that the user is going to "
    "that event.  You can also specify a date by itself (such as \"Thursday, "
    "June 19, 2014\"), which will be used as the day for the event "
    "definitions after it (they then only need to specify the time).\n");
  printf ("<LI>\"Favourite\" is followed by the show name.  That show is then "
    "marked as being one of the higher priority ones for the user to see.\n");
  printf ("<LI>\"ShowURL\" has a second field that names a show and a third "
    "that specifies a web link to show information about the show.\n");
  printf ("<LI>\"ShowDuration\" is followed by the show name and the duration "
    "(number of minutes) of that show.  If not specified, a default (usually "
    "one hour, there's a setting for it) is used.\n");
  printf ("<LI>\"VenueURL\" is followed by the name of a venue and then the "
    "URL for information about that venue.\n");
  printf ("<LI>\"Selected\" is followed by a date & time field, and a field "
    "naming the venue.  That specifies an event the user will be attending, "
    "and is the preferred way of selecting events rather than the old "
    "\"Selected\" after the event definition way (easier for the user to cut "
    "and paste).\n");
  printf ("<LI>\"Setting\" is followed by the name of the setting and then "
    "a field with the value to be used for that setting.  Here are some of "
    "the settings you can use:\n<UL>\n");
  printf ("<LI>TitleEdit - followed by HTML for the title text shown "
    "while editing.  Useful for noting the date when the schedule "
    "times were last updated with data from the festival.\n");
  printf ("<LI>TitlePrint - followed by HTML for the title text shown "
    "on the printable listing.  You may want to customise it with your "
    "own name and other information.\n");
  printf ("<LI>Bleeble - need to finish writing this documentation.\n");

  printf ("</UL></UL>\n");

  printf ("</FORM>\n");
}


/******************************************************************************
 * Write out HTML for just the user's events, using only plain text, no form
 * controls or other things that would look bad on a printed page.
 */

void WritePrintableListing ()
{
  struct tm BrokenUpDate;
  EventIterator iEvent;
  char TimeString[40];
  int NewDayGapMinutes = atoi (g_AllSettings["NewDayGapMinutes"].c_str ());

  printf ("%s\n", g_AllSettings["TitlePrint"].c_str ());

  // Write out the event listing, with just the user's selected events.  Done
  // as a table with four columns: event time, duration, show name, venue name.

  printf ("<TABLE BORDER=\"1\" CELLPADDING=\"1\">\n");

  time_t PreviousTime = 0;
  for (iEvent = g_AllEvents.begin(); iEvent != g_AllEvents.end(); ++iEvent)
  {
    if (!iEvent->second.m_IsSelectedByUser)
      continue;

    // If long enough time has gone by, print out a new day heading.

    time_t EventTime = iEvent->first.m_EventTime;
    localtime_r (&EventTime, &BrokenUpDate);
    double DeltaTime = difftime (EventTime, PreviousTime);
    if (fabs (DeltaTime) > NewDayGapMinutes * 60)
    {
      strftime (TimeString, sizeof (TimeString), "%A, %B %d, %Y",
        &BrokenUpDate);
      printf ("<TR><TH COLSPAN=\"4\">%s</TH></TR>\n", TimeString);
    }
    PreviousTime = EventTime;

    // Print out the event itself.

    strftime (TimeString, sizeof (TimeString), "%H:%M", &BrokenUpDate);

    printf ("<TR VALIGN=\"TOP\"><TD>%s</TD><TD>%d</TD><TD>%s</TD>"
      "<TD>%s</TD></TR>\n",
      TimeString,
      iEvent->second.m_ShowIter->second.m_ShowDuration / 60,
      iEvent->second.m_ShowIter->first.c_str(),
      iEvent->first.m_Venue->first.c_str());
  }

  printf ("</TABLE>\n");

  // Print a footer with the statistics.

  printf ("<P ALIGN=\"CENTER\">");
  printf ("<P ALIGN=\"CENTER\">You have %d conflicts and you are seeing %d performances "
    "(total time %d:%02d).<BR>There are %d redundant, %d unseen favourites and %d unseen shows.\n",
    g_Statistics.m_TotalNumberOfConflicts,
    g_Statistics.m_TotalNumberOfEventsScheduled,
    g_Statistics.m_TotalSecondsWatched / 60 / 60,
    g_Statistics.m_TotalSecondsWatched / 60 % 60,
    g_Statistics.m_TotalNumberOfRedundantShows,
    g_Statistics.m_TotalNumberOfUnseenFavouriteShows,
    g_Statistics.m_TotalNumberOfUnseenShows);

  // Include the time when the table was printed, so you can tell different
  // versions of the table apart.

  time_t CurrentTime;
  time (&CurrentTime);
  localtime_r (&CurrentTime, &BrokenUpDate);
  strftime (TimeString, sizeof (TimeString), "%A, %B %d, %Y at %T", &BrokenUpDate);
  printf ("<P><FONT SIZE=\"-1\">Printed on %s.&nbsp;  Software version "
    "$Id: FFVSO.cpp,v 1.29 2014/08/29 17:10:04 agmsmith Exp agmsmith $ "
    "was compiled on " __DATE__ " at " __TIME__ ".</FONT>\n", TimeString);
}


/******************************************************************************
 * Build a list of all the events sorted by time and show name, a more
 * convenient order for the user than time and venue name.
 */

bool CompareEventByTimeAndShowName (EventIterator ItemA, EventIterator ItemB)
{
  // Compare the starting time of the events first.

  if (ItemA->first.m_EventTime < ItemB->first.m_EventTime)
    return true;

  if (ItemA->first.m_EventTime > ItemB->first.m_EventTime)
    return false;

  // If at the same time, then compare show names to sort them out.

  int StringTest = strcasecmp (
    ItemA->second.m_ShowIter->first.c_str (),
    ItemB->second.m_ShowIter->first.c_str ());

  if (StringTest < 0)
    return true;

  if (StringTest > 0)
    return false;

  // Fall back to comparing venue names.  Could happen if the show is showing
  // at two venues at the same time.

  StringTest = strcasecmp (
    ItemA->first.m_Venue->first.c_str (),
    ItemB->first.m_Venue->first.c_str ());

  if (StringTest < 0)
    return true;

  return false;
}


void SortEventsByTimeAndShow ()
{
  EventIterator iEvent;

  g_EventsSortedByTimeAndShow.clear ();
  g_EventsSortedByTimeAndShow.reserve (g_AllEvents.size ());

  for (iEvent = g_AllEvents.begin(); iEvent != g_AllEvents.end(); ++iEvent)
    g_EventsSortedByTimeAndShow.push_back (iEvent);

  std::sort (g_EventsSortedByTimeAndShow.begin (),
    g_EventsSortedByTimeAndShow.end (),
    CompareEventByTimeAndShowName);
}


/******************************************************************************
 * Compute conflicts, estimate walking times for the user, count number of
 * times the user sees each show and compute various other statistics.
 */

void ComputeConflictsAndStatistics ()
{
  EventIterator iEvent;

  /* Count up the scheduled events.  Each one gets added to the total for the
  show, and to the grand total. */

  for (iEvent = g_AllEvents.begin(); iEvent != g_AllEvents.end(); ++iEvent)
  {
    if (!iEvent->second.m_IsSelectedByUser)
      continue;

    iEvent->second.m_ShowIter->second.m_ScheduledCount++;

    g_Statistics.m_TotalNumberOfEventsScheduled++;

    g_Statistics.m_TotalSecondsWatched +=
      iEvent->second.m_ShowIter->second.m_ShowDuration;
  }

  /* Look for conflicts, where the start time of a show is inside the time
  range of the previous show (including the time it takes to walk between
  venues and the time it takes to wait in line to buy tickets).  Fortunately
  the events are already sorted by start time.  Future feature: do a path
  search through the time it takes to go from one venue to another to find a
  path between any two venues and the corresponding time. */

  int TimeBetweenEvents = // Currently the same for all venues.
    atoi (g_AllSettings["DefaultTravelTime"].c_str ()) * 60 +
    atoi (g_AllSettings["DefaultLineupTime"].c_str ()) * 60;
  EventIterator iPreviousEvent = g_AllEvents.end();
  time_t PreviousEventEndTime = 0;

  for (iEvent = g_AllEvents.begin(); iEvent != g_AllEvents.end(); ++iEvent)
  {
    if (!iEvent->second.m_IsSelectedByUser)
      continue;

    if (iEvent->first.m_EventTime < PreviousEventEndTime &&
    iPreviousEvent != g_AllEvents.end())
    {
      // Have a conflict with the previous event.

      iEvent->second.m_IsConflicting = true;
      g_Statistics.m_TotalNumberOfConflicts++;

      if (!iPreviousEvent->second.m_IsConflicting)
      {
        iPreviousEvent->second.m_IsConflicting = true;
        g_Statistics.m_TotalNumberOfConflicts++;
      }
    }

    iPreviousEvent = iEvent;
    PreviousEventEndTime = iEvent->first.m_EventTime +
      iEvent->second.m_ShowIter->second.m_ShowDuration +
      TimeBetweenEvents;
  }

  // Count up the shows seen more than once and ones not seen at all.

  ShowIterator iShow;
  for (iShow = g_AllShows.begin(); iShow != g_AllShows.end(); ++iShow)
  {
    if (iShow->second.m_ScheduledCount > 1)
      g_Statistics.m_TotalNumberOfRedundantShows++;
    else if (iShow->second.m_ScheduledCount <= 0)
    {
      g_Statistics.m_TotalNumberOfUnseenShows++;

      if (iShow->second.m_IsFavourite)
        g_Statistics.m_TotalNumberOfUnseenFavouriteShows++;
    }
  }
}


/******************************************************************************
 * Finally, the main program which drives it all.
 */

int main (int argc, char **argv)
{
  FormNameToValuesMap::iterator iFormPair;

  const int MAX_CONTENT_LENGTH = 50000000;
    /* Should be big enough for the largest Fringe show, but not so large that
    it will cause an out of memory problem (max 800MB user space in BeOS, minus
    overhead of web server). */

  /* Read the data from the web browser, via standard input.  An environment
  variable specifies the maximum length to be read, if known.  Use it so that
  multiple transfers per HTTP session work.  If not present, read until end of
  file.  Stuff it all into a big memory buffer which will be worked over later.
  */

  int ContentLength = MAX_CONTENT_LENGTH; // Default if none specified.
  const char *pContentLength;
  pContentLength = getenv ("CONTENT_LENGTH");
  if (pContentLength != NULL)
    ContentLength = atoi (pContentLength);
  if (ContentLength < 0)
    ContentLength = 0;
  if (ContentLength > MAX_CONTENT_LENGTH)
    ContentLength = MAX_CONTENT_LENGTH;

  g_InputFormText = new char [ContentLength+1];

  int AmountRead = fread (g_InputFormText, 1, ContentLength, stdin);
  g_InputFormText[AmountRead ] = 0;

  WriteHTMLHeader ();

#if 0
  printf ("<PRE>Argc %d, argv: ", argc);
  for (int i = 0; i < argc; i++)
    printf ("%s%s", argv[i], (i < argc - 1) ? ", " : "\n");
  printf ("Content length is %d.\n", ContentLength);
  printf ("AmountRead is %d.\n", AmountRead);
  printf ("Original text: %s\n", g_InputFormText);
  printf ("</PRE>\n");
#endif

  BuildFormNameAndValuePairsFromFormInput ();

#if 0
  printf ("Converted form input:\n<PRE>");
  for (iFormPair = g_FormNameValuePairs.begin();
  iFormPair != g_FormNameValuePairs.end(); ++iFormPair)
  {
    printf ("Name \"%s\", value: %s\n", iFormPair->first, iFormPair->second);
  }
  printf ("</PRE>\n");
#endif

  // Set up the global list of settings, for things like the HTML strings that
  // highlight selected items.  They will be overwritten by user provided
  // settings.  Some are also used for defaults while reading other values.

  InitialiseDefaultSettings ();

  // Load up the saved state information first, we'll apply the user's changes
  // (checkbox markings) afterwards.  If there isn't any saved data, just leave
  // it empty.

  iFormPair = g_FormNameValuePairs.find (
    const_cast<char *>("SavedState"));
  if (iFormPair != g_FormNameValuePairs.end ())
  {
    LoadStateInformation (iFormPair->second);
  }

  // Check that the submitted form controls match the SavedState data.  They
  // can mismatch if someone pastes in some old or new data into the SavedState
  // text box.  If so, don't read the controls from the form, they are invalid
  // in so many ways.

  bool bFormDataMatchesStateData = false;
  iFormPair = g_FormNameValuePairs.find (
    const_cast<char *> ("LastUpdateTime"));
  if (iFormPair != g_FormNameValuePairs.end ())
  {
    bFormDataMatchesStateData =
     (g_AllSettings["LastUpdateTime"] == iFormPair->second);
  }

  if (bFormDataMatchesStateData)
  {
    ReadFormControls ();
  }

  SortEventsByTimeAndShow ();
  ComputeConflictsAndStatistics ();
  ResetDynamicSettings (); // New form needs new date.

  if (g_FormNameValuePairs.find (const_cast<char *> ("PrintSchedule")) !=
  g_FormNameValuePairs.end ()) // Was the printable schedule button used?
    WritePrintableListing (); // Just the user's events listed plainly.
  else // Output the user interface with all the bells and whistles.
    WriteHTMLForm ();

  // Dump out some debug information.

#if 0
  printf ("<PRE>\n");
  printf ("List of %lu shows:\n", g_AllShows.size ());
  ShowIterator iShow;
  for (iShow = g_AllShows.begin(); iShow != g_AllShows.end(); ++iShow)
  {
    printf ("Show \"%s\", Favourite %d, EventCount %d, ScheduledCount %d, URL \"%s\".\n",
      iShow->first.c_str(), iShow->second.m_IsFavourite,
      iShow->second.m_EventCount, iShow->second.m_ScheduledCount,
      iShow->second.m_ShowURL.c_str());
  }

  printf ("List of %lu venues:\n", g_AllVenues.size ());
  VenueIterator iVenue;
  for (iVenue = g_AllVenues.begin(); iVenue != g_AllVenues.end(); ++iVenue)
  {
    printf ("Venue \"%s\", EventCount %d, URL \"%s\".\n",
      iVenue->first.c_str(), iVenue->second.m_EventCount,
      iVenue->second.m_VenueURL.c_str());
  }

  printf ("List of %lu events:\n", g_AllEvents.size ());
  EventIterator iEvent;
  for (iEvent = g_AllEvents.begin(); iEvent != g_AllEvents.end(); ++iEvent)
  {
    struct tm BrokenUpDate;
    char TimeString[32];

    localtime_r (&iEvent->first.m_EventTime, &BrokenUpDate);
    strcpy (TimeString, asctime (&BrokenUpDate));
    TimeString[strlen(TimeString)-1] = 0; // Trash trailing linefeed.

    printf ("Event %s/%s, \"%s\"%s\n",
      TimeString, iEvent->first.m_Venue->first.c_str(),
      iEvent->second.m_ShowIter->first.c_str(),
      (iEvent->second.m_IsSelectedByUser) ? ", Selected" : "");
  }

  printf ("List of %lu settings:\n", g_AllSettings.size ());
  SettingIterator iSetting;
  for (iSetting = g_AllSettings.begin(); iSetting != g_AllSettings.end(); ++iSetting)
  {
    printf ("Setting \"%s\" is \"%s\".\n",
      iSetting->first.c_str(), iSetting->second.c_str());
  }
  printf ("</PRE>\n");
#endif

  printf ("</BODY>\n</HTML>\n");

  g_EventsSortedByTimeAndShow.clear();
  g_AllEvents.clear();
  g_AllShows.clear();
  g_AllVenues.clear();
  g_AllSettings.clear();
  g_FormNameValuePairs.clear ();
  delete [] g_InputFormText;
  g_InputFormText = NULL;
  return 0;
}
