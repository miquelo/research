/*
 * sender.cxx 12/01/29
 *
 * Miquel Ferran <mikilaguna@gmail.com>
 */

#include <cstdlib>
#include <cstring>
#include <iostream>

#include <client.h>
#include <connectionlistener.h>
#include <jid.h>
#include <siprofileft.h>
#include <siprofilefthandler.h>
#include <socks5bytestream.h>
#include <socks5bytestreamdatahandler.h>
#include <socks5bytestreamserver.h>
#include <stanza.h>

#include "common.h"

class sender_handler;

struct sender_task
{
	sender_handler* send_h;
	pthread_t th;
	gloox::SOCKS5Bytestream* bs;
};

class sender_handler:
public gloox::ConnectionListener,
public gloox::SIProfileFTHandler,
public gloox::SOCKS5BytestreamDataHandler
{
	public:
	~sender_handler(void);
	void init(gloox::SIProfileFT* ft, gloox::Client* cli,
			gloox::SOCKS5BytestreamServer* serv, const char* r_uid,
			const char* filename);
	void onConnect(void);
	bool onTLSConnect(const gloox::CertInfo& info);
	void onDisconnect(gloox::ConnectionError error);
	void onStreamEvent(gloox::StreamEvent event);
	void onResourceBindError(gloox::ResourceBindError error);
	void onSessionCreateError(gloox::SessionCreateError error);
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
	
	private:
	gloox::SIProfileFT* _ft;
	gloox::Client* _cli;
	gloox::SOCKS5BytestreamServer* _serv;
	const char* _r_uid;
	const char* _filename;
	std::list<sender_task*> _task_l;
};

using namespace std;
using namespace gloox;

static void* data_transfer_f(void* arg);

int sender_main(const char* p_id, const char* p_port, const char* s_uid,
		const char* s_pass, const char* r_uid, const char* filename)
{
	JID jid(s_uid);
	Client* cli = new Client(jid, s_pass);
	
	sender_handler send_h;
	
	int p_port_i = atoi(p_port);
	SOCKS5BytestreamServer* server = new SOCKS5BytestreamServer(
			cli->logInstance(), p_port_i);
	if (server->listen() not_eq ConnNoError)
	{
		cerr << "Port " << p_port_i << " in use!" << endl;
		delete server;
		delete cli;
		
		return EXIT_FAILURE;
	}
	
	SIProfileFT* ft = new SIProfileFT(cli, &send_h);
	ft->registerSOCKS5BytestreamServer(server);
	ft->addStreamHost(JID(p_id), "localhost", p_port_i);
	send_h.init(ft, cli, server, r_uid, filename);

	cli->registerConnectionListener(&send_h);
	cli->connect();

	delete server;
	delete ft;
	delete cli;
	
	return EXIT_SUCCESS;
}

sender_handler::~sender_handler(void)
{
	list<sender_task*>::const_iterator it = _task_l.begin();
	while (it not_eq _task_l.end())
		pthread_join((*it++)->th, NULL);
}

void sender_handler::init(SIProfileFT* ft, Client* cli,
		SOCKS5BytestreamServer* serv, const char* r_uid, const char* filename)
{
	_ft = ft;
	_serv = serv;
	_cli = cli;
	_r_uid = r_uid;
	_filename = filename;
}

void sender_handler::onConnect(void)
{
	clog << "Connection established..." << endl;
	clog << "Starting transfer" << endl;
	
	JID jid(_r_uid);
	if (not _ft->requestFT(jid, _filename, 851).empty())
	{
		clog << " Request done." << endl;
		clog << " Connection status: " << _cli->recv(1) << endl;
		clog << " Receiving bytestreams from server...";
		while (true)
			_serv->recv(1);
	}
}

bool sender_handler::onTLSConnect(const CertInfo& info)
{
	clog << "TLS Connection established..." << endl;
	return true;
}

void sender_handler::onStreamEvent(StreamEvent event)
{
	clog << "Stream event" << endl;
}

void sender_handler::onDisconnect(ConnectionError error)
{
	clog << "Connection closed." << endl;
	if (error not_eq ConnNoError)
	{
		cerr << "Because there was a connection error (" << error << ") ...";
		cerr << endl;
	}
}

void sender_handler::onResourceBindError(ResourceBindError error)
{
	cerr << "Resource bind error!" << endl;
}

void sender_handler::onSessionCreateError(SessionCreateError error)
{
	cerr << "Session create error!" << endl;
}

void sender_handler::handleFTRequest(const JID& from, const string& id,
		const string& sid, const string& name, long size, const string& hash,
		const string& date, const string& mimetype, const string& desc,
		int stypes, long offset, long length)
{
	clog << "Handle FT request" << endl;
}

void sender_handler::handleFTRequestError(Stanza* stanza, const string& sid)
{
	clog << "Handle FT request error" << endl;
}

void sender_handler::handleFTSOCKS5Bytestream(SOCKS5Bytestream* s5b)
{
	clog << "Handle bytestream" << endl;
	
	// XXX No arriba aquí!!!
}

void sender_handler::handleSOCKS5Data(SOCKS5Bytestream* s5b,
		const string& data)
{
	clog << "Handle SOCKS5 data" << endl;
}

void sender_handler::handleSOCKS5Error(SOCKS5Bytestream* s5b, Stanza* stanza)
{
	clog << "Handle SOCKS5 error" << endl;
}

void sender_handler::handleSOCKS5Open(SOCKS5Bytestream* s5b)
{
	clog << "Handle SOCKS5 open" << endl;
}

void sender_handler::handleSOCKS5Close(SOCKS5Bytestream* s5b)
{
	clog << "Handle SOCKS5 close" << endl;
}

void* data_transfer_f(void* arg)
{
	sender_handler* send_h = static_cast<sender_handler*>(arg);
	return NULL;
}
