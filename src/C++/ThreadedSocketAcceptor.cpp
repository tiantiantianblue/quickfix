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

#include "ThreadedSocketAcceptor.h"
#include "Settings.h"
#include "Utility.h"
#include "Session.h"
#include "HttpServer.h"
#include "SessionFactory.h"
#include <algorithm>
#include <fstream>

namespace FIX
{
	ThreadedSocketAcceptor::ThreadedSocketAcceptor(
		Application& application,
		MessageStoreFactory& factory,
		LogFactory& logFactory) throw(ConfigError)
		:m_application(application),
		m_messageStoreFactory(factory),
		m_pLogFactory(&logFactory),
		m_pLog(logFactory.create())
	{
		initialize();
		socket_init();
	}

	ThreadedSocketAcceptor::~ThreadedSocketAcceptor()
	{
		socket_term();
		if (m_pLogFactory && m_pLog)
			m_pLogFactory->destroy(m_pLog);
	}

	void ThreadedSocketAcceptor::initialize() throw (ConfigError)
	{
		SessionFactory factory(m_application, m_messageStoreFactory, m_pLogFactory);
		for (const auto& kv : SessionSettings::instance().getDictionaries())
		{
			if (kv.second.getString(CONNECTION_TYPE) == "acceptor")
			{
				m_sessionIDs.insert(kv.first);
				m_sessions[kv.first] = factory.create(kv.first, kv.second);
			}
		}
		if (m_sessions.empty())
			throw ConfigError("No sessions defined for acceptor");

		auto settings = SessionSettings::instance().get();

		m_port = settings.getInt(SOCKET_ACCEPT_PORT);
		if (settings.has(SOCKET_NODELAY))
			m_SOCKET_NODELAY = settings.getBool(SOCKET_NODELAY);

		if (settings.has(SOCKET_SEND_BUFFER_SIZE))
			m_SOCKET_SEND_BUFFER_SIZE = settings.getInt(SOCKET_SEND_BUFFER_SIZE);

		if (settings.has(SOCKET_RECEIVE_BUFFER_SIZE))
			m_SOCKET_RECEIVE_BUFFER_SIZE = settings.getInt(SOCKET_RECEIVE_BUFFER_SIZE);
	}

	void ThreadedSocketAcceptor::start() throw (ConfigError, RuntimeError)
	{
		m_stop = false;

		m_socket = socket_createAcceptor(m_port, SessionSettings::instance().get().has(SOCKET_REUSE_ADDRESS) ?
			SessionSettings::instance().get().getBool(SOCKET_REUSE_ADDRESS) : true);

		if (m_socket < 0)
		{
			socket_close(m_socket);
			throw RuntimeError("Unable to create, bind, or listen to port ");
		}

		HttpServer::startGlobal();
		if (!thread_spawn(&startThread, this, m_threadid))
			throw RuntimeError("Unable to spawn thread");
	}

	void ThreadedSocketAcceptor::stop(bool force)
	{
		if (isStopped()) 
			return;
		HttpServer::stopGlobal();
		std::vector<std::shared_ptr<Session> > enabledSessions;

		Sessions sessions = m_sessions;
		for (Sessions::iterator i = sessions.begin(); i != sessions.end(); ++i)
		{
			auto pSession = Session::lookupSession(i->first);
			if (pSession && pSession->isEnabled())
			{
				enabledSessions.push_back(pSession);
				pSession->logout();
			}
		}

		if (!force)
		{
			for (int second = 1; second <= 10 && isLoggedOn(); ++second)
				process_sleep(1);
		}

		m_stop = true;
		onStop();
		if (m_threadid)
			thread_join(m_threadid);
		m_threadid = 0;

		std::vector<std::shared_ptr<Session>>::iterator session = enabledSessions.begin();
		for (; session != enabledSessions.end(); ++session)
			(*session)->logon();
	}

	bool ThreadedSocketAcceptor::isLoggedOn()
	{
		Sessions sessions = m_sessions;
		Sessions::iterator i = sessions.begin();
		for (; i != sessions.end(); ++i)
		{
			if (i->second->isLoggedOn())
				return true;
		}
		return false;
	}

	THREAD_PROC ThreadedSocketAcceptor::startThread(void* p)
	{
		ThreadedSocketAcceptor * pAcceptor = static_cast <ThreadedSocketAcceptor*> (p);
		pAcceptor->socketAccept();
		return 0;
	}


	void ThreadedSocketAcceptor::onStop()
	{
		std::map < int, thread_id > threads;

		{
			Locker l(m_mutex);

			time_t start = 0;
			time_t now = 0;

			time(&start);
			while (isLoggedOn())
			{
				if (time(&now) - 5 >= start)
					break;
			}

			threads = m_threads;
			m_threads.clear();
		}

		for (auto i = threads.begin(); i != threads.end(); ++i)
			socket_close(i->first);
		for (auto i = threads.begin(); i != threads.end(); ++i)
			thread_join(i->second);
	}

	void ThreadedSocketAcceptor::addThread(int s, thread_id t)
	{
		Locker l(m_mutex);
		m_threads[s] = t;
	}

	void ThreadedSocketAcceptor::removeThread(int s)
	{
		Locker l(m_mutex);
		auto i = m_threads.find(s);
		if (i != m_threads.end())
		{
			thread_detach(i->second);
			m_threads.erase(i);
		}
	}

	void ThreadedSocketAcceptor::socketAccept()
	{
		int socket = 0;
		while ((!isStopped() && (socket = socket_accept(m_socket)) >= 0))
		{
			if (m_SOCKET_NODELAY)
				socket_setsockopt(socket, TCP_NODELAY);
			if (m_SOCKET_SEND_BUFFER_SIZE)
				socket_setsockopt(socket, SO_SNDBUF, m_SOCKET_SEND_BUFFER_SIZE);
			if (m_SOCKET_RECEIVE_BUFFER_SIZE)
				socket_setsockopt(socket, SO_RCVBUF, m_SOCKET_RECEIVE_BUFFER_SIZE);

			ThreadedSocketConnection * pConnection =
				new ThreadedSocketConnection(socket, m_pLog);

			ConnectionThreadInfo* info = new ConnectionThreadInfo(this, pConnection);

			{
				Locker l(m_mutex);
				m_pLog->onEvent(std::string("Accepted connection from ")+
					socket_peername(socket)+ " on port " + std::to_string(m_port));

				thread_id thread;
				if (!thread_spawn(&socketConnectionThread, info, thread))
					delete info;
				addThread(socket, thread);
			}
		}

		if (!isStopped())
			removeThread(m_socket);
	}

	THREAD_PROC ThreadedSocketAcceptor::socketConnectionThread(void* p)
	{
		ConnectionThreadInfo * info = static_cast <ConnectionThreadInfo*> (p);

		ThreadedSocketAcceptor* pAcceptor = info->m_pAcceptor;
		ThreadedSocketConnection* pConnection = info->m_pConnection;
		delete info;

		while (pConnection->read()) {}
		delete pConnection;
		if (!pAcceptor->isStopped())
			pAcceptor->removeThread(pConnection->getSocket());
		return 0;
	}
}
