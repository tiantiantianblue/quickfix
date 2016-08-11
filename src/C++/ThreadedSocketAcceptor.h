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

#ifndef FIX_THREADEDSOCKETACCEPTOR_H
#define FIX_THREADEDSOCKETACCEPTOR_H

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 4290 )
#endif

#include "ThreadedSocketConnection.h"
#include "Mutex.h"
#include "Application.h"
#include "MessageStore.h"
#include "Log.h"
#include "Responder.h"
#include "SessionSettings.h"
#include "Exceptions.h"
#include <map>
#include <string>
namespace FIX
{
	/// Threaded Socket implementation of Acceptor.
	class ThreadedSocketAcceptor
	{
		friend class SocketConnection;
	public:
		ThreadedSocketAcceptor(
			Application& application,
			MessageStoreFactory& factory,
			LogFactory& logFactory) throw(ConfigError);

		~ThreadedSocketAcceptor();

		/// Start acceptor.
		void start() throw (ConfigError, RuntimeError);

		/// Stop acceptor.
		void stop(bool force = false);

		/// Check to see if any sessions are currently logged on
		bool isLoggedOn();

		bool has(const SessionID& id)
		{
			return m_sessions.find(id) != m_sessions.end();
		}

		bool isStopped() { return m_stop; }

		void socketAccept();

	private:
		struct ConnectionThreadInfo
		{
			ConnectionThreadInfo(ThreadedSocketAcceptor* pAcceptor,
				ThreadedSocketConnection* pConnection)
				: m_pAcceptor(pAcceptor), m_pConnection(pConnection) {}

			ThreadedSocketAcceptor* m_pAcceptor;
			ThreadedSocketConnection* m_pConnection;
		};

		bool readSettings(const SessionSettings&);

		typedef std::set < SessionID > SessionIDs;
		typedef std::map < SessionID, std::shared_ptr<Session> > Sessions;

		void initialize() throw (ConfigError);
		bool onPoll(double timeout);
		void onStop();

		void addThread(int s, thread_id t);
		void removeThread(int s);

		static THREAD_PROC startThread(void* p);
		static THREAD_PROC socketConnectionThread(void* p);

		int m_socket;
		int m_port;
		Mutex m_mutex;

		thread_id m_threadid{ 0 };
		std::map < int, thread_id > m_threads;
		Sessions m_sessions;
		SessionIDs m_sessionIDs;
		Application& m_application;
		MessageStoreFactory& m_messageStoreFactory;
		LogFactory* m_pLogFactory;
		Log* m_pLog;
		bool m_firstPoll{ true };
		bool m_stop{ true };

		bool m_SOCKET_NODELAY{ false };
		int m_SOCKET_SEND_BUFFER_SIZE{ 0 };
		int m_SOCKET_RECEIVE_BUFFER_SIZE{ 0 };
	};
}

#endif //FIX_THREADEDSOCKETACCEPTOR_H
