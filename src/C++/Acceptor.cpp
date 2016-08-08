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

#include "Acceptor.h"
#include "Utility.h"
#include "Session.h"
#include "HttpServer.h"
#include "SessionFactory.h"
#include <algorithm>
#include <fstream>

namespace FIX
{
	Acceptor::Acceptor(Application& application,
		MessageStoreFactory& messageStoreFactory,
		LogFactory& logFactory)
		throw(ConfigError)
		: m_threadid(0),
		m_application(application),
		m_messageStoreFactory(messageStoreFactory),
		m_pLogFactory(&logFactory),
		m_pLog(logFactory.create()),
		m_firstPoll(true),
		m_stop(true)
	{
		initialize();
	}

	void Acceptor::initialize() throw (ConfigError)
	{
		SessionFactory factory(m_application, m_messageStoreFactory,
			m_pLogFactory);
		for (const auto kv : SessionSettings::instance().getDictionaries())
		{
			if (kv.second.getString(CONNECTION_TYPE) == "acceptor")
			{
				m_sessionIDs.insert(kv.first);
				m_sessions[kv.first] = factory.create(kv.first, kv.second);
			}
		}

		if (m_sessions.empty())
			throw ConfigError("No sessions defined for acceptor");
	}

	Acceptor::~Acceptor()
	{
		if (m_pLogFactory && m_pLog)
			m_pLogFactory->destroy(m_pLog);
	}

	std::shared_ptr<Session> Acceptor::getSession
	(const std::string& msg, Responder& responder)
	{
		Message message;
		if (!message.setStringHeader(msg))
			return 0;

		BeginString beginString;
		SenderCompID clSenderCompID;
		TargetCompID clTargetCompID;
		MsgType msgType;
		try
		{
			message.getHeader().getField(beginString);
			message.getHeader().getField(clSenderCompID);
			message.getHeader().getField(clTargetCompID);
			message.getHeader().getField(msgType);
			if (msgType != "A") return 0;

			SenderCompID senderCompID(clTargetCompID);
			TargetCompID targetCompID(clSenderCompID);
			SessionID sessionID(beginString, senderCompID, targetCompID);

			Sessions::iterator i = m_sessions.find(sessionID);
			if (i != m_sessions.end())
			{
				i->second->setResponder(&responder);
				return i->second;
			}
		}
		catch (FieldNotFound&) {}
		return 0;
	}

	std::shared_ptr<Session> Acceptor::getSession(const SessionID& sessionID)
	{
		return m_sessions[sessionID];
	}

	const Dictionary* const Acceptor::getSessionSettings(const SessionID& sessionID) const
	{
		try
		{
			return &SessionSettings::instance().get(sessionID);
		}
		catch (ConfigError&)
		{
			return 0;
		}
	}

	void Acceptor::start() throw (ConfigError, RuntimeError)
	{
		m_stop = false;
		onInitialize();

		HttpServer::startGlobal();

		if (!thread_spawn(&startThread, this, m_threadid))
			throw RuntimeError("Unable to spawn thread");
	}

	void Acceptor::stop(bool force)
	{
		if (isStopped()) return;

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
				Session::unregisterSession(pSession->getSessionID());
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

	bool Acceptor::isLoggedOn()
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

	THREAD_PROC Acceptor::startThread(void* p)
	{
		Acceptor * pAcceptor = static_cast <Acceptor*> (p);
		pAcceptor->onStart();
		return 0;
	}
}
