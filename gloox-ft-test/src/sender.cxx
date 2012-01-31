/*
 * sender.cxx 12/01/29
 *
 * Miquel Ferran <mikilaguna@gmail.com>
 */

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include <gloox/client.h>
#include <gloox/connectionlistener.h>
#include <gloox/jid.h>
#include <gloox/siprofileft.h>
#include <gloox/siprofilefthandler.h>
#include <gloox/socks5bytestream.h>
#include <gloox/socks5bytestreamdatahandler.h>
#include <gloox/socks5bytestreamserver.h>
#include <gloox/stanza.h>

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
	void run_server_poll(void);
	void run_data_transfer(gloox::SOCKS5Bytestream* s5b);
	void remove_task(sender_task* task);
	
	private:
	gloox::SIProfileFT* _ft;
	gloox::Client* _cli;
	gloox::SOCKS5BytestreamServer* _serv;
	const char* _r_uid;
	const char* _filename;
	pthread_t _server_th;
	std::list<sender_task*> _task_l;
	void _server_poll(void);
	void _create_task(gloox::SOCKS5Bytestream* s5b);
};

using namespace std;
using namespace gloox;

static void* server_poll_f(void* arg);
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
	clog << "Starting transfer";
	
	ifstream ifs(_filename);
	ifs.seekg(0, ios::end);
	long f_size = ifs.tellg() + 1l;
	clog << " of " << f_size << " bytes" << endl;
	
	JID jid(_r_uid);
	if (not _ft->requestFT(jid, _filename, f_size).empty())
	{
		clog << " Request done." << endl;
		_server_poll();
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
	_create_task(s5b);
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

void sender_handler::run_server_poll(void)
{
	clog << " Connection status: " << _cli->recv(1) << endl;
	clog << " Receiving bytestreams from server..." << endl;
	
	ConnectionError error = _serv->recv(100000);
	while (error == ConnNoError)
	{
		clog << "New bytestream received" << endl;
		error = _serv->recv(100000);
	}
	clog << "No more bytestreams" << error << endl;
}

void sender_handler::run_data_transfer(SOCKS5Bytestream* s5b)
{
	s5b->registerSOCKS5BytestreamDataHandler(this);
	if (not s5b->connect())
		cerr << " Bytestream connection error!" << endl;
	else
	{
		clog << "Running data transfer" << endl;
		bool opened = s5b->isOpen();
		clog << "Bytestream opened: " << opened << endl;
		if (opened)
		{
			s5b->send("data");
			clog << "Send done." << endl;
		}
		s5b->recv(1);
	}
}

void sender_handler::remove_task(sender_task* task)
{
	_task_l.remove(task);
	delete task;
}

void sender_handler::_server_poll(void)
{
	pthread_create(&_server_th, NULL, server_poll_f, this);
}

void sender_handler::_create_task(SOCKS5Bytestream* s5b)
{
	sender_task* task = new sender_task;
	task->send_h = this;
	task->bs = s5b;
	pthread_create(&task->th, NULL, data_transfer_f, task);
	
	_task_l.push_back(task);
}

void* server_poll_f(void* arg)
{
	static_cast<sender_handler*>(arg)->run_server_poll();
	return NULL;
}

void* data_transfer_f(void* arg)
{
	sender_task* task = static_cast<sender_task*>(arg);
	sender_handler* send_h = task->send_h;

	send_h->run_data_transfer(task->bs);
	
	send_h->remove_task(task);
	return NULL;
}
