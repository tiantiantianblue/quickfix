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

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#include "Settings.h"

namespace FIX
{
bool isComment( const std::string& line )
{
  return line[0] == '#';
}

bool isSection( const std::string& line )
{
  return line[0] == '[' && line[line.size()-1] == ']';
}

std::string splitSection( const std::string& line )
{
  return string_strip(std::string( line, 1, line.size() - 2 ));
}

bool isKeyValue( const std::string& line )
{
  return line.find( '=' ) != std::string::npos;
}

std::pair<std::string, std::string> splitKeyValue( const std::string& line )
{
  size_t equals = line.find( '=' );
  std::string key = std::string( line, 0, equals );
  std::string value = std::string( line, equals + 1, std::string::npos );
  return std::pair<std::string, std::string>( std::move(key), std::move(value));
}

std::istream& operator>>( std::istream& stream, Settings& s )
{
  std::string buffer;
  std::string line;

  while (getline(stream, buffer))
  {
    line = string_strip( buffer );
    if(line.empty()||isComment(line) )
      continue;
    if( isSection(line) )
    {
      s.m_sections.emplace_back(splitSection(line));
    }
    else if(!s.m_sections.empty()&&isKeyValue(line) )
    {
		std::pair<std::string, std::string> keyValue = splitKeyValue(line);
		s.m_sections.back().setString(keyValue.first, keyValue.second);
    }
  }
  return stream;
}

Settings::Sections Settings::get( const std::string& name ) const
{
  Sections sections;
  for ( Sections::size_type i = 0; i < m_sections.size(); ++i )
    if ( m_sections[ i ].getName() == name )
      sections.push_back( m_sections[ i ] );
  return sections;
}
}
