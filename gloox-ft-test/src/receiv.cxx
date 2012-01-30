/*
 * receiv.cxx 12/01/29
 *
 * Miquel Ferran <mikilaguna@gmail.com>
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>

#include <pthread.h>

#include <client.h>
#include <connectionlistener.h>
#include <jid.h>
#include <presencehandler.h>
#include <siprofileft.h>
#include <siprofilefthandler.h>
#include <socks5bytestream.h>
#include <socks5bytestreamdatahandler.h>
#include <stanza.h>

#include "common.h"

class receiver_handler;

struct receiver_task
{
	receiver_handler* recv_h;
	pthread_t th;
	gloox::SOCKS5Bytestream* bs;
};

class receiver_handler:
public gloox::ConnectionListener,
public gloox::PresenceHandler,
public gloox::SIProfileFTHandler,
public gloox::SOCKS5BytestreamDataHandler
{
	public:
	receiver_handler(void);
	~receiver_handler(void);
	void init(gloox::SIProfileFT* ft);
	void onConnect(void);
	bool onTLSConnect(const gloox::CertInfo& info);
	void onDisconnect(gloox::ConnectionError error);
	void onStreamEvent(gloox::StreamEvent event);
	void onResourceBindError(gloox::ResourceBindError error);
	void onSessionCreateError(gloox::SessionCreateError error);
	void handlePresence(gloox::Stanza* stanza);
	void handleFTRequest (const gloox::JID& from, const std::string& id,
			const std::string& sid, const std::string& name, long size,
			const std::string& hash, const std::string& date,
			const std::string& mimetype, const std::string& desc, int stypes,
			long offset, long length);
	void handleFTRequestError(gloox::Stanza* stanza, const std::string& sid);
	void handleFTSOCKS5Bytestream(gloox::SOCKS5Bytestream* s5b);
	void handleSOCKS5Data(gloox::SOCKS5Bytestream* s5b,
			const std::string& data);
	void handleSOCKS5Error(gloox::SOCKS5Bytestream* s5b, gloox::Stanza* stanza);
	void handleSOCKS5Open(gloox::SOCKS5Bytestream* s5b);
	void handleSOCKS5Close(gloox::SOCKS5Bytestream* s5b);
	void run_data_transfer(gloox::SOCKS5Bytestream* s5b);
	void remove_task(receiver_task* task);
	
	private:
	gloox::SIProfileFT* _ft;
	std::list<receiver_task*> _task_l;
	void _create_task(gloox::SOCKS5Bytestream* s5b);
};

using namespace std;
using namespace gloox;

int receiver_main(const char* p_id, const char* p_port, const char* r_uid,
		const char* r_pass)
{
	JID jid(r_uid);
	Client* cli = new Client(jid, r_pass);
	receiver_handler recv_h;
	
	SIProfileFT* ft = new SIProfileFT(cli, &recv_h);
	ft->addStreamHost(JID(p_id), "localhost", atoi(p_port));
	recv_h.init(ft);

	cli->registerConnectionListener(&recv_h);
	cli->registerPresenceHandler(&recv_h);
	cli->connect();

	delete ft;
	delete cli;
	
	return EXIT_SUCCESS;
}

void* data_transfer_f(void* arg);

receiver_handler::receiver_handler(void):
_ft(NULL)
{
}

receiver_handler::~receiver_handler(void)
{
	list<receiver_task*>::const_iterator it = _task_l.begin();
	while (it not_eq _task_l.end())
		pthread_join((*it++)->th, NULL);
}

void receiver_handler::init(SIProfileFT* ft)
{
	_ft = ft;
}

void receiver_handler::onConnect(void)
{
	clog << "Connection established..." << endl;
}

bool receiver_handler::onTLSConnect(const CertInfo& info)
{
	clog << "TLS Connection established..." << endl;
	return true;
}

void receiver_handler::onStreamEvent(StreamEvent event)
{
	clog << "Stream event" << endl;
}

void receiver_handler::onDisconnect(ConnectionError error)
{
	clog << "Connection closed." << endl;
	if (error not_eq ConnNoError)
		cerr << "Because there was a connection error..." << endl;
}

void receiver_handler::onResourceBindError(ResourceBindError error)
{
	cerr << "Resource bind error!" << endl;
}

void receiver_handler::onSessionCreateError(SessionCreateError error)
{
	cerr << "Session create error!" << endl;
}

void receiver_handler::handlePresence(Stanza* stanza)
{
	clog << "Handle presence" << endl;
}

void receiver_handler::handleFTRequest(const JID& from, const string& id,
		const string& sid, const string& name, long size, const string& hash,
		const string& date, const string& mimetype, const string& desc,
		int stypes, long offset, long length)
{
	clog << "Handle FT request from " << from.full() << endl;
	clog << " They want to transfer a file called '" << name << "'" << endl;
	clog << " That's an example so we don't check anything..." << endl;
	
	_ft->acceptFT(from, id);
}

void receiver_handler::handleFTRequestError(Stanza* stanza, const string& sid)
{
	clog << "Handle FT request error" << endl;
}

void receiver_handler::handleFTSOCKS5Bytestream(SOCKS5Bytestream* s5b)
{
	clog << "Handle bytestream" << endl;
	s5b->registerSOCKS5BytestreamDataHandler(this);
	_create_task(s5b);
}

void receiver_handler::handleSOCKS5Data(SOCKS5Bytestream* s5b,
		const string& data)
{
	clog << "Handle SOCKS5 data" << endl;
}

void receiver_handler::handleSOCKS5Error(SOCKS5Bytestream* s5b, Stanza* stanza)
{
	clog << "Handle SOCKS5 error" << endl;
}

void receiver_handler::handleSOCKS5Open(SOCKS5Bytestream* s5b)
{
	clog << "Handle SOCKS5 open" << endl;
}

void receiver_handler::handleSOCKS5Close(SOCKS5Bytestream* s5b)
{
	clog << "Handle SOCKS5 close" << endl;
}

void receiver_handler::run_data_transfer(SOCKS5Bytestream* s5b)
{
	clog << "Running data transfer" << endl;
	
	ConnectionError error = ConnNoError;
	while ((error = s5b->recv()) == ConnNoError)
	{
	}
	clog << "Transfer finished. Temination code: " << error << endl;
}

void receiver_handler::remove_task(receiver_task* task)
{
	_task_l.remove(task);
	delete task;
}

void receiver_handler::_create_task(SOCKS5Bytestream* s5b)
{
	receiver_task* task = new receiver_task;
	task->recv_h = this;
	task->bs = s5b;
	pthread_create(&task->th, NULL, data_transfer_f, task);
	
	_task_l.push_back(task);
}

void* data_transfer_f(void* arg)
{
	receiver_task* task = static_cast<receiver_task*>(arg);
	SOCKS5Bytestream* s5b = task->bs;
	receiver_handler* recv_h = task->recv_h;

	if (not s5b->connect())
		cerr << " Bytestream connection error!" << endl;
	else
		recv_h->run_data_transfer(s5b);
	
	recv_h->remove_task(task);
	return NULL;
}