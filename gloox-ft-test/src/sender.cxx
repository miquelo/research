/*
 * sender.cxx 12/01/29
 *
 * Miquel Ferran <mikilaguna@gmail.com>
 */

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include <gloox/bytestream.h>
#include <gloox/bytestreamdatahandler.h>
#include <gloox/client.h>
#include <gloox/connectionlistener.h>
#include <gloox/iq.h>
#include <gloox/jid.h>
#include <gloox/siprofileft.h>
#include <gloox/siprofilefthandler.h>
#include <gloox/socks5bytestreamserver.h>

#include "common.h"

class sender_handler;

struct sender_task
{
	sender_handler* send_h;
	pthread_t th;
	gloox::Bytestream* bs;
};

class sender_handler:
public gloox::ConnectionListener,
public gloox::SIProfileFTHandler,
public gloox::BytestreamDataHandler
{
	public:
	~sender_handler(void);
	void init(gloox::SIProfileFT* ft, gloox::Client* cli, const char* r_uid,
			const char* filename);
	void onConnect(void);
	bool onTLSConnect(const gloox::CertInfo& info);
	void onDisconnect(gloox::ConnectionError error);
	void onStreamEvent(gloox::StreamEvent event);
	void onResourceBindError(gloox::ResourceBindError error);
	void onSessionCreateError(gloox::SessionCreateError error);
	void handleFTRequest(const gloox::JID& from, const gloox::JID& to,
			const std::string& sid, const std::string& name, long size,
			const std::string& hash, const std::string& date,
			const std::string& mimetype, const std::string& desc, int stypes);
	void handleFTRequestError(const gloox::IQ& iq, const std::string& sid);
	void handleFTBytestream(gloox::Bytestream* bs);
	const std::string handleOOBRequestResult(const gloox::JID& from,
			const gloox::JID& to, const std::string& sid);
	void handleBytestreamData(gloox::Bytestream* bs,
			const std::string& data);
	void handleBytestreamError(gloox::Bytestream* bs, const gloox::IQ& iq);
	void handleBytestreamOpen(gloox::Bytestream* bs);
	void handleBytestreamClose(gloox::Bytestream* bs);
	void run_data_transfer(gloox::Bytestream* bs);
	void remove_task(sender_task* task);
	
	private:
	gloox::SIProfileFT* _ft;
	gloox::Client* _cli;
	const char* _r_uid;
	const char* _filename;
	pthread_t _server_th;
	std::list<sender_task*> _task_l;
	void _create_task(gloox::Bytestream* bs);
};

using namespace std;
using namespace gloox;

static void* server_exec_f(void* arg);
static void* data_transfer_f(void* arg);

int sender_main(const char* s_uid, const char* s_pass, const char* r_uid,
		const char* filename, const char* p_port)
{
	JID jid(s_uid);
	Client* cli = new Client(jid, s_pass);
	
	sender_handler send_h;
	
	SIProfileFT* ft = new SIProfileFT(cli, &send_h);
	send_h.init(ft, cli, r_uid, filename);
	
	if (p_port not_eq NULL)
	{
		int p_port_i = atoi(p_port);
		SOCKS5BytestreamServer* serv = new SOCKS5BytestreamServer(
				cli->logInstance(), p_port_i);
		if (serv->listen() not_eq ConnNoError)
			cerr << "Port " << p_port_i << " in use" << endl;
			
		ft->registerSOCKS5BytestreamServer(serv);
		ft->addStreamHost(cli->jid(), "localhost", p_port_i);
		
		pthread_t serv_th;
		pthread_create(&serv_th, NULL, server_exec_f, serv);
	}
	else
	{
		ft->addStreamHost(JID("reflector.amessage.eu"),
				"reflector.amessage.eu", 6565);
		ft->addStreamHost(JID("proxy.jabber.org"), "208.245.212.98", 7777);
	}
	
	cli->registerConnectionListener(&send_h);
	cli->connect();

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

void sender_handler::init(SIProfileFT* ft, Client* cli, const char* r_uid,
		const char* filename)
{
	_ft = ft;
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
		clog << " Request done." << endl;
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

void sender_handler::handleFTRequest(const JID& from, const JID& to,
		const string& sid, const string& name, long size,
		const string& hash, const string& date, const string& mimetype,
		const string& desc, int stypes)
{
	clog << "Handle FT request" << endl;
}

void sender_handler::handleFTRequestError(const IQ& iq, const string& sid)
{
	clog << "Handle FT request error" << endl;
}

void sender_handler::handleFTBytestream(Bytestream* bs)
{
	clog << "Handle bytestream" << endl;
	_create_task(bs);
}

const string sender_handler::handleOOBRequestResult(const JID& from,
		const JID& to, const string& sid)
{
	clog << "Handle OOB request result" << endl;
}

void sender_handler::handleBytestreamData(Bytestream* bs, const string& data)
{
	clog << "Handle data" << endl;
}

void sender_handler::handleBytestreamError(Bytestream* bs, const IQ& iq)
{
	clog << "Handle error" << endl;
}

void sender_handler::handleBytestreamOpen(Bytestream* bs)
{
	clog << "Handle open" << endl;
}

void sender_handler::handleBytestreamClose(Bytestream* bs)
{
	clog << "Handle close" << endl;
}

void sender_handler::run_data_transfer(Bytestream* bs)
{
	bs->registerBytestreamDataHandler(this);
	if (not bs->connect())
		cerr << " Bytestream connection error!" << endl;
	else
	{
		clog << "Running data transfer" << endl;
		while (not bs->isOpen())
			bs->recv(1);
		clog << "Bytestream opened" << endl;
		
		const size_t buf_size = 32 * 1024;
		char input[buf_size];
		ifstream file(_filename);
		
		while (bs->isOpen() and not file.eof())
		{
			file.read(input, buf_size);
			if (not bs->send(string(input, file.gcount())))
				cerr << "Sending failed" << endl;
		}
		bs->close();
		clog << "Send done." << endl;
	}
}

void sender_handler::remove_task(sender_task* task)
{
	_task_l.remove(task);
	delete task;
}

void sender_handler::_create_task(Bytestream* bs)
{
	sender_task* task = new sender_task;
	task->send_h = this;
	task->bs = bs;
	pthread_create(&task->th, NULL, data_transfer_f, task);
	
	_task_l.push_back(task);
}

void* server_exec_f(void* arg)
{
	SOCKS5BytestreamServer* serv = static_cast<SOCKS5BytestreamServer*>(arg);
	while (serv->recv(1) == ConnNoError);
}

void* data_transfer_f(void* arg)
{
	sender_task* task = static_cast<sender_task*>(arg);
	sender_handler* send_h = task->send_h;

	send_h->run_data_transfer(task->bs);
	
	send_h->remove_task(task);
	return NULL;
}
