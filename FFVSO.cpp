/******************************************************************************
 * $Header: /home/agmsmith/Programming/Fringe\040Festival\040Visitor\040Schedule\040Optimiser/RCS/FFVSO.cpp,v 1.9 2014/06/18 03:11:40 agmsmith Exp agmsmith $
 *
 * This is a web server CGI program for selecting events (shows) at the Ottawa
 * Fringe Theatre Festival to make up an individual's custom list.  Choices are
 * made on a web page and the results saved as a big blob of text in a text box
 * on the same web page.  Statistics showing conflicts in time, missing
 * favourite shows and other such info guide the user in selecting shows.
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

/* parsedate library taken from Haiku OS source for Unix, native in BeOS. */

#include <parsedate.h>

/* Standard C++ library. */

/* STL (Standard Template Library) headers. */

#include <map>
#include <string>


/******************************************************************************
 * Class which contains information about a single show.  Basically just the
 * title and favourite status, since the show can have multiple venues and
 * times.
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

  std::string m_ShowURL;
    /* A link to a web page about the show. */

  ShowStruct () : m_EventCount(0), m_IsFavourite(false), m_ScheduledCount(0)
  {};

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

  ShowIterator m_ShowIter;
    /* Identifies the show that is being performed at this place and time. */

  EventStruct () : m_IsSelectedByUser(false)
  {};

} EventRecord, *EventPointer;

typedef std::map<EventKeyStruct, EventRecord, EventKeyStruct> EventMap;
typedef EventMap::iterator EventIterator;

EventMap g_AllEvents;
  /* A collection of all the events. */


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
 * charaters.
 *
 * We look for a time or keyword first.  If it's just a date (no more tab
 * separated fields after it) then it becomes the current default date.  If
 * it's a keyword, then the number of fields after it depend on the keyword.
 * If it's a time with three or more fields in total, then it's an event, where
 * the first field is the time (combine with the default date to get an
 * absolute time), the second field is the show name and the third is the venue
 * name, and the fourth is the selected flag ("Selected" to pick the event,
 * anything else or missing to not pick it).
 *
 * The keywords are used for storing extra information.  They are:
 *   "Favourite" with the second field being the show name.  That show is then
 *      marked as being one of the higher priority ones for the user to see.
 *   "ShowURL" has a second field that names a show and a third that specifies
 *     a web link to show information about the show.
 *   "VenueURL" second field names a venue, third has the URL for information
 *     about it.
 */

void LoadStateInformation (const char *pBuffer)
{

  // Set the running date to be the start of the current year, in case they
  // specify events without mentioning the year.

  struct tm BrokenUpDate;
  time_t RunningDate;

  time (&RunningDate);
  localtime_r (&RunningDate, &BrokenUpDate);
  BrokenUpDate.tm_sec = 0;
  BrokenUpDate.tm_min = 0;
  BrokenUpDate.tm_hour = 2; // Avoid daylight savings time problems.
  BrokenUpDate.tm_mday = 1;
  BrokenUpDate.tm_mon = 0;
  BrokenUpDate.tm_isdst = 0;
  RunningDate = mktime (&BrokenUpDate);

  const int MAX_FIELDS = 4;
  std::string aFields[MAX_FIELDS];

  const char *pSource = pBuffer;
  while (*pSource != 0)
  {
    // Starting a new line of text, reset the field markers.

    int iField;
    for (iField = 0; iField < MAX_FIELDS; iField++)
      aFields[iField].clear();
    iField = 0;

    // Start hunting for fields.

    char Letter = *pSource;
    while (Letter != 0 && Letter != '\n')
    {
      while (Letter == ' ') // Skip leading spaces, but not tabs.
        Letter = *++pSource;
      const char *pFieldStart = pSource;

      while (Letter != '\t' && Letter != '\n' && Letter != 0)
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

      if (Letter == '\t') // Leave LF and NUL alone so outer loop exits.
      {
        Letter = *++pSource;
        pFieldStart = pSource;
      }
    }
    int nFields = iField; // Keep the count of the number of fields for later.

    // Finished reading a line of input, now process the fields.

    if (nFields > 0)
    {
#if 1
      printf ("%d fields: ", nFields);
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
#endif

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

        if (nFields >= 3) // Have at least show and venue after the date.
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
            printf ("<P>Slight problem: ignoring redundant occurance of an "
              "identical event (same place and time).  It's show \"%s\" "
              "(prior show is \"%s\"), venue \"%s\", at time %s",
              NewEvent.m_ShowIter->first.c_str(),
              InsertEventResult.first->second.m_ShowIter->first.c_str(),
              NewEventKey.m_Venue->first.c_str(),
              asctime (&BrokenUpDate));
          }
          else // Successfully added an event.
          {
            InsertShowResult.first->second.m_EventCount++;
            InsertVenueResult.first->second.m_EventCount++;
          }
        }
      }
    }

    if (Letter == '\n') // Leave NUL alone so outer loop exits.
      Letter = *++pSource;
  }
}


/******************************************************************************
 * Finally, the main program which drives it all.
 */

int main (int argc, char**)
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

  printf ("Content-Type: text/plain\r\n\r\n"); // Magic CGI header.
  printf ("Content length is %d.\n", ContentLength);
  printf ("AmountRead is %d.\n", AmountRead);
//  printf ("Original text: %s\n", g_InputFormText);

  BuildFormNameAndValuePairsFromFormInput ();

#if 0
  printf ("Converted form input:\n");
  for (iFormPair = g_FormNameValuePairs.begin();
  iFormPair != g_FormNameValuePairs.end(); ++iFormPair)
  {
    printf ("Name \"%s\", value: %s\n", iFormPair->first, iFormPair->second);
  }
#endif

  // Load up the saved state information first, we'll apply the user's changes
  // (checkbox markings) afterwards.  If there isn't any saved data, just leave
  // it empty.

  iFormPair = g_FormNameValuePairs.find ((char *) "SavedState");
  if (iFormPair != g_FormNameValuePairs.end ())
  {
    LoadStateInformation (iFormPair->second);
  }

#if 1
  printf ("List of %lu shows:\n", g_AllShows.size ());
  ShowIterator iShow;
  for (iShow = g_AllShows.begin(); iShow != g_AllShows.end(); ++iShow)
  {
    printf ("Show \"%s\", Favourite %d, EventCount %d, ScheduledCount %d, URL \"%s\".\n",
      iShow->first.c_str(), iShow->second.m_IsFavourite,
      iShow->second.m_EventCount, iShow->second.m_ScheduledCount,
      iShow->second.m_ShowURL.c_str());
  }
#endif

#if 1
  printf ("List of %lu venues:\n", g_AllVenues.size ());
  VenueIterator iVenue;
  for (iVenue = g_AllVenues.begin(); iVenue != g_AllVenues.end(); ++iVenue)
  {
    printf ("Venue \"%s\", EventCount %d, URL \"%s\".\n",
      iVenue->first.c_str(), iVenue->second.m_EventCount,
      iVenue->second.m_VenueURL.c_str());
  }
#endif

#if 1
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
#endif

  g_AllEvents.clear();
  g_AllShows.clear();
  g_AllVenues.clear();
  g_FormNameValuePairs.clear ();
  delete [] g_InputFormText;
  return 0;
}
