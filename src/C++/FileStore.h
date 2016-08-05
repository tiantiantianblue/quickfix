/* -*- C++ -*- */

/****************************************************************************
** Copyright (c) 2001-2014
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifndef FIX_FILESTORE_H
#define FIX_FILESTORE_H

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 4290 )
#endif

#include "MessageStore.h"
#include "SessionSettings.h"
#include <fstream>
#include <string>

namespace FIX
{
class Session;

/// Creates a file based implementation of MessageStore.
class FileStoreFactory : public MessageStoreFactory
{
public:
  FileStoreFactory()
 {};
  FileStoreFactory( const std::string& path )
: m_path( path ) {};

  MessageStore* create( const SessionID& );
  void destroy( MessageStore* );
private:
  std::string m_path;
  
};
/*! @} */

/**
 * File based implementation of MessageStore.
 *
 * Four files are created by this implementation.  One for storing outgoing
 * messages, one for indexing message locations, one for storing sequence numbers,
 * and one for storing the session creation time.
 *
 * The formats of the files are:<br>
 * &nbsp;&nbsp;
 *   [path]+[BeginString]-[SenderCompID]-[TargetCompID].body<br>
 * &nbsp;&nbsp;
 *   [path]+[BeginString]-[SenderCompID]-[TargetCompID].header<br>
 * &nbsp;&nbsp;
 *   [path]+[BeginString]-[SenderCompID]-[TargetCompID].seqnums<br>
 * &nbsp;&nbsp;
 *   [path]+[BeginString]-[SenderCompID]-[TargetCompID].session<br>
 *
 *
 * The messages file is a pure stream of %FIX messages.<br><br>
 * The sequence number file is in the format of<br>
 * &nbsp;&nbsp;
 *   [SenderMsgSeqNum] : [TargetMsgSeqNum]<br><br>
 * The session file is a UTC timestamp in the format of<br>
 * &nbsp;&nbsp;
 *   YYYYMMDD-HH:MM:SS
 */
class FileStore : public MessageStore
{
public:
  FileStore( std::string, const SessionID& s );
  virtual ~FileStore();

  bool set( int, const std::string& ) throw ( IOException );
  void get( int, int, std::vector < std::string > & ) const throw ( IOException );

  int getNextSenderMsgSeqNum() const throw ( IOException );
  int getNextTargetMsgSeqNum() const throw ( IOException );
  void setNextSenderMsgSeqNum( int value ) throw ( IOException );
  void setNextTargetMsgSeqNum( int value ) throw ( IOException );
  void incrNextSenderMsgSeqNum() throw ( IOException );
  void incrNextTargetMsgSeqNum() throw ( IOException );

  UtcTimeStamp getCreationTime() const throw ( IOException );

  void reset() throw ( IOException );
  void refresh() throw ( IOException );

private:
  typedef std::pair < int, int > OffsetSize;
  typedef std::map < int, OffsetSize > NumToOffset;

  void open( bool deleteFile );
  void populateCache();
  void setSeqNum();
  void setSession();

  bool get( int, std::string& ) const throw ( IOException );

  MemoryStore m_cache;
  NumToOffset m_offsets;

  std::string m_msgFileName;
  std::string m_headerFileName;
  std::string m_seqNumsFileName;
  std::string m_sessionFileName;

  FILE* m_msgFile;
  FILE* m_headerFile;
  FILE* m_seqNumsFile;
  FILE* m_sessionFile;
};
}

#endif //FIX_FILESTORE_H
