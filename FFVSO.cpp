/******************************************************************************
 * $Header: /CommonBe/agmsmith/Programming/Fringe\040Festival\040Visitor\040Schedule\040Optimiser/RCS/FFVSO.cpp,v 1.3 2014/06/11 01:09:01 agmsmith Exp agmsmith $
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

typedef struct ShowStruct
{
  std::string m_ShowName;
    /* A redundant copy of the show's name (it's also in the map). */

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
  /* A list of all the uniquely named shows. */

char *g_InputStateText;
  /* The state of the whole thing, as received from the web browser textarea
  box.  Will be decoded and line ends fixed up before being used to set the
  list of shows and their selection times.  NUL terminated string. */


/******************************************************************************
 * Convert the given string from Form URL Encoded to plain text.  That means
 * replacing all "%xy" codes (byte hex encoded) with the corresponding hex
 * character, and "+" with a space.  Then run through the text again and
 * replace CRLF or plain CR with just LF, our standard end of line character.
 */
void FormEncodedToPlainText (char *pBuffer)
{
  char *pDest;
  char *pSource;

  pSource = pDest = pBuffer;
  while (*pSource != 0)
  {
    char Letter = *pSource++;

    if (Letter == '+') // Just a space.
    {
      *pDest++ = ' ';
    }
    else if (Letter == '\r' || Letter == '\n')
      break; // A real end of line marks the end of the text.
    else if (Letter == '%') // Hex encoded byte.
    {
      char Hex1 = tolower (pSource[0]);
      int Value1 = -1;
      if (Hex1 >= '0' && Hex1 <= '9')
        Value1 = Hex1 - '0';
      else if (Hex1 >= 'a' && Hex1 <= 'f')
        Value1 = Hex1 - 'a' + 10;

      if (Value1 != -1) // Valid hex digit previously, also not NUL end of string.
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
      if (Value1 == -1)
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
}


/******************************************************************************
 * Finally, the main program which drives it all.
 */

int main (int argc, char**)
{
  const int MAX_CONTENT_LENGTH = 50000000;
    /* Should be big enough for the largest Fringe show, but not so large that
    it will cause an out of memory problem (max 800MB user space in BeOS, minus
    overhead of web server). */

  /* Read the data from the web browser, via standard input.  An environment
  specifies the maximum length to be read, if known.  Use it so that multiple
  transfers per HTTP session work.  If not present, read until end of file.
  Stuff it all into a big memory buffer which will be worked over later. */

  int ContentLength = MAX_CONTENT_LENGTH; // Default if none specified.
  const char *pContentLength;
  pContentLength = getenv ("CONTENT_LENGTH");
  if (pContentLength != NULL)
    ContentLength = atoi (pContentLength);
  if (ContentLength < 0)
    ContentLength = 0;
  if (ContentLength > MAX_CONTENT_LENGTH)
    ContentLength = MAX_CONTENT_LENGTH;

  g_InputStateText = new char [ContentLength+1];

  int AmountRead = fread (g_InputStateText, 1, ContentLength, stdin);
  g_InputStateText[AmountRead ] = 0;

  printf ("Content-Type: text/plain\r\n\r\n"); // Magic CGI header.
  printf ("Content length is %d.\n", ContentLength);
  printf ("AmountRead is %d.\n", AmountRead);
  printf ("Original text: %s\n", g_InputStateText);

  const char *StartString = "SavedState=";
  char *pStartOfState = strstr (g_InputStateText, StartString); 
  if (pStartOfState != NULL)
  {
    pStartOfState += strlen (StartString); // Skip over starting string.
    FormEncodedToPlainText (pStartOfState);
    int AmountWritten = fwrite (pStartOfState, 1, strlen (pStartOfState), stdout);
    printf ("\nAmountWritten: %d\nThe end.\n", AmountWritten);
  }

  delete [] g_InputStateText;
  return 0;
}
