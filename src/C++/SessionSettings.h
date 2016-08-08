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

#ifndef FIX_SESSIONSETTINGS_H
#define FIX_SESSIONSETTINGS_H

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 4290 )
#endif

#include "KeyWord.h"
#include "Dictionary.h"
#include "SessionID.h"
#include "Exceptions.h"
#include <map>
#include <set>

namespace FIX
{
	/// Container for setting dictionaries mapped to sessions.
	class SessionSettings
	{
		typedef std::map < SessionID, Dictionary > Dictionaries;
	public:
		static void init(const std::string& file)
		{
			m_file = file;
		}
		static const SessionSettings& instance()
		{
			static SessionSettings ss(m_file);
			return ss;
		}

		/// Check if session setings are present
		const bool has(const SessionID&) const;

		const Dictionaries& getDictionaries() const;

		/// Get a dictionary for a session.
		const Dictionary& get(const SessionID&) const throw(ConfigError);

		/// Get global default settings
		const Dictionary& get() const { return m_defaults; }


		/// Number of session settings
		size_t size() const { return m_settings.size(); }

		std::set < SessionID > getSessions() const;

	private:
		SessionSettings(std::istream& stream) throw(ConfigError);
		SessionSettings(const std::string& file) throw(ConfigError);
		void validate(const Dictionary&) const throw(ConfigError);
		void set(const SessionID&, Dictionary) throw(ConfigError);
		void setDeault(const Dictionary& defaults) throw(ConfigError);
		Dictionaries m_settings;
		Dictionary m_defaults;
		static std::string m_file;

		friend std::ostream& operator<<(std::ostream&, const SessionSettings&);
		friend std::istream& operator >> (std::istream& stream, SessionSettings& s);
	};
	/*! @} */

	std::istream& operator >> (std::istream&, SessionSettings&)
		throw(ConfigError);
	std::ostream& operator<<(std::ostream&, const SessionSettings&);
}

#endif //FIX_SESSIONSETTINGS_H
